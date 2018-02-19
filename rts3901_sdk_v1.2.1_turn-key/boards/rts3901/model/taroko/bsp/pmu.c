#include <linux/kernel.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>
#include <linux/mfd/rtp-mfd.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/i2c.h>

int rtp_ic_type;
static struct rtp_pmu_info rtp_pmu_info[] = {
	{
		.regulator_name = "rtp_ldo",
		.regulator_id = RTP_ID_LDO2,
		.regulator_vol = 1200000,
		.regulator_stb_vol = 1200000,
		.regulator_stb_en = 0,
	},
	{
		.regulator_name = "LDO1",
		.regulator_id = RTP_ID_LDO3,
		.regulator_vol = 1800000,
		.regulator_stb_vol = 3300000,
		.regulator_stb_en = 0,
	},
	{
		.regulator_name = "rtp_ldo4",
		.regulator_id = RTP_ID_LDO4,
		.regulator_vol = 3300000,
		.regulator_stb_vol = 3300000,
		.regulator_stb_en = 0,
	},
	{
		.regulator_name = "LDO2",
		.regulator_id = RTP_ID_LDO5,
		.regulator_vol = 3300000,
		.regulator_stb_vol = 3000000,
		.regulator_stb_en = 0,
	},
	{
		.regulator_name = "DC_1V0",
		.regulator_id = RTP_ID_BUCK1,
		.regulator_vol = 1000000,
		.regulator_stb_vol = 900000,
		.regulator_stb_en = 0,
	},
	{
		.regulator_name = "SWR_OUT_RSV",
		.regulator_id = RTP_ID_BUCK2,
		.regulator_vol = 1050000,
		.regulator_stb_vol = 900000,
		.regulator_stb_en = 0,
	},
	{
		.regulator_name = "DC_1V5",
		.regulator_id = RTP_ID_BUCK3,
		.regulator_vol = 1500000,
		.regulator_stb_vol = 1500000,
		.regulator_stb_en = 0,
	},
	{
		.regulator_name = "DC_3V3",
		.regulator_id = RTP_ID_BUCK4,
		.regulator_vol = 3300000,
		.regulator_stb_vol = 3100000,
		.regulator_stb_en = 0,
	},
};

/* init continuous r/w */
int register_rw_init(struct rtp_mfd_chip *chip)
{
	int ret;
	ret = rtp_update_bits(chip, RTP_REG_SYS_PAD_CTL, 0x80, 0xC0);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP_REG_SYS_PAD_CTL reg\n");
		return ret;
	}
	return 0;
}

/* initial all interrupts */
int interrupts_init(struct rtp_mfd_chip *chip)
{
	int ret;
	if (chip->init_irqs) {
		ret = chip->init_irqs(chip);
		if (ret)
			return ret;
	}
	return 0;
}

/* config UV threshold and UV_PD_EN */
int system_uv_init(struct rtp_mfd_chip *chip)
{
	int ret;
	uint8_t val;

	if (RTP_IC_TYPE_G == rtp_ic_type) {
		val = 0x0F;
		ret = rtp_reg_write(chip, RTP_REG_DC_UVOV_PD_PMU, &val);
		if (ret < 0) {
			printk(KERN_ERR "Failed to write RTP_REG_DC_UVOV_PD_PMU reg\n");
			return ret;
		}
	}

	ret = rtp_set_bits(chip, RTP_REG_SYS_UVOV_EN, 0x10);
	if (ret < 0) {
		printk(KERN_ERR "Failed to set RTP_REG_SYS_UVOV_EN\n");
		return ret;
	}

	ret = rtp_clr_bits(chip, RTP_REG_SYS_UVOV_SET, 0x30);
	if (ret < 0) {
		printk(KERN_ERR "Failed to clr RTP_REG_SYS_UVOV_SET\n");
		return ret;
	}

	ret = rtp_update_bits(chip, RTP_REG_PWR_OFF_CFG, 0x0C, 0x0C);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP_REG_PWR_OFF_CFG reg\n");
		return ret;
	}
	return 0;
}

