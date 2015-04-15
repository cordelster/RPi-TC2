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
*   Author: 2011-2013 Vadim Grinchishin
*
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

#include "dev.h"
#include "crossapi.h"
#include "module.h"
#include "log.h"
#include "message_queue.h"
#include "trans.h"
#include "vhub.h"
#include "kmsg_decl.h"
#include "platform.h"

#include <stdbool.h>

#include <linux/gfp.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

static atomic_t evechardev_available = ATOMIC_INIT(1);

/*
 * offset is a pointer to a “long offset type” object that indicates the file position the user is accessing.
 */
static ssize_t evechardev_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
	ssize_t copied = 0;

	int err = 0;
	size_t bytes_read = 0;
	const struct MsgHeader *hdr = 0;

	// might_sleep();

	struct message_up *msgUp = message_up_read_submitted(); // call message_up_put!
	if (!msgUp) {
		return copied;
	}

	bytes_read = msgUp->bytes_read;

	err = fill_message_kernel_to_daemon(msgUp); // initializes msgUp->msg
	hdr = err ? 0 : msgUp->msg;

	if (!hdr) {
		WARN_ON(!err);
		WARN_ON(bytes_read);

		if (msgUp->urb) {
			vDev_complete_urb(msgUp, msgUp->urb, err);
		}

		message_up_dequeue_force(msgUp);
		goto quit;
	}

	// first call reads MsgHeader
	WARN(!bytes_read && length != sizeof(*hdr), "length %zu != sizeof(MsgHeader)", length);

	// second call reads rest of message
	WARN(bytes_read && length != (hdr->length - bytes_read),
	     "length(%zu) != (message->length(%u) - bytes_read(%zu))", length, hdr->length, bytes_read);

	if (bytes_read < hdr->length) {
		copied = min(hdr->length - bytes_read, length);
		copied -= copy_to_user(buffer, (uint8_t*)(hdr) + bytes_read, copied); // returns number of bytes that could not be copied
		msgUp->bytes_read += copied;
	}

	if (msgUp->bytes_read >= hdr->length) {

		WARN_ON(msgUp->bytes_read != hdr->length);

		if (msgUp->urb) {
			message_up_dequeue_submitted(msgUp);
		} else {
			message_up_dequeue_force(msgUp);
		}
	}

quit:
	message_up_put(msgUp);
	return copied;
}

/*
 * vmalloc instead of kmalloc causes bug: unable to handle kernel paging request at ...
 * for isoc devices.
 */
static ssize_t evechardev_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
	ssize_t copied = 0;

	size_t ksize = 0;
	size_t usage_cnt = 0;
	struct MsgHeader *hdr = 0;

	// might_sleep();
	
	if (length < sizeof(*hdr) || length > MAX_MSG_HDR_LEN) {
		edev_err("%s: invalid message size: sizeof(MsgHeader) %zu <= %zu <= MAX_MSG_HDR_LEN(%d)",
				__func__, sizeof(*hdr), length, MAX_MSG_HDR_LEN);

		return -EINVAL;
	}

	hdr = eve_alloc_msg_hdr(length, GFP_KERNEL); // do not zero memory, copy_from_user will overwrite it

	if (hdr) {
		ksize = hdr->ksize;
		usage_cnt = hdr->usage_cnt;
	} else {
		return -ENOMEM;
	}

	if (copy_from_user(hdr, buffer, length)) { // overwrites hdr->ksize & usage_cnt
		edev_err("%s: copy_from_user(%zu) failed", __func__, length);
		copied = -EACCES;
	} else if (hdr->length != length) {
		edev_err("%s: incorrect message from daemon. %zu != %u", __func__, length, hdr->length);
		copied = -EINVAL;
	} else {
		fill_message_daemon_to_kernel(hdr);
		copied = length;
		WARN_ON(copied != length);
	}

	hdr->ksize = ksize;
	hdr->usage_cnt = usage_cnt;

	eve_free_msg_hdr(hdr);
	
	return copied;
}

static bool is_device_opened(void)
{
	int i = 0;
	for ( ; i < 3; ++i) {
		if (atomic_read(&evechardev_available) == 0) {
			return true;
		}
		mdelay(50);
	}

	return false;
}

static unsigned int evechardev_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &message_up_submitted_wait, wait); // see message_up_enqueue
	
	if (message_up_has_submitted()) {
		return POLLIN | POLLRDNORM;
	}
	
	return is_device_opened() ? 0 : POLLERR | POLLHUP;
}

/*
 * The device is single-open.
 * See: Linux Device Drivers, Access Control on a Device File, Single-Open Devices.
 */
static int evechardev_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct VHub *vhub = hcd_to_vhub(get_eveusb_hcd());

	if (!atomic_dec_and_test(&evechardev_available)) { // true if the result is 0
		err = -EBUSY; // already opened
	} else if (!vhub) {
		edev_err("%s: platform driver did not registered", __func__);
	} else if (vhub->shutdown) {
		vhub->shutdown = 0; // see eve_platform_driver_probe
	} else {
		err = -EBUSY;
		edev_err("%s: hub not in shutdown state on character device open", __func__); // should never happen
	}
		
	if (err) {
		atomic_inc(&evechardev_available);
	}

	return err;
}

/*
 * Device's file closed, its reference count is zero.
 */
static int evechardev_release(struct inode *inode, struct file *file)
{
	eve_destroy_all();
	wake_up(&message_up_submitted_wait);

	atomic_inc(&evechardev_available); // can be opened again
	return 0;
}

static struct file_operations evechardev_fops =
{
	.owner   = THIS_MODULE,
	.read    = evechardev_read,
	.write   = evechardev_write,
	.poll    = evechardev_poll,
	.open    = evechardev_open,
	.release = evechardev_release,
};

static struct miscdevice eveusb_device =
{
  /* NX - Change names */
	.minor = MISC_DYNAMIC_MINOR,
	.name = "nxusb",
	.fops = &evechardev_fops,
	.list = LIST_HEAD_INIT(eveusb_device.list),

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	.nodename = "nxusb" // the same as name
#elif LINUX_VERSION_CODE == KERNEL_VERSION(2,6,31)
	.devnode = "nxusb"
#endif
  /* NX */ 
};

int dev_ctor(void)
{
	int err = misc_register(&eveusb_device);

	if (err) {
		edev_err("misc_register error %d", err);
	} else {
		edev_info("%s char device %u:%u", eveusb_device.name, MISC_MAJOR, eveusb_device.minor);
	}

	return err;
}

void dev_dtor(void)
{
	int err = misc_deregister(&eveusb_device);

	if (err) {
		edev_err("misc_deregister error %d", err);
	}
}

