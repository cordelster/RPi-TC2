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
#include "usb_descr.h"
//----------------------------------------------------------------------------

static const char *g_usb_recipients[] = { "DEVICE", "INTERFACE", "ENDPOINT", "OTHER", "PORT", "RPIPE" };

// USB_ENDPOINT_XFER_XXX from <linux/usb/ch9.h>
static const char *g_transfer_types[] = { "CTRL", "ISOC", "BULK", "INT" };

// PIPE_XXX from <linux/usb.h> constants mapping to USB_ENDPOINT_XFER_XXX.
static const int g_pipetype_map[] =
{ 
	USB_ENDPOINT_XFER_ISOC,
	USB_ENDPOINT_XFER_INT, 
	USB_ENDPOINT_XFER_CONTROL, 
	USB_ENDPOINT_XFER_BULK 
};

//----------------------------------------------------------------------------

static const char *g_desc_types[] = 
{
	"INVALID",
	"DT_DEVICE", // 1
	"DT_CONFIG",
	"DT_STRING",
	"DT_INTERFACE",
	"DT_ENDPOINT",
	"DT_DEVICE_QUALIFIER",
	"DT_OTHER_SPEED_CONFIG",
	"DT_INTERFACE_POWER"
};
//----------------------------------------------------------------------------

static const char* g_usb_speed_names[] = // indexed by enum usb_device_speed
{
	"unknown",
	"low",
	"full",
	"high",
	"wireless",
	"super"
};
//----------------------------------------------------------------------------

const char *get_speed_str(enum usb_device_speed speed)
{
	int idx = speed;
	int max_idx = sizeof(g_usb_speed_names)/sizeof(*g_usb_speed_names);

	return idx >= 0 && idx < max_idx ? g_usb_speed_names[idx] : "bad get_speed_str() argument";
}
//----------------------------------------------------------------------------

/*
	USB types, the second of three bRequestType fields.
*/
const char *get_usb_type(int typ)
{
	const char *s = 0;

	switch (typ) {
	case USB_TYPE_STANDARD:
		s = "STANDARD";
		break;
	case USB_TYPE_CLASS:
		s = "CLASS";
		break;
	case USB_TYPE_VENDOR:
		s = "VENDOR";
		break;
	case USB_TYPE_RESERVED:
		s = "RESERVED";
		break;
	default:
		s = "bad get_usb_type() argument";
	}

	return s;
}
//----------------------------------------------------------------------------

/*
	USB recipients, the third of three bRequestType fields
*/
const char *get_usb_recipient(int recip)
{
	int max_idx = sizeof(g_usb_recipients)/sizeof(*g_usb_recipients);
	return recip >= 0 && recip < max_idx ? g_usb_recipients[recip] : "bad get_usb_recipient() argument";
}
//----------------------------------------------------------------------------

const char *get_descr_type(int descr_type)
{
	int max_idx = sizeof(g_desc_types)/sizeof(*g_desc_types);
	const char *s = 0;

	if (descr_type > 0 && descr_type < max_idx) {
		s = g_desc_types[descr_type];
	} else {
		s = "bad get_descr_type() argument";
	}

	return s;
}
//----------------------------------------------------------------------------

static inline struct usb_interface_descriptor *next_descriptor(struct usb_interface_descriptor *d) 
{
	return d ? (struct usb_interface_descriptor*)((char*)d + d->bLength) : 0;
}
//----------------------------------------------------------------------------

/*
 * Including ones with alternate settings. 
 * Additionally it checks the descriptor's correctness.
 * Descriptors often follow each other in a rather chaotical way.
 *
 * Device can have interfaces with chaotical numbers.
 * For example, one real device has four interfaces with numbers #0, #2, #3, #8.
 */
int get_total_interfaces(const struct usb_config_descriptor *d)
{
	struct usb_interface_descriptor *id = 0;

	int total_len = 0;
	int len = 0;

	int ifcnt = 0;
	int ok = 0;

	if (!d) {
		return -1;
	}

	ok = d->bLength == USB_DT_CONFIG_SIZE &&
	     d->bDescriptorType == USB_DT_CONFIG &&
	     d->wTotalLength > d->bLength;

	if (!ok) {
		return -2;
	}

	total_len = d->wTotalLength; // FIXME: le16_to_cpu
	len = d->bLength;

	id = first_interface(d, 0);

	while (((char*)id - (char*)d) < total_len) {

		int ifsz = get_interface_size(id);

		if (ifsz) {
			++ifcnt;
			len += ifsz;
			id = next_interface(id);
		} else { // not an interface
			len += id->bLength;
			id = next_descriptor(id);
		}
	}

	if (ifcnt < d->bNumInterfaces) { // yes, not !=, Logitech C500 fails otherwise
		return -3;
	}

	if (len != d->wTotalLength) {
		return -4;
	}

	return ((char*)id - (char*)d) == total_len ? ifcnt : -5;
}
//----------------------------------------------------------------------------

