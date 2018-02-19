/*
 * rtp-regulator.c  --  Realsil RTP
 *
 * Copyright 2013 Realsil Semiconductor Corp.
 *
 * Author: Wind Han <wind_han@realsil.com.cn>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/mfd/rtp-mfd.h>
#include <linux/interrupt.h>

struct rtp_regulator_info {
	struct regulator_desc desc;
	int min_uV;
	int max_uV;
	int step_uV;
	int vol_reg;
	int vol_shift;
	int vol_nbits;
	int enable_reg;
	int enable_bit;
	int stb_vol_reg;
	int stb_enable_reg;
	uint8_t vol_offset;
};

#define RTP_ENABLE_TIME_US	1000

struct rtp_regu {
	struct rtp_mfd_chip *chip;
	struct regulator_dev **rdev;
	struct rtp_regulator_info **info;
	struct mutex mutex;
	int num_regulators;
	int mode;
	int  (*get_ctrl_reg)(int);
	unsigned int *ext_sleep_control;
	unsigned int board_ext_control[RTP_ID_REGULATORS_NUM];
};

static inline int check_range(struct rtp_regulator_info *info,
			      int min_uV, int max_uV)
{
	if (min_uV < info->min_uV || min_uV > info->max_uV)
		return -EINVAL;

	return 0;
}

/* rtp regulator common operations */
static int rtp_set_voltage(struct regulator_dev *rdev, int min_uV,
			       int max_uV, unsigned *selector)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	printk(KERN_DEBUG "Set regulator-%d voltage as %d uV\n", id, min_uV);
	if (check_range(info, min_uV, max_uV)) {
		pr_err("invalid voltage range (%d, %d) uV\n", min_uV, max_uV);
		return -EINVAL;
	}

	val = (min_uV - info->min_uV + info->step_uV - 1) / info->step_uV;
	*selector = val;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp_update_bits(chip, info->vol_reg, val, mask);
}

static int rtp_get_voltage(struct regulator_dev *rdev)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;
	int ret;

	printk(KERN_DEBUG "Get regulator-%d voltage\n", id);
	ret = rtp_reg_read(chip, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	val -= info->vol_offset;
	ret = info->min_uV + info->step_uV * val;

	return ret;
}

static int rtp_set_voltage_sel(struct regulator_dev *rdev,
				   unsigned selector)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	printk(KERN_DEBUG "Set regulator-%d voltage selector as %d\n", id, selector);
	if (selector < 0 || selector >= info->desc.n_voltages) {
		pr_err("invalid selector range (%d, %d) uV, selector = %d\n", 0, info->desc.n_voltages - 1, selector);
		return -EINVAL;
	}

	val = selector;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp_update_bits(chip, info->vol_reg, val, mask);
}

static int rtp_get_voltage_sel(struct regulator_dev *rdev)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;
	int ret;

	printk(KERN_DEBUG "Get regulator-%d voltage selector\n", id);
	ret = rtp_reg_read(chip, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	val -= info->vol_offset;
	if (val < 0 || val >= info->desc.n_voltages) {
		pr_err("invalid selector range (%d, %d) uV, val = %d\n", 0, info->desc.n_voltages - 1, val);
		return -EINVAL;
	}
	ret = val;

	return ret;
}

static int rtp_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	int ret;

	printk(KERN_DEBUG "List regulator-%d voltage of selector-%d\n", id, selector);
	ret = info->min_uV + info->step_uV * selector;

	if (ret > info->max_uV)
		return -EINVAL;

	return ret;
}

static int rtp_set_voltage_time_sel(struct regulator_dev *rdev,
		unsigned int old_selector, unsigned int new_selector)
{
	int old_volt, new_volt, ret, step;
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_mfd_chip *chip = pmic->chip;
	uint8_t val;

	printk(KERN_DEBUG "Set regulator-%d voltage time from selector %u to selector %u\n", id, old_selector, new_selector);
	old_volt = rtp_list_voltage(rdev, old_selector);
	if (old_volt < 0)
		return old_volt;

	new_volt = rtp_list_voltage(rdev, new_selector);
	if (new_volt < 0)
		return new_volt;

	ret = rtp_reg_read(chip, RTP_REG_DVS_STEP_CTRL, &val);
	if (ret)
		return ret;

	switch (id) {
	case RTP_ID_BUCK1:
		step = 0x20 >> (val & 0x03);
		break;
	case RTP_ID_BUCK2:
		step = 0x20 >> ((val >> 2) & 0x03);
		break;
	case RTP_ID_BUCK3:
		step = 0x20 >> ((val >> 4) & 0x03);
		break;
	case RTP_ID_BUCK4:
		step = 0x20 >> ((val >> 6) & 0x03);
		break;
	default:
		return -EINVAL;
	}
	ret = DIV_ROUND_UP(abs(old_volt - new_volt), RTP_DVS_STEP_US)
			* step;
	return ret;
}