int ldo_init(struct rtp_mfd_chip *chip)
{
	int ret;
	uint8_t val;

	if (RTP_IC_TYPE_D == rtp_ic_type) {
		val = 0x42;
		ret = rtp_reg_write(chip, RTP_REG_LDO1_CTL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_LDO1_CTL reg\n");
			return ret;
		}
	} else {
		val = 0x42;
		ret = rtp_reg_write(chip, RTP_REG_LDO1_CTL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Failed to write RTP_REG_LDO1_CTL reg\n");
			return ret;
		}

		ret = rtp_clr_bits(chip, RTP_REG_POWER_EN, 0x50);
		if (ret < 0) {
			printk(KERN_ERR "Failed to clr RTP_REG_POWER_EN reg\n");
			return ret;
		}
	}
	return 0;
}

/* BUCK init */
int buck_init(struct rtp_mfd_chip *chip)
{
	int ret;
	uint8_t val;

	if (RTP_IC_TYPE_D == rtp_ic_type) {
		val = 0xCF;
		ret = rtp_reg_write(chip, RTP_REG_DUMMY_FF, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_DUMMY_FF reg\n");
			return ret;
		}

		val = 0x5A;
		ret = rtp_reg_write(chip, RTP_REG_DUMMY_5A, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_DUMMY_5A reg\n");
			return ret;
		}

		val = 0xB5;
		ret = rtp_reg_write(chip, RTP_REG_DUMMY_A5, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_DUMMY_A5 reg\n");
			return ret;
		}

		/* ic version */
		ret = rtp_reg_read(chip, RTP_REG_CHIP_VERSION, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to read RTP_REG_CHIP_VERSION reg\n");
			return ret;
		}
		if (val == 0x11) {
			val = 0xB8;
			ret = rtp_reg_write(chip, RTP_REG_DC1_CTL_03, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC1_CTL_03 reg\n");
				return ret;
			}

			val = 0xB8;
			ret = rtp_reg_write(chip, RTP_REG_DC2_CTL_03, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC2_CTL_03 reg\n");
				return ret;
			}

			val = 0xA8;
			ret = rtp_reg_write(chip, RTP_REG_DC3_CTL_03, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC3_CTL_03 reg\n");
				return ret;
			}

			val = 0xBA;
			ret = rtp_reg_write(chip, RTP_REG_DC4_CTL_03, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC4_CTL_03 reg\n");
				return ret;
			}

			val = 0x78;
			ret = rtp_reg_write(chip, RTP_REG_DC4_CTL_05, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC4_CTL_05 reg\n");
				return ret;
			}

			/* BUCK 1, 2 PM config */
			val = 0xAE;
			ret = rtp_reg_write(chip, RTP_REG_DC1_CTL_01, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC1_CTL_01 reg\n");
				return ret;
			}

			val = 0x13;
			ret = rtp_reg_write(chip, RTP_REG_DC1_CTL_02, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC1_CTL_02 reg\n");
				return ret;
			}

			val = 0xAE;
			ret = rtp_reg_write(chip, RTP_REG_DC2_CTL_01, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC2_CTL_01 reg\n");
				return ret;
			}

			val = 0x13;
			ret = rtp_reg_write(chip, RTP_REG_DC2_CTL_02, &val);
			if (ret < 0) {
				printk(KERN_ERR "Unable to write RTP_REG_DC2_CTL_02 reg\n");
				return ret;
			}
		}

		val = 0x8F;
		ret = rtp_reg_write(chip, RTP_REG_DC_UVOV_PD_PMU, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_DC_UVOV_PD_PMU reg\n");
			return ret;
		}

		val = 0x0A;
		ret = rtp_reg_write(chip, RTP_REG_PFM_PWM_CTRL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_PFM_PWM_CTRL reg\n");
			return ret;
		}
	} else {
		val = 0x0A;
		ret = rtp_reg_write(chip, RTP_REG_PFM_PWM_CTRL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_PFM_PWM_CTRL reg\n");
			return ret;
		}

		val = 0xDF;
		ret = rtp_reg_write(chip, RTP_REG_DUMMY_FF, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_DUMMY_FF reg\n");
			return ret;
		}

		val = 0x25;
		ret = rtp_reg_write(chip, RTP_REG_DUMMY_5A, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_DUMMY_5A reg\n");
			return ret;
		}
	}
	return 0;
}

