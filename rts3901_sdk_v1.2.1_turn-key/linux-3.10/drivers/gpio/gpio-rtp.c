/*
 * rtp-gpio.c  --  Realsil RTP
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
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/mfd/rtp-mfd.h>

struct rtp_gpio {
	struct gpio_chip gpio_chip;
	struct rtp_mfd_chip *chip;
};

/*
*  gpio 0: gpio0
*	1: gpio1
*	2: gpio3
*	3: rtc_clk
*	4: ldo1
*	5: exten
*/

static int rtp_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	uint8_t val;
	struct rtp_gpio *rtp_gpio = dev_get_drvdata(gc->dev);
	struct rtp_mfd_chip *chip = rtp_gpio->chip;

	printk(KERN_DEBUG "Get rtp gpio-%d value\n", offset);
	switch (offset) {
	case 0:
	case 1:
		rtp_reg_read(chip, RTP_REG_GPIO0_CTRL + offset, &val);
		val &= 0x07;
		if (val == 0x01)
			return 1;
		else
			return 0;
	case 2:
		rtp_reg_read(chip, RTP_REG_GPIO3_CTRL, &val);
		val &= 0x07;
		if (val == 0x01)
			return 1;
		else
			return 0;
	case 3:
		rtp_reg_read(chip, RTP_REG_XTAL_CLK_SET, &val);
		val &= 0x18;
		if (val == 0x10)
			return 1;
		else
			return 0;
	case 4:
		rtp_reg_read(chip, RTP_REG_LDO1_CTL, &val);
		val &= 0x10;
		if (val == 0x10)
			return 1;
		else
			return 0;
	case 5:
		rtp_reg_read(chip, RTP_REG_EXTEN_EN, &val);
		val &= 0x01;
		if (val == 0x01)
			return 1;
		else
			return 0;
	default:
		dev_warn(gc->dev, "rtp gpio 0-5, offset = %d\n", offset);
		return -EINVAL;
	}

	return 0;
}

static void rtp_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	uint8_t val;
	struct rtp_gpio *rtp_gpio = dev_get_drvdata(gc->dev);
	struct rtp_mfd_chip *chip = rtp_gpio->chip;

	printk(KERN_DEBUG "Set rtp gpio-%d value, value = %d\n", offset, value);
	if (value) {
		switch (offset) {
		case 0:
		case 1:
			rtp_reg_read(chip,
					 RTP_REG_GPIO0_CTRL + offset,
					 &val);
			val &= 0x07;
			if (val == 0x02)
				return;
			rtp_update_bits(chip,
					    RTP_REG_GPIO0_CTRL + offset,
					    0x01, 0x07);
			break;
		case 2:
			rtp_reg_read(chip, RTP_REG_GPIO3_CTRL, &val);
			val &= 0x07;
			if (val == 0x02)
				return;
			rtp_update_bits(chip,
					    RTP_REG_GPIO3_CTRL,
					    0x01, 0x07);
			break;
		case 3:
			rtp_update_bits(chip, RTP_REG_XTAL_CLK_SET,
					    0x10, 0x18);
			break;
		case 4:
			rtp_set_bits(chip, RTP_REG_LDO1_CTL, 0x10);
			break;
		case 5:
			rtp_set_bits(chip, RTP_REG_EXTEN_EN, 0x01);
			rtp_clr_bits(chip, RTP_REG_EXTEN_EN, 0x04);
			break;
		default:
			dev_warn(gc->dev, "rtp gpio 0-5, offset = %d\n", offset);
			return;
		}
	} else {
		switch (offset) {
		case 0:
		case 1:
			rtp_reg_read(chip,
					 RTP_REG_GPIO0_CTRL + offset,
					 &val);
			val &= 0x07;
			if (val == 0x02)
				return;
			rtp_clr_bits(chip,
					 RTP_REG_GPIO0_CTRL + offset,
					 0x07);
			break;
		case 2:
			rtp_reg_read(chip, RTP_REG_GPIO3_CTRL, &val);
			val &= 0x07;
			if (val == 0x02)
				return;
			rtp_clr_bits(chip, RTP_REG_GPIO3_CTRL, 0x07);
			break;
		case 3:
			rtp_update_bits(chip, RTP_REG_XTAL_CLK_SET,
					    0x08, 0x18);
			break;
		case 4:
			rtp_clr_bits(chip, RTP_REG_LDO1_CTL, 0x10);
			break;
		case 5:
			rtp_clr_bits(chip, RTP_REG_EXTEN_EN, 0x01);
			rtp_set_bits(chip, RTP_REG_EXTEN_EN, 0x04);
			break;
		default:
			dev_warn(gc->dev, "rtp gpio 0-5, offset = %d\n", offset);
			return;
		}
	}
}

