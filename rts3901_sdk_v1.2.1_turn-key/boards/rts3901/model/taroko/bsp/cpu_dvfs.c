/*
 * Realtek Semiconductor Corp.
 *
 * bsp/cpu_dvfs.c:
 *     bsp CPU DVFS initialization code
 *
 * Copyright (C) 2016      Wind HAN (wind_han@realsil.com.cn)
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/clk/rts_cpu_clk.h>
#include <asm/clock.h>

struct cpufreq_frequency_table rts_freq_table[] = {
	{.index = 0, .frequency = 300000},
	{.index = 0, .frequency = 400000},
	{.index = 0, .frequency = 500000},
	{.frequency = CPUFREQ_TABLE_END},
};

struct platform_device bsp_cpu_dvfs_device = {
	.name = "rts-cpu-dvfs",
	.id = -1,
	.dev = {
		.platform_data = rts_freq_table,
	}
};

static struct platform_device *bsp_cpu_dvfs_devs[] __initdata = {
	&bsp_cpu_dvfs_device,
};

int rts_set_cpu_clk(struct clk *rts_cpu_clk, unsigned long rate)
{
	struct clk *rts_pll0_clk;
	struct clk *rts_pll2_clk;

	rts_pll0_clk = clk_get(NULL, "sys_pll0");
	if (IS_ERR(rts_pll0_clk))
		return PTR_ERR(rts_pll0_clk);
	rts_pll2_clk = clk_get(NULL, "sys_pll2");
	if (IS_ERR(rts_pll2_clk))
		return PTR_ERR(rts_pll2_clk);

	switch (rate) {
	case 300000000:
	case 400000000:
	case 600000000:
		clk_set_parent(rts_cpu_clk, rts_pll2_clk);
		break;
	case 500000000:
	default:
		clk_set_parent(rts_cpu_clk, rts_pll0_clk);
		break;
	}

	clk_set_rate(rts_cpu_clk, rate);

	clk_put(rts_pll0_clk);
	rts_pll0_clk = NULL;
	clk_put(rts_pll2_clk);
	rts_pll2_clk = NULL;

	return 0;
}

static int rts_set_pll2_clk(void)
{
	struct clk *rts_pll2_clk;

	rts_pll2_clk = clk_get(NULL, "sys_pll2");
	if (IS_ERR(rts_pll2_clk))
		return PTR_ERR(rts_pll2_clk);

	clk_set_rate(rts_pll2_clk, 1200000000);
	clk_prepare(rts_pll2_clk);
	clk_enable(rts_pll2_clk);

	clk_put(rts_pll2_clk);
	rts_pll2_clk = NULL;

	return 0;
}

static int __init bsp_cpu_dvfs_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing cpu dvfs device ...\n");

	ret = rts_set_pll2_clk();
	if (ret < 0) {
		printk("ERROR: unable to enable pll2 clock\n");
		return ret;
	}

	ret = platform_add_devices(bsp_cpu_dvfs_devs,
			ARRAY_SIZE(bsp_cpu_dvfs_devs));
	if (ret < 0) {
		printk("ERROR: unable to add cpu dvfs device\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_cpu_dvfs_init);
