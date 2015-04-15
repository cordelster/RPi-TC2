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

#include "usb_device.h"
#include "log.h"
#include "crossapi.h"

#include <stdbool.h>
#include <linux/module.h> // MODULE_DEVICE_TABLE compiles without warning
#include <linux/usb.h>

/*
 * From driver.h:
 * Note that those functions are likely to be called with the device semaphore
 * held in the core, so be careful.
 *
 * See also: BUS_NOTIFY_BOUND_DRIVER, BUS_NOTIFY_UNBIND_DRIVER, BUS_NOTIFY_UNBOUND_DRIVER.
 */
int on_eveusb_notify(struct notifier_block *self, unsigned long action, void *ptr)
{
	struct usb_device *udev = ptr;
	struct usb_bus *bus = ptr;

	int res = NOTIFY_OK;

	if (!ptr) {
		return res;
	}

	switch (action) {
	case USB_DEVICE_ADD:
		eve_usb_device_add_remove(udev, false);
		break;
	case USB_DEVICE_REMOVE:
		eve_usb_device_add_remove(udev, true);
		break;
	case USB_BUS_ADD:
		edev_info("bus %s added", bus->bus_name);
		eve_usb_device_add_remove(bus->root_hub, false);
		break;
	case USB_BUS_REMOVE:
		edev_info("bus %s removed", bus->bus_name);
		eve_usb_device_add_remove(bus->root_hub, true);
		break;
	}

	return res;
}

struct notifier_block eveusb_notifier_block = 
{
	.notifier_call = on_eveusb_notify
};

/*
 * Call usb_put_dev when you finish with returned *usb_device.
 */
struct usb_device *eve_find_usb_device(const char *devname) 
{
	struct bus_type *bus = eve_get_usb_bus();

	struct device *dev = devname ? bus_find_device_by_name(bus, 0, devname) : 0;
	struct usb_device *udev = dev ? to_usb_device(dev) : 0; // on USB bus all devices are usb_device

	return udev;
}

/*
 * struct device_type in 2.6.21 doesn't have member "name" and is absent in older kernels.
 */
enum EDeviceType get_device_type(struct device *dev)
{
	const char *name = 0;
	enum EDeviceType typ = DTYPE_UNKNOWN;

	if (!dev) {
		return typ;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,21)

	name = dev->type ? dev->type->name : 0;

	if (!strcmp(name, "usb_interface")) {
		typ = DTYPE_USB_INTERFACE;
	} else if (!strcmp(name, "usb_device")) {
		typ = DTYPE_USB_DEVICE;
	} else {
		dev_err(dev, "unexpected type->name %s", name);
	}
#else
	name = dev_name(dev);

	if (!name) {
		dev_err(dev, "dev_name() => null");
	} else if (strchr(name, ':')) { // usb_interface name is hub-port[.port]:#cfg.#if
		typ = DTYPE_USB_INTERFACE;
	} else {
		typ = DTYPE_USB_DEVICE; // usb_device name is hub-port[.port]
	}
#endif

	return typ;
}

/*
 * probe() routines may install different altsettings and
 * may claim() any interfaces not yet bound.  Many class
 * drivers need that: CDC, audio, video, etc.

 * Accept or decline an interface. If you accept the device return 0,
 * otherwise -ENODEV or -ENXIO. Other error codes should be used only if a
 * genuine error occurred during initialisation which prevented a driver
 * from accepting a device that would else have been accepted.
 */
static int on_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	return -ENODEV;
}

static void on_disconnect(struct usb_interface *iface)
{
//	usb_set_intfdata(iface, 0);
}

/*
* Table of devices that work with this driver.
* Identifies USB devices for probing and hotplugging.
* Driver will be called for every USB device in the system.
*/
static const struct usb_device_id eveusb_id_table[] = 
{
	{ .driver_info = 1 },
	{}
};
MODULE_DEVICE_TABLE(usb, eveusb_id_table);

static struct usb_driver eveusb_driver =
{
	.name =	"nxusb",
	.probe = on_probe,
	.disconnect = on_disconnect,
	.id_table = eveusb_id_table
};

/*
 * FIXME: can't find another way to get usb_bus_type.
 * drivers/usb/core/driver.c, usb_bus_type didn't exported,
 * EXPORT_SYMBOL_GPL(usb_bus_type) missed.
 */
struct bus_type *eve_get_usb_bus(void)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18)
	struct bus_type *usb = eveusb_driver.drvwrap.driver.bus;
#else
	struct bus_type *usb = eveusb_driver.driver.bus;
#endif

	WARN_ON(!usb);
	return usb;
}

int eveusb_iface_driver_init(void)
{
	int err = usb_register(&eveusb_driver);
	if (err) {
		edev_err("usb_register error %d", err);
		return err;
	}

	usb_register_notify(&eveusb_notifier_block); // returns void
	return 0;
}

void eveusb_iface_driver_exit(void)
{
	usb_unregister_notify(&eveusb_notifier_block); // bus_unregister_notifier(eve_get_usb_bus(), &eveusb_notifier_block);
	usb_deregister(&eveusb_driver);
}
