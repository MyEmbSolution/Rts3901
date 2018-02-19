/*
 * Realtek Semiconductor Corp.
 *
 * bsp/clock.c:
 *     bsp clock initialization code
 *
 * Copyright (C) 2014      Peter Sun (peter_sun@realsil.com.cn)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "bspchip.h"

struct clk_hw_rlx {
	struct clk_hw hw;
	u8 clksel;
	void __iomem *clkreg;
	u32 clk_change;
	u32 rate;
};
#define SYS_PLL0_CFG0 0
#define SYS_PLL0_CFG1 4
#define SYS_PLL0_CTL 8
#define SYS_PLL0_STAT 0xc
#define SYS_PLL3_CFG0 0
#define SYS_PLL3_CFG1 4
#define SYS_PLL3_CFG2 8
#define SYS_PLL3_STAT 0xc
#define SYS_PLL3_SSCG_CTL 0x10
#define SYS_PLL3_SSCG_CFG0 0x14
#define SYS_PLL3_SSCG_CFG1 0x18
#define SYS_PLL3_SSCG_CFG2 0x1c

#define CMU_EN_PLL0 (1<<3)
#define LDO_EN_PLL0 (1<<2)
#define CMU_EN_CKOOBS_PLL0 (1<<1)
#define PLL_EN_WD_PLL0 (1<<0)

#define DEFINE_STRUCT_CLK(_name, _parent_array_name, _clkops_name)	\
	static struct clk _name = {				\
		.name = #_name,					\
		.hw = &_name##_hw.hw,				\
		.parent_names = _parent_array_name,		\
		.num_parents = ARRAY_SIZE(_parent_array_name),	\
		.ops = &_clkops_name,				\
	};

#define DEFINE_CLK_RLX(_name, _clksel,	\
			    _clksel_reg, _clk_change, _parent_names, _ops) \
	static struct clk _name;				\
	static struct clk_hw_rlx _name##_hw = {		\
		.hw = {						\
			.clk = &_name,				\
		},						\
		.clksel	= _clksel,			\
		.clkreg	= (void __iomem	*)_clksel_reg,			\
		.clk_change	= _clk_change,			\
	};							\
	DEFINE_STRUCT_CLK(_name, _parent_names, _ops);

#define to_clk_hw_rlx(_hw) container_of(_hw, struct clk_hw_rlx, hw)

#ifdef CONFIG_SOC_FPGA_CODE
static const char *rlx_ck_parent_names[] = {
	"usb_pll_2", "usb_pll_3", "usb_pll_5", "usb_pll_7",
	"sys_pll0_2", "sys_pll0_3", "sys_pll0_5", "sys_pll0_7",
};
#else
static const char *rlx_ck_parent_names[] = {
	"usb_pll", "sys_pll0", "sys_pll1", "sys_pll2", "sys_pll3_f",
	"sys_pll4_f"
};

static const char *rlx_root_parent_names[] = {
	"sys_osc",
};

static const char *rlx_pll_parent_names_m3[] = {
	"sys_pll3_m",
};

static const char *rlx_pll_parent_names_m4[] = {
	"sys_pll4_m",
};

static const char *rlx_pll_parent_names_n3[] = {
	"sys_pll3_n",
};

static const char *rlx_pll_parent_names_n4[] = {
	"sys_pll4_n",
};

#endif

static u8 div_array[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

static long rlx_round_rate(struct clk_hw *hw, unsigned long rate,
			   unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	unsigned long divisor = (parent_rate + rate / 2) / rate;

#ifdef CONFIG_SOC_FPGA_CODE
	if (divisor > 128)
		divisor = 128;
	else if (divisor < 1)
		divisor = 1;
#else
	if (divisor > 896)
		divisor = 896;
	else if (divisor < 2)
		divisor = 2;
#endif
	return parent_rate / divisor;
}

static int rlx_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	u32 divreg, reg;
#ifndef CONFIG_SOC_FPGA_CODE
	u32 div1, div2;
#endif
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 div = (parent_rate + rate / 2) / rate;
	u32 i;

	reg = __raw_readl(clk->clkreg) & ~0xff;

	pr_debug("%s div: %u p:%lu r:%lu\n", hw->clk->name,
		 div, parent_rate, rate);

	clk->rate = rate;

#ifdef CONFIG_SOC_FPGA_CODE
	for (i = 0; i < ARRAY_SIZE(div_array); i++)
		if (div <= div_array[i])
			break;

	if (i == ARRAY_SIZE(div_array))
		i--;

	divreg = clk->clksel | (i << 5);
#else
	if (div % 7 == 0)
		div1 = 7;
	else if (div % 5 == 0)
		div1 = 5;
	else if (div % 3 == 0)
		div1 = 3;
	else
		div1 = 2;

	div2 = div / div1;
	div2 = div2 ? div2 : 1;

	for (i = 0; i < ARRAY_SIZE(div_array); i++)
		if (div2 <= div_array[i])
			break;

	if (i == ARRAY_SIZE(div_array))
		i--;

	if (div1 == 2)
		div1 = 0;
	else
		div1 /=  2;
	divreg = div1 | (clk->clksel << 2) | (i<<5);
#endif

	pr_debug("%s div:%u reg:0x%x\n", hw->clk->name, div_array[i], divreg);

	__raw_writel(1<<clk->clk_change, (void __iomem *)CLK_CHANGE);
	reg |= divreg;
	__raw_writel(reg, (void __iomem *)clk->clkreg);
	__raw_writel(0, (void __iomem *)CLK_CHANGE);

	xb2flush();

	return 0;
}

static int rlx_enable_clk(struct clk_hw *hw)
{
	u32 time = 5000;

	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	u32 reg = __raw_readl(clk->clkreg);
	reg |= CLK_ENABLE;
	__raw_writel(reg, clk->clkreg);

	while (--time) {
		if (__raw_readl(clk->clkreg) & CLK_ENABLE)
			break;
		udelay(1);
	}

	if (time == 0) {
		pr_err("%s enable failed\n", hw->clk->name);
		return -ETIMEDOUT;
	}

	return 0;
}

static void rlx_disable_clk(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	u32 reg = __raw_readl(clk->clkreg);
	reg &= ~CLK_ENABLE;
	__raw_writel(reg, clk->clkreg);

	xb2flush();
}

static int rlx_set_parent(struct clk_hw *hw, u8 field_val)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	clk->clksel = field_val;

	pr_debug("%s clksel:%u\n", hw->clk->name, field_val);

	return 0;
}

static u8 rlx_get_parent(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	return clk->clksel;
}

static unsigned long rlx_recalc(struct clk_hw *hw, unsigned long parent_rate)
{
	u32 div, div2;
#ifndef CONFIG_SOC_FPGA_CODE
	u32 div1;
#endif
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

#ifndef CONFIG_SOC_FPGA_CODE
	div1 = __raw_readl(clk->clkreg) & 0x3;
#endif
	div2 = __raw_readl(clk->clkreg) & 0xe0;

#ifndef CONFIG_SOC_FPGA_CODE
	div1 = div1 ? ((div1 << 1) + 1) : 2;
#endif
	div2 = div_array[div2 >> 5];

#ifdef CONFIG_SOC_FPGA_CODE
	div = div2;
#else
	div = div1 * div2;
#endif

	pr_debug("%s prate: %lu, div: %u\n", hw->clk->name, parent_rate, div);

	if (div)
		return parent_rate / div;
	else
		return 0;
}

static const struct clk_ops rlx_divider_ops = {
	.enable = rlx_enable_clk,
	.disable = rlx_disable_clk,
	.round_rate = rlx_round_rate,
	.set_rate = rlx_set_rate,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
	.recalc_rate = rlx_recalc,
};

static long rlx_round_rate_mul(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	int mul = (rate + parent_rate / 2) / parent_rate;
	unsigned long round_rate = parent_rate * mul;

	pr_debug("%s round:%lu\n", hw->clk->name, round_rate);

	return round_rate;
}

static int rlx_set_rate_m0(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	u32 mul, reg;

	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	mul = (rate + parent_rate / 2) / parent_rate;

	pr_debug("%s setrate:%d\n", hw->clk->name, mul);

	if (mul < 4)
		return -1;

	clk->rate = rate;

	reg = __raw_readl(clk->clkreg + SYS_PLL0_CFG0) & ~0xff0000;
	reg |= (mul - 4) << 16;
	__raw_writel(reg, (void __iomem *)clk->clkreg + SYS_PLL0_CFG0);

	xb2flush();

	return 0;
}

static int rlx_enable_clk_m0(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	__raw_writel(LDO_EN_PLL0, (void __iomem *)(clk->clkreg + SYS_PLL0_CTL));
	__raw_writel((LDO_EN_PLL0 | PLL_EN_WD_PLL0),
		     (void __iomem *)clk->clkreg + SYS_PLL0_CTL);
	udelay(1);
	__raw_writel((PLL_EN_WD_PLL0 | CMU_EN_CKOOBS_PLL0
		      | LDO_EN_PLL0 | CMU_EN_PLL0),
		     (void __iomem *)clk->clkreg + SYS_PLL0_CTL);
	xb2flush();

	return 0;
}

static void rlx_disable_clk_m0(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	__raw_writel(0, clk->clkreg + SYS_PLL0_CTL);
	xb2flush();
}

static unsigned long rlx_recalc_m0(struct clk_hw *hw, unsigned long parent_rate)
{
	u32 mul, rate;
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	mul = ((__raw_readl(clk->clkreg + SYS_PLL0_CFG0) & 0xff0000) >> 16) + 4;

	rate = parent_rate * mul;

	pr_debug("%s prate: %u, mul: %u\n", hw->clk->name, rate, mul);

	return rate;
}

static const struct clk_ops rlx_pll0_ops = {
	.round_rate = rlx_round_rate_mul,
	.set_rate = rlx_set_rate_m0,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
	.recalc_rate = rlx_recalc_m0,
};

static const struct clk_ops rlx_pll3_4_ops = {
	.enable = rlx_enable_clk_m0,
	.disable = rlx_disable_clk_m0,
	.round_rate = rlx_round_rate_mul,
	.set_rate = rlx_set_rate_m0,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
	.recalc_rate = rlx_recalc_m0,
};

static int rlx_enable_clk_pll3_4(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 reg;

	if (clk->rate < 200000000 || clk->rate > 540000000)
		return -EINVAL;

	/* Dpll power on */
	reg = __raw_readl(clk->clkreg + SYS_PLL3_CFG1) & 0xffffefff;
	__raw_writel(reg, (void __iomem *)(clk->clkreg + SYS_PLL3_CFG1));

	/*Dpll release */
	reg = __raw_readl(clk->clkreg + SYS_PLL3_CFG2) | 0x400000;
	__raw_writel(reg, (void __iomem *)(clk->clkreg + SYS_PLL3_CFG2));

	xb2flush();

	return 0;
}

