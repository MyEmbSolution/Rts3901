/* Driver for Realtek PWM
 *
 * Copyright(c) 2014 Realtek Semiconductor Corp. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 *   Peter Sun <peter_sun@realsil.com.cn>
 *   No. 128, West Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>

#define NUM_PWM 4

#define XB2_PWM_EN 0
#define XB2_PWM_KEEP 4
#define XB2_PWM_HIGH 8
#define XB2_PWM_LOW 0xc
#define XB2_PWM_FLAG 0x10
#define XB2_PWM_NUM 0x14
#define XB2_PWM_CNT 0x1c

struct rtsx_pwm_chip {
	struct pwm_chip chip;
	struct pinctrl *pinctrl;
	struct pinctrl_state *default_state;
	struct device *dev;
	struct clk *clk;
	void __iomem *mmio_base;
};

static inline struct rtsx_pwm_chip *to_rtsx_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct rtsx_pwm_chip, chip);
}

static inline u32 pwm_readl(struct rtsx_pwm_chip *chip, unsigned int num,
			    int offset)
{
	return readl(chip->mmio_base + (offset + num * 0x40));
}

static inline void pwm_writel(struct rtsx_pwm_chip *chip, unsigned int num,
			      int offset, unsigned int val)
{
	writel(val, chip->mmio_base + offset + num * 0x40);
}

#define NS_IN_HZ (1000000000UL)

static int rtsx_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			   int duty_ns, int period_ns)
{
	struct rtsx_pwm_chip *pc = to_rtsx_pwm_chip(chip);
	unsigned long offset;
	unsigned long high, total, low;
	unsigned long tin_ns;

	offset = pwm->hwpwm;

	total = clk_get_rate(pc->clk);
	tin_ns = NS_IN_HZ / total;

	high = duty_ns / tin_ns;
	total = period_ns / tin_ns;
	low = total - high;

	writel(high, pc->mmio_base + offset * 0x40 + XB2_PWM_HIGH);
	writel(low, pc->mmio_base + offset * 0x40 + XB2_PWM_LOW);

	return 0;
}

static int rtsx_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct rtsx_pwm_chip *pc = to_rtsx_pwm_chip(chip);
	pwm_writel(pc, pwm->hwpwm, XB2_PWM_EN, 1);

	return 0;
}

static void rtsx_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct rtsx_pwm_chip *pc = to_rtsx_pwm_chip(chip);
	pwm_writel(pc, pwm->hwpwm, XB2_PWM_EN, 0);

	return;
}

static const struct pwm_ops rtsx_pwm_ops = {
	.config = rtsx_pwm_config,
	.enable = rtsx_pwm_enable,
	.disable = rtsx_pwm_disable,
	.owner = THIS_MODULE,
};

static int rtsx_pwm_probe(struct platform_device *pdev)
{
	struct rtsx_pwm_chip *pwm;
	struct resource *r;
	int ret;

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pwm->dev = &pdev->dev;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pwm->mmio_base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(pwm->mmio_base))
		return PTR_ERR(pwm->mmio_base);

	platform_set_drvdata(pdev, pwm);

	pwm->clk = devm_clk_get(&pdev->dev, "xb2_ck");
	if (IS_ERR(pwm->clk)) {
		dev_err(&pdev->dev, "failed to get xb2 clock\n");
		return PTR_ERR(pwm->clk);
	}

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &rtsx_pwm_ops;
	pwm->chip.base = -1;
	pwm->chip.npwm = NUM_PWM;

	pwm->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pwm->pinctrl))
		return PTR_ERR(pwm->pinctrl);

	pwm->default_state = pinctrl_lookup_state(pwm->pinctrl,
						  PINCTRL_STATE_DEFAULT);

	/* Allow pins to be muxed in and configured */
	if (IS_ERR(pwm->default_state)) {
		dev_err(&pdev->dev, "could not get default status\n");
		return PTR_ERR(pwm->default_state);
	}

	ret = pinctrl_select_state(pwm->pinctrl, pwm->default_state);
	if (ret) {
		dev_err(&pdev->dev, "could not set default pins\n");
		return ret;
	}

	writel(1, pwm->mmio_base + XB2_PWM_KEEP);
	writel(1, pwm->mmio_base + 0x40 + XB2_PWM_KEEP);
	writel(1, pwm->mmio_base + 0x80 + XB2_PWM_KEEP);
	writel(1, pwm->mmio_base + 0xc0 + XB2_PWM_KEEP);

	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int rtsx_pwm_remove(struct platform_device *pdev)
{
	struct pinctrl_state *sleep_state;
	int ret = 0;

	struct rtsx_pwm_chip *pwm = platform_get_drvdata(pdev);
	int i;

	if (WARN_ON(!pwm))
		return -ENODEV;

	for (i = 0; i < NUM_PWM; i++)
		pwm_writel(pwm, i, XB2_PWM_EN, 0);

	ret = pwmchip_remove(&pwm->chip);
	if (ret)
		return ret;

	sleep_state = pinctrl_lookup_state(pwm->pinctrl, PINCTRL_STATE_SLEEP);

	/* Allow pins to be muxed in and configured */
	if (IS_ERR(sleep_state)) {
		dev_err(&pdev->dev, "could not get sleep status\n");
		return PTR_ERR(sleep_state);
	} else {
		ret = pinctrl_select_state(pwm->pinctrl, sleep_state);
		if (ret)
			dev_err(&pdev->dev, "could not set sleep status\n");
	}

	return ret;

}

static struct platform_driver pwm_driver = {
	.driver = {
		   .name = "pwm_platform",
		   .owner = THIS_MODULE,
		   },
	.probe = rtsx_pwm_probe,
	.remove = rtsx_pwm_remove,
};

static int __init pwm_init(void)
{
	return platform_driver_register(&pwm_driver);
}

module_init(pwm_init);

static void __exit pwm_exit(void)
{
	platform_driver_unregister(&pwm_driver);
}

module_exit(pwm_exit);
