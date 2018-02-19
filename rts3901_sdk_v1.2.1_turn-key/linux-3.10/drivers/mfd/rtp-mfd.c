/*
 * rtp-mfd.c  --  Realsil RTP
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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/mfd/core.h>
#include <linux/mfd/rtp-mfd.h>
#include <linux/delay.h>
#include <linux/regmap.h>

static struct rtp_mfd_chip *g_chip;
static uint8_t rtp_reg_addr = 0;

static struct mfd_cell rtps[] = {
	{
		.name = "rtp-pmic",
	},
	{
		.name = "rtp-rtc",
	},
	{
		.name = "rtp-power",
	},
};

int rtp_reg_read(struct rtp_mfd_chip *chip, uint8_t reg, uint8_t *val)
{
	unsigned int regval = 0;
	int ret;

	ret = regmap_read(chip->regmap, reg, &regval);
	*val = regval & 0xff;

	return ret;
}
EXPORT_SYMBOL_GPL(rtp_reg_read);

int rtp_reg_write(struct rtp_mfd_chip *chip, uint8_t reg, uint8_t *val)
{
	unsigned int regval = 0;

	regval = regval | (*val);

	return regmap_write(chip->regmap, reg, regval);
}
EXPORT_SYMBOL_GPL(rtp_reg_write);

int rtp_bulk_read(struct rtp_mfd_chip *chip, uint8_t reg, int count,
		      uint8_t *val)
{
	return regmap_bulk_read(chip->regmap, reg, val, count);
}
EXPORT_SYMBOL_GPL(rtp_bulk_read);

int rtp_bulk_write(struct rtp_mfd_chip *chip, uint8_t reg, int count,
		       uint8_t *val)
{
	return regmap_bulk_write(chip->regmap, reg, val, count);
}
EXPORT_SYMBOL_GPL(rtp_bulk_write);

int rtp_set_bits(struct rtp_mfd_chip *chip, int reg, uint8_t bit_mask)
{
	return regmap_update_bits(chip->regmap, reg, bit_mask, bit_mask);
}
EXPORT_SYMBOL_GPL(rtp_set_bits);

int rtp_clr_bits(struct rtp_mfd_chip *chip, int reg, uint8_t bit_mask)
{
	return regmap_update_bits(chip->regmap, reg, bit_mask, 0);
}
EXPORT_SYMBOL_GPL(rtp_clr_bits);

int rtp_update_bits(struct rtp_mfd_chip *chip, int reg,
			uint8_t reg_val, uint8_t mask)
{
	return regmap_update_bits(chip->regmap, reg, mask, reg_val);
}
EXPORT_SYMBOL_GPL(rtp_update_bits);

int rtp_register_notifier(struct rtp_mfd_chip *chip,
			      struct notifier_block *nb, uint64_t irqs)
{
	if (NULL != nb) {
		chip->irq_enable |= irqs;
		chip->update_irqs_en(chip);
		return blocking_notifier_chain_register(&chip->notifier_list,
							nb);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(rtp_register_notifier);

int rtp_unregister_notifier(struct rtp_mfd_chip *chip,
				struct notifier_block *nb, uint64_t irqs)
{
	if (NULL != nb) {
		chip->irq_enable &= ~irqs;
		chip->update_irqs_en(chip);
		return blocking_notifier_chain_unregister(&chip->notifier_list,
							  nb);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(rtp_unregister_notifier);

static int rtp_init_irqs(struct rtp_mfd_chip *chip)
{
	uint8_t v1[7] = {0x00, 0xff, 0x00, 0x00, 0xc0, 0x0f, 0x37};
	uint8_t v2[7] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int err;

	printk(KERN_DEBUG "Init rtp irqs\n");
	err =  rtp_bulk_write(chip, RTP_REG_ALDOIN_IRQ_EN, 7, v1);
	if (err) {
		printk("[RTP-MFD] try to init irq_en failed!\n");
		return err;
	}

	err =  rtp_bulk_write(chip, RTP_REG_AVCC_IRQ_STS2, 7, v2);
	if (err) {
		printk("[RTP-MFD] try to init irq_sts failed!\n");
		return err;
	}

	chip->irq_enable = 0x00000000 | (uint64_t) 0x00000000 << 32;
	chip->update_irqs_en(chip);

	return 0;
}

static int rtp_update_irqs_enable(struct rtp_mfd_chip *chip)
{
	int ret = 0;
	uint64_t irqs;
	uint8_t v[7] = {0, 0, 0, 0, 0, 0, 0};

	printk(KERN_DEBUG "Update rtp irqs enable\n");
	ret =  rtp_bulk_read(chip, RTP_REG_ALDOIN_IRQ_EN, 7, v);
	if (ret < 0)
		return ret;

	irqs = (((uint64_t) v[6]) << 48) | (((uint64_t) v[5]) << 40) |
	       (((uint64_t) v[4]) << 32) | (((uint64_t) v[3]) << 24) |
	       (((uint64_t) v[2]) << 16) | (((uint64_t) v[1]) << 8) |
	       ((uint64_t) v[0]);

	if (chip->irq_enable != irqs) {
		v[0] = ((chip->irq_enable) & 0xff);
		v[1] = ((chip->irq_enable) >> 8) & 0xff;
		v[2] = ((chip->irq_enable) >> 16) & 0xff;
		v[3] = ((chip->irq_enable) >> 24) & 0xff;
		v[4] = ((chip->irq_enable) >> 32) & 0xff;
		v[5] = ((chip->irq_enable) >> 40) & 0xff;
		v[6] = ((chip->irq_enable) >> 48) & 0xff;
		ret =  rtp_bulk_write(chip, RTP_REG_ALDOIN_IRQ_EN,
					  7, v);
	}

	return ret;
}

static int rtp_read_irqs(struct rtp_mfd_chip *chip, uint64_t *irqs)
{
	uint8_t v[7] = {0, 0, 0, 0, 0, 0, 0};
	int ret;

	printk(KERN_DEBUG "Read rtp irqs status\n");
	ret =  rtp_bulk_read(chip, RTP_REG_AVCC_IRQ_STS2, 7, v);
	if (ret < 0)
		return ret;

	*irqs = (((uint64_t) v[0]) << 48) | (((uint64_t) v[6]) << 40) |
		(((uint64_t) v[2]) << 32) | (((uint64_t) v[4]) << 24) |
		(((uint64_t) v[3]) << 16) | (((uint64_t) v[5]) << 8) |
		((uint64_t) v[1]);

	return 0;
}

static int rtp_write_irqs(struct rtp_mfd_chip *chip, uint64_t irqs)
{
	uint8_t v[7];
	int ret;

	printk(KERN_DEBUG "Write rtp irqs status\n");
	v[0] = (irqs >> 48) & 0xff;
        v[1] = (irqs >> 0) & 0xff;
        v[2] = (irqs >> 32) & 0xff;
        v[3] = (irqs >> 16) & 0xff;
        v[4] = (irqs >> 24) & 0xff;
        v[5] = (irqs >> 8) & 0xff;
        v[6] = (irqs >> 40) & 0xff;

	ret = rtp_bulk_write(chip, RTP_REG_AVCC_IRQ_STS2, 7, v);
	if (ret < 0)
		return ret;

	return 0;
}

static ssize_t rtp_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	uint8_t val;
	struct rtp_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	rtp_reg_read(chip, rtp_reg_addr, &val);

	return sprintf(buf, "REG[%x]=%x\n", rtp_reg_addr, val);
}

static ssize_t rtp_reg_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	struct rtp_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	tmp = simple_strtoul(buf, NULL, 16);
	if(tmp < 256)
		rtp_reg_addr = tmp;
	else {
		val = tmp & 0x00FF;
		rtp_reg_addr= (tmp >> 8) & 0x00FF;
		rtp_reg_write(chip, rtp_reg_addr, &val);
	}

	return count;
}

static uint8_t rtp_regs_addr = 0;
static int rtp_regs_len = 0;
static int rtp_regs_rw = 0;
static uint8_t rtp_regs_datas[256];
static ssize_t rtp_regs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
        int count = 0;
	int i;
        struct rtp_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	if (rtp_regs_rw == 0) {
		rtp_bulk_read(chip, rtp_regs_addr, rtp_regs_len,
				  rtp_regs_datas);
		for (i = 0; i < rtp_regs_len; i++) {
			count += sprintf(buf + count, "REG[%x]=%x\n",
					 rtp_regs_addr + i,
					 rtp_regs_datas[i]);
		}
	} else if (rtp_regs_rw == 1) {
		rtp_bulk_write(chip, rtp_regs_addr, rtp_regs_len,
				   rtp_regs_datas);
		sprintf(buf, "bulk write ok\n");
	}

        return count;
}

static ssize_t rtp_regs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
        int tmp;
	char buftemp[3];

	if (count < 6)
		return count;

	/* rw flag */
	buftemp[0] = buf[0];
	buftemp[1] = '\0';
	rtp_regs_rw = simple_strtoul(buftemp, NULL, 16);
	printk("<0>" "rtp_regs_store, rtp_regs_rw = %d\n", rtp_regs_rw);
	/* addr */
	buftemp[0] = buf[1];
	buftemp[1] = buf[2];
        buftemp[2] = '\0';
        rtp_regs_addr = simple_strtoul(buftemp, NULL, 16);
	printk("<0>" "rtp_regs_store, rtp_regs_addr = 0x%x\n", rtp_regs_addr);
	/* addr */
        buftemp[0] = buf[3];
        buftemp[1] = buf[4];
        buftemp[2] = '\0';
        rtp_regs_len = simple_strtoul(buftemp, NULL, 16);
	printk("<0>" "rtp_regs_store, rtp_regs_len = %d\n", rtp_regs_len);
	if (rtp_regs_rw == 1) {
		if (count != 5 + rtp_regs_len * 2 + 1) {
			printk("<0>" "rtp_regs_store error, count = %d\n", count);
		}
		for (tmp = 0; tmp < rtp_regs_len; tmp++) {
			buftemp[0] = buf[tmp * 2 + 5];
			buftemp[1] = buf[tmp * 2 + 6];
			buftemp[2] = '\0';
			rtp_regs_datas[tmp] = simple_strtoul(buftemp,
								 NULL, 16);
			printk("<0>" "rtp_regs_store, val[%x] = 0x%x\n", tmp + rtp_regs_addr, rtp_regs_datas[tmp]);
		}
	}

        return count;
}

