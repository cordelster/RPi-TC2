/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2009, 2014 NoMachine, http://www.nomachine.com.          */
/*                                                                        */
/* NXUSB, NX protocol compression and NX extensions to this software      */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

/*****************************************************************************
*
*   Copyright (C) 2008-2014 Eltima software
*
*   Author: 2008-2010 Kuzma Shapran <Kuzma[dot]Shapran[at]gmail[dot]com>
*   Author: 2011-2014 Vadim Grinchishin
*
*   License: GPLv3
*
*
* This file is part of 'EVE USB kernel' project.
*
* 'EVE USB kernel' is short for Eltima virtual equipment USB kernel module.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/

#include "vhub.h"
#include "log.h"
#include "trans.h"
#include "usb_descr.h"
#include "message_queue.h"
#include "kmsg_types.h"
#include "kmsg_decl.h"
#include "platform.h"
#include "crossapi.h"

#include <stdbool.h>
#include <linux/slab.h>

static LIST_HEAD(vDev_list);
static DEFINE_SPINLOCK(vDev_lock);

// copied from drivers/usb/host/xhci-hub.c
/* usb 1.1 root hub device descriptor */
static const u8 usb_bos_descr [] = {
	USB_DT_BOS_SIZE,		/*  __u8 bLength, 5 bytes */
	USB_DT_BOS,			/*  __u8 bDescriptorType */
	0x0F, 0x00,			/*  __le16 wTotalLength, 15 bytes */
	0x1,				/*  __u8 bNumDeviceCaps */
	/* First device capability */
	USB_DT_USB_SS_CAP_SIZE,		/*  __u8 bLength, 10 bytes */
	USB_DT_DEVICE_CAPABILITY,	/* Device Capability */
	USB_SS_CAP_TYPE,		/* bDevCapabilityType, SUPERSPEED_USB */
	0x00,				/* bmAttributes, LTM off by default */
	USB_5GBPS_OPERATION, 0x00,	/* wSpeedsSupported, 5Gbps only */
	0x03,				/* bFunctionalitySupport,
					   USB 3.0 speed only */
	0x00,				/* bU1DevExitLat, set later. */
	0x00, 0x00			/* __le16 bU2DevExitLat, set later. */
};

static bool is_hcd_usb3(struct usb_hcd *hcd)
{
	int speed = get_hcd_speed(hcd);
	WARN_ON(speed > HCD_USB3);
	return speed == HCD_USB3;
}

/*
 * See: xhci_hub_status_data.
 */
static inline int bytesForPorts(int max_ports)
{
	return (max_ports + 8)/8; // DIV_ROUND_UP(ports + 1, 8); // 1 + ports/8;
}

static int getPortNumber(u16 wIndex, int *nport)
{
	int num = (wIndex & 0xFF) - 1;
	bool ok = num >= 0 && num < VHUB_NUM_PORTS;

	*nport = ok ? num : -1;
	return ok ? 0 : -EPIPE;
}

static struct device *get_rh_dev(struct usb_hcd *hcd)
{
	struct usb_bus *bus = hcd ? hcd_to_bus(hcd) : 0;
	struct usb_device *root_hub = bus ? bus->root_hub : 0;

	return root_hub ? &root_hub->dev : 0; // to_devu(root_hub);
}

struct VHub *hcd_to_vhub(struct usb_hcd *hcd)
{
	return (struct VHub*)(hcd ? hcd->hcd_priv : 0);
}

void vDev_add_to_list(struct VDevice *vDev)
{
	unsigned long flags;

	spin_lock_irqsave(&vDev_lock, flags);
	list_add_tail(&vDev->list, &vDev_list);
	spin_unlock_irqrestore(&vDev_lock, flags);
}

void vDev_del_from_list(struct VDevice *vDev)
{
	unsigned long flags;

	spin_lock_irqsave(&vDev_lock, flags);
	list_del_init(&vDev->list); // list_empty() now will return true
	spin_unlock_irqrestore(&vDev_lock, flags);
}

/* "reset" is misnamed; its role is now one-time init. the controller
 * should already have been reset (and boot firmware kicked off etc).
 */
static int vhub_reset(struct usb_hcd *hcd)
{
	dev_dbg(get_rh_dev(hcd), "%s", __func__);
	return 0;
}

