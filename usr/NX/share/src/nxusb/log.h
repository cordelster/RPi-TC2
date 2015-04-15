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
#ifndef EVEUSB_LOG_H
#define EVEUSB_LOG_H
/*****************************************************************************/
#include <linux/kernel.h> // <linux/printk.h> doesn't exist on RedHat
/*****************************************************************************/

/*
 * Useful when you don't have device* and can't use dev_err, dev_dbg, etc.
 */
#define edev_crit(  FORMAT, ARGS ...) printk(KERN_CRIT    KBUILD_MODNAME ": " FORMAT "\n", ##ARGS)
#define edev_err(   FORMAT, ARGS ...) printk(KERN_ERR     KBUILD_MODNAME ": " FORMAT "\n", ##ARGS)
#define edev_warn(  FORMAT, ARGS ...) printk(KERN_WARNING KBUILD_MODNAME ": " FORMAT "\n", ##ARGS)
#define edev_notice(FORMAT, ARGS ...) printk(KERN_NOTICE  KBUILD_MODNAME ": " FORMAT "\n", ##ARGS)
#define edev_info(  FORMAT, ARGS ...) printk(KERN_INFO    KBUILD_MODNAME ": " FORMAT "\n", ##ARGS)

/*****************************************************************************/

extern _Bool eveusb_debug;

/*
 * Enable debub logging:
 * echo y | sudo tee /sys/bus/usb/drivers/eveusb/module/parameters/eveusb_debug
 * 
 * To print messages during module initialization, load module with enabled parameter:
 * sudo modprobe eveusb eveusb_debug=y
 */
#define edev_dbg(FORMAT, ARGS ...)		\
do { 						\
	if (eveusb_debug)			\
		edev_notice(FORMAT, ##ARGS);	\
} while (0);

/*****************************************************************************/
#endif // EVEUSB_LOG_H
/*****************************************************************************/
