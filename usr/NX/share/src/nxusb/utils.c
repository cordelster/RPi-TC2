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
*   Copyright (C) 2010-2013 Eltima software
*   Author: 2010-2013 Vadim Grinchishin
*   License: GPLv3
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

#include "utils.h"
#include "usb_descr.h"
#include "crossapi.h"

#include <stdbool.h>
#include <linux/slab.h>

unsigned int eve_make_pipe(struct usb_device *dev, int eptype, int epnum, int dir_in)
{
	unsigned int pipe = -1U;
	epnum &= USB_ENDPOINT_NUMBER_MASK;

	switch (eptype) {
	case USB_ENDPOINT_XFER_ISOC:
		pipe = dir_in ? usb_rcvisocpipe(dev, epnum) : usb_sndisocpipe(dev, epnum);
		break;
	case USB_ENDPOINT_XFER_BULK:
		pipe = dir_in ? usb_rcvbulkpipe(dev, epnum) : usb_sndbulkpipe(dev, epnum);
		break;
	case USB_ENDPOINT_XFER_INT:
		pipe = dir_in ? usb_rcvintpipe(dev, epnum) : usb_sndintpipe(dev, epnum);
		break;
	case USB_ENDPOINT_XFER_CONTROL:
		pipe = dir_in ? usb_rcvctrlpipe(dev, epnum) : usb_sndctrlpipe(dev, epnum);
		break;
	}

	WARN_ON(pipe == -1U);
	WARN_ON(dir_in != (bool)usb_pipein(pipe));

	return pipe;
}

/*
	EndpointAddress - field of endpoint descriptor.
*/
struct usb_host_endpoint *eve_get_endpoint(struct usb_device *dev, int EndpointAddress) {
	struct usb_host_endpoint *ep = 0;

	int epnum = EndpointAddress &USB_ENDPOINT_NUMBER_MASK;
	bool dir_in = (EndpointAddress &USB_ENDPOINT_DIR_MASK) == USB_DIR_IN;

	if (dev && dev->ep_in[0] && dev->ep_out[0]) {
		WARN_ON(dev->ep_in[0] != &dev->ep0 || dev->ep_out[0] != &dev->ep0);
	} else {
		return 0;
	}

	if (epnum >= 0 && epnum < sizeof(dev->ep_in) / sizeof(dev->ep_in[0])) {
		ep = dir_in ? dev->ep_in[epnum] : dev->ep_out[epnum];
	} else {
		dev_err(to_devu(dev), "epnum %d out of range", epnum);
	}

	return ep;
}

unsigned int eve_pipe_from_desc(struct usb_device *dev, struct usb_endpoint_descriptor *epd)
{
	if (!dev || !epd) {
		return -1;
	}

	return eve_make_pipe(dev, usb_endpoint_type(epd), usb_endpoint_num(epd), usb_endpoint_dir_in(epd));
}

/*
 * EndpointAddress - field of endpoint descriptor.
 */
unsigned int eve_pipe_from_address(struct usb_device *dev, int EndpointAddress)
{
	struct usb_host_endpoint *hep = eve_get_endpoint(dev, EndpointAddress);
	unsigned int pipe = -1;

	if (hep) {
		pipe = eve_pipe_from_desc(dev, &hep->desc);
	} else {
		dev_err(to_devu(dev), "bad EndpointAddress %d", EndpointAddress);
	}

	return pipe;
}

struct usb_host_endpoint *eve_find_host_endpoint(struct usb_host_interface *hif, int EndpointAddress) {
	int i = 0;

	for (i = 0; hif && i < hif->desc.bNumEndpoints; ++i) {

		struct usb_host_endpoint *ep = &hif->endpoint[i];

		if (ep->desc.bEndpointAddress == EndpointAddress) {
			return ep;
		}
	}

	return 0;
}

/*
	Call usb_put_intf when you complete with returned interface.
*/
struct usb_interface *eve_get_iface_of_endpoint(struct usb_device *udev, int EndpointAddress) {
	struct usb_host_config *cfg = udev->actconfig;
	size_t i = 0;

	for (i = 0; cfg && i < cfg->desc.bNumInterfaces; ++i) {

		struct usb_interface *iface = cfg->interface[i];
		struct usb_host_interface *cur_alt = iface ? iface->cur_altsetting : 0;
		size_t j = 0;

		for (j = 0; cur_alt && j < cur_alt->desc.bNumEndpoints; ++j) {

			if (cur_alt->endpoint[j].desc.bEndpointAddress == EndpointAddress) {
				return usb_get_intf(iface);
			}
		}
	}

	return 0;
}