static int rtp_gpio_doutput(struct gpio_chip *gc, unsigned offset,
				int value)
{
	struct rtp_gpio *rtp_gpio = dev_get_drvdata(gc->dev);
	struct rtp_mfd_chip *chip = rtp_gpio->chip;

	printk(KERN_DEBUG "Set rtp gpio-%d as output, value %d\n", offset, value);
	if (value) {
		switch (offset) {
		case 0:
		case 1:
			return rtp_update_bits(chip,
					RTP_REG_GPIO0_CTRL + offset,
					0x01, 0x07);
		case 2:
			return rtp_update_bits(chip,
						   RTP_REG_GPIO3_CTRL,
						   0x01, 0x07);
		case 3:
			return rtp_update_bits(chip,
						   RTP_REG_XTAL_CLK_SET,
						   0x10, 0x18);
		case 4:
			return rtp_set_bits(chip, RTP_REG_LDO1_CTL,
						0x10);
		case 5:
			rtp_set_bits(chip, RTP_REG_EXTEN_EN, 0x01);
			return rtp_clr_bits(chip, RTP_REG_EXTEN_EN,
						0x04);
		default:
			dev_warn(gc->dev, "rtp gpio 0-5, offset = %d\n", offset);
			return -EINVAL;
		}
	} else {
		switch (offset) {
		case 0:
		case 1:
			return rtp_clr_bits(chip,
					RTP_REG_GPIO0_CTRL + offset, 0x07);
		case 2:
			return rtp_clr_bits(chip, RTP_REG_GPIO3_CTRL,
						0x07);
		case 3:
			return rtp_update_bits(chip,
						   RTP_REG_XTAL_CLK_SET,
						   0x08, 0x18);
		case 4:
			return rtp_clr_bits(chip, RTP_REG_LDO1_CTL,
						0x10);
		case 5:
			rtp_clr_bits(chip, RTP_REG_EXTEN_EN, 0x01);
			return rtp_set_bits(chip, RTP_REG_EXTEN_EN,
						0x04);
		default:
			dev_warn(gc->dev, "rtp gpio 0-5, offset = %d\n", offset);
			return -EINVAL;
		}
	}

	return 0;
}

static int rtp_gpio_dinput(struct gpio_chip *gc, unsigned offset)
{
	struct rtp_gpio *rtp_gpio = dev_get_drvdata(gc->dev);
	struct rtp_mfd_chip *chip = rtp_gpio->chip;

	printk(KERN_DEBUG "Set rtp gpio-%d as input\n", offset);
	switch (offset) {
		case 0:
		case 1:
			return rtp_update_bits(chip,
					RTP_REG_GPIO0_CTRL + offset,
					0x02, 0x07);
		case 2:
			return rtp_update_bits(chip,
						   RTP_REG_GPIO3_CTRL,
						   0x02, 0x07);
		case 3:
		case 4:
		case 5:
			dev_warn(gc->dev, "rtp gpio-3/4/5 only output\n");
			return 0;
		default:
			dev_warn(gc->dev, "rtp gpio 0-5, offset = %d\n", offset);
			return -EINVAL;
	}

	return 0;
}

static ssize_t rtp_gpio_direction_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	return sprintf(buf, "Show rtp gpio direction\n");
}

static ssize_t rtp_gpio_direction_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_gpio *rtp_gpio = platform_get_drvdata(pdev);
	struct gpio_chip *gpio_chip = &rtp_gpio->gpio_chip;
	int gpio_dir, gpio_idx, gpio_val = -1;
	char buftemp[2];

	if (count != 3 && count != 4)
		return count;

	/* index */
	buftemp[0] = buf[0];
	buftemp[1] = '\0';
	gpio_idx = simple_strtoul(buftemp, NULL, 10);

	/* direction */
	buftemp[0] = buf[1];
	buftemp[1] = '\0';
	gpio_dir = simple_strtoul(buftemp, NULL, 10);

	if (count == 4) {
		/* val */
		buftemp[0] = buf[2];
		buftemp[1] = '\0';
		gpio_val = simple_strtoul(buftemp, NULL, 10);
	}

	printk(KERN_DEBUG "Set rtp gpio-%d direction, dir = %d\n", gpio_idx, gpio_dir);

	if (gpio_idx < 0 || gpio_idx >= gpio_chip->ngpio
			|| gpio_dir < 0 || gpio_dir > 1)
		return count;

	if (gpio_dir == 0)
		gpio_direction_input(gpio_chip->base + gpio_idx);
	else {
		if (gpio_val < 0 || gpio_val > 1)
			return count;
		gpio_direction_output(gpio_chip->base + gpio_idx, gpio_val);
	}

	return count;
}

