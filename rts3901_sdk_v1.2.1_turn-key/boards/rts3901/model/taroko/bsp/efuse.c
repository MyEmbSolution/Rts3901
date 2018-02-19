/*
 * Realtek Semiconductor Corp.
 *
 * bsp/efuse.c:
 *     efuse initialization code
 *
 * Copyright (C) 2014      Wei WANG (wei_wang@realsil.com.cn)
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include "bspchip.h"

static struct resource bsp_efuse_resource[] = {
	[0] = {
		.start = BSP_EFUSE_PADDR,
		.end   = BSP_EFUSE_PADDR + BSP_EFUSE_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device bsp_efuse_device = {
	.name          = "efuse-platform",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_efuse_resource),
	.resource      = bsp_efuse_resource,
};

static int __init bsp_efuse_init(void)
{
	int ret;

	ret = platform_device_register(&bsp_efuse_device);
	if (ret < 0) {
		printk("ERROR: unable to register bsp_efuse_device\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_efuse_init);
