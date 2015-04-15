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
#ifndef EVEUSB_KMSG_TYPES_H
#define EVEUSB_KMSG_TYPES_H
//----------------------------------------------------------------------------
#ifdef __KERNEL__
  #include <linux/types.h>
#else
  #include <stdint.h>
#endif // __KERNEL__
//----------------------------------------------------------------------------

enum { MAX_DEVNAME = 32 };

typedef uint32_t dev_id_t;
typedef uint32_t irp_t;
typedef uint8_t epaddr_t; // usb_endpoint_descriptor.bEndpointAddress

enum MSG_CMD // do not reorder these constants!
{
	MSG_CMD_INVALID, // trap to catch invalid requests when memory is zeroed

	MSG_CMD_CLIENT_URB,
	MSG_CMD_CLI_URB_CANCEL,
	MSG_CMD_CONNECT,
	MSG_CMD_DISCONNECT,
	MSG_CMD_DEVICES,
	MSG_CMD_DAEMON_MAX, // end of messages from module to daemon
	
	MSG_CMD_REPROBE_DEVICE = MSG_CMD_DAEMON_MAX // daemon => module
};
//----------------------------------------------------------------------------
#endif // EVEUSB_KMSG_TYPES_H
//----------------------------------------------------------------------------

