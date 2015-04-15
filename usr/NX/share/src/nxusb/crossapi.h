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
*   Copyright (C) 2010-2014 Eltima software
*   Author: 2010-2014 Vadim Grinchishin
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
#ifndef EVEUSB_CROSSAPI_H
#define EVEUSB_CROSSAPI_H
/*****************************************************************************/
/*
 * Linux kernel API stable across different kernel versions.
 */
/*****************************************************************************/
#include <linux/version.h>
#include <linux/compiler.h> // __aligned
#include <linux/completion.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
/*****************************************************************************/

/*
 * Kernel version 2.6.18 is too old, too many changes happened inside kernel:
 * => struct urb_anchor appeared in 2.6.23
 * => urb.ep missing
 * => usb_complete_t differs in arguments
 * => and many, many, many others incompatibilities
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
  #error Kernels older than 2.6.18 are not supported.
#endif

/*****************************************************************************/

/*
 * RedHat Enterprise Linux detection helper.
 * RHEL defines its version macros in <linux/version.h>
 *
 * All macros in the expression are expanded before actual computation
 * of the expression's value begins. 
 *
 * Thus, it should be defined anyway. This code will not compile:
 * #if !defined(RHEL_RELEASE_VERSION) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,1)
 */
#ifndef RHEL_RELEASE_VERSION
  #define RHEL_RELEASE_VERSION(a,b) 0
#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
#if !defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(5,3)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

static inline int usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}

static inline int usb_endpoint_dir_out(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

static inline struct usb_hcd *bus_to_hcd(struct usb_bus *bus)
{
	return container_of(bus, struct usb_hcd, self);
}

#endif // RHEL
#endif // 2.6.19

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
  #include <linux/kernel.h>
#else
  #include <linux/log2.h> // roundup_pow_of_two
#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)

#define __aligned(x)                    __attribute__((aligned(x)))

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)

typedef unsigned long           uintptr_t;

static inline int usb_endpoint_type(const struct usb_endpoint_descriptor *epd)
{
	return epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
}

static inline int usb_endpoint_num(const struct usb_endpoint_descriptor *epd)
{
	return epd->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)

static inline int match_name(struct device *dev, void *data)
{
	const char *name = data;

	if (strcmp(name, dev->bus_id) == 0)
		return 1;

	return 0;
}

static inline struct device *bus_find_device_by_name(struct bus_type *bus, struct device *start, const char *name) {
	return bus_find_device(bus, start, (void *)name, match_name);
}

static inline int wait_for_completion_killable(struct completion *x)
{
	wait_for_completion(x);
	return 0;
}

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)

/* time_is_before_jiffies(a) return true if a is before jiffies */
#define time_is_before_jiffies(a) time_after(jiffies, a)

#if !defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(5,9)
static inline const char *dev_name(const struct device *dev)
{
	/* will be changed into kobject_name(&dev->kobj) in the near future */
	return dev->bus_id;
}
#endif // RHEL

#include <asm/semaphore.h>

static inline int __must_check down_killable(struct semaphore *sem)
{
	down(sem);
	return 0;
}

#else
  #include <linux/semaphore.h>
#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#if !defined(RHEL_RELEASE_CODE) || RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(5,9)

#define WARN(condition, format...) WARN_ON(condition)

#endif // RHEL
#endif // 2.6.27

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)

#define HCD_USB3        0x0040          /* USB 3.0 */
#define HCD_MASK        0x0070

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)

static inline unsigned long __sched
wait_for_completion_killable_timeout(struct completion *x, unsigned long timeout)
{
	return wait_for_completion_killable(x);
}

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)

#define DEFINE_SEMAPHORE DECLARE_MUTEX
#define HCD_HW_ACCESSIBLE(hcd)  ((hcd)->flags & (1U << HCD_FLAG_HW_ACCESSIBLE))

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)

#define USB_SS_CAP_TYPE         3
#define USB_DT_BOS_SIZE         5
#define USB_DT_USB_SS_CAP_SIZE  10
#define USB_5GBPS_OPERATION             (1 << 3) /* Operation at 5Gbps */

/*
// RedHat applies patches from future versions of kernel
#if !defined(RHEL_RELEASE_CODE) || (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,1))

#include <asm/pgtable.h> // PAGE_KERNEL

static inline void *vzalloc(unsigned long size)
{
	return __vmalloc(size, GFP_KERNEL | __GFP_HIGHMEM | __GFP_ZERO, PAGE_KERNEL);
}

#endif // RHEL
*/

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)

#define USB_DT_SS_HUB                   (USB_TYPE_CLASS | 0x0a)
#define USB_DT_SS_HUB_SIZE              12

#endif

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
  #include <linux/module.h>
#else
  #include <linux/export.h> // THIS_MODULE moved here
#endif

/*****************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0)
static inline void reinit_completion(struct completion *x) // was macro INIT_COMPLETION
{
	x->done = 0;
}
#endif
/*****************************************************************************/
#ifndef RHEL_RELEASE_CODE
  #undef RHEL_RELEASE_VERSION
#endif
/*****************************************************************************/
#endif // EVEUSB_CROSSAPI_H
/*****************************************************************************/