/*
 * usb_add_hcd in kernel 3.0.X sets HC_STATE_RUNNING just before calling hcd->driver->start(hcd);
 */
static int vhub_start(struct usb_hcd *hcd)
{
	struct device *dev = get_rh_dev(hcd);

	dev_dbg(dev, "%s", __func__);
	
	if (hcd->state != HC_STATE_RUNNING) {
		WARN(hcd->state != HC_STATE_HALT, "state was %d on %s", hcd->state, __func__);
		hcd->state = HC_STATE_RUNNING;
	}
	
	return 0;
}

/* 
 * Cleanly make HCD stop writing memory and doing I/O.
 */
static void vhub_stop(struct usb_hcd *hcd)
{
	edev_dbg("%s", __func__); // dev_warn causes kernel panic on 2.6.24
}


#ifdef CONFIG_PM

static int vhub_bus_suspend(struct usb_hcd *hcd)
{
	struct VHub *vhub = hcd_to_vhub(hcd);
	unsigned long flags;

	spin_lock_irqsave(&vhub->lock, flags);
	hcd->state = HC_STATE_SUSPENDED;
	spin_unlock_irqrestore(&vhub->lock, flags);

	dev_dbg(get_rh_dev(hcd), "suspended");
	return 0;
}

static int vhub_bus_resume(struct usb_hcd *hcd)
{
	int err = 0;
	struct VHub *vhub = hcd_to_vhub(hcd);

	unsigned long flags;
	spin_lock_irqsave(&vhub->lock, flags);

	if (HCD_HW_ACCESSIBLE(hcd)) {
		hcd->state = HC_STATE_RUNNING;
	} else {
		err = -ESHUTDOWN;
	}

	spin_unlock_irqrestore(&vhub->lock, flags);

	dev_dbg(get_rh_dev(hcd), "resume => %d", err);
	return err;
}

#else // CONFIG_PM
  #define vhub_bus_suspend	(0)
  #define vhub_bus_resume	(0)
#endif // CONFIG_PM


static int vhub_get_frame_number(struct usb_hcd *hcd)
{
	return 0;
}


#define USB_PORT_STAT_MASK \
	( USB_PORT_STAT_CONNECTION \
	| USB_PORT_STAT_ENABLE \
	| USB_PORT_STAT_SUSPEND \
	| USB_PORT_STAT_OVERCURRENT \
	| USB_PORT_STAT_RESET )

#define USB_PORT_STAT_C_MASK (USB_PORT_STAT_MASK << 16)

/*
 * Called from usb_hcd_poll_rh_status.
 * Returns hub & ports status change bitmap.
 * Zero bit is hub change status, bits #1..N - port #i change status.
 * See: drivers/usb/host/ehci-hub.c, ehci_hub_status_data.
 */
static int vhub_hub_status(struct usb_hcd *hcd, char *buf) // buf => char buffer[6]; see usb_hcd_poll_rh_status
{
	unsigned long flags;

	bool changed = false;
	struct VHub *vhub = hcd_to_vhub(hcd);

	int written = bytesForPorts(VHUB_NUM_PORTS);
	memset(buf, 0, written); // initial status is no changes

	spin_lock_irqsave(&vhub->lock, flags);

	if (HCD_HW_ACCESSIBLE(hcd)) {
		int i = 0;
		for ( ; i < VHUB_NUM_PORTS; ++i) {
			if (vhub->port_status[i] & USB_PORT_STAT_C_MASK) { // ports change status, bits #1..
				int j = (i + 1)/8;
				buf[j] |= 1 << (i + 1) % 8; // set_bit(i + 1, buf) crashes kernel on Android
				changed = true;
			}
		}
	}

	if (changed && hcd->state == HC_STATE_SUSPENDED) {
		usb_hcd_resume_root_hub(hcd);
	}

	spin_unlock_irqrestore(&vhub->lock, flags);

	return changed ? written : 0;
}

/*
 * Common part for USB 2.0 & 3.0
 **/