static void rlx_disable_clk_pll3_4(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 reg;

	/* Dpll stop */
	__raw_writel(0, (void __iomem *)clk->clkreg + SYS_PLL3_SSCG_CTL);

	/* Dpll power down */
	reg = __raw_readl(clk->clkreg + SYS_PLL3_CFG1) | 0x1000;
	__raw_writel(reg, (void __iomem *)clk->clkreg + SYS_PLL3_CFG1);

	/* Dpll disable */
	reg = __raw_readl(clk->clkreg + SYS_PLL3_CFG2) & 0xffbfffff;
	__raw_writel(reg, (void __iomem *)clk->clkreg + SYS_PLL3_CFG2);

	xb2flush();
}

static int rlx_set_rate_m3(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	u32 reg;

	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 mul = (rate + parent_rate / 2) / parent_rate;

	if (mul < 2)
		return -1;

	clk->rate = rate;

	pr_debug("%s setrate:%d\n", hw->clk->name, mul);

	mul -= 2;
	reg = __raw_readl(clk->clkreg + SYS_PLL0_CFG0) & ~0x3ff00;
	reg |= mul << 8;
	__raw_writel(reg, (void __iomem *)clk->clkreg + SYS_PLL0_CFG0);

	xb2flush();

	return 0;
}

static unsigned long rlx_recalc_m3(struct clk_hw *hw, unsigned long parent_rate)
{
	u32 mul, rate;
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	mul = (__raw_readl(clk->clkreg + SYS_PLL0_CFG0) & 0x3ff00) >> 8;
	rate = parent_rate * (mul + 2);
	pr_debug("%s recalc: %u, mul: %u\n", hw->clk->name, rate, mul);

	return rate;
}

