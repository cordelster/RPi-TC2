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

#ifndef EVEUSB_VHUB_H
#define EVEUSB_VHUB_H

#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/usb.h>

#include <stdbool.h>
#include "kmsg_types.h"

struct usb_hcd;
struct usb_host_endpoint;
struct urb;
struct message_up;

enum { VHUB_NUM_PORTS = 15 }; // <= USB_MAXCHILDREN, linux/usb/ch11.h

struct VHub
{
	spinlock_t lock;

	struct VDevice *port_device[VHUB_NUM_PORTS];
	uint32_t        port_status[VHUB_NUM_PORTS];

	unsigned int shutdown: 1;
	unsigned int resuming: 1;
	unsigned long re_timeout;
};


struct VHub *hcd_to_vhub(struct usb_hcd *hcd);
void vhub_signal_shutdown(void);

struct hc_driver *get_eveusb_hc_driver(void);


struct VDevice
{
	struct list_head list;

	dev_id_t id;
	enum usb_device_speed speed;

	int port_number;
	int32_t address;

	struct usb_hcd *parent;
	unsigned int shutdown: 1;
};


void vDev_add_to_list(struct VDevice*);
void vDev_del_from_list(struct VDevice*);

int vDev_create(struct VDevice **pDev, dev_id_t id, enum usb_device_speed speed);
void vDev_start(void);

int  vDev_destroy(struct VDevice*);
void vDev_destroy_all(void);

struct VDevice *get_vDev_by_address(struct usb_hcd *, int);
struct VDevice *get_vDev_by_id(dev_id_t id);
struct VDevice *findDevice(struct urb *urb, bool clear_hcpriv);

void vDev_complete_urb(struct message_up *msg, struct urb *urb, int status);
int get_hcd_speed(const struct usb_hcd *hcd);

#endif // EVEUSB_VHUB_H