static void fill_common_hub_descriptor(struct usb_hub_descriptor *desc)
{
	desc->bNbrPorts = VHUB_NUM_PORTS;
	desc->wHubCharacteristics = cpu_to_le16(0x0001); // HUB_CHAR_INDV_PORT_LPSM
	desc->bPwrOn2PwrGood = 10; // ehci 1.0, 2.3.9 says 20ms max
	desc->bHubContrCurrent = 0;
}

static int fill_usb2_hub_descriptor(void *buf, u16 buf_len)
{
	struct usb_hub_descriptor *desc = (struct usb_hub_descriptor*)buf;
	
	u8 arr_offset = offsetof(struct usb_hub_descriptor, bHubContrCurrent) + 
			sizeof(desc->bHubContrCurrent); // sinse 2.6.39 next field's name changed 
	
	char *arr = (char*)desc + arr_offset; // DeviceRemovable[]
	u8 arr_len = bytesForPorts(VHUB_NUM_PORTS); // DeviceRemovable[arr_len] and PortPwrCtrlMask[arr_len]
	u8 total_len = arr_offset + 2*arr_len; // fixed part plus two arrays

	memset(buf, 0, buf_len);

	if (buf_len < total_len) {
		return -EPIPE; // "stall" on error
	}

	fill_common_hub_descriptor(desc);

	desc->bDescLength = total_len;
	desc->bDescriptorType = USB_DT_HUB;

	memset(arr, -1, arr_len); // DeviceRemovable[]
	*arr <<= 1; // bit 0 is reserved, bit 1 is for port 1, etc.

	memset(&arr[arr_len], -1, arr_len); // PortPwrCtrlMask[]
	return 0;
}

/*
 * Non-removable device is fixed USB connection to a downstream port of hub. It cannot be disconnected
 * from the port. An example is a USB keyboard with hub function. A device connected to a downstream port,
 * such as box hub, is called removable device which can be disconnected.
 * If a hub is connected to non-removable device, the hub shows host the attribute of the port
 * as "bDeviceRemovable" in Hub Descriptor.
 * Non-removable device reduces components such as connector, over-current protection, and power switch,
 * because the plug is not swapped and power supply is shared.
 **/
static int fill_usb3_hub_descriptor(void *buf, u16 buf_len)
{
	struct usb_hub_descriptor *desc = (struct usb_hub_descriptor*)buf;
	u16 port_removable = 0;
	unsigned int i = 0;

	memset(buf, 0, buf_len);

	if (buf_len < USB_DT_SS_HUB_SIZE) {
		return -EPIPE; // "stall" on error
	}


	WARN_ON(VHUB_NUM_PORTS >= sizeof(port_removable)*BITS_PER_BYTE); // limited by usb_hub_descriptor.DeviceRemovable

	for (i = 0; i < VHUB_NUM_PORTS; ++i) {
		port_removable |= 1 << (i + 1); // bit 0 is reserved, bit 1 is for port 1, etc.
	}

	port_removable = cpu_to_le16(port_removable);


	fill_common_hub_descriptor(desc);

	desc->bDescLength = USB_DT_SS_HUB_SIZE;
	desc->bDescriptorType = USB_DT_SS_HUB;

	// header decode latency should be zero for roothubs, see section 4.23.5.2
	// desc->u.ss.bHubHdrDecLat = 0;
	// desc->u.ss.wHubDelay = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39)
	memset(&desc->u.ss.DeviceRemovable, port_removable, sizeof(port_removable));	
#else
	{
		void *DeviceRemovable = (char*)desc + 10; // offset of desc->u.ss.DeviceRemovable
		memset(DeviceRemovable, port_removable, sizeof(port_removable));
	}
#endif

	return 0;
}

/*
 * See: <kernel>/drivers/usb/host/xhci-hub.c, xhci_hub_descriptor().
 */
static int fill_hub_descriptor(struct usb_hcd *hcd, void *buf, u16 buf_len, u16 wValue)
{
	bool usb3 = is_hcd_usb3(hcd);
	
	edev_info("USB %d.0 hub", usb3 ? 3 : 2);
	
	if (usb3 && (buf_len < USB_DT_SS_HUB_SIZE || wValue != (USB_DT_SS_HUB << 8))) {
		edev_err("wrong hub descriptor type for USB 3.0 roothub: wValue %#x, wLength %#x", wValue, buf_len);
		return -EPIPE;
	}

	return usb3 ? fill_usb3_hub_descriptor(buf, buf_len) : 
		      fill_usb2_hub_descriptor(buf, buf_len);
}