int power_stb_init(struct rtp_mfd_chip *chip)
{
	int ret;
	uint8_t val;

	if (RTP_IC_TYPE_D == rtp_ic_type) {
		ret = rtp_clr_bits(chip, RTP_REG_EXTEN_EN, 0x80);
		if (ret < 0) {
			printk(KERN_ERR "Unable to config RTP_REG_EXTEN_EN reg\n");
			return ret;
		}

		val = 0x8A;
		ret = rtp_reg_write(chip, RTP_REG_GPIO0_CTRL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_GPIO0_CTRL reg\n");
			return ret;
		}
	} else {
		val = 0x94;
		ret = rtp_reg_write(chip, RTP_REG_EXTEN_EN, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to config RTP_REG_EXTEN_EN reg\n");
			return ret;
		}
	}
	return 0;
}

int system_global_init(struct rtp_mfd_chip *chip)
{
	int ret;
	uint8_t val;

	if (RTP_IC_TYPE_G == rtp_ic_type) {
		val = 0x00;
		ret = rtp_reg_write(chip, RTP_REG_CAL_ALARM, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_CAL_ALARM reg\n");
			return ret;
		}
	}

	val = 0x85;
	ret = rtp_reg_write(chip, RTP_REG_OSC_CLK_SET, &val);
	if (ret < 0) {
		printk(KERN_ERR "Unable to write RTP_REG_OSC_CLK_SET reg\n");
		return ret;
	}

	ret = rtp_clr_bits(chip, RTP_REG_SYS_CLK_EN, 0x08);
	if (ret < 0) {
		printk(KERN_ERR "Failed to disable rtc output\n");
		return ret;
	}

	ret = rtp_update_bits(chip, RTP_REG_PWRHOLD, 0xB0, 0xF0);
	if (ret < 0) {
		printk(KERN_ERR "Failed to set RTP_REG_PWRHOLD\n");
		return ret;
	}

	val = 0x58;
	ret = rtp_reg_write(chip, RTP_REG_GLOBAL_CFG1, &val);
	if (ret < 0) {
		printk(KERN_ERR "Unable to write RTP_REG_GLOBAL_CFG1 reg\n");
		return ret;
	}

	val = 0x00;
	ret = rtp_reg_write(chip, RTP_REG_GLOBAL_CFG0, &val);
	if (ret < 0) {
		printk(KERN_ERR "Unable to write RTP_REG_GLOBAL_CFG0 reg\n");
		return ret;
	}

	ret = rtp_clr_bits(chip, RTP_REG_GLOBAL_CFG2, 0x02);
	if (ret < 0) {
		printk(KERN_ERR "Failed to clear RTP_REG_GLOBAL_CFG2\n");
		return ret;
	}

	val = 0x00;
	ret = rtp_reg_write(chip, RTP_REG_DEBUGO_EXT_MUX1, &val);
	if (ret < 0) {
		printk(KERN_ERR "Unable to write RTP_REG_DEBUGO_EXT_MUX1 reg\n");
		return ret;
	}
	return 0;
}

int rtp_post_init(struct rtp_mfd_chip *chip)
{
	struct regulator *regu;
	int i, ret;
	uint8_t val;

	for (i = 0; i < ARRAY_SIZE(rtp_pmu_info); i++) {
		if (RTP_ID_LDO3 == rtp_pmu_info[i].regulator_id ||
			RTP_ID_LDO5 == rtp_pmu_info[i].regulator_id ||
			RTP_ID_BUCK2 == rtp_pmu_info[i].regulator_id)
			continue;

		regu = regulator_get(NULL, rtp_pmu_info[i].regulator_name);
		if (IS_ERR(regu)) {
			dev_err(chip->dev, "regulator %s get failed\n", rtp_pmu_info[i].regulator_name);
			continue;
		}

		regulator_set_voltage(regu, rtp_pmu_info[i].regulator_vol,
		                      rtp_pmu_info[i].regulator_vol);

		ret = regulator_enable(regu);
		if (ret)
			printk(KERN_WARNING "enable regulator %s failed/n", rtp_pmu_info[i].regulator_name);

		printk("%s, %s = %d, mV end\n", __func__, rtp_pmu_info[i].regulator_name,
		       regulator_get_voltage(regu));

		regulator_put(regu);
		udelay(100);
	}

	if (RTP_IC_TYPE_D != rtp_ic_type) {
		val = 0x4A;
		ret = rtp_reg_write(chip, RTP_REG_GPIO0_CTRL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_GPIO0_CTRL reg\n");
			return ret;
		}

		val = 0x00;
		ret = rtp_reg_write(chip, RTP_REG_PWREN_CTRL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_PWREN_CTRL reg\n");
			return ret;
		}

		val = 0x4A;
		ret = rtp_reg_write(chip, RTP_REG_GPIO1_CTRL, &val);
		if (ret < 0) {
			printk(KERN_ERR "Unable to write RTP_REG_GPIO1_CTRL reg\n");
			return ret;
		}
	}

	return 0;
}

