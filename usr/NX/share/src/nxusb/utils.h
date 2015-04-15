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
#ifndef EVEUSB_KERNEL_UTILS_H
#define EVEUSB_KERNEL_UTILS_H
/*****************************************************************************/
#include <linux/types.h>
#include <linux/usb.h>
/*****************************************************************************/

static inline struct device *to_devu(struct usb_device *udev)
{
	return udev ? &udev->dev : 0;
}

static inline struct device *to_devi(struct usb_interface *intf) 
{
	return intf ? &intf->dev : 0;
}

unsigned int eve_make_pipe(struct usb_device *dev, int eptype, int epnum, int dir_in);
unsigned int eve_pipe_from_address(struct usb_device *dev, int EndpointAddress);
unsigned int eve_pipe_from_desc(struct usb_device *dev, struct usb_endpoint_descriptor *epd);
unsigned char eve_epaddr_from_pipe(unsigned int pipe);

struct usb_host_endpoint *eve_find_host_endpoint(struct usb_host_interface *hif, int EndpointAddress);
struct usb_host_endpoint *eve_get_endpoint(struct usb_device *dev, int EndpointAddress);

struct usb_interface *eve_get_iface_of_endpoint(struct usb_device *udev, int EndpointAddress);
int eve_get_ifnum_of_endpoint(struct usb_device *udev, int EndpointAddress);

int is_composite_device(struct usb_device *udev);
int read_config_descriptor(struct usb_config_descriptor **cd, struct usb_device *udev, unsigned char index);

int get_xfer_type(unsigned int pipe);

size_t get_list_size(const struct list_head *head);

static inline int get_refcount(const struct kref *ref)
{
	return ref ? atomic_read(&ref->refcount) : 0;
}

/*****************************************************************************/
#endif // EVEUSB_KERNEL_UTILS_H
/*****************************************************************************/