static int fill_bos_descriptor(struct usb_hcd *hcd, void *buf, u16 buf_len, u16 wValue)
{
	const int full_len = ((struct usb_bos_descriptor*)usb_bos_descr)->wTotalLength;
	int copied = min(full_len, (int)buf_len);

	WARN_ON(full_len != USB_DT_BOS_SIZE + USB_DT_USB_SS_CAP_SIZE);

	if (!((wValue & 0xff00) == (USB_DT_BOS << 8) && is_hcd_usb3(hcd))) {
		return -EPIPE;
	}

	memcpy(buf, usb_bos_descr, copied);
	return copied; // see drivers/usb/core/hcd.c, rh_call_control, nongeneric:
}

/*
 * See: source/drivers/usb/core/hub.c, hub_port_reset & hub_port_wait_reset.
 */
static void complete_port_reset(struct VHub *vhub, int nport)
{
	struct VDevice *vDev = vhub->port_device[nport];

	uint32_t set_flags = 1 << USB_PORT_FEAT_C_RESET;
	vhub->port_status[nport] &= ~USB_PORT_STAT_RESET;

	if (vDev) {
		vDev->address = 0;
		set_flags |= USB_PORT_STAT_ENABLE;

		switch (vDev->speed) {
		case USB_SPEED_HIGH:
			set_flags |= USB_PORT_STAT_HIGH_SPEED;
			break;
		case USB_SPEED_LOW:
			set_flags |= USB_PORT_STAT_LOW_SPEED;
			break;
		default:
			break;
		}
	}

	vhub->port_status[nport] |= set_flags;

}

static void getPortStatus(char *buf, struct device *dev, struct VHub *vhub, int nport)
{
	if (vhub->resuming && time_is_before_jiffies(vhub->re_timeout)) {

		vhub->resuming = 0;
		vhub->re_timeout = 0;

		vhub->port_status[nport] &= ~USB_PORT_STAT_SUSPEND; // 1 << USB_PORT_FEAT_SUSPEND
		vhub->port_status[nport] |= (1 << USB_PORT_FEAT_C_SUSPEND);

		dev_dbg(dev, "port #%d suspend state cleared", nport + 1);
	}
	
	if ((vhub->port_status[nport] & USB_PORT_STAT_RESET) &&
	     time_is_before_jiffies(vhub->re_timeout)
	) {
		vhub->re_timeout = 0;
		complete_port_reset(vhub, nport);
		dev_dbg(dev, "port #%d reset completed", nport + 1);
	}

	((__le16*)buf)[0] = cpu_to_le16(vhub->port_status[nport]); // port status flags
	((__le16*)buf)[1] = cpu_to_le16(vhub->port_status[nport] >> 16); // port change flags
}

static void setPortFeature(struct VHub *vhub, int nport, u16 wValue)
{
	switch (wValue) {
	case USB_PORT_FEAT_RESET:
		/* if it's already enabled, disable */
		vhub->port_status[nport] &= ~(USB_PORT_STAT_ENABLE | USB_PORT_STAT_LOW_SPEED | USB_PORT_STAT_HIGH_SPEED);
		/* 50msec reset signaling */
		vhub->re_timeout = jiffies + msecs_to_jiffies(50);
		WARN_ON(vhub->resuming);
	}

	if ((vhub->port_status[nport] & USB_PORT_STAT_POWER) || (wValue == USB_PORT_FEAT_POWER)) {
		vhub->port_status[nport] |= (1 << wValue);
	}
}

static void clearPortFeature(struct VHub *vhub, int nport, u16 wValue)
{
	switch (wValue) {
	case USB_PORT_FEAT_SUSPEND:
		if (vhub->port_status[nport] & USB_PORT_STAT_SUSPEND) {
			vhub->re_timeout = jiffies + msecs_to_jiffies(20);
			vhub->resuming = 1;
		}
		break;
	case USB_PORT_FEAT_POWER:
		vhub->port_status[nport] = 0;
		vhub->resuming = 0;
		break;
	}

	vhub->port_status[nport] &= ~(1 << wValue);
}

