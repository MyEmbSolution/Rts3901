/*
 * Realtek Semiconductor Corp.
 *
 * bsp/gmac.c:
 *     bsp GMAC initialization code
 *
 * Copyright (C) 2006-2012 Jethro Hsu (jethro@realtek.com)
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

static struct resource bsp_net_gmac_resource[] = {
	[0] = {
		.start = BSP_GMAC_MAPBASE,
		.end = BSP_GMAC_MAPBASE + BSP_GMAC_MAPSIZE - 1,
		.flags = IORESOURCE_IO, },
	[1] = {
		.start = BSP_IRQ_GMAC,
		.end = BSP_IRQ_GMAC,
		.flags = IORESOURCE_IRQ, }
};

static u64 bsp_gmac_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_net_gmac_device = {
	.name = "8139gb",
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_net_gmac_resource),
	.resource = bsp_net_gmac_resource,
	.dev = {
		.dma_mask = &bsp_gmac_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

static int __init bsp_gmac_init(void)
{
	u32 retval;
	printk("INFO: initializing GMAC devices ...\n");

	retval = platform_device_register(&bsp_net_gmac_device);
	if (retval < 0) {
		printk("ERROR: unable to add devices\n");
		return retval;
	}

	return retval;
}
arch_initcall(bsp_gmac_init);