static struct device_attribute rtp_mfd_attrs[] = {
	RTP_MFD_ATTR(rtp_reg),
	RTP_MFD_ATTR(rtp_regs),
};

static int rtp_mfd_create_attrs(struct rtp_mfd_chip *chip)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(rtp_mfd_attrs); j++) {
		ret = device_create_file(chip->dev, &rtp_mfd_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(chip->dev, &rtp_mfd_attrs[j]);
succeed:
	return ret;
}

static const struct regmap_config rtp_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = RTP_REG_MAX_REGISTER,
};

static int rtp_mfd_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct rtp_mfd_chip *chip;
	struct rtp_board *pmic_plat_data;
	int ret = 0;

	pmic_plat_data = dev_get_platdata(&i2c->dev);
	if (!pmic_plat_data)
		return -EINVAL;

	chip = kzalloc(sizeof(struct rtp_mfd_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, chip);
	chip->dev = &i2c->dev;
	chip->i2c_client = i2c;
	chip->id = id->driver_data;
	chip->init_irqs = rtp_init_irqs;
	chip->update_irqs_en = rtp_update_irqs_enable;
	chip->read_irqs = rtp_read_irqs;
	chip->write_irqs = rtp_write_irqs;
	chip->regmap = devm_regmap_init_i2c(i2c, &rtp_regmap_config);
	if (IS_ERR(chip->regmap)) {
		ret = PTR_ERR(chip->regmap);
		dev_err(chip->dev, "regmap init failed: %d\n", ret);
		goto err;
	}

	/* verify the ic
	ret = rtp_reg_read(chip, 0x22);
	if ((ret < 0) || (ret == 0xff)){
		printk("The device is not rtp\n");
		goto err;
	} */

	g_chip = chip;

	if (pmic_plat_data && pmic_plat_data->pre_init) {
		ret = pmic_plat_data->pre_init(chip);
		if (ret != 0) {
			dev_err(chip->dev, "pre_init() failed: %d\n", ret);
			goto err;
		}
	}

	ret = rtp_irq_init(chip, pmic_plat_data->irq);
	if (ret < 0)
		goto err;

	ret = mfd_add_devices(chip->dev, -1,
			      rtps, ARRAY_SIZE(rtps),
			      NULL, 0, NULL);
	if (ret < 0)
		goto mfd_err;

	if (pmic_plat_data && pmic_plat_data->post_init) {
		ret = pmic_plat_data->post_init(chip);
		if (ret != 0) {
			dev_err(chip->dev, "post_init() failed: %d\n", ret);
			goto mfd_err;
		}
	}

	ret = rtp_mfd_create_attrs(chip);
	if (ret)
		goto mfd_err;

	return ret;

mfd_err:
	mfd_remove_devices(chip->dev);
	rtp_irq_exit(chip);
err:
	kfree(chip);
	return ret;
}

