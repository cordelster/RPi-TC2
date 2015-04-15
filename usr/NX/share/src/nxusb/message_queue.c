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
*   Author: 2010-2014 Vadim Grinchishin
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

#include "message_queue.h"
#include "log.h"
#include "vhub.h"
#include "trans.h"
#include "utils.h"
#include "kmsg_decl.h"
#include "crossapi.h" // WARN, wait_for_completion_killable

#include <linux/slab.h>


DECLARE_WAIT_QUEUE_HEAD(message_up_submitted_wait);
static struct kmem_cache *message_up_cache;

static LIST_HEAD(message_up_list);
static LIST_HEAD(message_up_submitted_list);
static DEFINE_SPINLOCK(message_up_lock); // both lists protected by this lock

static atomic_t msg_up_cnt = ATOMIC_INIT(0); // live instances
static atomic_t msg_up_total = ATOMIC_INIT(0);
static atomic_t msg_up_new = ATOMIC_INIT(0);

static atomic_t msg_hdr_cnt = ATOMIC_INIT(0); // live instances
static atomic_t msg_hdr_total = ATOMIC_INIT(0);
static atomic_t msg_hdr_new = ATOMIC_INIT(0);

enum { HDR_CACHE_SIZE = 32 };
static struct MsgHeader* hdr_cache[HDR_CACHE_SIZE];


static void destroy_msg_hdr(struct MsgHeader *hdr)
{
	if (hdr) {
		kfree(hdr);
		atomic_dec(&msg_hdr_cnt);
	}
}

/*
 * slab.h declares ksize(), but slab.c export or not
 * this function many times in different kernel versions:
 * < 24 no, ... yes, 28 no, 29 yes, etc.
 */
struct MsgHeader *eve_alloc_msg_hdr(size_t size, gfp_t flags)
{
	int i = 0;
	struct MsgHeader *hdr = 0;
	
	atomic_inc(&msg_hdr_total);

	for ( ; i < HDR_CACHE_SIZE; ++i) {

		hdr = xchg(&hdr_cache[i], hdr);

		if (hdr && hdr->ksize >= size) {

			if (flags & __GFP_ZERO) {
				size_t len = hdr->ksize;
				size_t cnt = hdr->usage_cnt;

				memset(hdr, 0, size);

				hdr->ksize = len;
				hdr->usage_cnt = cnt + 1;
			}

			return hdr;
		}
	}
	
	destroy_msg_hdr(hdr);
	
	if (size) {
		size = roundup_pow_of_two(size); // argument must be non-zero
	}
	
	hdr = kmalloc(size, flags); // __GFP_ZERO => as kzalloc

	if (hdr) {
		hdr->ksize = size;
		WARN_ON(hdr->ksize != size); // truncated
		hdr->usage_cnt = 1;

		atomic_inc(&msg_hdr_cnt);
		atomic_inc(&msg_hdr_new);
	} else {
		edev_err("%s(size %zu, flags %#x) failed", __func__, size, flags);
		WARN(1, "%s(size %zu, flags %#x) failed", __func__, size, flags);
	}

	return hdr;
}

void eve_free_msg_hdr(struct MsgHeader *hdr)
{
	int i = 0;
	for ( ; hdr && i < HDR_CACHE_SIZE; ++i) {
		hdr = xchg(&hdr_cache[i], hdr);
	}

	destroy_msg_hdr(hdr);
}

static void destroy_msg_cache(void)
{
	int i = 0;
	for ( ; i < HDR_CACHE_SIZE; ++i) {
		struct MsgHeader *hdr = xchg(&hdr_cache[i], 0);
		if (hdr) {
			edev_dbg("#%02d ksize %u, reuse %u", i, hdr->ksize, hdr->usage_cnt);
			destroy_msg_hdr(hdr);
		}
	}
}

/*
 * Initialization of msg->msg_ready is deferred because it uses by client side only.
 */
