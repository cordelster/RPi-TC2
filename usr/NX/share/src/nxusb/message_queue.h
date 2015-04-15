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
*   Author: 2010-2013 Vadim Grinchishin
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

#ifndef EVEUSB_MESSAGE_QUEUE_H
#define EVEUSB_MESSAGE_QUEUE_H

#include <linux/list.h>
#include <linux/kref.h>
#include <linux/completion.h>

#include <stdbool.h>

struct MsgHeader;
struct VDevice;
struct urb;

extern wait_queue_head_t message_up_submitted_wait;

enum {
	MSGUP_WAIT_READY_INIT = 1,
	MSGUP_WAIT_READY  = 2, // client side only
	MSGUP_READ_BEGAN  = 4
};

/*
 * This message is container of MsgHeader (its payload).
 */
struct message_up
{
	unsigned int flags;
	size_t bytes_read; // for evechardev_read

	struct VDevice *vDev;
	struct MsgHeader *msg;
	struct urb *urb;

	struct kref ref;
	struct list_head list;
	struct list_head submitted_list;
	struct completion msg_ready; // client only
};

struct MsgHeader *eve_alloc_msg_hdr(size_t size, gfp_t flags);
void eve_free_msg_hdr(struct MsgHeader *hdr);

struct message_up *message_up_ctor(uint32_t cmd, size_t msg_size, gfp_t flags);
struct message_up *message_up_ctor_urb(struct urb *urb, struct VDevice *vDev, gfp_t flags);

void *message_up_alloc_msg(struct message_up *msg, uint32_t cmd, size_t msg_size, gfp_t flags);

static inline struct message_up *message_up_get(struct message_up *msg) 
{
	if (msg) {
		kref_get(&msg->ref);
	}

	return msg;
}

int message_up_put(struct message_up *msg);

void message_up_enqueue(struct message_up *msg);
void __message_up_dequeue(struct message_up *msg, bool submitted_only, bool force_submitted);

/*
 * Will not dequeue from submitted message marked with flag MSGUP_READ_BEGAN.
 */
static inline void message_up_dequeue(struct message_up *msg)
{
	__message_up_dequeue(msg, 0, 0);
}

/*
 * If you want to dequeue message marked with flag MSGUP_READ_BEGAN and sure
 * it's safe to do (as evechardev_read does). Usually you don't.
 */
static inline void message_up_dequeue_force(struct message_up *msg)
{
	__message_up_dequeue(msg, 0, 1);
}

static inline void message_up_dequeue_submitted(struct message_up *msg)
{
	__message_up_dequeue(msg, 1, 1);
}

void message_up_dequeue_vdev(const struct VDevice *vDev);

static inline void message_up_dequeue_all(void)
{
	message_up_dequeue_vdev(0);
}

bool message_up_has_submitted(void);
struct message_up *message_up_read_submitted(void);

struct message_up *message_up_find(const struct urb *urb);
void signal_message_ready(struct message_up *msg);

int message_queue_init(void);
void message_queue_exit(void);

#endif // EVEUSB_MESSAGE_QUEUE_H

