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
*   Copyright (C) 2009-2014 Eltima software
*   Author: 2009-2014 Vadim Grinchishin
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
#ifndef EVEUSB_KERNEL_USB_DESCR_H
#define EVEUSB_KERNEL_USB_DESCR_H
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------
#include "ch9.h"
//----------------------------------------------------------------------------

/*
 * 16 IN and 16 OUT per configuration, kUSBMaxPipes. 
 * See: drivers/usb/core/config.c
 */
#ifndef USB_MAXENDPOINTS
    enum { USB_MAXENDPOINTS = 30 };
#endif
//----------------------------------------------------------------------------
int get_total_interfaces(const struct usb_config_descriptor *d);
int eve_check_config(const struct usb_config_descriptor *d);
int is_config(const struct usb_config_descriptor *d);
int is_interface(const struct usb_interface_descriptor *d);
int is_endpoint(const struct usb_endpoint_descriptor *d);
//----------------------------------------------------------------------------
const char *get_descr_type(int descr_type);
const char *get_usb_type(int typ);
const char *get_usb_recipient(int recip);
const char *get_speed_str(enum usb_device_speed speed);
//----------------------------------------------------------------------------
int map_transfer_type(int pipetype);
int get_transfer_type(int EndpointAttributes);
const char *get_transfer_type_str(int transfer_type);
//----------------------------------------------------------------------------
unsigned char make_endpoint_address(int epnum, int dir_in);
int get_endpoint_num(int EndpointAddress);
int get_endpoint_dir(int EndpointAddress);
int is_endpoint_dir_in(int EndpointAddress);
int is_endpoint_dir_out(int EndpointAddress);
//----------------------------------------------------------------------------
int is_request_dir_in(const struct usb_ctrlrequest *r);
int is_request_dir_out(const struct usb_ctrlrequest *r);
//----------------------------------------------------------------------------

struct usb_interface_descriptor *first_interface(const struct usb_config_descriptor *d, int check_descr);
struct usb_interface_descriptor *next_interface(const struct usb_interface_descriptor *d);

struct usb_endpoint_descriptor *first_endpoint(const struct usb_interface_descriptor *d);
struct usb_endpoint_descriptor *next_endpoint(const struct usb_endpoint_descriptor *d);
//----------------------------------------------------------------------------
struct usb_interface_descriptor *find_interface(const struct usb_config_descriptor *d, int num, int altnum);
struct usb_endpoint_descriptor *find_endpoint(const struct usb_interface_descriptor *d, int EndpointAddress);
//----------------------------------------------------------------------------
int get_interface_size(const struct usb_interface_descriptor *d);
int getMaximumPacketSize(enum usb_device_speed speed, int wMaxPacketSize);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif // EVEUSB_KERNEL_USB_DESCR_H
//----------------------------------------------------------------------------
