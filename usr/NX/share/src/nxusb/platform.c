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
*   Author: 2011-2014 Vadim Grinchishin
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

#include "platform.h"
#include "log.h"
#include "vhub.h"
#include "crossapi.h" // dev_name

#include <stdbool.h>
#include <linux/platform_device.h>

static struct usb_hcd *g_eveusb_hcd;
static const char s_driver_name[] = "nxusb";

/*
 * Driver & device name.
 */
const char *get_eveusbhcd_name(void)
{
	return s_driver_name;
}

/*
 * usb 1.X devices does not work without that.
 * See: source/drivers/usb/core/hub.c, hub_port_init.
 *      source/drivers/staging/usbip/vhci_hcd.c, vhci_hcd_probe.
 * 
 * usb 3.0 root hub that has_tt=1 has bDeviceProtocol=1 => USB_HUB_PR_HS_SINGLE_TT 
 * (Hi-speed hub with single TT), but must have 3 => USB_HUB_PR_SS (Super speed hub).
 * See: drivers/usb/core/hcd.c, rh_call_control, if hcd->has_tt.
 */
static inline bool set_has_tt(struct usb_hcd *hcd)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	hcd->has_tt = 1;
	return true;
#else
	return false;
#endif
}

static void init_hcd(struct usb_hcd *hcd)
{
	WARN_ON(hcd->state != HC_STATE_HALT);
	WARN_ON(hcd->power_budget); // in mA, 0 = no limit
	WARN_ON(hcd->driver->irq);

	if (!(hcd->driver->irq || hcd->irq)) {
		hcd->irq = -1; // workaround for "Trying to free already-free IRQ 0"
	}

	hcd->uses_new_polling = 1;
//	hcd->poll_rh = 1; // poll for rh status?
	
	if (get_hcd_speed(hcd) == HCD_USB2 && set_has_tt(hcd)) {
		edev_info("integrated TT in USB 2.0 root hub");
	}
}

static int eve_platform_driver_probe(struct platform_device *pdev)
{
	int err = 0;
	struct VHub *vhub = 0;

	if (pdev->dev.dma_mask) {
		edev_crit("DMA is not supported");
		return -EINVAL;
	}

	g_eveusb_hcd = usb_create_hcd(get_eveusb_hc_driver(), &pdev->dev,
				(char*)dev_name(&pdev->dev)); // prior 2.6.27 third parameter is char*

	if (g_eveusb_hcd) {
		init_hcd(g_eveusb_hcd);
	} else {
		edev_crit("Cannot create usb hcd. Not enough memory");
		return -ENOMEM;
	}

	vhub = hcd_to_vhub(g_eveusb_hcd);
	memset(vhub, 0, sizeof(*vhub));
	spin_lock_init(&vhub->lock);
	vhub->shutdown = 1;

	err = usb_add_hcd(g_eveusb_hcd, 0, 0);
	if (err) {
		usb_put_hcd(g_eveusb_hcd);
		g_eveusb_hcd = 0;
	}

	return err;
}

static int eve_platform_driver_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	if (hcd) {
		usb_remove_hcd(hcd);
		usb_put_hcd(hcd);
		
		WARN_ON(hcd != g_eveusb_hcd);
		g_eveusb_hcd = 0;
	}

	return 0;
}

static int eve_platform_driver_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct VHub *vhub = hcd_to_vhub(hcd);

	int connected = 0;
	int port = 0;

	unsigned long flags;
	spin_lock_irqsave(&vhub->lock, flags);

	for (port = 0; port < VHUB_NUM_PORTS; ++port) {
		if (vhub->port_status[port] & USB_PORT_STAT_CONNECTION) {
			++connected;
		}
	}

	spin_unlock_irqrestore(&vhub->lock, flags);

	if (connected) {
		dev_err(&pdev->dev, "Cannot suspend with %d active connection(s)", connected);
		return -EBUSY;
	}
	
	dev_dbg(&pdev->dev, "suspend");
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	return 0;
}

static int eve_platform_driver_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "resume");
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	usb_hcd_poll_rh_status(hcd);

	return 0;
}

static struct platform_driver eveusbhcd_platform_driver = 
{
	.probe = eve_platform_driver_probe,
	.remove = eve_platform_driver_remove,
	
	.suspend = eve_platform_driver_suspend,
	.resume = eve_platform_driver_resume,

	.driver = {
		.name = s_driver_name,
		.owner = THIS_MODULE,
	},
};

struct platform_driver *get_eveusbhcd_platform_driver(void)
{
	return &eveusbhcd_platform_driver;
}

static void eve_platform_device_release(struct device *dev)
{
}

static struct platform_device eveusbhcd_platform_device = 
{
	.name = s_driver_name, // should be the same name as platform driver name
	.id = -1,

	.dev = {
		.release = eve_platform_device_release
	}
};

struct platform_device *get_eveusbhcd_platform_device(void)
{
	return &eveusbhcd_platform_device;
}

struct usb_hcd *get_eveusb_hcd(void)
{
	return g_eveusb_hcd;
}
