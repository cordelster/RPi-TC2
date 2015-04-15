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
*   Author: 2010-2013 Vadim Grinchishin
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

#include "trans.h"
#include "log.h"
#include "vhub.h"
#include "usb_device.h"
#include "usb_descr.h"
#include "utils.h"
#include "crossapi.h"
#include "message_queue.h"
#include "kmsg_types.h"
#include "kmsg_decl.h"
#include "platform.h"

#include <stdbool.h>
#include <linux/slab.h>

struct DeviceInfoImpl
{
	struct list_head list;
	struct DeviceInfo dev;
};

static inline void *msgURBRequest_buffer(const struct MsgURBRequest *msg)
{
	const void *ptr = msg + 1;
	return (void*)ptr;
}

static inline void *msgURBISORequest_buffer(const struct MsgURBISORequest *msg)
{
	const void *ptr = msg + 1;
	return (uint8_t*)ptr + msg->number_of_packets*sizeof(msg->frames[0]);
}

static inline void *msgURBResponse_buffer(const struct MsgURBResponse *msg)
{
	const void *ptr = msg + 1;
	return (void*)ptr;
}

static inline void *msgURBISOResponse_buffer(const struct MsgURBISOResponse *msg)
{
	const void *ptr = msg + 1;
	return (uint8_t*)ptr + msg->number_of_packets*sizeof(msg->frames[0]);
}

static inline int eve_device_reprobe(struct device *dev)
{
	int err = 0;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18)
	err = device_reprobe(dev);
#else
	device_reprobe(dev);
#endif
	return err;
}

/*
 * Try return device to system.
 * Do not send response on this message to daemon.
 */
static void eve_reprobe_device_cmd(struct MsgHeader *hdr)
{
	struct usb_device *udev = 0;
	struct MsgReprobeDevice *msg = (struct MsgReprobeDevice*)hdr;

	if (hdr->length != sizeof(*msg)) {
		edev_err("%s: MsgHeader.length %u != %zu", __func__, hdr->length, sizeof(*msg));
		return;
	}

	udev = eve_find_usb_device(msg->devname); // call usb_put_dev!

	if (udev) {
		struct device *dev = to_devu(udev);

		int err = eve_device_reprobe(dev);
		dev_info(dev, "%s(%s) => %d", __func__, msg->devname, err);

		usb_put_dev(udev);
	} else {
		edev_err("%s(%s): device not found", __func__, msg->devname);
	}
}

static bool on_eveusb_hub(const struct usb_device *udev)
{
	return udev && udev->bus ? !strcmp(udev->bus->bus_name, get_eveusbhcd_name()) : false;
}

static bool is_on_eveusb_hub(struct usb_device *udev)
{
	for (; udev; udev = udev->parent) {
		if (on_eveusb_hub(udev))
			return true;
	}

	return false;
}

static void fill_device_info(struct DeviceInfo *d, struct usb_device *udev, bool removed)
{
	const char *name = udev->product ? udev->product : udev->manufacturer; // udev->serial;

	strncpy(d->devname, dev_name(to_devu(udev)), sizeof(d->devname));

	if (name) {
		strncpy(d->name, name, sizeof(d->name));
	} else {
		snprintf(d->name, sizeof(d->name), "%#06x", le16_to_cpu(udev->descriptor.idVendor));
	}

	d->maxchild = udev->maxchild;
	d->removed = removed;
}

static int eve_device_visitor(struct device *dev, void *data)
{
	struct list_head *list_head = data;

	struct usb_device *udev = 0;
	struct DeviceInfoImpl *di = 0;

	if (get_device_type(dev) == DTYPE_USB_DEVICE) {
		udev = to_usb_device(dev);
	} else {
		return 0;
	}

	if (is_on_eveusb_hub(udev)) {
		return 0;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);

	if (!di) {
		WARN_ON("not enough memory");
		return ENOMEM;
	}

	fill_device_info(&di->dev, udev, false);

	INIT_LIST_HEAD(&di->list);
	list_add_tail(&di->list, list_head);

	return 0;
}

/*
 * Sends MsgDevices to daemon filled with information from list.
 */