static int rtp_enable(struct regulator_dev *rdev)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Enable regulator-%d\n", id);
	return rtp_set_bits(chip, info->enable_reg, 1 << info->enable_bit);
}

static int rtp_disable(struct regulator_dev *rdev)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Disable regulator-%d\n", id);
	return rtp_clr_bits(chip, info->enable_reg, 1 << info->enable_bit);
}

static int rtp_enable_time(struct regulator_dev *rdev)
{
	int id = rdev_get_id(rdev);

	printk(KERN_DEBUG "Get regulator-%d enable time\n", id);
	return RTP_ENABLE_TIME_US;
}

static int rtp_is_enabled(struct regulator_dev *rdev)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;
	uint8_t reg_val;
	int ret;

	printk(KERN_DEBUG "Get whether regulator-%d is enabled\n", id);
	ret = rtp_reg_read(chip, info->enable_reg, &reg_val);
	if (ret)
		return ret;

	ret = !!(reg_val & (1 << info->enable_bit));

	return ret;
}

static int rtp_suspend_enable(struct regulator_dev *rdev)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Enable regulator-%d on suspend state\n", id);
	return rtp_clr_bits(chip, info->stb_enable_reg,
				1 << info->enable_bit);
}

static int rtp_suspend_disable(struct regulator_dev *rdev)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Disable regulator-%d on suspend state\n", id);
	return rtp_set_bits(chip, info->stb_enable_reg,
				1 << info->enable_bit);
}

static int rtp_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
	struct rtp_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp_regulator_info *info = pmic->info[id];
	struct rtp_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	printk(KERN_DEBUG "Set regulator-%d suspend voltage as %d uV\n", id, uV);
	if (check_range(info, uV, uV)) {
		pr_err("invalid voltage range (%d, %d) uV\n", uV, uV);
		return -EINVAL;
	}

	val = (uV - info->min_uV + info->step_uV - 1) / info->step_uV;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp_update_bits(chip, info->stb_vol_reg, val, mask);
}

static struct regulator_ops rtp_ops_dcdc = {
	.set_voltage_sel = rtp_set_voltage_sel,
	.get_voltage_sel = rtp_get_voltage_sel,
	.list_voltage = rtp_list_voltage,
	.set_voltage_time_sel = rtp_set_voltage_time_sel,
	.enable	= rtp_enable,
	.disable = rtp_disable,
	.enable_time = rtp_enable_time,
	.is_enabled = rtp_is_enabled,
	.set_suspend_enable = rtp_suspend_enable,
	.set_suspend_disable = rtp_suspend_disable,
	.set_suspend_voltage = rtp_set_suspend_voltage,
};

static struct regulator_ops rtp_ops = {
	.set_voltage = rtp_set_voltage,
	.get_voltage = rtp_get_voltage,
	.list_voltage = rtp_list_voltage,
	.enable	= rtp_enable,
	.disable = rtp_disable,
	.enable_time = rtp_enable_time,
	.is_enabled = rtp_is_enabled,
	.set_suspend_enable = rtp_suspend_enable,
	.set_suspend_disable = rtp_suspend_disable,
	.set_suspend_voltage = rtp_set_suspend_voltage,
};

static struct rtp_regulator_info rtp_regulator_infos[] = {
	RTP_LDO("RTP_LDO1", RTP_ID_LDO1, RTP_LDO1_VOL,
		    RTP_LDO1_VOL, 0, RTP_REG_NONE_WORTH, 0, 0,
		    RTP_REG_NONE_WORTH, 0, RTP_REG_NONE_WORTH,
		    RTP_REG_NONE_WORTH, 0),
	RTP_LDO("RTP_LDO2", RTP_ID_LDO2, 1000, 2500, 100,
		    RTP_REG_LDO2VOUT_SET, 0, 7, RTP_REG_POWER_EN,
		    4, RTP_REG_STB_LDO2VOUT, RTP_REG_POWER_STB_EN, 0),
	RTP_LDO("RTP_LDO3", RTP_ID_LDO3, 1800, 3300, 100,
		    RTP_REG_LDO3VOUT_SET, 0, 7, RTP_REG_POWER_EN, 5,
		    RTP_REG_STB_LDO3VOUT, RTP_REG_POWER_STB_EN, 0),
	RTP_LDO("RTP_LDO4", RTP_ID_LDO4, 1800, 3300, 100,
		    RTP_REG_LDO4VOUT_SET, 0, 7, RTP_REG_POWER_EN, 6,
		    RTP_REG_STB_LDO4VOUT, RTP_REG_POWER_STB_EN, 0),
	RTP_LDO("RTP_LDO5", RTP_ID_LDO5, 1800, 3300, 50,
		    RTP_REG_LDO5VOUT_SET, 0, 7, RTP_REG_POWER_EN, 7,
		    RTP_REG_STB_LDO5VOUT, RTP_REG_POWER_STB_EN, 0),
	RTP_BUCK("RTP_BUCK1", RTP_ID_BUCK1, 700, 2275, 25,
		     RTP_REG_DC1VOUT_SET, 0, 6, RTP_REG_POWER_EN, 0,
		     RTP_REG_STB_DC1VOUT, RTP_REG_POWER_STB_EN, 0),
	RTP_BUCK("RTP_BUCK2", RTP_ID_BUCK2, 700, 2275, 25,
		     RTP_REG_DC2VOUT_SET, 0, 6, RTP_REG_POWER_EN, 1,
		     RTP_REG_STB_DC2VOUT, RTP_REG_POWER_STB_EN, 0),
	RTP_BUCK("RTP_BUCK3", RTP_ID_BUCK3, 700, 2275, 25,
		     RTP_REG_DC3VOUT_SET, 0, 6, RTP_REG_POWER_EN, 2,
		     RTP_REG_STB_DC3VOUT, RTP_REG_POWER_STB_EN, 0),
	RTP_BUCK("RTP_BUCK4", RTP_ID_BUCK4, 1700, 3500, 100,
		     RTP_REG_DC4VOUT_SET, 0, 6, RTP_REG_POWER_EN, 3,
		     RTP_REG_STB_DC4VOUT, RTP_REG_POWER_STB_EN, 0x0A),
};


