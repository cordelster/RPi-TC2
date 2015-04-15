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
#ifndef EVEUSB_KMSG_DECL_H
#define EVEUSB_KMSG_DECL_H

#include "kmsg_types.h"

#ifdef __KERNEL__
  #include "crossapi.h" // __aligned
#else
  #include "gcc_attrib.h"
#endif // __KERNEL__


enum { MAX_MSG_HDR_LEN = UINT_MAX }; // MsgHeader.length & ksize

/*
 * All structs must have the same layout on 32-bit and 64-bit architectures.
 * Thus, I use __aligned(8) because x64 uses this alignment.
 */
struct __aligned(8) MsgHeader
{
	uint32_t length; // total length of message including header
	uint32_t cmd; // enum MSG_CMD

// these members use in module and should be ignored in userspace
	uint32_t ksize; // allocated bytes
	uint32_t usage_cnt; // reuse count
};

struct __aligned(8) MsgConnect
{
	struct MsgHeader hdr; // MSG_CMD_CONNECT
	dev_id_t dev_id;
	uint8_t speed;
};

struct __aligned(8) MsgVDev
{
	struct MsgHeader hdr; // MSG_CMD_CONNECT, MSG_CMD_DISCONNECT
	dev_id_t dev_id;
};

struct __aligned(8) MsgReprobeDevice
{
	struct MsgHeader hdr;
	char devname[MAX_DEVNAME];
};

struct __aligned(8) DeviceInfo
{
	char devname[MAX_DEVNAME];
	char name[48];
	uint32_t maxchild; // number of ports if hub
	uint8_t removed;
};

/*
 * MSG_CMD_DEVICES, in/out.
 */
struct __aligned(8) MsgDevices
{
	struct MsgHeader hdr;
	int sockfd;
	uint32_t cnt;
	struct DeviceInfo dev[0];
};

struct __aligned(8) MsgURB_common
{
	struct MsgHeader hdr;
	uint64_t context; // opaque value from kernel (struct urb*), will be returned to kernel on response
	dev_id_t dev_id;
	uint32_t pipe;
};

struct __aligned(8) MsgURBCancel
{
	struct MsgURB_common common;
};
#define MsgURBCancel_context(msg) ((msg)->common.context)
#define MsgURBCancel_pipe(msg)    ((msg)->common.pipe)

struct __aligned(8) MsgURBRequest
{
	struct MsgURB_common common;
	uint32_t transfer_flags;
	uint32_t transfer_buffer_length;
	uint32_t actual_length;
};
#define MsgURBRequest_length(msg)  ((msg)->common.hdr.length)
#define MsgURBRequest_context(msg) ((msg)->common.context)
#define MsgURBRequest_dev_id(msg)  ((msg)->common.dev_id)
#define MsgURBRequest_pipe(msg)    ((msg)->common.pipe)

struct __aligned(8) MsgURBISOFrameRequest
{
	unsigned int offset;
	unsigned int length;
	int status;
};

struct __aligned(8) MsgURBISORequest
{
	struct MsgURBRequest common;
	int start_frame;
	int number_of_packets;
	int error_count;
	struct MsgURBISOFrameRequest frames[0];
};
#define MsgURBISORequest_pipe(msg) ((msg)->common.common.pipe)
#define MsgURBISORequest_transfer_flags(msg) ((msg)->common.transfer_flags)
#define MsgURBISORequest_transfer_buffer_length(msg) ((msg)->common.transfer_buffer_length)
#define MsgURBISORequest_start_frame(msg) ((msg)->start_frame)
#define MsgURBISORequest_number_of_packets(msg) ((msg)->number_of_packets)
#define MsgURBISORequest_frames(msg) ((msg)->frames)

struct __aligned(8) MsgURBResponse
{
	struct MsgHeader hdr;
	uint64_t urb; // struct urb*
	int status;
	uint32_t actual_length;
};

struct __aligned(8) MsgURBISOFrameResponse
{
	unsigned int length;
	int status;
};

struct __aligned(8) MsgURBISOResponse
{
	struct MsgURBResponse common;
	int start_frame;
	int number_of_packets;
	int error_count;
	struct MsgURBISOFrameResponse frames[0];
};

#endif // EVEUSB_KMSG_DECL_H
