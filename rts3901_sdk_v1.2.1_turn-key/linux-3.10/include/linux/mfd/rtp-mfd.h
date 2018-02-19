#ifndef __LINUX_RTP_MFD_H_
#define __LINUX_RTP_MFD_H_

#include <linux/gpio.h>
#include <linux/pm_wakeup.h>

/* rtp chip id list */
#define RTP_ID			0

#define RTP_ADDRESS			0x30

// gpio
#define RTP_GPIO_NUM		6
#define RTP_GPIO_BASE		0	// not fix

// irq
#define RTP_IRQ			0	// not fix

#define RTP_REG_NONE_WORTH		(0x8F)

// dvs
#define RTP_DVS_STEP_US		25000

// rtp register define
// rtp system register
#define RTP_REG_SYS_UVOV_EN		(0x01)
#define RTP_REG_SYS_UVOV_SET		(0x02)
#define RTP_REG_PWRHOLD			(0x05)
#define RTP_REG_SYS_CLK_EN		(0x07)
#define RTP_REG_OSC_CLK_SET		(0x08)
#define RTP_REG_XTAL_CLK_SET		(0x09)
#define RTP_REG_LDO1_CTL		(0x0B)
#define RTP_REG_SYS_PAD_CTL		(0x0E)

// rtp power config register
#define RTP_REG_POWER_EN		(0x10)
#define RTP_REG_EXTEN_EN		(0x11)
#define RTP_REG_POWER_STB_EN		(0x12)
#define RTP_REG_EXTEN_STB_EN		(0x13)
#define RTP_REG_POWER_CFG		(0x14)
#define RTP_REG_PMU_STATE_CTL		(0x17)
#define RTP_REG_STB_DC1VOUT		(0x18)
#define RTP_REG_STB_DC2VOUT		(0x19)
#define RTP_REG_STB_DC3VOUT		(0x1A)
#define RTP_REG_STB_DC4VOUT		(0x1B)
#define RTP_REG_STB_LDO2VOUT		(0x1C)
#define RTP_REG_STB_LDO3VOUT		(0x1D)
#define RTP_REG_STB_LDO4VOUT		(0x1E)
#define RTP_REG_STB_LDO5VOUT		(0x1F)

// rtp dcdc config register
#define RTP_REG_DC1VOUT_SET		(0x20)
#define RTP_REG_DC2VOUT_SET		(0x21)
#define RTP_REG_DC3VOUT_SET		(0x22)
#define RTP_REG_DC4VOUT_SET		(0x23)
#define RTP_REG_DVS_STEP_CTRL		(0x27)
#define RTP_REG_DVS_CTRL		(0x28)
#define RTP_REG_PFM_PWM_CTRL		(0x29)
#define RTP_REG_DC1_CTL_05		(0x2B)
#define RTP_REG_DC2_CTL_05		(0x2C)
#define RTP_REG_DC3_CTL_05		(0x2D)
#define RTP_REG_DC4_CTL_05		(0x2E)
#define RTP_REG_DC_OCP_EN		(0x30)
#define RTP_REG_DC_OCP_SET_01		(0x31)
#define RTP_REG_DC_OCP_SET_02		(0x32)
#define RTP_REG_DC1_CTL_01		(0x40)
#define RTP_REG_DC1_CTL_02		(0x41)
#define RTP_REG_DC2_CTL_01		(0x42)
#define RTP_REG_DC2_CTL_02		(0x43)
#define RTP_REG_DC4_CTL_01		(0x46)
#define RTP_REG_DC4_CTL_02		(0x47)
#define RTP_REG_DC1_CTL_03		(0x49)
#define RTP_REG_DC2_CTL_03		(0x4A)
#define RTP_REG_DC3_CTL_03		(0x4B)
#define RTP_REG_DC4_CTL_03		(0x4C)

// rtp ldo config register
#define RTP_REG_LDO2VOUT_SET		(0x50)
#define RTP_REG_LDO3VOUT_SET		(0x51)
#define RTP_REG_LDO4VOUT_SET		(0x52)
#define RTP_REG_LDO5VOUT_SET		(0x53)