/*
	Endpoint zero (default control pipe) does not belong to any interface.
*/
int eve_get_ifnum_of_endpoint(struct usb_device *udev, int EndpointAddress)
{
	struct usb_interface *iface = eve_get_iface_of_endpoint(udev, EndpointAddress);
	int ifnum = -EINVAL;

	if (iface) {
		if (iface->cur_altsetting) {
			ifnum = iface->cur_altsetting->desc.bInterfaceNumber;
		}

		usb_put_intf(iface);
	} else {
		dev_err(to_devu(udev), "can't find interface of epaddr %d", EndpointAddress);
	}

	return ifnum;
}

/*
        Returns number of nodes in the list.
*/
size_t get_list_size(const struct list_head *head)
{
	struct list_head *pos = 0;
	size_t cnt = 0;

	if (head) {
		for (pos = head->next; pos != head; pos = pos->next, ++cnt);
	}

	return cnt;
}

/*
	Endpoint Address
	Bits 0..3b Endpoint Number.
	Bits 4..6b Reserved. Set to Zero
	Bits 7 Direction 0 = Out, 1 = In (Ignored for Control Endpoints)
*/
unsigned char eve_epaddr_from_pipe(unsigned int pipe)
{
	unsigned int epaddr = usb_pipeendpoint(pipe);
	epaddr |= (usb_pipein(pipe) ? USB_DIR_IN : USB_DIR_OUT);
	return (unsigned char)epaddr;
}

/*
 * kfree returned descriptor!
 */
int read_config_descriptor(struct usb_config_descriptor **cd, struct usb_device *udev, unsigned char index)
{
	int cd_len = 0;
	int err = 0;
	*cd = 0;

	{
		struct usb_config_descriptor d = { .bLength = 0 };
		err = usb_get_descriptor(udev, USB_DT_CONFIG, index, &d, USB_DT_CONFIG_SIZE);
		WARN_ON(sizeof(d) < USB_DT_CONFIG_SIZE);

		if (err == USB_DT_CONFIG_SIZE) {
			cd_len = le16_to_cpu(d.wTotalLength);
		} else {
			return err;
		}
	}

	*cd = kzalloc(cd_len, GFP_KERNEL); // memory must be physically contiguous
	if (!*cd) {
		WARN_ON("not enough memory");
		return -ENOMEM;
	}

	err = usb_get_descriptor(udev, USB_DT_CONFIG, index, *cd, cd_len);
	if (err == cd_len) {
		return 0;
	}

	kfree(*cd);
	*cd = 0;

	return err;
}

static int get_num_interfaces(struct usb_device *udev)
{
	int cnt = 0;

	if (udev->actconfig) {
		cnt = udev->actconfig->desc.bNumInterfaces;
	} else {
		struct usb_config_descriptor cd = { .bLength = 0 };
		unsigned char config_index = 0;

		int bytes = usb_get_descriptor(udev, USB_DT_CONFIG, config_index, &cd, USB_DT_CONFIG_SIZE);

		WARN_ON(sizeof(cd) < USB_DT_CONFIG_SIZE);
		WARN_ON(udev->descriptor.bNumConfigurations != 1);

		cnt = bytes == USB_DT_CONFIG_SIZE ? cd.bNumInterfaces : 0;
	}

	return cnt;
}

/*
	A device that has multiple interfaces controlled independently of each other is referred
	to as a composite device.

	The bus driver also reports a compatible identifier (ID) of USB\COMPOSITE, if the device
	meets the following requirements:

	* The device class field of the device descriptor (bDeviceClass) must contain a value of zero,
	  or the class (bDeviceClass), subclass (bDeviceSubClass), and protocol (bDeviceProtocol) fields
	  of the device descriptor must have the values 0xEF, 0x02 and 0x01 respectively, as explained
	  in USB Interface Association Descriptor.

	* The device must have multiple interfaces.
	* The device must have a single configuration.
*/
int is_composite_device(struct usb_device *udev)
{
	struct usb_device_descriptor *dd = &udev->descriptor;

	bool ok = !dd->bDeviceClass || // generic composite device
	          (dd->bDeviceClass == 0xEF &&
	           dd->bDeviceSubClass == 0x02 &&
	           dd->bDeviceProtocol == 0x01); // IAD composite device

	return ok && dd->bNumConfigurations == 1 && get_num_interfaces(udev) > 1;
}

int get_xfer_type(unsigned int pipe)
{
	int xfer = -1;

	switch (usb_pipetype(pipe)) {
	case PIPE_ISOCHRONOUS:
		xfer = USB_ENDPOINT_XFER_ISOC;
		break;
	case PIPE_BULK:
		xfer = USB_ENDPOINT_XFER_BULK;
		break;
	case PIPE_CONTROL:
		xfer = USB_ENDPOINT_XFER_CONTROL;
		break;
	case PIPE_INTERRUPT:
		xfer = USB_ENDPOINT_XFER_INT;
		break;
	}

	WARN_ON(xfer == -1);
	return xfer;
}
