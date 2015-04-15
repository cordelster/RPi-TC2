/*****************************************************************************
*
*   Copyright (C) 2013 Eltima software
*   Author: 2013 Vadim Grinchishin
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

#include <linux/stat.h>
#include <linux/module.h> // moduleparam.h has no MODULE_PARM_DESC on old kernels

/*
 * stdbool.h => "#define bool" will break module_param(,bool,)
 */
_Bool eveusb_debug;
module_param(eveusb_debug, bool, S_IRUGO | S_IWUSR); // 0644
MODULE_PARM_DESC(eveusb_debug, "turn on/off debugging (verbose messages, etc.)");