// rtp calendar register
#define RTP_REG_CAL_SEC 		(0x54)
#define RTP_REG_CAL_MIN 		(0x55)
#define RTP_REG_CAL_HOUR 		(0x56)
#define RTP_REG_CAL_DAY 		(0x57)
#define RTP_REG_CAL_DATE 		(0x58)
#define RTP_REG_CAL_MONTH 		(0x59)
#define RTP_REG_CAL_CENT 		(0x5a)
#define RTP_REG_CAL_YEAR 		(0x5b)
#define RTP_REG_CAL_ALARM 		(0x5c)

// rtp ldo config register
#define RTP_REG_LDOOCP_EN		(0x60)
#define RTP_REG_LDO_OCP_SET_01		(0x61)
#define RTP_REG_LDO_OCP_SET_02		(0x62)
#define RTP_REG_LDO_PD_MODE		(0x63)
#define RTP_REG_DUMMY_FF		(0x65)
#define RTP_REG_DUMMY_5A		(0x66)
#define RTP_REG_DUMMY_A5		(0x67)

// rtp alarm control register
#define RTP_REG_ALARM_SEC 		(0x7A)
#define RTP_REG_ALARM_MIN 		(0x7B)
#define RTP_REG_ALARM_HOUR 		(0x7C)
#define RTP_REG_ALARM_DATE 		(0x7D)
#define RTP_REG_ALARM_MONTH 		(0x7E)
#define RTP_REG_ALARM_YEAR 		(0x7F)

// SYS/GPIO control register
#define RTP_REG_GPIO0_CTRL		(0x80)
#define RTP_REG_GPIO1_CTRL		(0x81)
#define RTP_REG_GPIO2_CTRL		(0x82)
#define RTP_REG_GPIO3_CTRL		(0x83)
#define RTP_REG_GPIO_IN_STAT		(0x84)
#define RTP_REG_PWREN_CTRL		(0x85)
#define RTP_REG_PWR_OFF_CFG		(0x88)
#define RTP_REG_DC_UVOV_PD_PMU		(0x89)

// interrupt register
#define RTP_REG_ALDOIN_IRQ_EN		(0x90)
#define RTP_REG_UVOV_IRQ_EN		(0x91)
#define RTP_REG_GPIO_IRQ_EN		(0x92)
#define RTP_REG_LDO_OCP_IRQ_EN		(0x93)
#define RTP_REG_PONKEY_IRQ_EN		(0x94)
#define RTP_REG_DC_OCOT_IRQ_EN		(0x95)
#define RTP_REG_AVCC_IRQ_EN2		(0x96)
#define RTP_REG_AVCC_IRQ_STS2		(0x97)
#define RTP_REG_ALDOIN_IRQ_STS		(0x98)
#define RTP_REG_PONKEY_IRQ_STS		(0x99)
#define RTP_REG_GPIO_IRQ_STS		(0x9A)
#define RTP_REG_LDO_OCP_IRQ_STS		(0x9B)
#define RTP_REG_DC_UVOV_IRQ_STS		(0x9C)
#define RTP_REG_DC_OCOT_IRQ_STS		(0x9D)

// global dubug/config register
#define RTP_REG_GLOBAL_CFG0		(0xF0)
#define RTP_REG_GLOBAL_CFG1		(0xF1)
#define RTP_REG_GLOBAL_CFG2		(0xF2)
#define RTP_REG_VID_VERSION1		(0xF4)
#define RTP_REG_VID_VERSION2		(0xF5)
#define RTP_REG_PID_VERSION1		(0xF6)
#define RTP_REG_PID_VERSION2		(0xF7)
#define RTP_REG_DEBUGO_EXT_MUX1		(0xFB)
#define RTP_REG_FSM_DEBUG		(0xFC)
#define RTP_REG_DEBUGO_MUX		(0xFD)
#define RTP_REG_CHIP_VERSION		(0xFF)
#define RTP_REG_MAX_REGISTER		(0xFF)

// rtp interrupter
#define	RTP_IRQ_ALDOIN_OFF		(((uint64_t) 1) << 2)
#define	RTP_IRQ_ALDOIN_ON		(((uint64_t) 1) << 3)
#define	RTP_IRQ_TIMEOUT			(((uint64_t) 1) << 4)
#define	RTP_IRQ_GLOBAL			(((uint64_t) 1) << 7)