static int eve_devices_cmd_response(struct list_head *list_head, int sockfd)
{
	struct message_up *msgUp = 0;
	struct MsgDevices *msg = 0;

	size_t cnt = get_list_size(list_head);
	size_t msg_sz = sizeof(*msg) + cnt * sizeof(*msg->dev);

	msgUp = message_up_ctor(MSG_CMD_DEVICES, msg_sz, GFP_KERNEL);

	if (msgUp) {
		struct DeviceInfoImpl *pos = 0;
		size_t i = 0;

		msg = (struct MsgDevices*)msgUp->msg;
		msg->sockfd = sockfd;
		msg->cnt = cnt;

		list_for_each_entry(pos, list_head, list) {
			msg->dev[i++] = pos->dev;
		}

		WARN_ON(i != cnt);
		WARN_ON(!cnt);

		message_up_enqueue(msgUp);
		message_up_put(msgUp);
		
		edev_dbg("%zu device(s) sent to daemon", cnt);
	}

	return msgUp ? 0 : -ENOMEM;
}

/*
 * Frees memory allocated for list's nodes.
 * Clears list.
 */
static void kfree_nodes(struct list_head *head)
{
	struct DeviceInfoImpl *pos = 0;
	struct DeviceInfoImpl *next = 0;

	list_for_each_entry_safe(pos, next, head, list) {
		list_del(&pos->list);
		kfree(pos);
	}
}

static int eve_devices_cmd(struct MsgHeader *hdr)
{
	const struct MsgDevices *msg = (struct MsgDevices*)hdr;
	struct bus_type *bus = eve_get_usb_bus();

	LIST_HEAD(list_head);
	int err = 0;

	if (!msg || hdr->length != sizeof(*msg) || msg->cnt) {
		edev_err("Bad 'devices' message from daemon: size %zu, hdr.length %u, cnt %u",
			  sizeof(*msg), hdr->length, msg->cnt);

		return -EINVAL;
	}

	err = bus_for_each_dev(bus, 0, &list_head, eve_device_visitor);

	if (!err && !list_empty(&list_head)) {
		err = eve_devices_cmd_response(&list_head, msg->sockfd);
	}

	kfree_nodes(&list_head);
	return err;
}

/*
 * Sends response to the daemon even if device does not found.
 */
static void eve_device_destroy_cmd(struct MsgHeader *hdr)
{
	struct MsgVDev *msg = (struct MsgVDev*)hdr;
	struct VDevice *dev = 0;

	if (hdr->length != sizeof(*msg)) {
		edev_err("%s: message from daemon has incorrect length %u != %zu",
			__func__, hdr->length, sizeof(*msg));
		return;
	}

	dev = get_vDev_by_id(msg->dev_id);

	if (dev) {
		int err = vDev_destroy(dev);
		WARN_ON(err);
	} else {
		edev_dbg("Cannot destroy unexisting device id %#x", msg->dev_id);
	}

	send_device_destroyed(msg->dev_id);
}

static int eve_device_create_cmd(struct MsgHeader *hdr)
{
	struct MsgConnect *msg_con = (struct MsgConnect*)hdr;
	struct VDevice *vDev = 0;
	struct message_up *msgUp = 0;
	int err = 0;

	if (hdr->length != sizeof(*msg_con)) {
		edev_err("Connect message from daemon has incorrect length %u != %zu", hdr->length, sizeof(*msg_con));
		return -EINVAL;
	}

	do {
		struct MsgVDev *msg_vDev = 0;

		if ((err = vDev_create(&vDev, msg_con->dev_id, (enum usb_device_speed)msg_con->speed))) {
			break;
		}

		msgUp = message_up_ctor(MSG_CMD_CONNECT, sizeof(*msg_vDev), GFP_KERNEL);

		if (msgUp) {
			msg_vDev = (struct MsgVDev*)msgUp->msg;
			msg_vDev->dev_id = msg_con->dev_id;
		} else {
			err = -ENOMEM;
		}

	} while (0);

	if (!err) {
		vDev_start();
		message_up_enqueue(msgUp);
	} else {
		vDev_destroy(vDev);
	}

	message_up_put(msgUp);
	return err;
}

static void eve_control_cmd(struct MsgHeader *msg)
{
	if (!msg) {
		WARN_ON("null message header");
		return;
	}

	switch ((enum MSG_CMD)(msg->cmd)) {
	case MSG_CMD_CONNECT:
		eve_device_create_cmd(msg);
		break;
	case MSG_CMD_DISCONNECT:
		eve_device_destroy_cmd(msg);
		break;
	case MSG_CMD_DEVICES:
		eve_devices_cmd(msg);
		break;
	case MSG_CMD_REPROBE_DEVICE:
		eve_reprobe_device_cmd(msg);
		break;
	default:
		edev_err("Unexpected control msg %u", msg->cmd);
		WARN(1, "Unexpected control msg %u", msg->cmd);
	}
}

/*
 * Sends response (MsgVDev) on processed MSG_CMD_DISCONNECT.
 */
