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
*   Copyright (C) 2009-2013 Eltima software
*   Author: 2009-2013 Vadim Grinchishin
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
#ifndef EVEUSB_USB_DEVICE_H
#define EVEUSB_USB_DEVICE_H
/*****************************************************************************/
struct device;
struct bus_type;
struct usb_device;
/*****************************************************************************/

int eveusb_iface_driver_init(void);
void eveusb_iface_driver_exit(void);

enum EDeviceType { DTYPE_UNKNOWN, DTYPE_USB_DEVICE, DTYPE_USB_INTERFACE };
enum EDeviceType get_device_type(struct device *dev);

struct bus_type *eve_get_usb_bus(void);
struct usb_device *eve_find_usb_device(const char *devname);

/*
 * Defined in trans.c
 */
int eve_usb_device_add_remove(struct usb_device *udev, _Bool removed);

/*****************************************************************************/
#endif // EVEUSB_USB_DEVICE_H
/*****************************************************************************/