#define	RTP_IRQ_DC1_OVP			(((uint64_t) 1) << 8)
#define	RTP_IRQ_DC2_OVP			(((uint64_t) 1) << 9)
#define	RTP_IRQ_DC3_OVP			(((uint64_t) 1) << 10)
#define	RTP_IRQ_DC4_OVP			(((uint64_t) 1) << 11)
#define	RTP_IRQ_DCDC1_UV		(((uint64_t) 1) << 12)
#define	RTP_IRQ_DCDC2_UV		(((uint64_t) 1) << 13)
#define	RTP_IRQ_DCDC3_UV		(((uint64_t) 1) << 14)
#define	RTP_IRQ_DCDC4_UV		(((uint64_t) 1) << 15)

#define	RTP_IRQ_GPIO0_F			(((uint64_t) 1) << 16)
#define	RTP_IRQ_GPIO1_F			(((uint64_t) 1) << 17)
#define	RTP_IRQ_GPIO2_F			(((uint64_t) 1) << 18)
#define	RTP_IRQ_GPIO3_F			(((uint64_t) 1) << 19)
#define	RTP_IRQ_GPIO0_R			(((uint64_t) 1) << 20)
#define	RTP_IRQ_GPIO1_R			(((uint64_t) 1) << 21)
#define	RTP_IRQ_GPIO2_R			(((uint64_t) 1) << 22)
#define	RTP_IRQ_GPIO3_R			(((uint64_t) 1) << 23)

#define	RTP_IRQ_LDO2_OCP		(((uint64_t) 1) << 28)
#define	RTP_IRQ_LDO3_OCP		(((uint64_t) 1) << 29)
#define	RTP_IRQ_LDO4_OCP		(((uint64_t) 1) << 30)
#define	RTP_IRQ_LDO5_OCP		(((uint64_t) 1) << 31)

#define	RTP_IRQ_ALARM			(((uint64_t) 1) << 34)
#define	RTP_IRQ_PONKEY_SHORT		(((uint64_t) 1) << 35)
#define	RTP_IRQ_PONKEY_F		(((uint64_t) 1) << 36)
#define	RTP_IRQ_PONKEY_R		(((uint64_t) 1) << 37)
#define	RTP_IRQ_PONKEY_OFF		(((uint64_t) 1) << 38)
#define	RTP_IRQ_PONKEY_LONG		(((uint64_t) 1) << 39)

#define	RTP_IRQ_DC1_OTP			(((uint64_t) 1) << 40)
#define	RTP_IRQ_DC2_OTP			(((uint64_t) 1) << 41)
#define	RTP_IRQ_DC3_OTP			(((uint64_t) 1) << 42)
#define	RTP_IRQ_DC4_OTP			(((uint64_t) 1) << 43)
#define	RTP_IRQ_DC1_OCP			(((uint64_t) 1) << 44)
#define	RTP_IRQ_DC2_OCP			(((uint64_t) 1) << 45)
#define	RTP_IRQ_DC3_OCP			(((uint64_t) 1) << 46)
#define	RTP_IRQ_DC4_OCP			(((uint64_t) 1) << 47)

#define	RTP_IRQ_AVCC_UV1		(((uint64_t) 1) << 48)
#define	RTP_IRQ_AVCC_UV2		(((uint64_t) 1) << 49)
#define	RTP_IRQ_AVCC_UV_OFF		(((uint64_t) 1) << 50)
#define	RTP_IRQ_AVCC_OV1		(((uint64_t) 1) << 52)
#define	RTP_IRQ_AVCC_OV2		(((uint64_t) 1) << 53)

#define RTP_RTC_NOTIFIER_ON		RTP_IRQ_ALARM

// irq index
#define	RTP_IRQ_NUM_ALDOIN_OFF		(2)
#define	RTP_IRQ_NUM_ALDOIN_ON		(3)
#define	RTP_IRQ_NUM_TIMEOUT	 	(4)
#define	RTP_IRQ_NUM_GLOBAL		(7)

#define	RTP_IRQ_NUM_DC1_OVP		(8)
#define	RTP_IRQ_NUM_DC2_OVP		(9)
#define	RTP_IRQ_NUM_DC3_OVP		(10)
#define	RTP_IRQ_NUM_DC4_OVP		(11)
#define	RTP_IRQ_NUM_DCDC1_UV		(12)
#define	RTP_IRQ_NUM_DCDC2_UV		(13)
#define	RTP_IRQ_NUM_DCDC3_UV		(14)
#define	RTP_IRQ_NUM_DCDC4_UV		(15)