int send_device_destroyed(dev_id_t id)
{
	struct MsgVDev *msg_vDev = 0;
	struct message_up *msg = message_up_ctor(MSG_CMD_DISCONNECT, sizeof(*msg_vDev), GFP_KERNEL);

	if (msg) {
		msg_vDev = (struct MsgVDev*)msg->msg;
		msg_vDev->dev_id = id;

		message_up_enqueue(msg);
		message_up_put(msg);
	}

	return msg ? 0 : -ENOMEM;
}

/*
 * Declared in usb_device.h
 */
int eve_usb_device_add_remove(struct usb_device *udev, _Bool removed)
{
	struct MsgDevices *msg = 0;
	size_t msg_sz = sizeof(*msg) + sizeof(msg->dev[0]);

	struct message_up *msgUp = 0;

	if (!udev) { // USB_BUS_ADD
		edev_err("%s(%p, %d): null pointer", __func__, udev, removed);
		return -EINVAL;
	}

	if (on_eveusb_hub(udev)) {
		dev_dbg(to_devu(udev), "virtual device(%s) %sed, notification skipped",
					udev->product, removed ? "remov" : "add");
		return 0;
	}

	msgUp = message_up_ctor(MSG_CMD_DEVICES, msg_sz, GFP_KERNEL);

	if (msgUp) {
		msg = (struct MsgDevices*)msgUp->msg;
	} else {
		return -ENOMEM;
	}
	
	msg->sockfd = -1; // broadcast
	msg->cnt = 1;
	fill_device_info(msg->dev, udev, removed);

	message_up_enqueue(msgUp);
	message_up_put(msgUp);
	
	dev_dbg(to_devu(udev), "device(%s) %sed sent to daemon", udev->product, removed ? "remov" : "add");
	return 0;
}

static void *process_isoc_response(struct urb *urb, struct MsgURBISOResponse *isoc)
{
	int i = 0;
	size_t hdr_len = isoc->common.hdr.length;

	urb->start_frame = isoc->start_frame;

	if (hdr_len >= sizeof(*isoc) && urb->number_of_packets == isoc->number_of_packets) {

		if (hdr_len >= sizeof(*isoc) + isoc->number_of_packets*sizeof(isoc->frames[0])) {

			urb->error_count = isoc->error_count;

			for (i = 0; i < urb->number_of_packets; ++i) {
				urb->iso_frame_desc[i].actual_length = isoc->frames[i].length;
				urb->iso_frame_desc[i].status = isoc->frames[i].status;
			}

			return msgURBISOResponse_buffer(isoc);
		}
	}
	
	// bad isoc response

	urb->error_count = urb->number_of_packets;

	for (i = 0; i < urb->number_of_packets; ++i) {
		urb->iso_frame_desc[i].actual_length = 0;
	}

	return 0;
}

static int fill_cbi_msg(struct message_up *msgUp, struct urb *urb)
{
	const struct usb_ctrlrequest *setup = (struct usb_ctrlrequest*)urb->setup_packet;
	int pipe_type = usb_pipetype(urb->pipe);
	struct MsgURBRequest *req = 0;

	size_t buf_pos = 0;
	size_t buf_len = 0;
	size_t urb_len = 0;

	switch (pipe_type) {
	case PIPE_CONTROL:
		buf_len += sizeof(*setup);
		WARN_ON(urb->transfer_buffer_length < setup->wLength);
		// FALL THROUGH
	case PIPE_BULK:
	case PIPE_INTERRUPT:
		if (usb_pipeout(urb->pipe)) {
			buf_len += urb->transfer_buffer_length;
		}
		break;
	default:
		edev_err("%s: wrong pipe type %d", __func__, pipe_type);
		return -EINVAL;
	}

	urb_len = sizeof(*req) + buf_len;

	req = message_up_alloc_msg(msgUp, MSG_CMD_CLIENT_URB, urb_len, GFP_KERNEL);
	if (!req) {
		return -ENOMEM;
	}

	req->common.dev_id = msgUp->vDev->id;
	req->common.context = (uintptr_t)urb;
	req->common.pipe = urb->pipe;

	req->transfer_flags = urb->transfer_flags;
	req->transfer_buffer_length = urb->transfer_buffer_length;
	req->actual_length = urb->actual_length;

	if (pipe_type == PIPE_CONTROL) {
		size_t len = sizeof(*setup);
		memcpy(msgURBRequest_buffer(req), setup, len);
		buf_pos += len;
		buf_len -= len;
	}

	if (buf_len) {
		memcpy(msgURBRequest_buffer(req) + buf_pos, urb->transfer_buffer, buf_len);
	}
	
	return 0;
}