int rtp_power_off(void)
{
	int ret;
	struct rtp_mfd_chip *chip = g_chip;
	struct rtp_board *pmic_plat_data = dev_get_platdata(chip->dev);

	printk("%s\n", __func__);

	if (pmic_plat_data && pmic_plat_data->late_exit) {
		ret = pmic_plat_data->late_exit(chip);
		if (ret != 0)
			dev_err(chip->dev, "late_exit() failed: %d\n", ret);
	}

	mdelay(20);
	rtp_set_bits(chip, RTP_REG_PMU_STATE_CTL, 0x80);
	mdelay(20);
	printk("warning!!! rtp can't power-off, maybe some error happend!\n");

	return 0;
}
EXPORT_SYMBOL_GPL(rtp_power_off);

static int rtp_mfd_remove(struct i2c_client *client)
{
	struct rtp_mfd_chip *chip = i2c_get_clientdata(client);
	int j;

	printk("<0>""rtp_mfd_remove\n");
	mfd_remove_devices(chip->dev);
	rtp_irq_exit(chip);
	i2c_set_clientdata(client, NULL);
	for (j = 0; j < ARRAY_SIZE(rtp_mfd_attrs); j++)
		device_remove_file(chip->dev, &rtp_mfd_attrs[j]);
	kfree(chip);
	g_chip = NULL;

	return 0;
}

static const struct i2c_device_id rtp_mfd_id_table[] = {
	{ "rtp_mfd", RTP_ID },
	{},
};
MODULE_DEVICE_TABLE(i2c, rtp_mfd_id_table);

static struct i2c_driver rtp_mfd_driver = {
	.driver = {
		.name = "rtp_mfd",
		.owner = THIS_MODULE,
	},
	.probe = rtp_mfd_probe,
	.remove = rtp_mfd_remove,
	.id_table = rtp_mfd_id_table,
};

static int __init rtp_mfd_init(void)
{
	return i2c_add_driver(&rtp_mfd_driver);
}
subsys_initcall_sync(rtp_mfd_init);

static void __exit rtp_mfd_exit(void)
{
	i2c_del_driver(&rtp_mfd_driver);
}
module_exit(rtp_mfd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
