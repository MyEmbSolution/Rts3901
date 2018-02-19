/*
 * Realtek Semiconductor Corp.
 *
 * bsp/crypto.c:
 *     bsp CRYPTO initialization code
 *
 * Copyright (C) 2014      Wind HAN (wind_han@realsil.com.cn)
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

static struct resource bsp_crypto_resource[] = {
	[0] = {
		.start = BSP_CRYPTO_PADDR,
		.end = BSP_CRYPTO_PADDR + BSP_CRYPTO_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_ENCPY,
		.end = BSP_IRQ_ENCPY,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 bsp_crypto_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_crypto_device = {
	.name = "rlx-crypto",
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_crypto_resource),
	.resource = bsp_crypto_resource,
	.dev = {
		.dma_mask = &bsp_crypto_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
	}
};

static struct platform_device *bsp_crypto_devs[] __initdata = {
	&bsp_crypto_device,
};

static int __init bsp_crypto_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing crypto device ...\n");

	ret = platform_add_devices(bsp_crypto_devs,
				   ARRAY_SIZE(bsp_crypto_devs));
	if (ret < 0) {
		printk("ERROR: unable to add crypto device\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_crypto_init);