/*
 * drivers/usb/core/hcd.c calls this routine in rh_call_control, look for hcd->driver->hub_control.
 * See: drivers/usb/host/xhci-hub.c, xhci_hub_control.
 */
static int vhub_hub_control(struct usb_hcd *hcd, u16 typeReq, u16 wValue, u16 wIndex, char *buf, u16 wLength)
{
	const int protocol_stall = -EPIPE;
	int status = 0; // error if < 0, bytes copied if > 0, success if zero
	int nport = -1;
	
	unsigned long flags;
	
	struct VHub *vhub = hcd_to_vhub(hcd);
	struct device *dev = get_rh_dev(hcd);

	if (!HCD_HW_ACCESSIBLE(hcd)) {
		dev_notice(dev, "HCD is not accessible");
		return -ETIMEDOUT;
	}

	spin_lock_irqsave(&vhub->lock, flags);

	switch (typeReq) { // hcd.c says these are non-generic requests
	case GetHubStatus:
		WARN_ON(wLength != 4);
		memset(buf, 0, wLength); // No power source, over-current reported per port
		break;
	case GetHubDescriptor:
		status = fill_hub_descriptor(hcd, buf, wLength, wValue);
		break;
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		status = fill_bos_descriptor(hcd, buf, wLength, wValue); // returns bytes copied
		break;
	case GetPortStatus:
		WARN_ON(wLength != 4); // u16 port status flags + u16 port change flags
		if (!(status = getPortNumber(wIndex, &nport))) {
			getPortStatus(buf, dev, vhub, nport);
		}
		break;
	case SetPortFeature:
		if (!(status = getPortNumber(wIndex, &nport))) {
			setPortFeature(vhub, nport, wValue);
		}
		break;
	case ClearPortFeature:
		if (!(status = getPortNumber(wIndex, &nport))) {
			clearPortFeature(vhub, nport, wValue);
		}
		break;
	case SetHubFeature:
		status = protocol_stall;
		break;
	case ClearHubFeature:
		break;
	default:
		status = protocol_stall;
	}

	spin_unlock_irqrestore(&vhub->lock, flags);

	if (status < 0) {
		dev_err(dev, "%s(typeReq %#x, wValue %#x, wIndex %#x, wLength %#x) => %d",
				__func__, typeReq, wValue, wIndex, wLength, status);
	}

	if (nport != -1 && (vhub->port_status[nport] & USB_PORT_STAT_C_MASK)) {
		usb_hcd_poll_rh_status(hcd);
	}

	return status;
}

static void check_stream_id(struct urb *urb)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
	if (urb && urb->stream_id) {
		edev_err("urb.stream_id %u: bulk stream support is not implemented", urb->stream_id);
	}
#endif
}