static const struct clk_ops rlx_pll3_ops_m = {
	.round_rate = rlx_round_rate_mul,
	.set_rate = rlx_set_rate_m3,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
	.recalc_rate = rlx_recalc_m3,
};

static long rlx_round_rate_n3(struct clk_hw *hw, unsigned long rate,
			      unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	int divisor = (parent_rate + rate / 2) / rate;
	unsigned long round_rate = parent_rate / divisor;

	pr_debug("%s round:%lu\n", hw->clk->name, round_rate);

	return round_rate;
}

static int rlx_set_rate_n3(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	u32 div, reg;

	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	div = (parent_rate + rate / 2) / rate;

	if (div < 2)
		return -1;

	clk->rate = rate;

	pr_debug("%s setrate: %d\n", hw->clk->name, div);

	div -= 2;
	reg = __raw_readl(clk->clkreg + SYS_PLL0_CFG0);
	reg = (reg & ~0xff) | div;
	__raw_writel(reg, (void __iomem *)clk->clkreg + SYS_PLL0_CFG0);

	xb2flush();

	return 0;
}

static unsigned long rlx_recalc_n3(struct clk_hw *hw, unsigned long parent_rate)
{
	u32 div, rate;
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	div = __raw_readl(clk->clkreg + SYS_PLL0_CFG0) & 0xff;
	rate = parent_rate / (div + 2);

	pr_debug("%s recalc: %u, div: %u\n", hw->clk->name, rate, div);

	return rate;
}

