/*
 * ocsram.c
 *
 * Copyright(C) 2015 Jim Cao, All Rights Reserved.
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
#include <linux/device.h>
#include <linux/platform_device.h>

#include "bspcpu.h"

static struct resource bsp_dmem_resource[] = {
	/* Merge cpu dmem0 and dmem1 */
	[0] = {
		.start = cpu_dmem0_base,
		.end   = cpu_dmem1_top,
		.flags = IORESOURCE_MEM,
	}
};

struct platform_device bsp_dmem_device = {
	.name          = "sram",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_dmem_resource),
	.resource      = bsp_dmem_resource,
};

static struct platform_device *bsp_dmem_devs[] __initdata = {
	&bsp_dmem_device,
};

static int __init bsp_ocsram_init(void)
{
	int ret;

	pr_warn("INFO: initializing on chip memory device ...\n");

	ret = platform_add_devices(bsp_dmem_devs, ARRAY_SIZE(bsp_dmem_devs));
	if (ret < 0) {
		pr_err("ERROR: unable to add on chip memory device\n");
		return ret;
	}

	return 0;
}

arch_initcall(bsp_ocsram_init);
MODULE_AUTHOR("Jim Cao <jim_cao@realsil.com.cn");
MODULE_DESCRIPTION("On-chip SRAM(IMEM/DMEM) device");
MODULE_LICENSE("GPL");