/*
 * Can be called in any context.
 * Successful submissions return 0, otherwise this routine returns a negative error number.
 * usb_submit_urb => usb_hcd_submit_urb => hcd->driver->urb_enqueue.
 *
 * See: drivers/staging/usbip/vhci_hcd.c, vhci_urb_enqueue.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
static int vhub_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags)
#else
static int vhub_urb_enqueue(struct usb_hcd *hcd, struct usb_host_endpoint *ep, struct urb *urb, gfp_t mem_flags)
#endif
{
	struct message_up *msg = 0;
	struct VDevice *vDev = 0;

	struct device *dev = get_rh_dev(hcd);
	struct VHub *vhub = hcd_to_vhub(hcd);

	if (!urb->transfer_buffer && urb->transfer_buffer_length) {
		dev_err(dev, "transfer buffer is null");
		return -EINVAL;
	}

	if (urb->status != -EINPROGRESS) {
		dev_err(dev, "urb %p already unlinked, status %d", urb, urb->status);
		WARN_ON(urb->status >= 0);
		return urb->status;
	}

	if (vhub->shutdown) {
		dev_err(dev, "Cannot enqueue urb %p, HCD is shutting down", urb);
		return -ESHUTDOWN;
	}
	
	check_stream_id(urb);
	
	vDev = get_vDev_by_address(hcd, usb_pipedevice(urb->pipe));

	if (!vDev) {
		dev_err(dev, "Cannot enqueue urb %p, device not found", urb);
		return -ENODEV;
	} else if (vDev->shutdown) {
		dev_err(dev, "Cannot enqueue urb %p, shutdown in progress", urb);
		return -ESHUTDOWN;
	} else {
		urb->hcpriv = vDev;
		WARN_ON(vDev->parent != hcd);
	}

	if (!usb_pipedevice(urb->pipe) && usb_pipetype(urb->pipe) == PIPE_CONTROL) {

		struct usb_ctrlrequest *req= (struct usb_ctrlrequest*)urb->setup_packet;

		if (req && req->bRequest == USB_REQ_SET_ADDRESS) {

			unsigned long flags;

			spin_lock_irqsave(&vhub->lock, flags);
			vDev->address = le16_to_cpu(req->wValue);
			spin_unlock_irqrestore(&vhub->lock, flags);

			dev_dbg(dev, "USB_REQ_SET_ADDRESS %d", req->wValue);

			if (urb->status == -EINPROGRESS) {
				urb->status = 0;
			}

			vDev_complete_urb(0, urb, urb->status);
			return 0;
		}
	}

	msg = message_up_ctor_urb(urb, vDev, mem_flags);

	if (msg) {
		message_up_enqueue(msg);
		message_up_put(msg);
	}

	return msg ? 0 : -ENOMEM;
}

/*
 * Can be called in any context.
 * Successful call returns 0, otherwise this routine returns a negative error number.
 * usb_unlink_urb => usb_hcd_unlink_urb => hcd->driver->urb_dequeue.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
static int vhub_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
#else
static int vhub_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
{
	int status = urb->status;
#endif
	struct message_up *msg = 0;
	struct VDevice *vDev = 0;

	struct device *dev = get_rh_dev(hcd);
	struct VHub *vhub = hcd_to_vhub(hcd);

	if (vhub->shutdown) {
		return -ESHUTDOWN;
	}

	msg = message_up_find(urb); // call message_up_put!

	if (msg) {
		message_up_dequeue(msg);
	} else {
		dev_err(dev, "Dequeueing URB %p is not found among submitted URBs", urb);
		vDev_complete_urb(0, urb, status);
		return 0;
	}

	vDev = findDevice(urb, false);

	if (vDev) {
		struct MsgURBCancel *req = 0;
		struct message_up *up = message_up_ctor(MSG_CMD_CLI_URB_CANCEL, sizeof(*req), GFP_ATOMIC);
		if (up) {
			req = (struct MsgURBCancel*)up->msg;
			req->common.dev_id = vDev->id;
			req->common.context = (uintptr_t)urb;
			req->common.pipe = urb->pipe;

			message_up_enqueue(up);
			vDev_complete_urb(up, urb, status); // signal_message_ready(up);

			message_up_put(up);
		}
	} else {
		dev_err(dev, "Dequeueing URB %p does not belong to our device", urb);
		vDev_complete_urb(msg, urb, status);
	}

	message_up_put(msg);
	return 0;
}

static bool add_device(struct VDevice *dev, struct VHub *hub)
{
	unsigned long flags;

	int port = 0;
	dev->port_number = -1;

	spin_lock_irqsave(&hub->lock, flags);

	for (port = 0; port < VHUB_NUM_PORTS; ++port) {

		uint32_t flags = USB_PORT_STAT_CONNECTION | (1 << USB_PORT_FEAT_C_CONNECTION); // USB_PORT_STAT_C_CONNECTION << 16

		if (hub->port_device[port]) {
			continue;
		}

		hub->port_device[port] = dev;
		dev->port_number = port;
		
		switch (dev->speed) {
		case USB_SPEED_HIGH:
			flags |= USB_PORT_STAT_HIGH_SPEED;
			break;
		case USB_SPEED_LOW:
			flags |= USB_PORT_STAT_LOW_SPEED;
			break;
		default:
			break;
		}

		hub->port_status[port] |= flags;
		break;
	}

	spin_unlock_irqrestore(&hub->lock, flags);
	
	return dev->port_number >= 0;
}

/*
 * QEMU usb-hub.c, usb_hub_detach clears PORT_STAT_ENABLE.
 */