static int fill_isoc_msg(struct message_up *msgUp, struct urb *urb)
{
	int i = 0;
	struct MsgURBISORequest *req = 0;

	size_t buf_len = usb_pipeout(urb->pipe) ? urb->transfer_buffer_length : 0;
	size_t urb_len = sizeof(*req) + urb->number_of_packets*sizeof(req->frames[0]) + buf_len;

	req = message_up_alloc_msg(msgUp, MSG_CMD_CLIENT_URB, urb_len, GFP_KERNEL);
	if (!req) {
		return -ENOMEM;
	}

	req->common.common.dev_id = msgUp->vDev->id;
	req->common.common.context = (uintptr_t)urb;
	req->common.common.pipe = urb->pipe;
	
	req->common.transfer_flags = urb->transfer_flags;
	req->common.transfer_buffer_length = urb->transfer_buffer_length;
	req->common.actual_length = urb->actual_length;
	
	req->start_frame = urb->start_frame;
	req->number_of_packets = urb->number_of_packets;
	req->error_count = urb->error_count;

	for (i = 0; i < urb->number_of_packets; ++i) {
		req->frames[i].offset = urb->iso_frame_desc[i].offset;
		req->frames[i].length = urb->iso_frame_desc[i].length;
		req->frames[i].status = urb->iso_frame_desc[i].status;
	}

	if (buf_len) {
		void *buf = msgURBISORequest_buffer(req);
		memcpy(buf, urb->transfer_buffer, buf_len);
	}

	return 0;
}

/*
 * Calls from read() function of driver's character device.
 * message_up - data to send to userspace (daemon).
 *
 * Initializes msgUp->msg.
 */
int fill_message_kernel_to_daemon(struct message_up *msgUp)
{
	struct urb *urb = msgUp->urb;

	if (msgUp->msg || !msgUp->vDev || !urb) {
		return 0;
	}

	if (msgUp->vDev->shutdown) {
		return -ESHUTDOWN;
	}

	return usb_pipeisoc(urb->pipe) ? fill_isoc_msg(msgUp, urb) : fill_cbi_msg(msgUp, urb);
}

/*
 * Calls from write() function of driver's character device.
 * hdr - data received from userspace (daemon).
 */
void fill_message_daemon_to_kernel(struct MsgHeader *hdr)
{
	struct MsgURBResponse *resp = (struct MsgURBResponse*)hdr;
	struct message_up *msgUp = 0;
	struct urb *urb = 0;
	int status = 0;

	if (!hdr) {
		edev_err("Null message header");
		return;
	}

	if (hdr->length < sizeof(*hdr)) {
		edev_err("Message from daemon is too small %u < %zu", hdr->length, sizeof(*hdr));
		return;
	}

	if (hdr->cmd != MSG_CMD_CLIENT_URB) {
		eve_control_cmd(hdr);
		return;
	}

	if (resp->hdr.length < sizeof(*resp)) {
		edev_err("Message from daemon is too small %u < %zu", resp->hdr.length, sizeof(*resp));
		return;
	}

	urb = (struct urb*)((uintptr_t)resp->urb);
	if (!urb) {
		edev_err("Incorrect message from daemon");
		return;
	}

	msgUp = message_up_find(urb); // call message_up_put!
	if (!msgUp) {
		edev_err("Unexpected URB message %p from daemon", urb);
		return;
	}

	if (resp->actual_length <= urb->transfer_buffer_length) {

		void *data = 0;

		if (usb_pipeisoc(urb->pipe)) {
			data = process_isoc_response(urb, (struct MsgURBISOResponse*)resp);
			status = data ? 0 : resp->status;
		} else {
			data = msgURBResponse_buffer(resp);
			status = resp->status;
		}

		if (urb->transfer_buffer && data) {
			if (usb_pipein(urb->pipe)) {
				memcpy(urb->transfer_buffer, data, resp->actual_length);
			}
			urb->actual_length = resp->actual_length;
		} else {
			urb->actual_length = 0;
		}

	} else {
		edev_notice("URB message %p from daemon has too long buffer: %u > %u", urb, resp->actual_length, urb->transfer_buffer_length);
		status = -EMSGSIZE;
	}

	if (status == -EINPROGRESS) { // urb posted
		signal_message_ready(msgUp);
	} else {
		if (status) {
			edev_info("Completing URB %p with status %d", urb, status);
		}
		vDev_complete_urb(msgUp, urb, status);
		message_up_dequeue(msgUp); // FIXME: possible it's a bug, see message_up_dequeue
	}

	message_up_put(msgUp);
}