static void message_up_init(
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26)
	void *obj
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
	struct kmem_cache *cachep, void *obj
#else
	void *obj, struct kmem_cache *cachep, unsigned long flags
#endif
)
{
	struct message_up *msg = obj;
	memset(msg, 0, sizeof(*msg));

	kref_init(&msg->ref);
	INIT_LIST_HEAD(&msg->list);
	INIT_LIST_HEAD(&msg->submitted_list);
	
	atomic_inc(&msg_up_new);
}

/*
 * Object no longer referenced, put it to cache in ready to use state.
 * kmem_cache_alloc/kmem_cache_free retain object's state.
 * Reinitialization of msg->msg_ready is deferred.
 */
static void message_up_release(struct kref *ref)
{
	struct message_up *msg = container_of(ref, struct message_up, ref);

	msg->flags &= MSGUP_WAIT_READY_INIT;
	msg->bytes_read = 0;
	msg->vDev = 0;

	if (msg->msg) {
		eve_free_msg_hdr(msg->msg);
		msg->msg = 0;
	}

	msg->urb = 0;
	kref_init(&msg->ref);

	WARN_ON(!list_empty(&msg->list));
	INIT_LIST_HEAD(&msg->list);

	WARN_ON(!list_empty(&msg->submitted_list));
	INIT_LIST_HEAD(&msg->submitted_list);
	
	kmem_cache_free(message_up_cache, msg);
	atomic_dec(&msg_up_cnt);
}

/*
 * Call message_up_put when object no longer needed.
 * kmem_cache_alloc zeroes memory if __GFP_ZERO bit is set, see kmem_cache_zalloc.
 */
struct message_up *message_up_ctor(uint32_t cmd, size_t msg_size, gfp_t flags)
{
	struct message_up *msg = kmem_cache_alloc(message_up_cache, flags & ~__GFP_ZERO);
	if (!msg) {
		edev_crit("%s: not enough memory", __func__);
		WARN_ON("not enough memory");
		return 0;
	}
	
	atomic_inc(&msg_up_cnt);
	atomic_inc(&msg_up_total);

	if (msg_size && !message_up_alloc_msg(msg, cmd, msg_size, flags)) {
		message_up_put(msg);
		return 0;
	}

	return msg;
}

/*
 * EINVAL conditions should never happen in production code, because they mean bug in code.
 */
void *message_up_alloc_msg(struct message_up *msg, uint32_t cmd, size_t msg_size, gfp_t flags)
{
	if (msg_size < sizeof(*msg->msg) || msg_size > MAX_MSG_HDR_LEN) {
		edev_err("%s: invalid message size: sizeof(MsgHeader) %zu <= %zu <= MAX_MSG_HDR_LEN(%d)",
			__func__, sizeof(*msg->msg), msg_size, MAX_MSG_HDR_LEN);
		WARN(1, "%s: invalid message size: sizeof(MsgHeader) %zu <= %zu <= MAX_MSG_HDR_LEN(%d)",
			__func__, sizeof(*msg->msg), msg_size, MAX_MSG_HDR_LEN); // will not be missed in syslog
		return 0;
	} else if (!msg) {
		edev_err("%s: message_up is null, cmd %u", __func__, cmd);
		WARN(1, "message_up is null, cmd %u", cmd);
		return 0;
	} else if (msg->msg) {
		edev_err("%s: message is not null, cmd %u", __func__, cmd);
		WARN(1, "message is not null, cmd %u", cmd);
		return 0;
	}

	msg->msg = eve_alloc_msg_hdr(msg_size, flags | __GFP_ZERO);
	
	if (msg->msg) {
		msg->msg->cmd = cmd;
		msg->msg->length = msg_size;
		WARN_ON(msg->msg->length != msg_size); // truncated
		WARN_ON(msg->msg->ksize < msg_size);
	}
	
	return msg->msg;
}