static void remove_device(struct VDevice *dev)
{
	struct VHub *hub = hcd_to_vhub(dev->parent);
	int port = dev->port_number;

	uint32_t add_flags = 0;
	uint32_t clear_flags = 0;

	unsigned long flags;
	
	if (port >= 0 && port < VHUB_NUM_PORTS) {
		dev->port_number = -1;
	} else {
		WARN_ON(port >= VHUB_NUM_PORTS);
		return;
	}

	edev_notice("Disconnecting emulated device from port #%d", port + 1);

	spin_lock_irqsave(&hub->lock, flags);

	hub->port_device[port] = 0;

	clear_flags |= USB_PORT_STAT_CONNECTION | USB_PORT_STAT_HIGH_SPEED | USB_PORT_STAT_LOW_SPEED;
	add_flags |= 1 << USB_PORT_FEAT_C_CONNECTION;

	if (hub->port_status[port] & USB_PORT_STAT_ENABLE) {
		clear_flags |= USB_PORT_STAT_ENABLE;
		add_flags |= 1 << USB_PORT_FEAT_C_ENABLE;
	}

	hub->port_status[port] &= ~clear_flags;
	hub->port_status[port] |= add_flags;

	spin_unlock_irqrestore(&hub->lock, flags);

	usb_hcd_poll_rh_status(dev->parent);
}

int vDev_create(struct VDevice **pDev, dev_id_t id, enum usb_device_speed speed)
{
	struct usb_hcd *hcd = get_eveusb_hcd();
	struct VDevice *dev = 0;
	
	if (pDev) {
		*pDev = 0;
	} else {
		return -EINVAL;
	} 
	
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		edev_crit("cannot create virtual USB device. Not enough memory");
		WARN_ON("not enough memory");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&dev->list);
	dev->id = id;
	dev->speed = speed;
	dev->port_number = -1;
	dev->address = -1;
	dev->parent = get_eveusb_hcd();
	
	if (speed > USB_SPEED_HIGH && !is_hcd_usb3(hcd)) {
		edev_warn("device speed '%s' will be reduced because HCD is high-speed", get_speed_str(speed));
		dev->speed = USB_SPEED_HIGH;
	}
	
	if (add_device(dev, hcd_to_vhub(dev->parent))) {
		vDev_add_to_list(dev);
		edev_notice("new %s-speed device emulated on port #%d",
				get_speed_str(dev->speed), dev->port_number + 1);
	} else {
		edev_err("no port found for a new device");
		vDev_destroy(dev);
		return -ENOSPC;
	}

	*pDev = dev;
	return 0;
}

/*
 * If call usb_hcd_resume_root_hub(hcd) here, the kernel 2.6.18 does not send
 * requests for USB device appeared on this hub. 
 * Later kernels do not have this problem, however this call was removed.
 */
void vDev_start(void)
{
	struct usb_hcd *hcd = get_eveusb_hcd();
	usb_hcd_poll_rh_status(hcd);
}

int vDev_destroy(struct VDevice *dev)
{
	if (!dev) {
		return -EINVAL;
	}

	dev->shutdown = 1;
	smp_wmb();

	message_up_dequeue_vdev(dev);

	vDev_del_from_list(dev);
	remove_device(dev);

	kfree(dev);
	return 0;
}

void vDev_destroy_all(void)
{
	unsigned long flags;
	spin_lock_irqsave(&vDev_lock, flags);

	while (!list_empty(&vDev_list)) {

		struct VDevice *vDev = list_first_entry(&vDev_list, struct VDevice, list);

		spin_unlock_irqrestore(&vDev_lock, flags);
		vDev_destroy(vDev);
		spin_lock_irqsave(&vDev_lock, flags);
	}

	spin_unlock_irqrestore(&vDev_lock, flags);
}

struct VDevice *get_vDev_by_address(struct usb_hcd *hcd, int32_t address)
{
	struct VHub *vhub = hcd_to_vhub(hcd);
	struct VDevice *ret = 0;
	struct list_head *item = 0;

	if (vhub && (address >= 0)) {
		unsigned long flags;
		spin_lock_irqsave(&vDev_lock, flags);

		list_for_each(item, &vDev_list) {

			ret = list_entry(item, struct VDevice, list);

			if (ret->address == address) {
				if (ret->port_number != -1)
					break;
			}

			ret = 0;
		}

		spin_unlock_irqrestore(&vDev_lock, flags);
	}

