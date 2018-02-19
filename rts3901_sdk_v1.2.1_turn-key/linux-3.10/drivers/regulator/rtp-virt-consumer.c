/*
 * rtp-virt-consumer.c  --  Realsil RTP
 *
 * Copyright 2015 Realsil Semiconductor Corp.
 *
 * Author: Steven Feng <steven_feng@realsil.com.cn>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct platform_device ldo1_device = {
	.name          = "reg-virt-consumer",
	.id            = 1,
	.dev           = {
		.platform_data = "LDO1",
	}
};

struct platform_device ldo2_device = {
	.name          = "reg-virt-consumer",
	.id            = 2,
	.dev           = {
		.platform_data = "LDO2",
	}
};

struct platform_device buck1_device = {
	.name          = "reg-virt-consumer",
	.id            = 3,
	.dev           = {
		.platform_data = "DC_1V0",
	}
};

struct platform_device buck2_device = {
	.name          = "reg-virt-consumer",
	.id            = 4,
	.dev           = {
		.platform_data = "SWR_OUT_RSV",
	}
};

struct platform_device buck3_device = {
	.name          = "reg-virt-consumer",
	.id            = 5,
	.dev           = {
		.platform_data = "DC_1V5",
	}
};

struct platform_device buck4_device = {
	.name          = "reg-virt-consumer",
	.id            = 6,
	.dev           = {
		.platform_data = "DC_3V3",
	}
};

static struct platform_device *pmu_virtual_devs[] = {
	&ldo1_device,
	&ldo2_device,
	&buck1_device,
	&buck2_device,
	&buck3_device,
	&buck4_device,
};

static int __init pmu_virtual_consumer_init(void)
{
	int ret;

	ret = platform_add_devices(pmu_virtual_devs, ARRAY_SIZE(pmu_virtual_devs));
	if (ret < 0) {
		printk("ERROR: unable to add regulator virtual consumer\n");
		return ret;
	}

	return 0;
}
module_init(pmu_virtual_consumer_init);

static void __exit pmu_virtual_consumer_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(pmu_virtual_devs); i++)
		platform_device_unregister(pmu_virtual_devs[i]);
}
module_exit(pmu_virtual_consumer_exit);

MODULE_AUTHOR("Steven Feng <steven_feng@realsil.com.cn>");
MODULE_DESCRIPTION("Virtual regulator consumer");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform_device: rtp virtual dev");