static const struct clk_ops rlx_pll3_ops_n = {
	.round_rate = rlx_round_rate_n3,
	.set_rate = rlx_set_rate_n3,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
	.recalc_rate = rlx_recalc_n3,
};

static long rlx_round_rate_f3(struct clk_hw *hw, unsigned long rate,
			      unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	unsigned long round_rate;
	u32 f, r, t;

	t = (parent_rate) / 256;
	r = rate - parent_rate;
	f = (r * 128 + t/2) / t;

	if (f > 4095)
		f = 4095;

	round_rate = (t * f + 64) / 128 + parent_rate;

	pr_debug("%s round: %lu %lu %lu\n",
		hw->clk->name, parent_rate, rate, round_rate);

	return round_rate;
}

static int rlx_set_rate_f3(struct clk_hw *hw, unsigned long rate,
			   unsigned long parent_rate)
{
	u32 reg;
	u32 f, r, t;
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	clk->rate = rate;

	t = (parent_rate) / 256;
	r = rate - parent_rate;
	f = (r * 128 + t/2) / t;

	reg = __raw_readl(clk->clkreg + SYS_PLL3_SSCG_CFG0) & ~0xfff;
	reg |= f;
	__raw_writel(reg, (void __iomem *)clk->clkreg + SYS_PLL3_SSCG_CFG0);

	reg = __raw_readl(clk->clkreg + SYS_PLL3_SSCG_CTL) & ~0x3;
	reg |= 3;
	__raw_writel(reg, (void __iomem *)clk->clkreg + SYS_PLL3_SSCG_CTL);

	xb2flush();

	pr_debug("%s round: %lu %lu %u\n",
		hw->clk->name, parent_rate, rate, f);

	return 0;
}

static unsigned long rlx_recalc_f3(struct clk_hw *hw, unsigned long parent_rate)
{
	u32 rate, f, t;
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);

	f = (__raw_readl(clk->clkreg + SYS_PLL3_SSCG_CFG0) & 0xfff);

	t = (parent_rate) / 256;

	rate = (t * f + 64) / 128 + parent_rate;

	pr_debug("%s recalc: %d\n", hw->clk->name, rate);

	return rate;
}

static const struct clk_ops rlx_pll3_ops_f = {
	.enable = rlx_enable_clk_pll3_4,
	.disable = rlx_disable_clk_pll3_4,
	.round_rate = rlx_round_rate_f3,
	.set_rate = rlx_set_rate_f3,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
	.recalc_rate = rlx_recalc_f3,
};

static int rlx_eth_prepare_clk(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 reg;

	reg = __raw_readl(clk->clkreg);
	reg &= ~ETH_EPHY_RST_N;
	__raw_writel(reg, clk->clkreg);
	xb2flush();

	return 0;
}

static void rlx_eth_unprepare_clk(struct clk_hw *hw)
{
}

static int rlx_eth_enable_clk(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 reg;

	reg = __raw_readl(clk->clkreg);
	reg |= CLK_EN_ETN_250M | ETH_EPHY_RST_N | ETH_EPHY_ADDR;
	__raw_writel(reg, clk->clkreg);
	xb2flush();

	return 0;
}