	return ret;
}

struct VDevice *get_vDev_by_id(dev_id_t id)
{
	struct VDevice *dev = 0;
	struct list_head *item;
	bool found = false;

	unsigned long flags;
	spin_lock_irqsave(&vDev_lock, flags);

	list_for_each(item, &vDev_list) {
		dev = list_entry(item, struct VDevice, list);
		found = dev->id == id;
		if (found) {
			break;
		}
	}

	spin_unlock_irqrestore(&vDev_lock, flags);
	return found ? dev : 0;
}

struct VDevice *findDevice(struct urb *urb, bool clear_hcpriv)
{
	struct VDevice *dev = (struct VDevice*)urb->hcpriv;
	struct VDevice *ret = 0;
	struct list_head *item = 0;

	unsigned long flags;
	spin_lock_irqsave(&vDev_lock, flags);

	list_for_each(item, &vDev_list) {

		ret = list_entry(item, struct VDevice, list);
		if (ret == dev) {
			break;
		}

		ret = 0;
	}

	spin_unlock_irqrestore(&vDev_lock, flags);
	
	if (clear_hcpriv) {
		urb->hcpriv = 0;
	}

	return ret;
}

void vDev_complete_urb(struct message_up *msg, struct urb *urb, int status)
{
	struct VDevice *vDev = findDevice(urb, true);

	if (vDev) {
		struct usb_hcd *hcd = vDev->parent;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
		usb_hcd_giveback_urb(hcd, urb, status);
#else
		urb->status = status;

	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18)
		usb_hcd_giveback_urb(hcd, urb);
	#else
		usb_hcd_giveback_urb(hcd, urb, 0); // struct pt_regs *regs
	#endif	
#endif

	} else {
		edev_err("!vDev urb %p", urb);
	}

	if (msg) {
		signal_message_ready(msg);
	}
}

void vhub_signal_shutdown(void)
{
	struct VHub *vhub = hcd_to_vhub(get_eveusb_hcd());

	if (vhub) {
		vhub->shutdown = 1;
		smp_wmb();
	}
}

/*
 * Returns [HCD_USB11, HCD_USB2, HCD_USB3].
 */
int get_hcd_speed(const struct usb_hcd *hcd)
{
	WARN_ON(!(hcd && hcd->driver));
	WARN_ON(HCD_USB11 == 0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39)
	return hcd ? hcd->speed : 0; // declared as int
#else
	return hcd && hcd->driver ? hcd->driver->flags & HCD_MASK : 0;
#endif
}

/*
 * I have to use HCD_USB2 for kernels < 2.6.37 to avoid issue with unassigned udev->devnum:
 * usb 1-1: new high speed USB device using eveusbhcd and address 0
 * usb 1-1: device not accepting address 0, error -22
 * hub 1-0:1.0: unable to enumerate USB device on port 1
 *
 * See: source/drivers/usb/core/hub.c, hub_port_connect_change
 *
 * # up to 2.6.36
 * if (!(hcd->driver->flags & HCD_USB3)) {
 *       choose_address(udev);
 *
 * # 2.6.37+
 * choose_devnum(udev);
 */
static struct hc_driver eveusb_hc_driver =
{
	.description = "nxusb",
	.product_desc = "NoMachine USB Host Controller",
	.hcd_priv_size = sizeof(struct VHub),

//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
//	.flags = HCD_USB3, // appeared in 2.6.31
//#else
	.flags = HCD_USB2,
//#endif

	.reset = vhub_reset,
	.start = vhub_start,
	.stop = vhub_stop,

	.get_frame_number = vhub_get_frame_number,

	.urb_enqueue = vhub_urb_enqueue,
	.urb_dequeue = vhub_urb_dequeue,

	.hub_status_data = vhub_hub_status,
	.hub_control = vhub_hub_control,

	.bus_suspend = vhub_bus_suspend,
	.bus_resume = vhub_bus_resume,
};

struct hc_driver *get_eveusb_hc_driver(void)
{
	return &eveusb_hc_driver;
}