/*
 * Returns zero on success.
 */
int eve_check_config(const struct usb_config_descriptor *d)
{
	int cnt = get_total_interfaces(d);
	return cnt > 0 ? 0 : cnt;
}
//----------------------------------------------------------------------------

int is_config(const struct usb_config_descriptor *d)
{
	return !eve_check_config(d);
}
//----------------------------------------------------------------------------

int is_interface(const struct usb_interface_descriptor *d)
{
	return d &&
	       d->bLength == USB_DT_INTERFACE_SIZE &&
	       d->bDescriptorType == USB_DT_INTERFACE;
}
//----------------------------------------------------------------------------

int is_endpoint(const struct usb_endpoint_descriptor *d)
{
	return d &&
	       (d->bLength == USB_DT_ENDPOINT_SIZE || d->bLength == USB_DT_ENDPOINT_AUDIO_SIZE) &&
	       d->bDescriptorType == USB_DT_ENDPOINT;
}
//----------------------------------------------------------------------------

struct usb_interface_descriptor *first_interface(const struct usb_config_descriptor *d, int check_descr)
{
	char *ptr = (char*)d;

	if (check_descr && !is_config(d)) {
		return 0;
	}

	ptr += d->bLength;
	return (struct usb_interface_descriptor *)ptr;
}
//----------------------------------------------------------------------------

struct usb_interface_descriptor *next_interface(const struct usb_interface_descriptor *d)
{
	struct usb_endpoint_descriptor *ep = first_endpoint(d);
	unsigned int i = 0;

	for (i = 0; ep && i < d->bNumEndpoints; i += is_endpoint(ep), ep = next_endpoint(ep)) {}

	return (struct usb_interface_descriptor *)ep;
}
//----------------------------------------------------------------------------

struct usb_endpoint_descriptor *first_endpoint(const struct usb_interface_descriptor *d)
{
	char *ptr = (char*)d;

	if (!is_interface(d)) {
		return 0;
	}

	ptr += d->bLength;
	return (struct usb_endpoint_descriptor*)ptr;
}
//----------------------------------------------------------------------------

/*
	The descriptor returned can be not only USB_DT_ENDPOINT, that's why
	is_endpoint should be called for check. Example:

	struct usb_endpoint_descriptor *ep =  first_endpoint(ifd);

	for (i = 0; ep && i < ifd->bNumEndpoints; ep = next_endpoint(ep)) {

		if (is_endpoint(ep)) {
			// use ep
			++i;
		}
	}
*/
struct usb_endpoint_descriptor *next_endpoint(const struct usb_endpoint_descriptor *d)
{
	char *ptr = (char*)d;
	ptr += d ? d->bLength : 0;
	return (struct usb_endpoint_descriptor*)ptr;
}
//----------------------------------------------------------------------------

struct usb_interface_descriptor *find_interface(
        const struct usb_config_descriptor *d, int num, int altnum)
{
	int cnt = get_total_interfaces(d);
	struct usb_interface_descriptor *iface = cnt > 0 ? first_interface(d, 0) : 0;
	int i = 0;

	for (i = 0; i < cnt && iface;) {

		if (!is_interface(iface)) {
			iface = next_descriptor(iface);
		} else if (iface->bInterfaceNumber == num && iface->bAlternateSetting == altnum) {
			return iface;
		} else {
			iface = next_interface(iface);
			++i;
		}
	}

	return 0;
}
//----------------------------------------------------------------------------

struct usb_endpoint_descriptor *find_endpoint(
        const struct usb_interface_descriptor *d, int EndpointAddress)
{
	struct usb_endpoint_descriptor *ed = first_endpoint(d);
	int i = 0;
	
	if (!d) {
		return 0;
	}
	
	for (i = 0; ed && i < d->bNumEndpoints; ed = next_endpoint(ed)) {

		int ok = is_endpoint(ed);
		i += ok;

		if (ok && ed->bEndpointAddress == EndpointAddress) {
			return ed;
		}
	}

	return 0;
}
//----------------------------------------------------------------------------