static void rlx_eth_disable_clk(struct clk_hw *hw)
{
}

static const struct clk_ops rlx_eth_clk_ops = {
	.prepare = rlx_eth_prepare_clk,
	.unprepare = rlx_eth_unprepare_clk,
	.enable = rlx_eth_enable_clk,
	.disable = rlx_eth_disable_clk,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
};

static int usbphy_enable_clk(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 reg;

	reg = __raw_readl(clk->clkreg);
	if (!strcmp(hw->clk->name, "usbphy_host_ck"))
		reg |= USBPHY_HOST_CLK_EN;
	else if (!strcmp(hw->clk->name, "usbphy_dev_ck"))
		reg |= USBPHY_DEV_CLK_EN;
	__raw_writel(reg, clk->clkreg);
	xb2flush();

	return 0;
}

static void usbphy_disable_clk(struct clk_hw *hw)
{
	struct clk_hw_rlx *clk = to_clk_hw_rlx(hw);
	u32 reg;

	reg = __raw_readl(clk->clkreg);
	if (!strcmp(hw->clk->name, "usbphy_host_ck"))
		reg &= ~USBPHY_HOST_CLK_EN;
	else if (!strcmp(hw->clk->name, "usbphy_dev_ck"))
		reg &= ~USBPHY_DEV_CLK_EN;
	__raw_writel(reg, clk->clkreg);
	xb2flush();
}

static const struct clk_ops usbphy_divider_ops = {
	.enable = usbphy_enable_clk,
	.disable = usbphy_disable_clk,
	.set_parent = rlx_set_parent,
	.get_parent = rlx_get_parent,
};

int rts_pll3_ssc_config(u32 range, u32 period)
{
	u32 r;

	r = __raw_readl(BSP_CLK_PLL3_BASE + SYS_PLL3_SSCG_CFG0) & 0xffc00fff;
	r |= (range & 0xf) << 12;
	r |= (period & 0x3f) << 16;

	__raw_writel(r, BSP_CLK_PLL3_BASE + SYS_PLL3_SSCG_CFG0);
	xb2flush();

	return 0;
}
EXPORT_SYMBOL_GPL(rts_pll3_ssc_config);

int rts_pll4_ssc_config(u32 range, u32 period)
{
	u32 r;

	r = __raw_readl(BSP_CLK_PLL4_BASE + SYS_PLL3_SSCG_CFG0) & 0xffc00fff;
	r |= (range & 0xf) << 12;
	r |= (period & 0x3f) << 16;

	__raw_writel(r, BSP_CLK_PLL4_BASE + SYS_PLL3_SSCG_CFG0);
	xb2flush();

	return 0;
}
EXPORT_SYMBOL_GPL(rts_pll4_ssc_config);

#ifdef CONFIG_SOC_FPGA_CODE
DEFINE_CLK_FIXED_RATE(usb_pll, CLK_IS_ROOT, 480000000, 0x0);
DEFINE_CLK_FIXED_RATE(sys_pll0, CLK_IS_ROOT, 1200000000, 0x0);
DEFINE_CLK_FIXED_RATE(usb_pll_2, CLK_IS_ROOT, 240000000, 0x0);
DEFINE_CLK_FIXED_RATE(usb_pll_3, CLK_IS_ROOT, 171000000, 0x0);
DEFINE_CLK_FIXED_RATE(usb_pll_5, CLK_IS_ROOT, 100000000, 0x0);
DEFINE_CLK_FIXED_RATE(usb_pll_7, CLK_IS_ROOT, 24000000, 0x0);
DEFINE_CLK_FIXED_RATE(sys_pll0_2, CLK_IS_ROOT, 300000000, 0x0);
DEFINE_CLK_FIXED_RATE(sys_pll0_3, CLK_IS_ROOT, 400000000, 0x0);
DEFINE_CLK_FIXED_RATE(sys_pll0_5, CLK_IS_ROOT, 240000000, 0x0);
DEFINE_CLK_FIXED_RATE(sys_pll0_7, CLK_IS_ROOT, 171000000, 0x0);
#else
DEFINE_CLK_FIXED_RATE(sys_osc, CLK_IS_ROOT, 25000000, 0x0);

DEFINE_CLK_RLX(sys_pll0, 0, BSP_CLK_PLL0_BASE, 0,
	       rlx_root_parent_names, rlx_pll0_ops);