int rtpg_pre_init(struct rtp_mfd_chip *chip)
{
	int ret;

	printk("%s,line=%d\n", __func__, __LINE__);

	ret = register_rw_init(chip);
	if (ret < 0)
		return ret;

	ret = buck_init(chip);
	if (ret < 0)
		return ret;

	ret = ldo_init(chip);
	if (ret < 0)
		return ret;

	ret = system_uv_init(chip);
	if (ret < 0)
		return ret;

	ret = interrupts_init(chip);
	if (ret < 0)
		return ret;

	ret = system_global_init(chip);
	if (ret < 0)
		return ret;

	ret = power_stb_init(chip);
	if (ret < 0)
		return ret;

	return 0;
}

int rtpd_pre_init(struct rtp_mfd_chip *chip)
{
	int ret;

	printk("%s,line=%d\n", __func__, __LINE__);

	ret = register_rw_init(chip);
	if (ret < 0)
		return ret;

	ret = interrupts_init(chip);
	if (ret < 0)
		return ret;

	ret = system_uv_init(chip);
	if (ret < 0)
		return ret;

	ret = ldo_init(chip);
	if (ret < 0)
		return ret;

	ret = buck_init(chip);
	if (ret < 0)
		return ret;

	ret = power_stb_init(chip);
	if (ret < 0)
		return ret;

	ret = system_global_init(chip);
	if (ret < 0)
		return ret;

	return 0;
}