/*
	The interface can be followed not only by endpoint descriptors,
	but by other types of descriptors too.
*/
int get_interface_size(const struct usb_interface_descriptor *d)
{
	struct usb_endpoint_descriptor *ed = 0;
	unsigned int i = 0;
	int len = 0;

	if (!is_interface(d)) {
		return 0;
	}

	len = d->bLength;
	ed = first_endpoint(d);

	for (i = 0; ed && i < d->bNumEndpoints; i += is_endpoint(ed), ed = next_endpoint(ed)) {
		len += ed->bLength;
	}

	return len;
}
//----------------------------------------------------------------------------

int get_endpoint_num(int EndpointAddress)
{
	return EndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}
//----------------------------------------------------------------------------

/*
 * Returns USB_DIR_IN, USB_DIR_OUT.
 */
int get_endpoint_dir(int EndpointAddress)
{
	return EndpointAddress & USB_ENDPOINT_DIR_MASK;
}
//----------------------------------------------------------------------------

int is_endpoint_dir_in(int EndpointAddress)
{
	return get_endpoint_dir(EndpointAddress) == USB_DIR_IN;
}
//----------------------------------------------------------------------------

int is_endpoint_dir_out(int EndpointAddress)
{
	return get_endpoint_dir(EndpointAddress) == USB_DIR_OUT;
}
//----------------------------------------------------------------------------

/*
 * See: usb_endpoint_type.
 */
int get_transfer_type(int EndpointAttributes)
{
	return EndpointAttributes & USB_ENDPOINT_XFERTYPE_MASK;
}
//----------------------------------------------------------------------------

/*
 * transfer_type -> USB_ENDPOINT_XFER_XXX
 */
const char *get_transfer_type_str(int transfer_type)
{
	int max_idx = sizeof(g_transfer_types)/sizeof(*g_transfer_types);

	return transfer_type >= 0 && transfer_type < max_idx ? 
		g_transfer_types[transfer_type] : "bad get_transfer_type_str() argument";
}
//----------------------------------------------------------------------------

/*
 * PIPE_ISOCHRONOUS => USB_ENDPOINT_XFER_ISOC, etc.
 */
int map_transfer_type(int pipetype)
{
	int max_idx = sizeof(g_pipetype_map)/sizeof(*g_pipetype_map);

	return pipetype >= 0 && pipetype < max_idx ? g_pipetype_map[pipetype] : -1;
}
//----------------------------------------------------------------------------

/*
	Endpoint Address
	Bits 0..3b Endpoint Number.
	Bits 4..6b Reserved. Set to Zero
	Bits 7 Direction 0 = Out, 1 = In (Ignored for Control Endpoints)
*/
unsigned char make_endpoint_address(int epnum, int dir_in)
{
	return (epnum & USB_ENDPOINT_NUMBER_MASK) | (dir_in ? USB_DIR_IN : USB_DIR_OUT);
}
//----------------------------------------------------------------------------

/*
	See usb_submit_urb implementation.
*/
int is_request_dir_out(const struct usb_ctrlrequest *r)
{
	return !(r->bRequestType & USB_DIR_IN) || !r->wLength;
}
//----------------------------------------------------------------------------

int is_request_dir_in(const struct usb_ctrlrequest *r)
{
	return !is_request_dir_out(r);
}
//----------------------------------------------------------------------------

/*
 * wMaxPacketSize -> field of struct usb_endpoint_descriptor
 * 12 11 10 9 8 7 6 5 4 3 2 1 0 <- its bits
 * <-N-> <-MaximumPacketSize ->
 *
 * N is number of transactions per microframe, each transaction can transfer MaximumPacketSize bytes
 * MaximumPacketSize has 11 bits, [0..1024].
 * Thus, ISO packet descriptor's length is (N+1)*MaximumPacketSize.
 *
 * P.S. This code taken from implementation of usb_submit_urb:
 * if (xfertype == USB_ENDPOINT_XFER_ISOC) {
 */
int getMaximumPacketSize(enum usb_device_speed speed, int wMaxPacketSize)
{
	int max = wMaxPacketSize;

	if (speed == USB_SPEED_HIGH) {
		int mult = 1 + ((max >> 11) & 0x03); // bits 12..11 of wMaxPacketSize
		max &= 0x07ff; // bits 10..0 of wMaxPacketSize
		max *= mult; // (number of transactions per microframe + 1)*MaximumPacketSize
	}

	return max;
}
//----------------------------------------------------------------------------

