/*
 * dmac.c
 *
 * Copyright(C) 2015 Micky Ching, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
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
#include <linux/dw_dmac.h>


#include "bspchip.h"

static struct resource bsp_dma_resource[] = {
	[0] = {
		.start = BSP_DMA_PADDR,
		.end   = BSP_DMA_PADDR + BSP_DMA_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_DMA,
		.end   = BSP_IRQ_DMA,
		.flags = IORESOURCE_IRQ,
	}
};

static struct dw_dma_platform_data rts_dma_data = {
	.nr_channels	= 1,
	.block_size	= 4095U,
	.nr_masters	= 1,
	.data_width	= { 2, 2, 0, 0 },
};

static u64 bsp_dma_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_dmac_device = {
	.name          = "rts_dmac",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_dma_resource),
	.resource      = bsp_dma_resource,
	.dev           = {
		.dma_mask = &bsp_dma_dmamask,
		.platform_data = &rts_dma_data,
		.coherent_dma_mask = 0xffffffffUL
	}
};

static struct platform_device *bsp_dma_devs[] __initdata = {
	&bsp_dmac_device,
};

static int __init bsp_dma_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing DMA controller ...\n");

	ret = platform_add_devices(bsp_dma_devs, ARRAY_SIZE(bsp_dma_devs));
	if (ret < 0) {
		printk("ERROR: unable to add DMA controller\n");
		return ret;
	}

	return 0;
}

arch_initcall(bsp_dma_init);
MODULE_LICENSE("GPL");
