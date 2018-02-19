/*
 * Realtek Semiconductor Corp.
 *
 * bsp/sd.c:
 *     bsp SD controller initialization code
 *
 * Copyright (C) 2014      Wei WANG (wei_wang@realsil.com.cn)
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

static struct resource bsp_sd_resource[] = {
	[0] = {
		.start = BSP_SD_PADDR,
		.end   = BSP_SD_PADDR + BSP_SD_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_SD,
		.end   = BSP_IRQ_SD,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 bsp_sd_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_sd_device = {
#ifdef CONFIG_SOC_FPGA_CODE
	.name          = "rts3901-fpga-sdhc",
#else
	.name          = "rts3901-sdhc",
#endif
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_sd_resource),
	.resource      = bsp_sd_resource,
	.dev           = {
		.dma_mask = &bsp_sd_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

static struct platform_device *bsp_sd_devs[] __initdata = {
	&bsp_sd_device,
};

static int __init bsp_sd_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing SD controller ...\n");

	ret = platform_add_devices(bsp_sd_devs, ARRAY_SIZE(bsp_sd_devs));
	if (ret < 0) {
		printk("ERROR: unable to add SD controller\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_sd_init);
MODULE_LICENSE("GPL");
