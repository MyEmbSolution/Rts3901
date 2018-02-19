/*
 * Realtek Semiconductor Corp.
 *
 * bsp/watchdog.c:
 *     bsp watchdog initialization code
 *
 * Copyright (C) 2014      Peter Sun (peter_sun@realsil.com.cn)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sizes.h>

#include "bspchip.h"

static struct resource bsp_watchdog_resource[] = {
	[0] = {
		.start = BSP_WATCHDOG_PADDR,
		.end   = BSP_WATCHDOG_PADDR + BSP_WATCHDOG_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device bsp_watchdog_device = {
	.name          = "watchdog-platform",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_watchdog_resource),
	.resource      = bsp_watchdog_resource,
};

static struct platform_device *bsp_watchdog_devs[] __initdata = {
	&bsp_watchdog_device,
};

static int __init bsp_watchdog_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing watchdog controller ...\n");

	ret = platform_add_devices(bsp_watchdog_devs,
		ARRAY_SIZE(bsp_watchdog_devs));
	if (ret < 0) {
		printk("ERROR: unable to add watchdog controller\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_watchdog_init);
MODULE_LICENSE("GPL");