DEFINE_CLK_RLX(sys_pll1, 0, BSP_CLK_PLL1_BASE, 0,
	       rlx_root_parent_names, rlx_pll3_4_ops);
DEFINE_CLK_RLX(sys_pll2, 0, BSP_CLK_PLL2_BASE, 0,
	       rlx_root_parent_names, rlx_pll3_4_ops);

DEFINE_CLK_RLX(sys_pll3_m, 0, BSP_CLK_PLL3_BASE, 0,
	       rlx_root_parent_names, rlx_pll3_ops_m);
DEFINE_CLK_RLX(sys_pll4_m, 0, BSP_CLK_PLL4_BASE, 0,
	       rlx_root_parent_names, rlx_pll3_ops_m);

DEFINE_CLK_RLX(sys_pll3_n, 0, BSP_CLK_PLL3_BASE, 0,
	       rlx_pll_parent_names_m3, rlx_pll3_ops_n);
DEFINE_CLK_RLX(sys_pll4_n, 0, BSP_CLK_PLL4_BASE, 0,
	       rlx_pll_parent_names_m4, rlx_pll3_ops_n);

DEFINE_CLK_RLX(sys_pll3_f, 0, BSP_CLK_PLL3_BASE, 0,
	       rlx_pll_parent_names_n3, rlx_pll3_ops_f);
DEFINE_CLK_RLX(sys_pll4_f, 0, BSP_CLK_PLL4_BASE, 0,
	       rlx_pll_parent_names_n4, rlx_pll3_ops_f);

DEFINE_CLK_FIXED_RATE(usb_pll, CLK_IS_ROOT, 480000000, 0x0);
DEFINE_CLK_FIXED_RATE(ddr_pll, CLK_IS_ROOT, 80000000, 0x0);
DEFINE_CLK_FIXED_RATE(etn_pll, CLK_IS_ROOT, 25000000, 0x0);
#endif

DEFINE_CLK_GATE(mcu_ck, "usb_pll", &usb_pll, 0x0, MCU_CLK_EN, 0, 0x0, NULL);
DEFINE_CLK_RLX(dram_ck, 1, DRAM_OCP_BUS_CLK_CFG_REG, DRAM_CLK_CHANGE,
	       rlx_ck_parent_names, rlx_divider_ops);
DEFINE_CLK_RLX(cpu_ck, 1, CPU_CLK_CFG_REG, CPU_CLK_CHANGE, rlx_ck_parent_names,
	       rlx_divider_ops);
DEFINE_CLK_RLX(bus_ck, 1, BUS_CLK_CFG_REG, BUS_CLK_CHANGE, rlx_ck_parent_names,
	       rlx_divider_ops);
DEFINE_CLK_RLX(xb2_ck, 1, XB2_CLK_CFG_REG, XB2_CLK_CHANGE, rlx_ck_parent_names,
	       rlx_divider_ops);
DEFINE_CLK_RLX(i2s_ck, 0, I2S_CLK_CFG_REG, I2S_CLK_CHANGE, rlx_ck_parent_names,
	       rlx_divider_ops);
DEFINE_CLK_RLX(cipher_ck, 0, CIPHER_CLK_CFG_REG, CIPHER_CLK_CHANGE,
	       rlx_ck_parent_names, rlx_divider_ops);
DEFINE_CLK_RLX(ethernet_ck, 0, ETHERNET_CLK_CFG_REG, 0,
	       rlx_ck_parent_names, rlx_eth_clk_ops);
DEFINE_CLK_RLX(uart_ck, 0, UART_CLK_CFG_REG, UART_CLK_CHANGE,
	       rlx_ck_parent_names, rlx_divider_ops);
DEFINE_CLK_RLX(i2c_ck, 0, I2C_CLK_CFG_REG, I2C_CLK_CHANGE, rlx_ck_parent_names,
	       rlx_divider_ops);
DEFINE_CLK_RLX(h264_ck, 0, H264_CLK_CFG_REG, H264_CLK_CHANGE,
	       rlx_ck_parent_names, rlx_divider_ops);
DEFINE_CLK_RLX(jpeg_ck, 0, JPEG_CLK_CFG_REG, 0,
	       rlx_ck_parent_names, rlx_divider_ops);
DEFINE_CLK_RLX(usbphy_host_ck, 0, USBPHY_CLK_CFG, 0,
	       rlx_ck_parent_names, usbphy_divider_ops);