#define	RTP_IRQ_NUM_GPIO0_F		(16)
#define	RTP_IRQ_NUM_GPIO1_F		(17)
#define	RTP_IRQ_NUM_GPIO2_F		(18)
#define	RTP_IRQ_NUM_GPIO3_F		(19)
#define	RTP_IRQ_NUM_GPIO0_R		(20)
#define	RTP_IRQ_NUM_GPIO1_R		(21)
#define	RTP_IRQ_NUM_GPIO2_R		(22)
#define	RTP_IRQ_NUM_GPIO3_R		(23)

#define	RTP_IRQ_NUM_LDO2_OCP		(28)
#define	RTP_IRQ_NUM_LDO3_OCP		(29)
#define	RTP_IRQ_NUM_LDO4_OCP		(30)
#define	RTP_IRQ_NUM_LDO5_OCP		(31)

#define	RTP_IRQ_NUM_ALARM		(34)
#define	RTP_IRQ_NUM_PONKEY_SHORT	(35)
#define	RTP_IRQ_NUM_PONKEY_F		(36)
#define	RTP_IRQ_NUM_PONKEY_R		(37)
#define	RTP_IRQ_NUM_PONKEY_OFF		(38)
#define	RTP_IRQ_NUM_PONKEY_LONG		(39)

#define	RTP_IRQ_NUM_DC1_OTP		(40)
#define	RTP_IRQ_NUM_DC2_OTP		(41)
#define	RTP_IRQ_NUM_DC3_OTP		(42)
#define	RTP_IRQ_NUM_DC4_OTP		(43)
#define	RTP_IRQ_NUM_DC1_OCP		(44)
#define	RTP_IRQ_NUM_DC2_OCP		(45)
#define	RTP_IRQ_NUM_DC3_OCP		(46)
#define	RTP_IRQ_NUM_DC4_OCP		(47)

#define	RTP_IRQ_NUM_AVCC_UV1		(48)
#define	RTP_IRQ_NUM_AVCC_UV2		(49)
#define	RTP_IRQ_NUM_AVCC_UV_OFF		(50)
#define	RTP_IRQ_NUM_AVCC_OV1		(52)
#define	RTP_IRQ_NUM_AVCC_OV2		(53)