static int rtp_gpio_offset = 0;
static ssize_t rtp_gpio_value_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_gpio *rtp_gpio = platform_get_drvdata(pdev);
	struct gpio_chip *gpio_chip = &rtp_gpio->gpio_chip;

	return sprintf(buf, "%d\n",
			gpio_get_value(gpio_chip->base + rtp_gpio_offset));
}

static ssize_t rtp_gpio_value_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_gpio *rtp_gpio = platform_get_drvdata(pdev);
	struct gpio_chip *gpio_chip = &rtp_gpio->gpio_chip;

	char buftemp[2];
	int val;

	if (count != 3)
		return count;

	/* offset */
	buftemp[0] = buf[0];
	buftemp[1] = '\0';
	rtp_gpio_offset = simple_strtoul(buftemp, NULL, 10);

	/* value */
	buftemp[0] = buf[1];
	buftemp[1] = '\0';
	val = simple_strtoul(buftemp, NULL, 10);

	gpio_set_value(gpio_chip->base + rtp_gpio_offset, val);

	return count;
}

static struct device_attribute rtp_gpio_attrs[] = {
	RTP_MFD_ATTR(rtp_gpio_direction),
	RTP_MFD_ATTR(rtp_gpio_value),
};

static int rtp_gpio_probe(struct platform_device *pdev)
{
	int ret, err, j;
	struct rtp_mfd_chip *chip = dev_get_drvdata(pdev->dev.parent);
	struct rtp_board *pmic_plat_data = dev_get_platdata(chip->dev);
	struct rtp_gpio *rtp_gpio;

	rtp_gpio = kzalloc(sizeof(*rtp_gpio), GFP_KERNEL);
	if (rtp_gpio == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, rtp_gpio);
	rtp_gpio->chip = chip;



	rtp_gpio->gpio_chip.owner = THIS_MODULE;
	rtp_gpio->gpio_chip.label = chip->i2c_client->name;
	rtp_gpio->gpio_chip.dev = &pdev->dev;
	rtp_gpio->gpio_chip.base = pmic_plat_data->gpio_base;

	switch(rtp_chip_id(chip)) {
		case RTP_ID:
			rtp_gpio->gpio_chip.ngpio = RTP_GPIO_NUM;
			break;
		default:
			return -EINVAL;
	}

	rtp_gpio->gpio_chip.direction_input = rtp_gpio_dinput;
	rtp_gpio->gpio_chip.direction_output = rtp_gpio_doutput;
	rtp_gpio->gpio_chip.set = rtp_gpio_set;
	rtp_gpio->gpio_chip.get = rtp_gpio_get;

	ret = gpiochip_add(&rtp_gpio->gpio_chip);
	if (ret) {
		dev_warn(chip->dev, "GPIO registration failed: %d\n", ret);
		return ret;
	}

	for (j = 0; j < ARRAY_SIZE(rtp_gpio_attrs); j++) {
		ret = device_create_file(&pdev->dev, &rtp_gpio_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}

	return 0;

sysfs_failed:
	while (j--)
		device_remove_file(&pdev->dev, &rtp_gpio_attrs[j]);
	err = gpiochip_remove(&rtp_gpio->gpio_chip);
	if (err) {
		dev_warn(chip->dev, "GPIO unregistration failed: %d\n", err);
		return err;
	}
	return ret;
}

static int rtp_gpio_remove(struct platform_device *pdev)
{
	int j;
	struct rtp_gpio *rtp_gpio = platform_get_drvdata(pdev);

	for (j = 0; j < ARRAY_SIZE(rtp_gpio_attrs); j++)
		device_remove_file(&pdev->dev, &rtp_gpio_attrs[j]);

	return gpiochip_remove(&rtp_gpio->gpio_chip);
}

static struct platform_driver rtp_gpio_driver = {
	.driver = {
		.name = "rtp-gpio",
		.owner = THIS_MODULE,
	},
	.probe = rtp_gpio_probe,
	.remove = rtp_gpio_remove,
};

static int __init rtp_gpio_init(void)
{
	return platform_driver_register(&rtp_gpio_driver);
}
subsys_initcall(rtp_gpio_init);

static void __exit rtp_gpio_exit(void)
{
	platform_driver_unregister(&rtp_gpio_driver);
}
module_exit(rtp_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
