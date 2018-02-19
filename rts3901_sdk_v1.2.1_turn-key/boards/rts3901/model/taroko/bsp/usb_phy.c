/*
 * Realtek Semiconductor Corp.
 *
 * bsp/usb_phy.c:
 *     bsp USB PHY initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 *               2014      Wei WANG (wei_wang@realsil.com.cn)
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include "bspchip.h"

/* USB Phy */

static struct resource bsp_usb_phy_resource[] = {
	[0] = {
		.start = BSP_USB_PHY_PADDR,
		.end   = BSP_USB_PHY_PADDR + BSP_USB_PHY_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_USB_DEV_PHY_PADDR,
		.end   = BSP_USB_DEV_PHY_PADDR + BSP_USB_DEV_PHY_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device bsp_usb_phy_device = {
	.name          = "usbphy-platform",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_usb_phy_resource),
	.resource      = bsp_usb_phy_resource,
};

static struct platform_device *bsp_usb_phy_devs[] __initdata = {
	&bsp_usb_phy_device,
};

static int __init bsp_usb_phy_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing USB phy ...\n");

	ret = platform_add_devices(bsp_usb_phy_devs, ARRAY_SIZE(bsp_usb_phy_devs));
	if (ret < 0) {
		printk("ERROR: unable to add usb phy\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_usb_phy_init);