struct message_up *message_up_ctor_urb(struct urb *urb, struct VDevice *vDev, gfp_t flags)
{
	struct message_up *msg = message_up_ctor(0, 0, flags);
	if (!msg) {
		return 0;
	}

	msg->vDev = vDev;
	msg->urb = urb;

	if ((flags & __GFP_WAIT) && !(usb_pipein(urb->pipe) || usb_pipeisoc(urb->pipe) || in_interrupt())) {

		if (msg->flags & MSGUP_WAIT_READY_INIT) {
			reinit_completion(&msg->msg_ready);
		} else {
			init_completion(&msg->msg_ready);
		}

		msg->flags |= (MSGUP_WAIT_READY | MSGUP_WAIT_READY_INIT);
	}

	return msg;
}

/*
 * See also: message_up_enqueue.
 */
void signal_message_ready(struct message_up *msg)
{
	if (msg->flags & MSGUP_WAIT_READY) {
		WARN_ON(!(msg->flags & MSGUP_WAIT_READY_INIT));
		complete(&msg->msg_ready);
	}
}

/*
 * See also: signal_message_ready.
 */
void message_up_enqueue(struct message_up *msg)
{
	unsigned long flags;

	WARN_ON(msg->bytes_read);

	message_up_get(msg);
	message_up_get(msg);

	spin_lock_irqsave(&message_up_lock, flags);
	{
		WARN_ON(!list_empty(&msg->list)); // must not be enqueued twice
		list_add_tail(&msg->list, &message_up_list);
		list_add_tail(&msg->submitted_list, &message_up_submitted_list);
	}
	spin_unlock_irqrestore(&message_up_lock, flags);

	wake_up(&message_up_submitted_wait); // -> evechardev_read

	if (msg->flags & MSGUP_WAIT_READY) {
		WARN_ON(!(msg->flags & MSGUP_WAIT_READY_INIT));
		wait_for_completion_killable(&msg->msg_ready); // TASK_KILLABLE, can be interrupted by a kill signal
	}
}

static bool do_list_del(struct list_head *head)
{
	bool del = !list_empty(head);

	if (del) {
		list_del_init(head); // list_empty(head) now true
	}

	return del;
}

/*
 * Do not use this routine directly.
 */
void __message_up_dequeue(struct message_up *msg, bool submitted_only, bool force_submitted)
{
	unsigned long flags;
	int puts = 0;

	if (!msg) {
		return;
	}

	spin_lock_irqsave(&message_up_lock, flags);
	{
		bool rm_submitted = force_submitted || !(msg->flags & MSGUP_READ_BEGAN);

		if (rm_submitted && do_list_del(&msg->submitted_list)) {
			++puts;
			WARN(msg->msg && msg->bytes_read && msg->bytes_read != msg->msg->length,
			     "partially read message removed from submitted queue: "
			     "message.length(%u) != bytes_read(%zu)", msg->msg->length, msg->bytes_read);
		}

		puts += submitted_only ? 0 : do_list_del(&msg->list);
	}
	spin_unlock_irqrestore(&message_up_lock, flags);

	while (puts--) {
		message_up_put(msg);
	}
}

static struct message_up *find_vdev(const struct VDevice *vDev) 
{
	struct message_up *msg = 0;

	list_for_each_entry(msg, &message_up_list, list) {
		if (vDev == msg->vDev) {
			return msg;
		}
	}

	return 0;
}

/*
 * Zero vDev means dequeue all.
 */
void message_up_dequeue_vdev(const struct VDevice *vDev)
{
	unsigned long flags;
	spin_lock_irqsave(&message_up_lock, flags);

	while (!list_empty(&message_up_list)) {

		struct message_up *victim = vDev ? find_vdev(vDev) :
		                            list_first_entry(&message_up_list, struct message_up, list);

		if (!victim) { // find_vdev not found
			break;
		}

		message_up_get(victim);
		spin_unlock_irqrestore(&message_up_lock, flags);

		__message_up_dequeue(victim, false, !vDev);

		if (victim->urb) {
			vDev_complete_urb(victim, victim->urb, -ESHUTDOWN);
		}

		message_up_put(victim);
		spin_lock_irqsave(&message_up_lock, flags);
	}

	spin_unlock_irqrestore(&message_up_lock, flags);
}