DEFINE_CLK_RLX(usbphy_dev_ck, 0, USBPHY_CLK_CFG, 0,
	       rlx_ck_parent_names, usbphy_divider_ops);


static struct clk_lookup rlx_clks[] = {
#ifdef CONFIG_SOC_FPGA_CODE
	CLKDEV_INIT(NULL, "usb_pll", &usb_pll),
	CLKDEV_INIT(NULL, "usb_pll_2", &usb_pll_2),
	CLKDEV_INIT(NULL, "usb_pll_3", &usb_pll_3),
	CLKDEV_INIT(NULL, "usb_pll_5", &usb_pll_5),
	CLKDEV_INIT(NULL, "usb_pll_7", &usb_pll_7),
	CLKDEV_INIT(NULL, "sys_pll0_2", &sys_pll0_2),
	CLKDEV_INIT(NULL, "sys_pll0_3", &sys_pll0_3),
	CLKDEV_INIT(NULL, "sys_pll0_5", &sys_pll0_5),
	CLKDEV_INIT(NULL, "sys_pll0_7", &sys_pll0_7),
#else
	CLKDEV_INIT(NULL, "sys_osc", &sys_osc),
	CLKDEV_INIT(NULL, "sys_pll0", &sys_pll0),
	CLKDEV_INIT(NULL, "sys_pll1", &sys_pll1),
	CLKDEV_INIT(NULL, "sys_pll2", &sys_pll2),
	CLKDEV_INIT(NULL, "sys_pll3_m", &sys_pll3_m),
	CLKDEV_INIT(NULL, "sys_pll4_m", &sys_pll4_m),
	CLKDEV_INIT(NULL, "sys_pll3_n", &sys_pll3_n),
	CLKDEV_INIT(NULL, "sys_pll4_n", &sys_pll4_n),
	CLKDEV_INIT(NULL, "sys_pll3_f", &sys_pll3_f),
	CLKDEV_INIT(NULL, "sys_pll4_f", &sys_pll4_f),
	CLKDEV_INIT(NULL, "usb_pll", &usb_pll),
	CLKDEV_INIT(NULL, "ddr_pll", &ddr_pll),
	CLKDEV_INIT(NULL, "etn_pll", &etn_pll),
#endif
	CLKDEV_INIT(NULL, "dram_ck", &dram_ck),
	CLKDEV_INIT(NULL, "cpu_ck", &cpu_ck),
	CLKDEV_INIT(NULL, "bus_ck", &bus_ck),
	CLKDEV_INIT(NULL, "xb2_ck", &xb2_ck),
	CLKDEV_INIT(NULL, "i2s_ck", &i2s_ck),
	CLKDEV_INIT(NULL, "cipher_ck", &cipher_ck),
	CLKDEV_INIT(NULL, "ethernet_ck", &ethernet_ck),
	CLKDEV_INIT(NULL, "uart_ck", &uart_ck),
	CLKDEV_INIT(NULL, "i2c_ck", &i2c_ck),
	CLKDEV_INIT(NULL, "h264_ck", &h264_ck),
	CLKDEV_INIT(NULL, "mcu_ck", &mcu_ck),
	CLKDEV_INIT(NULL, "jpeg_ck", &jpeg_ck),
	CLKDEV_INIT(NULL, "usbphy_host_ck", &usbphy_host_ck),
	CLKDEV_INIT(NULL, "usbphy_dev_ck", &usbphy_dev_ck),
};

static void __init rlx_clocks_register(struct clk_lookup oclks[], int cnt)
{
	struct clk_lookup *c = &oclks[0];

	for (c = oclks; c < oclks + cnt; c++) {

		clkdev_add(c);
		if (__clk_init(NULL, c->clk)) {
			pr_debug("init clock:%s error\n", c->clk->name);
			break;
		}
	}
}

static void rlx_clock_hw_init(void)
{
	u32 reg;

	/* disable usbphy clk */
	reg = __raw_readl(USBPHY_CLK_CFG);
	reg &= ~(USBPHY_HOST_CLK_EN | USBPHY_DEV_CLK_EN);
	__raw_writel(reg, USBPHY_CLK_CFG);
}

void __init rlx_clock_init(void)
{
	rlx_clock_hw_init();
	rlx_clocks_register(rlx_clks, ARRAY_SIZE(rlx_clks));
}
