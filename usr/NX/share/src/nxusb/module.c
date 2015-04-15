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
*   Copyright (C) 2008-2013 Eltima software
*
*   Author: 2008-2010 Kuzma Shapran <Kuzma[dot]Shapran[at]gmail[dot]com>
*   Author: 2009-2013 Vadim Grinchishin
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
#include "platform.h"
#include "dev.h"
#include "message_queue.h"
#include "vhub.h"
#include "usb_device.h"
#include "log.h"

#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * What does it mean for a module to be tainted?
 * 
 * (REG, contributed by John Levon) Some vendors distribute binary modules (i.e. modules without 
 * available source code under a free software license). As the source is not freely available, 
 * any bugs uncovered whilst such modules are loaded cannot be investigated by the kernel hackers.
 * All problems discovered whilst such a module is loaded must be reported to the vendor of that module,
 * not the Linux kernel hackers and the linux-kernel mailing list. The tainting scheme is used 
 * to identify bug reports from kernels with binary modules loaded: such kernels are marked as "tainted" 
 * by means of the MODULE_LICENSE tag. If a module is loaded that does not specify an approved license, 
 * the kernel is marked as tainted. The canonical list of approved license strings is 
 * in linux/include/linux/module.h.
 * "oops" reports marked as tainted are of no use to the kernel developers and will be ignored. 
 * A warning is output when such a module is loaded. Note that you may come across module source 
 * that is under a compatible license, but does not have a suitable MODULE_LICENSE tag. 
 * If you see a warning from modprobe or insmod for a module under a compatible license, 
 * please report this bug to the maintainers of the module, so that they can add the necessary tag.
 *
 * (KO) If a symbol has been exported with EXPORT_SYMBOL_GPL then it appears as unresolved for modules 
 * that do not have a GPL compatible MODULE_LICENSE string, and prints a warning. 
 * A module can also taint the kernel if you do a forced load. This bypasses the kernel/module 
 * verification checks and the result is undefined, when it breaks you get to keep the pieces.  
 */

MODULE_AUTHOR("NoMachine Sarl");
MODULE_LICENSE("GPL");
MODULE_VERSION("4.3.b1");
MODULE_DESCRIPTION("NoMachine USB Service");

enum {
	MSG_QUEUE = 1,
	PLATFORM_DRIVER = 2,
	PLATFORM_DEVICE = 4,
	DESTROY_ALL = 8,
	CHRDEV = 16,
	INTF_DRIVER = 32
};

void eve_destroy_all(void)
{
	vhub_signal_shutdown();
	vDev_destroy_all();
	message_up_dequeue_all();
}

/*
	messageUp can be not read, e.g. when daemon crashes. They can contain URBs,
	and each URB increased the eveusb_device links counter,thus the device
	is not released after on_disconnect. This interferes giving away the device to the system and also
	leads to memory leaks.

	If daemon crashes, kernel closes device file, which should result in deallocating of
	message_up queue. In any case eveusb_driver_exit should be called after
	eve_destroy_all, so that it would release all resources and devices' interfaces for sure.
*/
static void do_module_exit(unsigned long flags)
{
	if (flags & CHRDEV) {
		dev_dtor();
	}

	if (flags & DESTROY_ALL) {
		eve_destroy_all();
	}

	if (flags & INTF_DRIVER) {
		eveusb_iface_driver_exit(); // must be called after eve_destroy_all
	}

	if (flags & PLATFORM_DEVICE) {
		platform_device_unregister(get_eveusbhcd_platform_device());
	}

	if (flags & PLATFORM_DRIVER) {
		platform_driver_unregister(get_eveusbhcd_platform_driver());
	}

	if (flags & MSG_QUEUE) {
		message_queue_exit();
	}
}

static int __init eveusbhcd_module_init(void)
{
	unsigned long flags = 0;
	int err = 0;

	edev_notice("starting");

	do {
		if (usb_disabled()) {
			edev_crit("USB subsystem disabled");
			err = -ENODEV;
			break;
		}

		err = message_queue_init();
		if (!err) {
			flags |= MSG_QUEUE;
		} else {
			break;
		}

		err = platform_driver_register(get_eveusbhcd_platform_driver());
		if (!err) {
			flags |= PLATFORM_DRIVER;
		} else {
			edev_crit("cannot register platform driver, error %d", err);
			break;
		}

		err = platform_device_register(get_eveusbhcd_platform_device());
		if (!err) {
			flags |= PLATFORM_DEVICE | DESTROY_ALL;
		} else {
			edev_crit("cannot register platform device, error =%d", err);
			break;
		}

		err = dev_ctor();
		if (!err) {
			flags |= CHRDEV;
		} else {
			break;
		}

		err = eveusb_iface_driver_init();
		if (!err) {
			flags |= INTF_DRIVER;
		} else {
			edev_crit("cannot register interface driver, error %d", err);
			break;
		}

		edev_notice("started");
		
	} while (0);

	if (err) {
		do_module_exit(flags);
	}

	return err;
}

static void __exit eveusbhcd_module_exit(void)
{
	edev_notice("finishing");
	do_module_exit(-1);
	edev_notice("finished");
}

module_init(eveusbhcd_module_init);
module_exit(eveusbhcd_module_exit);