#define RTP_MFD_ATTR(_name) 			\
{ 						\
	.attr = {.name = #_name, .mode = 0644}, \
	.show =  _name##_show, 			\
	.store = _name##_store, 		\
}

// regulator
#define RTP_LDO1_VOL		1300

#define RTP_LDO(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit, svreg, sereg, voffset) 	\
{												\
	.desc = {										\
		.name 		= (_pmic),							\
		.type 		= REGULATOR_VOLTAGE,						\
		.id 		= (_id),							\
		.n_voltages 	= (step) ? ((max - min) / step + 1) : 1,			\
		.owner 		= THIS_MODULE,							\
	},											\
	.min_uV			= (min) * 1000,							\
	.max_uV			= (max) * 1000,							\
	.step_uV 		= (step) * 1000,						\
	.vol_reg 		= (vreg),							\
	.vol_shift 		= (shift),							\
	.vol_nbits 		= (nbits),							\
	.enable_reg 		= (ereg),							\
	.enable_bit 		= (ebit),							\
	.stb_vol_reg 		= (svreg),							\
	.stb_enable_reg 	= (sereg),							\
	.vol_offset		= (voffset),							\
}

#define RTP_BUCK(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit, svreg, sereg, voffset)	\
{												\
	.desc	= {										\
		.name		= (_pmic),							\
		.type		= REGULATOR_VOLTAGE,						\
		.id		= (_id),							\
		.n_voltages 	= (step) ? ((max - min) / step + 1) : 1,			\
		.owner		= THIS_MODULE,							\
	},											\
	.min_uV			= (min) * 1000,							\
	.max_uV			= (max) * 1000,							\
	.step_uV		= (step) * 1000,						\
	.vol_reg		= (vreg),							\
	.vol_shift		= (shift),							\
	.vol_nbits		= (nbits),							\
	.enable_reg		= (ereg),							\
	.enable_bit		= (ebit),							\
	.stb_vol_reg		= (svreg),							\
	.stb_enable_reg		= (sereg),							\
	.vol_offset		= (voffset),							\
}

#define RTP_REGU_ATTR(_name)							\
{										\
	.attr = {.name = #_name, .mode = 0644},					\
	.show = _name##_show,							\
	.store = _name##_store, 						\
}
////////////////////////////////////////////////////////////

enum {
	RTP_ID_LDO1,
	RTP_ID_LDO2,
	RTP_ID_LDO3,
	RTP_ID_LDO4,
	RTP_ID_LDO5,

	RTP_ID_BUCK1,
	RTP_ID_BUCK2,
	RTP_ID_BUCK3,
	RTP_ID_BUCK4,

	RTP_ID_REGULATORS_NUM,
};

enum {
	RTP_DEV_PMIC,
	RTP_DEV_RTC,
	RTP_DEV_GPIO,
};

enum {
	RTP_IC_TYPE_A,
	RTP_IC_TYPE_B,
	RTP_IC_TYPE_D,
	RTP_IC_TYPE_G,
};

struct rtp_mfd_chip {
	struct device *dev;
	struct i2c_client *i2c_client;
	struct regmap *regmap;
	unsigned int id;

	/* GPIO Handling */
	struct gpio_chip gpio;

	/* IRQ Handling */
	struct mutex irq_lock;
	int chip_irq;
	uint64_t irq_enable;
	struct work_struct irq_work;
	struct blocking_notifier_head notifier_list;

	int (*init_irqs)(struct rtp_mfd_chip *chip);
	int (*update_irqs_en)(struct rtp_mfd_chip *chip);
	int (*read_irqs)(struct rtp_mfd_chip *chip, uint64_t *irqs);
	int (*write_irqs)(struct rtp_mfd_chip *chip, uint64_t irqs);
};

struct rtp_board {
	int gpio_base;
	int irq;
	unsigned long regulator_ext_sleep_control[RTP_ID_REGULATORS_NUM];
	struct regulator_init_data *rtp_pmic_init_data[RTP_ID_REGULATORS_NUM];

	/** Called before subdevices are set up */
	int (*pre_init)(struct rtp_mfd_chip *chip);
	/** Called after subdevices are set up */
	int (*post_init)(struct rtp_mfd_chip *chip);
	/** Called before ic is power down */
	int (*late_exit)(struct rtp_mfd_chip *chip);
};

struct rtp_pmu_info {
	char *regulator_name;
	int regulator_id;
	int regulator_vol;
	int regulator_stb_vol;
	int regulator_stb_en;
};

int rtp_set_bits(struct rtp_mfd_chip *chip, int reg, uint8_t bit_mask);
int rtp_clr_bits(struct rtp_mfd_chip *chip, int reg, uint8_t bit_mask);
int rtp_update_bits(struct rtp_mfd_chip *chip, int reg, uint8_t reg_val, uint8_t mask);
int rtp_reg_read(struct rtp_mfd_chip *chip, uint8_t reg, uint8_t *val);
int rtp_reg_write(struct rtp_mfd_chip *chip, uint8_t reg, uint8_t *val);
int rtp_bulk_read(struct rtp_mfd_chip *chip, uint8_t reg, int count, uint8_t *val);
int rtp_bulk_write(struct rtp_mfd_chip *chip, uint8_t reg, int count, uint8_t *val);

int rtp_register_notifier(struct rtp_mfd_chip *chip, struct notifier_block *nb, uint64_t irqs);
int rtp_unregister_notifier(struct rtp_mfd_chip *chip, struct notifier_block *nb, uint64_t irqs);

int rtp_irq_init(struct rtp_mfd_chip *chip, int irq);
int rtp_irq_exit(struct rtp_mfd_chip *chip);

int rtp_power_off(void);

extern uint8_t rtp_regu_init_vol[RTP_ID_REGULATORS_NUM];

static inline int rtp_chip_id(struct rtp_mfd_chip *chip)
{
	return chip->id;
}

int rk_sleep_config(void);
int register_rw_init(struct rtp_mfd_chip *chip);
int interrupts_init(struct rtp_mfd_chip *chip);
int system_uv_init(struct rtp_mfd_chip *chip);
int ldo_init(struct rtp_mfd_chip *chip);
int buck_init(struct rtp_mfd_chip *chip);
int power_stb_init(struct rtp_mfd_chip *chip);
int system_global_init(struct rtp_mfd_chip *chip);

#endif