int message_up_put(struct message_up *msg)
{
	return msg ? kref_put(&msg->ref, message_up_release) : 0;
}

/*
 * It is safe do not use spinlock here.
 * If really list is empty, read will return zero.
 * See: evechardev_read, evechardev_poll.
 */
bool message_up_has_submitted(void)
{
//	spin_lock_irqsave(&message_up_lock, flags);
	return !list_empty(&message_up_submitted_list);
//	spin_unlock_irqrestore(&message_up_lock, flags);
}

/*
 * Call message_up_put when object no longer needed.
 */
struct message_up *message_up_read_submitted(void)
{
	struct message_up *msg = 0;

	unsigned long flags;
	spin_lock_irqsave(&message_up_lock, flags);

	if (!list_empty(&message_up_submitted_list)) {
		msg = list_first_entry(&message_up_submitted_list, struct message_up, submitted_list);
		msg->flags |= MSGUP_READ_BEGAN;
		message_up_get(msg);
	}

	spin_unlock_irqrestore(&message_up_lock, flags);
	return msg;
}

/*
 * Call message_up_put when object no longer needed.
 */
struct message_up *message_up_find(const struct urb *urb)
{
	struct message_up *msg = 0;
	bool found = false;

	unsigned long flags;
	spin_lock_irqsave(&message_up_lock, flags);

	list_for_each_entry(msg, &message_up_list, list) {

		found = msg->urb == urb;
		if (found) {
			message_up_get(msg);
			break;
		}
	}

	spin_unlock_irqrestore(&message_up_lock, flags);
	return found ? msg : 0;
}

static void check_resource_leakes(void)
{
	int msg_up = atomic_read(&msg_up_cnt);
	int msg_hdr = atomic_read(&msg_hdr_cnt);

	WARN(msg_up, "kmem_cache_alloc(message_up) lost %d", msg_up);
	WARN(msg_hdr, "kmalloc(message_up->msg) lost %d", msg_hdr);
	
	edev_info("message_up: %d new of %d allocations", 
		  atomic_read(&msg_up_new), atomic_read(&msg_up_total));

	edev_info("MsgHeader: %d new of %d allocations", 
		  atomic_read(&msg_hdr_new), atomic_read(&msg_hdr_total));

	WARN_ON(!list_empty(&message_up_list));
	WARN_ON(!list_empty(&message_up_submitted_list));
}

static void precache_msg_hdr(size_t size, int cnt)
{
	struct MsgHeader *hdr[cnt];
	int i = 0;

	for (i = 0; i < cnt; ++i) {
		hdr[i] = eve_alloc_msg_hdr(size, GFP_KERNEL);
	}
	
	for (i = 0; i < cnt; ++i) {
		eve_free_msg_hdr(hdr[i]);
	}
}

int message_queue_init(void)
{
	unsigned long flags = eveusb_debug ? SLAB_DEBUG_FREE | SLAB_RED_ZONE | SLAB_POISON : 0; // module parameter

	message_up_cache = kmem_cache_create("message_up", // see KMEM_CACHE
						sizeof(struct message_up),
						__alignof__(struct message_up),
						flags,
						message_up_init
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
						, 0 // dtor
#endif					     
					    );

	if (message_up_cache) {
		edev_info("message_up cache created"); // sudo cat /proc/slabinfo | grep message_up
	} else {
		edev_err("cannot create message_up cache");
		return -ENOMEM;
	}

	precache_msg_hdr(1 << 16, 4);
	return 0;
}

void message_queue_exit(void)
{
	kmem_cache_destroy(message_up_cache);
	message_up_cache = 0;

	destroy_msg_cache();
	check_resource_leakes();
}