int rtp_pre_init(struct rtp_mfd_chip *chip)
{
	uint8_t val;
	int ret;

	ret = rtp_reg_read(chip, RTP_REG_CHIP_VERSION, &val);
	if (ret < 0) {
		printk(KERN_ERR "Unable to read RTP_REG_CHIP_VERSION reg\n");
		return ret;
	}

	switch (val) {
	case 0x11:
		rtp_ic_type = RTP_IC_TYPE_D;
		ret = rtpd_pre_init(chip);
		if (ret < 0)
			return ret;
	case 0x44:
		rtp_ic_type = RTP_IC_TYPE_G;
		ret = rtpg_pre_init(chip);
		if (ret < 0)
			return ret;
	default:
		ret = rtpg_pre_init(chip);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int rtp_late_exit(struct rtp_mfd_chip *chip)
{
	uint8_t val;

	printk("%s,line=%d\n", __func__, __LINE__);

	/* disable extern en */
	rtp_clr_bits(chip, RTP_REG_EXTEN_EN, 0x04);

	/* go to active state */
	rtp_reg_read(chip, RTP_REG_FSM_DEBUG, &val);
	if ((val & 0x07) != 0x06)
		rtp_set_bits(chip, RTP_REG_PMU_STATE_CTL, 0x02);

	printk("%s,line=%d END\n", __func__, __LINE__);

	return 0;
}

static struct regulator_consumer_supply ldo1_data[] = {
	{
		.supply = "rtp_ldo1",
	},
};

static struct regulator_consumer_supply ldo2_data[] = {
	{
		.supply = "rtp_ldo",
	},
};

static struct regulator_consumer_supply ldo3_data[] = {
	{
		.supply = "LDO1",
	},
};

static struct regulator_consumer_supply ldo4_data[] = {
	{
		.supply = "rtp_ldo4",
	},
};

static struct regulator_consumer_supply ldo5_data[] = {
	{
		.supply = "LDO2",
	},
};

static struct regulator_consumer_supply buck1_data[] = {
	{
		.supply = "DC_1V0",
	},
};

static struct regulator_consumer_supply buck2_data[] = {
	{
		.supply = "SWR_OUT_RSV",
	},
};

static struct regulator_consumer_supply buck3_data[] = {
	{
		.supply = "DC_1V5",
	},
};

static struct regulator_consumer_supply buck4_data[] = {
	{
		.supply = "DC_3V3",
	},
};

static struct regulator_init_data rtp_init_rdatas[] = {
	[RTP_ID_LDO1] = {
		.constraints = {
			.name = "RTP_LDO",
			.min_uV = RTP_LDO1_VOL * 1000,
			.max_uV = RTP_LDO1_VOL * 1000,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},
	[RTP_ID_LDO2] = {
		.constraints = {
			.name = "RTP_LDO",
			.min_uV = 1000000,
			.max_uV = 2500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 1200000,
				.disabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},
	[RTP_ID_LDO3] = {
		.constraints = {
			.name = "LDO1",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 1800000,
				.disabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},
	[RTP_ID_LDO4] = {
		.constraints = {
			.name = "RTP_LDO4",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 3300000,
				.disabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},
	[RTP_ID_LDO5] = {
		.constraints = {
			.name = "LDO2",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 3300000,
				.disabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo5_data),
		.consumer_supplies = ldo5_data,
	},
	[RTP_ID_BUCK1] = {
		.constraints = {
			.name = "DC_1V0",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 1075000,
				.enabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(buck1_data),
		.consumer_supplies = buck1_data,
	},
	[RTP_ID_BUCK2] = {
		.constraints = {
			.name = "SWR_OUT_RSV",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 1050000,
				.enabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(buck2_data),
		.consumer_supplies = buck2_data,
	},
	[RTP_ID_BUCK3] = {
		.constraints = {
			.name = "DC_1V5",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 1500000,
				.enabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(buck3_data),
		.consumer_supplies = buck3_data,
	},
	[RTP_ID_BUCK4] = {
		.constraints = {
			.name = "DC_3V3",
			.min_uV = 1700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 3300000,
				.enabled = 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(buck4_data),
		.consumer_supplies = buck4_data,
	},
};

static struct rtp_board rtp_data = {
	.irq = (unsigned) RTP_IRQ,
	.gpio_base = RTP_GPIO_BASE,

	.pre_init = rtp_pre_init,
	.post_init = rtp_post_init,
	.late_exit = rtp_late_exit,

	/* Regulators */
	.rtp_pmic_init_data[RTP_ID_LDO1] = NULL,
	.rtp_pmic_init_data[RTP_ID_LDO2] = NULL,
	.rtp_pmic_init_data[RTP_ID_LDO3] = &rtp_init_rdatas[RTP_ID_LDO3],
	.rtp_pmic_init_data[RTP_ID_LDO4] = NULL,
	.rtp_pmic_init_data[RTP_ID_LDO5] = &rtp_init_rdatas[RTP_ID_LDO5],
	.rtp_pmic_init_data[RTP_ID_BUCK1] = &rtp_init_rdatas[RTP_ID_BUCK1],
	.rtp_pmic_init_data[RTP_ID_BUCK2] = &rtp_init_rdatas[RTP_ID_BUCK2],
	.rtp_pmic_init_data[RTP_ID_BUCK3] = &rtp_init_rdatas[RTP_ID_BUCK3],
	.rtp_pmic_init_data[RTP_ID_BUCK4] = &rtp_init_rdatas[RTP_ID_BUCK4],
};

static struct i2c_board_info rtp_i2c_info[] = {
	{
		.type = "rtp_mfd",
		.addr = RTP_ADDRESS,
		.platform_data = &rtp_data,
		.irq = RTP_IRQ,
	},
};

static int __init bsp_pmu_init(void)
{
	/* core init */
	printk("INFO: initializing pmu device ...\n");
	i2c_register_board_info(0, rtp_i2c_info,
				ARRAY_SIZE(rtp_i2c_info));

	return 0;
}
arch_initcall(bsp_pmu_init);