static int rtp_regulator_probe(struct platform_device *pdev)
{
	struct rtp_mfd_chip *chip = dev_get_drvdata(pdev->dev.parent);
	struct regulator_config config = { };
	struct rtp_regulator_info *info;
	struct regulator_init_data *reg_data;
	struct regulator_dev *rdev;
	struct rtp_regu *pmic;
	struct rtp_board *pmic_plat_data;
	int i, err;

	pmic_plat_data = dev_get_platdata(chip->dev);
	if (!pmic_plat_data)
		return -EINVAL;

	pmic = kzalloc(sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	mutex_init(&pmic->mutex);
	pmic->chip = chip;
	platform_set_drvdata(pdev, pmic);

	switch(rtp_chip_id(chip)) {
	case RTP_ID:
		pmic->num_regulators = ARRAY_SIZE(rtp_regulator_infos);
		info = rtp_regulator_infos;
		break;
	default:
		pr_err("Invalid rtp chip version\n");
		kfree(pmic);
		return -ENODEV;
	}

	pmic->info = kcalloc(pmic->num_regulators,
			     sizeof(struct rtp_regulator_info *),
			     GFP_KERNEL);
	if (!pmic->info) {
		err = -ENOMEM;
		goto err_free_pmic;
	}

	pmic->rdev = kcalloc(pmic->num_regulators,
			     sizeof(struct regulator_dev *), GFP_KERNEL);
	if (!pmic->rdev) {
		err = -ENOMEM;
		goto err_free_info;
	}

	for (i = 0; i < pmic->num_regulators && i < RTP_ID_REGULATORS_NUM;
			i++, info++) {
		reg_data = pmic_plat_data->rtp_pmic_init_data[i];

		if (!reg_data)
			continue;

		if (i < RTP_ID_BUCK1)
			info->desc.ops = &rtp_ops;
		else
			info->desc.ops = &rtp_ops_dcdc;

		pmic->info[i] = info;

		config.dev = chip->dev;
		config.init_data = reg_data;
		config.driver_data = pmic;
		config.regmap = chip->regmap;

		rdev = regulator_register(&pmic->info[i]->desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(chip->dev,
				"failed to register %s regulator\n",
				pdev->name);
			err = PTR_ERR(rdev);
			goto err_unregister_regulator;
		}

		pmic->rdev[i] = rdev;
	}


	return 0;

err_unregister_regulator:
	while (--i >= 0)
		regulator_unregister(pmic->rdev[i]);
	kfree(pmic->rdev);
err_free_info:
	kfree(pmic->info);
err_free_pmic:
	kfree(pmic);
	return err;
}

static int rtp_regulator_remove(struct platform_device *pdev)
{
	struct rtp_regu *pmic = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < pmic->num_regulators; i++)
		regulator_unregister(pmic->rdev[i]);


	kfree(pmic->rdev);
	kfree(pmic->info);
	kfree(pmic);
	return 0;
}


static struct platform_driver rtp_regulator_driver = {
	.driver	= {
		.name = "rtp-pmic",
		.owner = THIS_MODULE,
	},
	.probe = rtp_regulator_probe,
	.remove	= rtp_regulator_remove,
};

static int __init rtp_regulator_init(void)
{
	return platform_driver_register(&rtp_regulator_driver);
}
subsys_initcall(rtp_regulator_init);

static void __exit rtp_regulator_exit(void)
{
	platform_driver_unregister(&rtp_regulator_driver);
}
module_exit(rtp_regulator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
