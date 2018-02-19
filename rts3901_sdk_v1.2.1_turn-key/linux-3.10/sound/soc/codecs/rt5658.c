/*
 * rt5658.c  --  RT5658 ALSA SoC audio codec driver
 *
 * Copyright 2014 Realtek Semiconductor Corp.
 * Author: Bard Liao <bardliao@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/jack.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/rt5658.h>

#include "rt5658.h"

/* Delay(ms) after powering on DMIC for avoiding pop */
static int dmic_power_delay = 450;
module_param(dmic_power_delay, int, 0644);

#define DEVICE_ID 0x6311

#define VERSION "0.2 alsa 1.0.25"

struct rt5658_pll_code {
	bool m_bp; /* Indicates bypass m code or not. */
	int m_code;
	int n_code;
	int k_code;
};

static struct reg_default init_list[] = {
	{RT5658_SPO_VOL,		0x0808},/*Unmute SPVOL L/R*/
	{RT5658_STO_DAC_MIXER,		0x2a8a},/*DACL1/R1->Sto DAC L/R Mixer*/
	{RT5658_MONO_DAC_MIXER,		0x2a8a},/*DACL1/R1->Mono DAC L/R Mixer*/
	{RT5658_A_DAC_MUX,		0x000f},/*Stereo DAC MIX->DAC1/2*/
	{RT5658_AD_DA_MIXER,		0x8080},
	{RT5658_SPK_L_MIXER,		0x001e},/*DAC L2->SPK L MIX*/
	{RT5658_SPK_R_MIXER,		0x001e},/*DAC R2->SPK R MIX*/
	{RT5658_SPO_AMP_GAIN,		0x1103},/*SPKVOL -> SPKOMIX*/
	{RT5658_CHOP_DAC,		0xd0f0},
	{RT5658_PRE_DIV_1,		0xef00},
	{RT5658_PRE_DIV_2,		0xeffc},
	{RT5658_GPIO_CTRL_3,		0x8000},/*Set I2S2 pin as gpio*/
	/*DMIC1_SDA from GPIO5*/
	{RT5658_DMIC_CTRL_1,		0x24a8},
	{RT5658_GPIO_CTRL_1,		0x4800},
#if 1	/*DMIC1 path*/
	{RT5658_STO1_ADC_MIXER,		0x2800},/*ADC1,DMIC1->STO1 ADC MIX*/
	{RT5658_MONO_ADC_MIXER,		0x181a},/*ADC1,DMIC1->MONO ADC MIX*/
#else	/*DMIC2 path*/
	{RT5658_STO1_ADC_MIXER,		0x2900},/*ADC1,DMIC2->STO1ADCMIX*/
	{RT5658_MONO_ADC_MIXER,		0x191b},/*ADC1,DMIC2->MONO ADC MIX*/
#endif
	/* Headset mic (IN1) */
	{RT5658_IN1_IN2,		0x4000}, /*Set BST1 to 30dB*/
	{RT5658_REC1_L2_MIXER,		0x005f}, /*BST1->RECMIX1L*/
	{RT5658_REC1_R2_MIXER,		0x005f}, /*BST1->RECMIX1R*/
	{RT5658_EJD_CTRL_1,		0x60c0},
#if 1	/* Jack detect (JD3 to IRQ)*/
	{RT5658_RC_CLK_CTRL,		0x9000},
	{RT5658_GPIO_CTRL_1,		0xc800}, /*set GPIO1 to IRQ*/
	{RT5658_PWR_ANLG_2,		0x0001}, /*JD3 power on */
	{RT5658_IRQ_CTRL_2,		0x0050},
#endif
	/*Headset detect*/
	{RT5658_IRQ_CTRL_5,		0x0008},
	{RT5658_EJD_CTRL_2,		0x4160},
};
#define RT5658_INIT_REG_LEN ARRAY_SIZE(init_list)

static const struct reg_default rt5658_reg[RT5658_ADC_R_EQ_POST_VOL + 1] = {
	{ 0x0000, 0x0000 },
	{ 0x0001, 0x4848 },
	{ 0x0002, 0x8080 },
	{ 0x0003, 0xc8c8 },
	{ 0x0004, 0xc80a },
	{ 0x0005, 0x0000 },
	{ 0x0006, 0x0000 },
	{ 0x0007, 0x0103 },
	{ 0x0008, 0x0000 },
	{ 0x0009, 0x0000 },
	{ 0x000a, 0x0000 },
	{ 0x000c, 0x0000 },
	{ 0x000d, 0x0000 },
	{ 0x000f, 0x0808 },
	{ 0x0010, 0x3080 },
	{ 0x0011, 0x4a00 },
	{ 0x0012, 0x4e00 },
	{ 0x0015, 0x42c1 },
	{ 0x0016, 0x0000 },
	{ 0x0018, 0x030b },
	{ 0x0019, 0xafaf },
	{ 0x001b, 0x0011 },
	{ 0x001c, 0x2f2f },
	{ 0x001d, 0x2f2f },
	{ 0x001e, 0x2f2f },
	{ 0x001f, 0x0000 },
	{ 0x0020, 0x0000 },
	{ 0x0021, 0x0000 },
	{ 0x0022, 0x5757 },
	{ 0x0023, 0x0000 },
	{ 0x0026, 0x7060 },
	{ 0x0027, 0xc2c4 },
	{ 0x0029, 0x8080 },
	{ 0x002a, 0xaaaa },
	{ 0x002b, 0xaaaa },
	{ 0x002c, 0x00af },
	{ 0x002d, 0x0000 },
	{ 0x002f, 0x1002 },
	{ 0x0031, 0x5000 },
	{ 0x0032, 0x0000 },
	{ 0x0033, 0x0000 },
	{ 0x0034, 0x0000 },
	{ 0x0035, 0x0000 },
	{ 0x0036, 0x8000 },
	{ 0x003a, 0x0000 },
	{ 0x003b, 0x0000 },
	{ 0x003c, 0x007f },
	{ 0x003d, 0x0000 },
	{ 0x003e, 0x007f },
	{ 0x0040, 0x0000 },
	{ 0x0046, 0x001f },
	{ 0x0047, 0x001f },
	{ 0x0048, 0x0003 },
	{ 0x0049, 0xe061 },
	{ 0x004a, 0x0000 },
	{ 0x004b, 0x031f },
	{ 0x004d, 0x0000 },
	{ 0x004e, 0x001f },
	{ 0x004f, 0x001f },
	{ 0x0050, 0x001f },
	{ 0x0052, 0xf0f0 },
	{ 0x0053, 0x0111 },
	{ 0x0054, 0x0064 },
	{ 0x0055, 0x0000 },
	{ 0x0056, 0xef0e },
	{ 0x0057, 0xf0f0 },
	{ 0x0058, 0xef0e },
	{ 0x0059, 0xf0f0 },
	{ 0x005a, 0xef0e },
	{ 0x005b, 0xf0f0 },
	{ 0x005c, 0xf000 },
	{ 0x005d, 0x0000 },
	{ 0x005e, 0x1f2c },
	{ 0x005f, 0x1f2c },
	{ 0x0060, 0x0707 },
	{ 0x0061, 0x0300 },
	{ 0x0062, 0x0000 },
	{ 0x0063, 0x003e },
	{ 0x0064, 0x0000 },
	{ 0x0065, 0x0000 },
	{ 0x0066, 0x0000 },
	{ 0x0067, 0x0000 },
	{ 0x006a, 0x0000 },
	{ 0x006b, 0x0000 },
	{ 0x006c, 0x0000 },
	{ 0x006e, 0x0000 },
	{ 0x006f, 0x0000 },
	{ 0x0070, 0x8000 },
	{ 0x0071, 0x8000 },
	{ 0x0072, 0x8000 },
	{ 0x0073, 0x1110 },
	{ 0x0074, 0xfe00 },
	{ 0x0075, 0x1405 },
	{ 0x0076, 0x000a },
	{ 0x0077, 0x0c00 },
	{ 0x0078, 0x0000 },
	{ 0x0079, 0x0000 },
	{ 0x007a, 0x0123 },
	{ 0x007b, 0x8000 },
	{ 0x0080, 0x0000 },
	{ 0x0081, 0x0000 },
	{ 0x0082, 0x0000 },
	{ 0x0083, 0xc000 },
	{ 0x0084, 0x0000 },
	{ 0x0085, 0x0000 },
	{ 0x0086, 0x0008 },
	{ 0x0087, 0x0000 },
	{ 0x0088, 0x0000 },
	{ 0x0089, 0x0000 },
	{ 0x008a, 0x0000 },
	{ 0x008b, 0x0020 },
	{ 0x008c, 0x0003 },
	{ 0x008e, 0x0000 },
	{ 0x008f, 0x1100 },
	{ 0x0090, 0x0646 },
	{ 0x0091, 0x0c16 },
	{ 0x0092, 0x0073 },
	{ 0x0093, 0x0000 },
	{ 0x0094, 0x0000 },
	{ 0x0097, 0x0000 },
	{ 0x0098, 0x0000 },
	{ 0x0099, 0x0000 },
	{ 0x009a, 0x0000 },
	{ 0x009b, 0x0000 },
	{ 0x009c, 0x007f },
	{ 0x009d, 0x0000 },
	{ 0x009e, 0x007f },
	{ 0x009f, 0x0000 },
	{ 0x00a0, 0x0060 },
	{ 0x00a1, 0x0059 },
	{ 0x00ae, 0x2000 },
	{ 0x00af, 0x0000 },
	{ 0x00b0, 0x2000 },
	{ 0x00b1, 0x0000 },
	{ 0x00b2, 0x0000 },
	{ 0x00b6, 0x0000 },
	{ 0x00b7, 0x0000 },
	{ 0x00b8, 0x0000 },
	{ 0x00b9, 0x0000 },
	{ 0x00ba, 0x0000 },
	{ 0x00bb, 0x0000 },
	{ 0x00be, 0x0000 },
	{ 0x00bf, 0x0000 },
	{ 0x00c0, 0x0400 },
	{ 0x00c1, 0x0000 },
	{ 0x00c2, 0x0000 },
	{ 0x00c3, 0x0000 },
	{ 0x00c4, 0x0003 },
	{ 0x00c5, 0x0000 },
	{ 0x00cb, 0x802f },
	{ 0x00cc, 0x0000 },
	{ 0x00cd, 0x4000 },
	{ 0x00d6, 0x0400 },
	{ 0x00d7, 0x0400 },
	{ 0x00d9, 0x0809 },
	{ 0x00da, 0x0000 },
	{ 0x00db, 0x0003 },
	{ 0x00dc, 0x0049 },
	{ 0x00dd, 0x6724 },
	{ 0x00de, 0x0049 },
	{ 0x00df, 0x0003 },
	{ 0x00e0, 0x4000 },
	{ 0x00e1, 0x0049 },
	{ 0x00e4, 0x400c },
	{ 0x00e5, 0x2031 },
	{ 0x00ea, 0xb320 },
	{ 0x00eb, 0x0000 },
	{ 0x00ec, 0xb300 },
	{ 0x00ed, 0x0000 },
	{ 0x00f0, 0x0000 },
	{ 0x00f1, 0x0000 },
	{ 0x00f2, 0x0ddd },
	{ 0x00f3, 0x0ddd },
	{ 0x00f4, 0x0ddd },
	{ 0x00f6, 0x0000 },
	{ 0x00f7, 0x0000 },
	{ 0x00f8, 0x0000 },
	{ 0x00f9, 0x0000 },
	{ 0x00fa, 0x8000 },
	{ 0x00fb, 0x0000 },
	{ 0x00fc, 0x0000 },
	{ 0x00fd, 0x0001 },
	{ 0x00fe, 0x10ec },
	{ 0x00ff, 0x6311 },
	{ 0x0100, 0xaaaa },
	{ 0x010a, 0xaaaa },
	{ 0x010b, 0x00a0 },
	{ 0x010c, 0xaeae },
	{ 0x010d, 0xaaaa },
	{ 0x010e, 0xaaaa },
	{ 0x010f, 0xa0aa },
	{ 0x0110, 0xa02a },
	{ 0x0111, 0xa702 },
	{ 0x0112, 0xa2aa },
	{ 0x0113, 0x2800 },
	{ 0x0116, 0x0000 },
	{ 0x0117, 0x0000 },
	{ 0x011a, 0x0011 },
	{ 0x011b, 0x0011 },
	{ 0x011c, 0x0157 },
	{ 0x011d, 0x0000 },
	{ 0x011e, 0x0000 },
	{ 0x011f, 0x0000 },
	{ 0x0120, 0x0000 },
	{ 0x0121, 0x009b },
	{ 0x0122, 0x5014 },
	{ 0x0123, 0x0421 },
	{ 0x0124, 0x7cea },
	{ 0x0125, 0x0420 },
	{ 0x0126, 0x5550 },
	{ 0x0132, 0x0000 },
	{ 0x0133, 0x0000 },
	{ 0x0137, 0x5155 },
	{ 0x0138, 0x3700 },
	{ 0x0139, 0x79a1 },
	{ 0x013a, 0x2020 },
	{ 0x013b, 0x2020 },
	{ 0x013c, 0x2020 },
	{ 0x013e, 0x1f08 },
	{ 0x013f, 0x0000 },
	{ 0x0145, 0x0002 },
	{ 0x0146, 0x0000 },
	{ 0x0147, 0x0000 },
	{ 0x0148, 0x0000 },
	{ 0x0150, 0x0013 },
	{ 0x0151, 0x0680 },
	{ 0x0152, 0x1c17 },
	{ 0x0153, 0x6883 },
	{ 0x0154, 0xe3ce },
	{ 0x0155, 0x352d },
	{ 0x0156, 0x00eb },
	{ 0x0157, 0x3717 },
	{ 0x0158, 0x4c6a },
	{ 0x0159, 0xe41b },
	{ 0x015a, 0x2a13 },
	{ 0x015b, 0x3600 },
	{ 0x015c, 0x78e6 },
	{ 0x015d, 0xc6bb },
	{ 0x0160, 0x0000 },
	{ 0x0161, 0x0000 },
	{ 0x0162, 0x0000 },
	{ 0x0163, 0x0000 },
	{ 0x0164, 0x0000 },
	{ 0x0165, 0x0000 },
	{ 0x0166, 0x0000 },
	{ 0x0167, 0x001f },
	{ 0x0170, 0x0000 },
	{ 0x0171, 0x0020 },
	{ 0x0172, 0x0080 },
	{ 0x0173, 0x0800 },
	{ 0x0174, 0x100c },
	{ 0x0175, 0x000c },
	{ 0x0190, 0x3300 },
	{ 0x0191, 0x2200 },
	{ 0x0192, 0x0000 },
	{ 0x01b0, 0x033f },
	{ 0x01b1, 0x0000 },
	{ 0x01b2, 0xafaf },
	{ 0x01b3, 0x0000 },
	{ 0x01c0, 0x0000 },
	{ 0x01c1, 0x0540 },
	{ 0x01c2, 0x0000 },
	{ 0x01c3, 0x0000 },
	{ 0x01c7, 0x0000 },
	{ 0x01c8, 0x5757 },
	{ 0x01c9, 0x5757 },
	{ 0x01ca, 0x5757 },
	{ 0x01cb, 0x5757 },
	{ 0x01cc, 0x5757 },
	{ 0x01cd, 0x5757 },
	{ 0x01ce, 0x5757 },
	{ 0x01da, 0x0000 },
	{ 0x01db, 0x0000 },
	{ 0x01de, 0x0000 },
	{ 0x01df, 0x10c0 },
	{ 0x01e0, 0x06a1 },
	{ 0x01e1, 0x0000 },
	{ 0x01e2, 0x0000 },
	{ 0x01e3, 0x0000 },
	{ 0x01e4, 0x0001 },
	{ 0x01e6, 0x0000 },
	{ 0x01e7, 0x0000 },
	{ 0x01e8, 0x0000 },
	{ 0x01ea, 0x0000 },
	{ 0x01eb, 0x0000 },
	{ 0x01ec, 0x0000 },
	{ 0x01ed, 0x0000 },
	{ 0x01ee, 0x0000 },
	{ 0x01ef, 0x0000 },
	{ 0x01f0, 0x0000 },
	{ 0x01f1, 0x0000 },
	{ 0x01f2, 0x0000 },
	{ 0x01f6, 0x0000 },
	{ 0x01f7, 0x0000 },
	{ 0x01f8, 0x0000 },
	{ 0x01f9, 0x0000 },
	{ 0x01fa, 0x0002 },
	{ 0x01fb, 0x0000 },
	{ 0x01fc, 0x0000 },
	{ 0x01fd, 0x0000 },
	{ 0x01fe, 0x0000 },
	{ 0x0256, 0x0000 },
	{ 0x0257, 0x0000 },
	{ 0x0258, 0x0000 },
	{ 0x0259, 0x0000 },
	{ 0x025a, 0x3333 },
	{ 0x025b, 0x3333 },
	{ 0x025c, 0x3333 },
	{ 0x025d, 0x3333 },
	{ 0x025e, 0x0000 },
	{ 0x025f, 0x0000 },
	{ 0x0260, 0x0000 },
	{ 0x0261, 0x0022 },
	{ 0x0262, 0x0300 },
	{ 0x0265, 0x7f03 },
	{ 0x0266, 0x0132 },
	{ 0x0267, 0x0000 },
	{ 0x0268, 0x0000 },
	{ 0x0269, 0x0000 },
	{ 0x026a, 0x0000 },
	{ 0x026b, 0x0000 },
	{ 0x026c, 0x0000 },
	{ 0x026d, 0x0000 },
	{ 0x026e, 0x0000 },
	{ 0x026f, 0x0000 },
	{ 0x0270, 0x0000 },
	{ 0x0271, 0x0000 },
	{ 0x0272, 0x0000 },
	{ 0x0273, 0x0000 },
	{ 0x0280, 0x0000 },
	{ 0x0281, 0x0000 },
	{ 0x0282, 0x0000 },
	{ 0x0283, 0x0000 },
	{ 0x0284, 0x0000 },
	{ 0x0290, 0x0140 },
	{ 0x0291, 0x0100 },
	{ 0x02fa, 0x0000 },
	{ 0x02fb, 0x0000 },
	{ 0x02fc, 0x0000 },
	{ 0x0300, 0x001f },
	{ 0x0301, 0x030c },
	{ 0x0302, 0x1f00 },
	{ 0x0303, 0x0000 },
	{ 0x0304, 0x4000 },
	{ 0x0305, 0x0000 },
	{ 0x0306, 0x8000 },
	{ 0x0307, 0x0200 },
	{ 0x0308, 0x001f },
	{ 0x0309, 0x030c },
	{ 0x030a, 0x1f00 },
	{ 0x030b, 0x0000 },
	{ 0x030c, 0x4000 },
	{ 0x030d, 0x0000 },
	{ 0x030e, 0x8000 },
	{ 0x030f, 0x0200 },
	{ 0x0310, 0x2d60 },
	{ 0x0311, 0xa480 },
	{ 0x0312, 0x7418 },
	{ 0x0313, 0x0000 },
	{ 0x0314, 0x0006 },
	{ 0x0315, 0x00ff },
	{ 0x0316, 0x0000 },
	{ 0x0317, 0x2d60 },
	{ 0x0318, 0xa480 },
	{ 0x0319, 0x7418 },
	{ 0x031a, 0x0000 },
	{ 0x031b, 0x0006 },
	{ 0x031c, 0x00ff },
	{ 0x031d, 0x0000 },
	{ 0x0320, 0x0f20 },
	{ 0x0321, 0x8700 },
	{ 0x0322, 0x7d2c },
	{ 0x0323, 0xa178 },
	{ 0x0324, 0x5383 },
	{ 0x0325, 0x7dc2 },
	{ 0x0326, 0xa178 },
	{ 0x0327, 0x5383 },
	{ 0x0328, 0x003e },
	{ 0x0329, 0x02c1 },
	{ 0x032a, 0xd37d },
	{ 0x0330, 0x0043 },
	{ 0x0331, 0x0406 },
	{ 0x0332, 0xa64c },
	{ 0x0333, 0x2048 },
	{ 0x0334, 0x0045 },
	{ 0x0335, 0x2007 },
	{ 0x0336, 0x2007 },
	{ 0x0337, 0x0501 },
	{ 0x0338, 0x0010 },
	{ 0x0339, 0x0000 },
	{ 0x033a, 0x0000 },
	{ 0x0340, 0x0800 },
	{ 0x0341, 0x0800 },
	{ 0x0342, 0x0800 },
	{ 0x0343, 0x0800 },
	{ 0x0344, 0x0000 },
	{ 0x0345, 0x0000 },
	{ 0x0346, 0x0000 },
	{ 0x0347, 0x0000 },
	{ 0x0348, 0x0000 },
	{ 0x0349, 0x0000 },
	{ 0x034a, 0x0000 },
	{ 0x034b, 0x0000 },
	{ 0x034c, 0x0000 },
	{ 0x034d, 0x0000 },
	{ 0x034e, 0x0000 },
	{ 0x034f, 0x0000 },
	{ 0x0350, 0x0000 },
	{ 0x0351, 0x0000 },
	{ 0x0352, 0x0000 },
	{ 0x0353, 0x0000 },
	{ 0x0354, 0x0000 },
	{ 0x0355, 0x0000 },
	{ 0x0356, 0x0000 },
	{ 0x0357, 0x0000 },
	{ 0x0358, 0x0000 },
	{ 0x0359, 0x0000 },
	{ 0x035a, 0x0000 },
	{ 0x035b, 0x0000 },
	{ 0x035c, 0x0000 },
	{ 0x035d, 0x0000 },
	{ 0x035e, 0x2000 },
	{ 0x035f, 0x0000 },
	{ 0x0360, 0x2000 },
	{ 0x0361, 0x2000 },
	{ 0x0362, 0x0000 },
	{ 0x0363, 0x2000 },
	{ 0x0364, 0x0200 },
	{ 0x0365, 0x0000 },
	{ 0x0366, 0x0000 },
	{ 0x0367, 0x0000 },
	{ 0x0368, 0x0000 },
	{ 0x0369, 0x0000 },
	{ 0x036a, 0x0000 },
	{ 0x036b, 0x0000 },
	{ 0x036c, 0x0000 },
	{ 0x036d, 0x0000 },
	{ 0x036e, 0x0000 },
	{ 0x036f, 0x0000 },
	{ 0x0370, 0x0000 },
	{ 0x0371, 0x0000 },
	{ 0x0372, 0x0000 },
	{ 0x0373, 0x0000 },
	{ 0x0374, 0x0000 },
	{ 0x0375, 0x0000 },
	{ 0x0376, 0x0000 },
	{ 0x0377, 0x0000 },
	{ 0x03d0, 0x0000 },
	{ 0x03d1, 0x0000 },
	{ 0x03d2, 0x0000 },
	{ 0x03d3, 0x0000 },
	{ 0x03d4, 0x2000 },
	{ 0x03d5, 0x2000 },
	{ 0x03d6, 0x0000 },
	{ 0x03d7, 0x0000 },
	{ 0x03d8, 0x2000 },
	{ 0x03d9, 0x2000 },
	{ 0x03da, 0x2000 },
	{ 0x03db, 0x2000 },
	{ 0x03dc, 0x0000 },
	{ 0x03dd, 0x0000 },
	{ 0x03de, 0x0000 },
	{ 0x03df, 0x2000 },
	{ 0x03e0, 0x0000 },
	{ 0x03e1, 0x0000 },
	{ 0x03e2, 0x0000 },
	{ 0x03e3, 0x0000 },
	{ 0x03e4, 0x0000 },
	{ 0x03e5, 0x0000 },
	{ 0x03e6, 0x0000 },
	{ 0x03e7, 0x0000 },
	{ 0x03e8, 0x0000 },
	{ 0x03e9, 0x0000 },
	{ 0x03ea, 0x0000 },
	{ 0x03eb, 0x0000 },
	{ 0x03ec, 0x0000 },
	{ 0x03ed, 0x0000 },
	{ 0x03ee, 0x0000 },
	{ 0x03ef, 0x0000 },
	{ 0x03f0, 0x0800 },
	{ 0x03f1, 0x0800 },
	{ 0x03f2, 0x0800 },
	{ 0x03f3, 0x0800 },
	{ 0x03d6, 0x0000 },
};

static bool rt5658_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RT5658_RESET:
	case RT5658_EJD_CTRL_2:
	case RT5658_SILENCE_CTRL:
	case RT5658_DAC1_DIG_VOL:
	case RT5658_DAC2_DIG_VOL:
	case RT5658_DAC_CTRL:
	case RT5658_HP_IMP_GAIN_2:
	case RT5658_AD_DA_MIXER:
	case RT5658_PDM_OUT_CTRL:
	case RT5658_PDM_DATA_CTRL_1:
	case RT5658_PDM_DATA_CTRL_4:
	case RT5658_HAPTIC_GEN_CTRL_1:
	case RT5658_HAPTIC_GEN_CTRL_3:
	case RT5658_HAPTIC_LPF_CTRL_3:
	case RT5658_CLK_DET:
	case RT5658_DMIC_CTRL_1:
	case RT5658_PLL_CTRL_2:
	case RT5658_MICBIAS_1:
	case RT5658_ASRC_11:
	case RT5658_ADC_EQ_CTRL_1:
	case RT5658_DAC_EQ_CTRL_1:
	case RT5658_INT_ST_1:
	case RT5658_INT_ST_2:
	case RT5658_GPIO_STA:
	case RT5658_SINE_GEN_CTRL_1:
	case RT5658_IL_CMD_1:
	case RT5658_4BTN_IL_CMD_1:
	case RT5658_PSV_IL_CMD_1:
	case RT5658_AJD1_CTRL:
	case RT5658_AJD2_AJD3_CTRL:
	case RT5658_JD_CTRL_3:
	case RT5658_VENDOR_ID:
	case RT5658_VENDOR_ID_1:
	case RT5658_DEVICE_ID:
	case RT5658_MEMORY_TEST:
	case RT5658_SOFT_RAMP_DEPOP_DAC_CLK_CTRL:
	case RT5658_VOL_TEST:
	case RT5658_STO_DRE_CTRL_1:
	case RT5658_STO_DRE_CTRL_5:
	case RT5658_STO_DRE_CTRL_6:
	case RT5658_STO_DRE_CTRL_7:
	case RT5658_MONO_DRE_CTRL_1:
	case RT5658_MONO_DRE_CTRL_5:
	case RT5658_MONO_DRE_CTRL_6:
	case RT5658_HP_IMP_SENS_CTRL_1:
	case RT5658_HP_IMP_SENS_CTRL_3:
	case RT5658_HP_IMP_SENS_CTRL_4:
	case RT5658_HP_CALIB_CTRL_1:
	case RT5658_HP_CALIB_CTRL_9:
	case RT5658_HP_CALIB_STA_1:
	case RT5658_HP_CALIB_STA_2:
	case RT5658_HP_CALIB_STA_3:
	case RT5658_HP_CALIB_STA_4:
	case RT5658_HP_CALIB_STA_5:
	case RT5658_HP_CALIB_STA_6:
	case RT5658_HP_CALIB_STA_7:
	case RT5658_HP_CALIB_STA_8:
	case RT5658_HP_CALIB_STA_9:
	case RT5658_MONO_AMP_CALIB_CTRL_1:
	case RT5658_MONO_AMP_CALIB_CTRL_3:
	case RT5658_MONO_AMP_CALIB_STA_1:
	case RT5658_MONO_AMP_CALIB_STA_2:
	case RT5658_MONO_AMP_CALIB_STA_3:
	case RT5658_MONO_AMP_CALIB_STA_4:
	case RT5658_SPK_PWR_LMT_STA_1:
	case RT5658_SPK_PWR_LMT_STA_2:
	case RT5658_SPK_PWR_LMT_STA_3:
	case RT5658_SPK_PWR_LMT_STA_4:
	case RT5658_SPK_PWR_LMT_STA_5:
	case RT5658_SPK_PWR_LMT_STA_6:
	case RT5658_SPK_DC_CAILB_CTRL_1:
	case RT5658_SPK_DC_CAILB_STA_1:
	case RT5658_SPK_DC_CAILB_STA_2:
	case RT5658_SPK_DC_CAILB_STA_3:
	case RT5658_SPK_DC_CAILB_STA_4:
	case RT5658_SPK_DC_CAILB_STA_5:
	case RT5658_SPK_DC_CAILB_STA_6:
	case RT5658_SPK_DC_CAILB_STA_7:
	case RT5658_SPK_DC_CAILB_STA_8:
	case RT5658_SPK_DC_CAILB_STA_9:
	case RT5658_SPK_DC_CAILB_STA_10:
	case RT5658_SPK_VDD_STA_1:
	case RT5658_SPK_VDD_STA_2:
	case RT5658_SPK_DC_DET_CTRL_1:
	case RT5658_PURE_DC_DET_CTRL_1:
	case RT5658_PURE_DC_DET_CTRL_2:
	case RT5658_DRC1_PRIV_1:
	case RT5658_DRC1_PRIV_4:
	case RT5658_DRC1_PRIV_5:
	case RT5658_DRC1_PRIV_6:
	case RT5658_DRC1_PRIV_7:
	case RT5658_DRC2_PRIV_1:
	case RT5658_DRC2_PRIV_4:
	case RT5658_DRC2_PRIV_5:
	case RT5658_DRC2_PRIV_6:
	case RT5658_DRC2_PRIV_7:
	case RT5658_ALC_PGA_STA_1:
	case RT5658_ALC_PGA_STA_2:
	case RT5658_ALC_PGA_STA_3:
		return true;
	default:
		return false;
	}
}

static bool rt5658_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RT5658_RESET:
	case RT5658_SPO_VOL:
	case RT5658_HP_VOL:
	case RT5658_LOUT:
	case RT5658_MONO_OUT:
	case RT5658_HPL_GAIN:
	case RT5658_HPR_GAIN:
	case RT5658_MONO_GAIN:
	case RT5658_SPDIF_CTRL_1:
	case RT5658_SPDIF_CTRL_2:
	case RT5658_CAL_BST_CTRL:
	case RT5658_IN1_IN2:
	case RT5658_IN3_IN4:
	case RT5658_INL1_INR1_VOL:
	case RT5658_EJD_CTRL_1:
	case RT5658_EJD_CTRL_2:
	case RT5658_EJD_CTRL_3:
	case RT5658_SILENCE_CTRL:
	case RT5658_PSV_CTRL:
	case RT5658_SIDETONE_CTRL:
	case RT5658_DAC1_DIG_VOL:
	case RT5658_DAC2_DIG_VOL:
	case RT5658_DAC_CTRL:
	case RT5658_STO1_ADC_DIG_VOL:
	case RT5658_MONO_ADC_DIG_VOL:
	case RT5658_STO2_ADC_DIG_VOL:
	case RT5658_STO1_BOOST:
	case RT5658_MONO_BOOST:
	case RT5658_STO2_BOOST:
	case RT5658_HP_IMP_GAIN_1:
	case RT5658_HP_IMP_GAIN_2:
	case RT5658_STO1_ADC_MIXER:
	case RT5658_MONO_ADC_MIXER:
	case RT5658_AD_DA_MIXER:
	case RT5658_STO_DAC_MIXER:
	case RT5658_MONO_DAC_MIXER:
	case RT5658_DIG_MIXER:
	case RT5658_A_DAC_MUX:
	case RT5658_DIG_INF23_DATA:
	case RT5658_PDM_OUT_CTRL:
	case RT5658_PDM_DATA_CTRL_1:
	case RT5658_PDM_DATA_CTRL_2:
	case RT5658_PDM_DATA_CTRL_3:
	case RT5658_PDM_DATA_CTRL_4:
	case RT5658_SPDIF_CTRL:
	case RT5658_REC1_GAIN:
	case RT5658_REC1_L1_MIXER:
	case RT5658_REC1_L2_MIXER:
	case RT5658_REC1_R1_MIXER:
	case RT5658_REC1_R2_MIXER:
	case RT5658_CAL_REC:
	case RT5658_REC2_L1_MIXER:
	case RT5658_REC2_L2_MIXER:
	case RT5658_REC2_R1_MIXER:
	case RT5658_REC2_R2_MIXER:
	case RT5658_SPK_L_MIXER:
	case RT5658_SPK_R_MIXER:
	case RT5658_SPO_AMP_GAIN:
	case RT5658_ALC_BACK_GAIN:
	case RT5658_MONOMIX_GAIN:
	case RT5658_MONOMIX_IN_GAIN:
	case RT5658_OUT_L_GAIN:
	case RT5658_OUT_L_MIXER:
	case RT5658_OUT_R_GAIN:
	case RT5658_OUT_R_MIXER:
	case RT5658_LOUT_MIXER:
	case RT5658_HAPTIC_GEN_CTRL_1:
	case RT5658_HAPTIC_GEN_CTRL_2:
	case RT5658_HAPTIC_GEN_CTRL_3:
	case RT5658_HAPTIC_GEN_CTRL_4:
	case RT5658_HAPTIC_GEN_CTRL_5:
	case RT5658_HAPTIC_GEN_CTRL_6:
	case RT5658_HAPTIC_GEN_CTRL_7:
	case RT5658_HAPTIC_GEN_CTRL_8:
	case RT5658_HAPTIC_GEN_CTRL_9:
	case RT5658_HAPTIC_GEN_CTRL_10:
	case RT5658_HAPTIC_GEN_CTRL_11:
	case RT5658_HAPTIC_LPF_CTRL_1:
	case RT5658_HAPTIC_LPF_CTRL_2:
	case RT5658_HAPTIC_LPF_CTRL_3:
	case RT5658_PWR_DIG_1:
	case RT5658_PWR_DIG_2:
	case RT5658_PWR_ANLG_1:
	case RT5658_PWR_ANLG_2:
	case RT5658_PWR_ANLG_3:
	case RT5658_PWR_MIXER:
	case RT5658_PWR_VOL:
	case RT5658_PRIV_INDEX:
	case RT5658_CLK_DET:
	case RT5658_PRIV_DATA:
	case RT5658_PRE_DIV_1:
	case RT5658_PRE_DIV_2:
	case RT5658_I2S1_SDP:
	case RT5658_I2S2_SDP:
	case RT5658_I2S3_SDP:
	case RT5658_ADDA_CLK_1:
	case RT5658_ADDA_CLK_2:
	case RT5658_DMIC_CTRL_1:
	case RT5658_DMIC_CTRL_2:
	case RT5658_TDM_CTRL_1:
	case RT5658_TDM_CTRL_2:
	case RT5658_TDM_CTRL_3:
	case RT5658_TDM_CTRL_4:
	case RT5658_TDM_CTRL_5:
	case RT5658_GLB_CLK:
	case RT5658_PLL_CTRL_1:
	case RT5658_PLL_CTRL_2:
	case RT5658_ASRC_1:
	case RT5658_ASRC_2:
	case RT5658_ASRC_3:
	case RT5658_ASRC_4:
	case RT5658_ASRC_5:
	case RT5658_ASRC_7:
	case RT5658_ASRC_8:
	case RT5658_ASRC_9:
	case RT5658_ASRC_10:
	case RT5658_DEPOP_1:
	case RT5658_DEPOP_2:
	case RT5658_DEPOP_3:
	case RT5658_HP_CHARGE_PUMP_1:
	case RT5658_HP_CHARGE_PUMP_2:
	case RT5658_MICBIAS_1:
	case RT5658_MICBIAS_2:
	case RT5658_ASRC_11:
	case RT5658_ASRC_12:
	case RT5658_ASRC_13:
	case RT5658_CLASSD_CTRL_1:
	case RT5658_CLASSD_CTRL_2:
	case RT5658_ADC_EQ_CTRL_1:
	case RT5658_ADC_EQ_CTRL_2:
	case RT5658_DAC_EQ_CTRL_1:
	case RT5658_DAC_EQ_CTRL_2:
	case RT5658_IRQ_CTRL_1:
	case RT5658_IRQ_CTRL_2:
	case RT5658_IRQ_CTRL_3:
	case RT5658_IRQ_CTRL_4:
	case RT5658_IRQ_CTRL_5:
	case RT5658_IRQ_CTRL_6:
	case RT5658_INT_ST_1:
	case RT5658_INT_ST_2:
	case RT5658_GPIO_CTRL_1:
	case RT5658_GPIO_CTRL_2:
	case RT5658_GPIO_CTRL_3:
	case RT5658_GPIO_CTRL_4:
	case RT5658_GPIO_CTRL_5:
	case RT5658_GPIO_STA:
	case RT5658_SINE_GEN_CTRL_1:
	case RT5658_SINE_GEN_CTRL_2:
	case RT5658_SINE_GEN_CTRL_3:
	case RT5658_MP3_PLUS_1:
	case RT5658_MP3_PLUS_2:
	case RT5658_ADJ_HPF_1:
	case RT5658_ADJ_HPF_2:
	case RT5658_HP_CALIB_AMP_DET:
	case RT5658_SV_ZCD_1:
	case RT5658_SV_ZCD_2:
	case RT5658_IL_CMD_1:
	case RT5658_IL_CMD_2:
	case RT5658_IL_CMD_3:
	case RT5658_IL_CMD_4:
	case RT5658_4BTN_IL_CMD_1:
	case RT5658_4BTN_IL_CMD_2:
	case RT5658_4BTN_IL_CMD_3:
	case RT5658_PSV_IL_CMD_1:
	case RT5658_PSV_IL_CMD_2:
	case RT5658_ADC_STO1_HP_CTRL_1:
	case RT5658_ADC_STO1_HP_CTRL_2:
	case RT5658_ADC_MONO_HP_CTRL_1:
	case RT5658_ADC_MONO_HP_CTRL_2:
	case RT5658_AJD1_CTRL:
	case RT5658_AJD2_AJD3_CTRL:
	case RT5658_JD1_THD:
	case RT5658_JD2_THD:
	case RT5658_JD3_THD:
	case RT5658_JD_CTRL_1:
	case RT5658_JD_CTRL_2:
	case RT5658_JD_CTRL_3:
	case RT5658_JD_CTRL_4:
	case RT5658_DIG_MISC:
	case RT5658_DUMMY_2:
	case RT5658_DUMMY_3:
	case RT5658_DAC_ADC_DIG_VOL:
	case RT5658_BIAS_CUR_CTRL_1:
	case RT5658_BIAS_CUR_CTRL_2:
	case RT5658_BIAS_CUR_CTRL_3:
	case RT5658_BIAS_CUR_CTRL_4:
	case RT5658_BIAS_CUR_CTRL_5:
	case RT5658_BIAS_CUR_CTRL_6:
	case RT5658_BIAS_CUR_CTRL_7:
	case RT5658_BIAS_CUR_CTRL_8:
	case RT5658_BIAS_CUR_CTRL_9:
	case RT5658_BIAS_CUR_CTRL_10:
	case RT5658_MEMORY_TEST:
	case RT5658_VREF_REC_OP_FB_CAP_CTRL:
	case RT5658_CLASSD_0:
	case RT5658_CLASSD_1:
	case RT5658_CLASSD_2:
	case RT5658_CLASSD_3:
	case RT5658_CLASSD_4:
	case RT5658_CLASSD_5:
	case RT5658_CLASSD_6:
	case RT5658_CLASSD_7:
	case RT5658_CLASSD_8:
	case RT5658_CLASSD_9:
	case RT5658_CLASSD_10:
	case RT5658_CHARGE_PUMP_1:
	case RT5658_CHARGE_PUMP_2:
	case RT5658_DIG_IN_CTRL_1:
	case RT5658_DIG_IN_CTRL_2:
	case RT5658_PAD_DRIVING_CTRL:
	case RT5658_SOFT_RAMP_DEPOP:
	case RT5658_PLL:
	case RT5658_CHOP_DAC:
	case RT5658_CHOP_ADC:
	case RT5658_CALIB_ADC_CTRL:
	case RT5658_SOFT_RAMP_DEPOP_DAC_CLK_CTRL:
	case RT5658_VOL_TEST:
	case RT5658_TEST_MODE_CTRL_1:
	case RT5658_TEST_MODE_CTRL_2:
	case RT5658_TEST_MODE_CTRL_3:
	case RT5658_TEST_MODE_CTRL_4:
	case RT5658_BASSBACK_CTRL:
	case RT5658_MP3_PLUS_CTRL_1:
	case RT5658_MP3_PLUS_CTRL_2:
	case RT5658_MP3_HPF_A1:
	case RT5658_MP3_HPF_A2:
	case RT5658_MP3_HPF_H0:
	case RT5658_MP3_LPF_H0:
	case RT5658_3D_SPK_CTRL:
	case RT5658_3D_SPK_COEF_1:
	case RT5658_3D_SPK_COEF_2:
	case RT5658_3D_SPK_COEF_3:
	case RT5658_3D_SPK_COEF_4:
	case RT5658_3D_SPK_COEF_5:
	case RT5658_3D_SPK_COEF_6:
	case RT5658_3D_SPK_COEF_7:
	case RT5658_STO_DRE_CTRL_1:
	case RT5658_STO_DRE_CTRL_2:
	case RT5658_STO_DRE_CTRL_3:
	case RT5658_STO_DRE_CTRL_4:
	case RT5658_STO_DRE_CTRL_5:
	case RT5658_STO_DRE_CTRL_6:
	case RT5658_STO_DRE_CTRL_7:
	case RT5658_STO_DRE_CTRL_8:
	case RT5658_MONO_DRE_CTRL_1:
	case RT5658_MONO_DRE_CTRL_2:
	case RT5658_MONO_DRE_CTRL_3:
	case RT5658_MONO_DRE_CTRL_4:
	case RT5658_MONO_DRE_CTRL_5:
	case RT5658_MONO_DRE_CTRL_6:
	case RT5658_MID_HP_AMP_DET:
	case RT5658_LOW_HP_AMP_DET:
	case RT5658_LDO_CTRL:
	case RT5658_HP_DECROSS_CTRL_1:
	case RT5658_HP_DECROSS_CTRL_2:
	case RT5658_HP_DECROSS_CTRL_3:
	case RT5658_HP_DECROSS_CTRL_4:
	case RT5658_HP_IMP_SENS_CTRL_1:
	case RT5658_HP_IMP_SENS_CTRL_2:
	case RT5658_HP_IMP_SENS_CTRL_3:
	case RT5658_HP_IMP_SENS_CTRL_4:
	case RT5658_HP_IMP_SENS_MAP_1:
	case RT5658_HP_IMP_SENS_MAP_2:
	case RT5658_HP_IMP_SENS_MAP_3:
	case RT5658_HP_IMP_SENS_MAP_4:
	case RT5658_HP_IMP_SENS_MAP_5:
	case RT5658_HP_IMP_SENS_MAP_6:
	case RT5658_HP_IMP_SENS_MAP_7:
	case RT5658_HP_IMP_SENS_MAP_8:
	case RT5658_HP_LOGIC_CTRL_1:
	case RT5658_HP_LOGIC_CTRL_2:
	case RT5658_HP_CALIB_CTRL_1:
	case RT5658_HP_CALIB_CTRL_2:
	case RT5658_HP_CALIB_CTRL_3:
	case RT5658_HP_CALIB_CTRL_4:
	case RT5658_HP_CALIB_CTRL_5:
	case RT5658_HP_CALIB_CTRL_6:
	case RT5658_HP_CALIB_CTRL_7:
	case RT5658_HP_CALIB_CTRL_8:
	case RT5658_HP_CALIB_CTRL_9:
	case RT5658_HP_CALIB_CTRL_10:
	case RT5658_HP_CALIB_CTRL_11:
	case RT5658_HP_CALIB_STA_1:
	case RT5658_HP_CALIB_STA_2:
	case RT5658_HP_CALIB_STA_3:
	case RT5658_HP_CALIB_STA_4:
	case RT5658_HP_CALIB_STA_5:
	case RT5658_HP_CALIB_STA_6:
	case RT5658_HP_CALIB_STA_7:
	case RT5658_HP_CALIB_STA_8:
	case RT5658_HP_CALIB_STA_9:
	case RT5658_MONO_AMP_CALIB_CTRL_1:
	case RT5658_MONO_AMP_CALIB_CTRL_2:
	case RT5658_MONO_AMP_CALIB_CTRL_3:
	case RT5658_MONO_AMP_CALIB_CTRL_4:
	case RT5658_MONO_AMP_CALIB_CTRL_5:
	case RT5658_MONO_AMP_CALIB_STA_1:
	case RT5658_MONO_AMP_CALIB_STA_2:
	case RT5658_MONO_AMP_CALIB_STA_3:
	case RT5658_MONO_AMP_CALIB_STA_4:
	case RT5658_SPK_PWR_LMT_CTRL_1:
	case RT5658_SPK_PWR_LMT_CTRL_2:
	case RT5658_SPK_PWR_LMT_CTRL_3:
	case RT5658_SPK_PWR_LMT_STA_1:
	case RT5658_SPK_PWR_LMT_STA_2:
	case RT5658_SPK_PWR_LMT_STA_3:
	case RT5658_SPK_PWR_LMT_STA_4:
	case RT5658_SPK_PWR_LMT_STA_5:
	case RT5658_SPK_PWR_LMT_STA_6:
	case RT5658_FLEX_SPK_BST_CTRL_1:
	case RT5658_FLEX_SPK_BST_CTRL_2:
	case RT5658_FLEX_SPK_BST_CTRL_3:
	case RT5658_FLEX_SPK_BST_CTRL_4:
	case RT5658_SPK_EX_LMT_CTRL_1:
	case RT5658_SPK_EX_LMT_CTRL_2:
	case RT5658_SPK_EX_LMT_CTRL_3:
	case RT5658_SPK_EX_LMT_CTRL_4:
	case RT5658_SPK_EX_LMT_CTRL_5:
	case RT5658_SPK_EX_LMT_CTRL_6:
	case RT5658_SPK_EX_LMT_CTRL_7:
	case RT5658_ADJ_HPF_CTRL_1:
	case RT5658_ADJ_HPF_CTRL_2:
	case RT5658_SPK_DC_CAILB_CTRL_1:
	case RT5658_SPK_DC_CAILB_CTRL_2:
	case RT5658_SPK_DC_CAILB_CTRL_3:
	case RT5658_SPK_DC_CAILB_CTRL_4:
	case RT5658_SPK_DC_CAILB_CTRL_5:
	case RT5658_SPK_DC_CAILB_STA_1:
	case RT5658_SPK_DC_CAILB_STA_2:
	case RT5658_SPK_DC_CAILB_STA_3:
	case RT5658_SPK_DC_CAILB_STA_4:
	case RT5658_SPK_DC_CAILB_STA_5:
	case RT5658_SPK_DC_CAILB_STA_6:
	case RT5658_SPK_DC_CAILB_STA_7:
	case RT5658_SPK_DC_CAILB_STA_8:
	case RT5658_SPK_DC_CAILB_STA_9:
	case RT5658_SPK_DC_CAILB_STA_10:
	case RT5658_SPK_VDD_STA_1:
	case RT5658_SPK_VDD_STA_2:
	case RT5658_SPK_DC_DET_CTRL_1:
	case RT5658_SPK_DC_DET_CTRL_2:
	case RT5658_SPK_DC_DET_CTRL_3:
	case RT5658_PURE_DC_DET_CTRL_1:
	case RT5658_PURE_DC_DET_CTRL_2:
	case RT5658_DUMMY_4:
	case RT5658_DUMMY_5:
	case RT5658_DUMMY_6:
	case RT5658_DRC1_CTRL_1:
	case RT5658_DRC1_CTRL_2:
	case RT5658_DRC1_CTRL_3:
	case RT5658_DRC1_CTRL_4:
	case RT5658_DRC1_CTRL_5:
	case RT5658_DRC1_CTRL_6:
	case RT5658_DRC1_HARD_LMT_CTRL_1:
	case RT5658_DRC1_HARD_LMT_CTRL_2:
	case RT5658_DRC2_CTRL_1:
	case RT5658_DRC2_CTRL_2:
	case RT5658_DRC2_CTRL_3:
	case RT5658_DRC2_CTRL_4:
	case RT5658_DRC2_CTRL_5:
	case RT5658_DRC2_CTRL_6:
	case RT5658_DRC2_HARD_LMT_CTRL_1:
	case RT5658_DRC2_HARD_LMT_CTRL_2:
	case RT5658_DRC1_PRIV_1:
	case RT5658_DRC1_PRIV_2:
	case RT5658_DRC1_PRIV_3:
	case RT5658_DRC1_PRIV_4:
	case RT5658_DRC1_PRIV_5:
	case RT5658_DRC1_PRIV_6:
	case RT5658_DRC1_PRIV_7:
	case RT5658_DRC2_PRIV_1:
	case RT5658_DRC2_PRIV_2:
	case RT5658_DRC2_PRIV_3:
	case RT5658_DRC2_PRIV_4:
	case RT5658_DRC2_PRIV_5:
	case RT5658_DRC2_PRIV_6:
	case RT5658_DRC2_PRIV_7:
	case RT5658_MULTI_DRC_CTRL:
	case RT5658_CROSS_OVER_1:
	case RT5658_CROSS_OVER_2:
	case RT5658_CROSS_OVER_3:
	case RT5658_CROSS_OVER_4:
	case RT5658_CROSS_OVER_5:
	case RT5658_CROSS_OVER_6:
	case RT5658_CROSS_OVER_7:
	case RT5658_CROSS_OVER_8:
	case RT5658_CROSS_OVER_9:
	case RT5658_CROSS_OVER_10:
	case RT5658_ALC_PGA_CTRL_1:
	case RT5658_ALC_PGA_CTRL_2:
	case RT5658_ALC_PGA_CTRL_3:
	case RT5658_ALC_PGA_CTRL_4:
	case RT5658_ALC_PGA_CTRL_5:
	case RT5658_ALC_PGA_CTRL_6:
	case RT5658_ALC_PGA_CTRL_7:
	case RT5658_ALC_PGA_CTRL_8:
	case RT5658_ALC_PGA_STA_1:
	case RT5658_ALC_PGA_STA_2:
	case RT5658_ALC_PGA_STA_3:
	case RT5658_DAC_L_EQ_PRE_VOL:
	case RT5658_DAC_R_EQ_PRE_VOL:
	case RT5658_DAC_L_EQ_POST_VOL:
	case RT5658_DAC_R_EQ_POST_VOL:
	case RT5658_DAC_L_EQ_LPF1_A1:
	case RT5658_DAC_L_EQ_LPF1_H0:
	case RT5658_DAC_R_EQ_LPF1_A1:
	case RT5658_DAC_R_EQ_LPF1_H0:
	case RT5658_DAC_L_EQ_BPF2_A1:
	case RT5658_DAC_L_EQ_BPF2_A2:
	case RT5658_DAC_L_EQ_BPF2_H0:
	case RT5658_DAC_R_EQ_BPF2_A1:
	case RT5658_DAC_R_EQ_BPF2_A2:
	case RT5658_DAC_R_EQ_BPF2_H0:
	case RT5658_DAC_L_EQ_BPF3_A1:
	case RT5658_DAC_L_EQ_BPF3_A2:
	case RT5658_DAC_L_EQ_BPF3_H0:
	case RT5658_DAC_R_EQ_BPF3_A1:
	case RT5658_DAC_R_EQ_BPF3_A2:
	case RT5658_DAC_R_EQ_BPF3_H0:
	case RT5658_DAC_L_EQ_BPF4_A1:
	case RT5658_DAC_L_EQ_BPF4_A2:
	case RT5658_DAC_L_EQ_BPF4_H0:
	case RT5658_DAC_R_EQ_BPF4_A1:
	case RT5658_DAC_R_EQ_BPF4_A2:
	case RT5658_DAC_R_EQ_BPF4_H0:
	case RT5658_DAC_L_EQ_HPF1_A1:
	case RT5658_DAC_L_EQ_HPF1_H0:
	case RT5658_DAC_R_EQ_HPF1_A1:
	case RT5658_DAC_R_EQ_HPF1_H0:
	case RT5658_DAC_L_EQ_HPF2_A1:
	case RT5658_DAC_L_EQ_HPF2_A2:
	case RT5658_DAC_L_EQ_HPF2_H0:
	case RT5658_DAC_R_EQ_HPF2_A1:
	case RT5658_DAC_R_EQ_HPF2_A2:
	case RT5658_DAC_R_EQ_HPF2_H0:
	case RT5658_DAC_L_BI_EQ_BPF1_H0_1:
	case RT5658_DAC_L_BI_EQ_BPF1_H0_2:
	case RT5658_DAC_L_BI_EQ_BPF1_B1_1:
	case RT5658_DAC_L_BI_EQ_BPF1_B1_2:
	case RT5658_DAC_L_BI_EQ_BPF1_B2_1:
	case RT5658_DAC_L_BI_EQ_BPF1_B2_2:
	case RT5658_DAC_L_BI_EQ_BPF1_A1_1:
	case RT5658_DAC_L_BI_EQ_BPF1_A1_2:
	case RT5658_DAC_L_BI_EQ_BPF1_A2_1:
	case RT5658_DAC_L_BI_EQ_BPF1_A2_2:
	case RT5658_DAC_R_BI_EQ_BPF1_H0_1:
	case RT5658_DAC_R_BI_EQ_BPF1_H0_2:
	case RT5658_DAC_R_BI_EQ_BPF1_B1_1:
	case RT5658_DAC_R_BI_EQ_BPF1_B1_2:
	case RT5658_DAC_R_BI_EQ_BPF1_B2_1:
	case RT5658_DAC_R_BI_EQ_BPF1_B2_2:
	case RT5658_DAC_R_BI_EQ_BPF1_A1_1:
	case RT5658_DAC_R_BI_EQ_BPF1_A1_2:
	case RT5658_DAC_R_BI_EQ_BPF1_A2_1:
	case RT5658_DAC_R_BI_EQ_BPF1_A2_2:
	case RT5658_ADC_L_EQ_LPF1_A1:
	case RT5658_ADC_R_EQ_LPF1_A1:
	case RT5658_ADC_L_EQ_LPF1_H0:
	case RT5658_ADC_R_EQ_LPF1_H0:
	case RT5658_ADC_L_EQ_BPF1_A1:
	case RT5658_ADC_R_EQ_BPF1_A1:
	case RT5658_ADC_L_EQ_BPF1_A2:
	case RT5658_ADC_R_EQ_BPF1_A2:
	case RT5658_ADC_L_EQ_BPF1_H0:
	case RT5658_ADC_R_EQ_BPF1_H0:
	case RT5658_ADC_L_EQ_BPF2_A1:
	case RT5658_ADC_R_EQ_BPF2_A1:
	case RT5658_ADC_L_EQ_BPF2_A2:
	case RT5658_ADC_R_EQ_BPF2_A2:
	case RT5658_ADC_L_EQ_BPF2_H0:
	case RT5658_ADC_R_EQ_BPF2_H0:
	case RT5658_ADC_L_EQ_BPF3_A1:
	case RT5658_ADC_R_EQ_BPF3_A1:
	case RT5658_ADC_L_EQ_BPF3_A2:
	case RT5658_ADC_R_EQ_BPF3_A2:
	case RT5658_ADC_L_EQ_BPF3_H0:
	case RT5658_ADC_R_EQ_BPF3_H0:
	case RT5658_ADC_L_EQ_BPF4_A1:
	case RT5658_ADC_R_EQ_BPF4_A1:
	case RT5658_ADC_L_EQ_BPF4_A2:
	case RT5658_ADC_R_EQ_BPF4_A2:
	case RT5658_ADC_L_EQ_BPF4_H0:
	case RT5658_ADC_R_EQ_BPF4_H0:
	case RT5658_ADC_L_EQ_HPF1_A1:
	case RT5658_ADC_R_EQ_HPF1_A1:
	case RT5658_ADC_L_EQ_HPF1_H0:
	case RT5658_ADC_R_EQ_HPF1_H0:
	case RT5658_ADC_L_EQ_PRE_VOL:
	case RT5658_ADC_R_EQ_PRE_VOL:
	case RT5658_ADC_L_EQ_POST_VOL:
	case RT5658_ADC_R_EQ_POST_VOL:
		return true;
	default:
		return false;
	}
}

static const DECLARE_TLV_DB_SCALE(hp_vol_tlv, -2325, 75, 0);
static const DECLARE_TLV_DB_SCALE(out_vol_tlv, -4650, 150, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -65625, 375, 0);
static const DECLARE_TLV_DB_SCALE(in_vol_tlv, -3450, 150, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -17625, 375, 0);
static const DECLARE_TLV_DB_SCALE(adc_bst_tlv, 0, 1200, 0);
static const DECLARE_TLV_DB_SCALE(in_bst_tlv, -1200, 75, 0);


/* Interface data select */
static const char * const rt5658_data_select[] = {
	"L/R", "R/L", "L/L", "R/R"
};

static const SOC_ENUM_SINGLE_DECL(rt5658_if2_dac_enum,
	RT5658_DIG_INF23_DATA, RT5658_IF2_DAC_SEL_SFT, rt5658_data_select);

static const SOC_ENUM_SINGLE_DECL(rt5658_if2_adc_enum,
	RT5658_DIG_INF23_DATA, RT5658_IF2_ADC_SEL_SFT, rt5658_data_select);

static const SOC_ENUM_SINGLE_DECL(rt5658_if3_dac_enum,
	RT5658_DIG_INF23_DATA, RT5658_IF3_DAC_SEL_SFT, rt5658_data_select);

static const SOC_ENUM_SINGLE_DECL(rt5658_if3_adc_enum,
	RT5658_DIG_INF23_DATA, RT5658_IF3_ADC_SEL_SFT, rt5658_data_select);

static const char * const rt5658_asrc_clk_src[] = {
	"clk_sysy_div_out", "clk_i2s1_track", "clk_i2s2_track",
	"clk_i2s3_track", "clk_sys2", "clk_sys3"
};

static int rt5658_asrc_clk_map_values[] = {
	0, 1, 2, 3, 5, 6,
};

static const SOC_VALUE_ENUM_SINGLE_DECL(
	rt5658_da_sto_asrc_enum, RT5658_ASRC_2, RT5658_DA_STO_T_SFT, 0x7,
	rt5658_asrc_clk_src, rt5658_asrc_clk_map_values);

static const SOC_VALUE_ENUM_SINGLE_DECL(
	rt5658_da_monol_asrc_enum, RT5658_ASRC_2, RT5658_DA_MONO_L_T_SFT, 0x7,
	rt5658_asrc_clk_src, rt5658_asrc_clk_map_values);

static const SOC_VALUE_ENUM_SINGLE_DECL(
	rt5658_da_monor_asrc_enum, RT5658_ASRC_2, RT5658_DA_MONO_R_T_SFT, 0x7,
	rt5658_asrc_clk_src, rt5658_asrc_clk_map_values);

static const SOC_VALUE_ENUM_SINGLE_DECL(
	rt5658_ad_sto1_asrc_enum, RT5658_ASRC_2, RT5658_AD_STO1_T_SFT, 0x7,
	rt5658_asrc_clk_src, rt5658_asrc_clk_map_values);

static const SOC_VALUE_ENUM_SINGLE_DECL(
	rt5658_ad_sto2_asrc_enum, RT5658_ASRC_3, RT5658_AD_STO2_T_SFT, 0x7,
	rt5658_asrc_clk_src, rt5658_asrc_clk_map_values);

static const SOC_VALUE_ENUM_SINGLE_DECL(
	rt5658_ad_monol_asrc_enum, RT5658_ASRC_3, RT5658_AD_MONO_L_T_SFT, 0x7,
	rt5658_asrc_clk_src, rt5658_asrc_clk_map_values);

static const SOC_VALUE_ENUM_SINGLE_DECL(
	rt5658_ad_monor_asrc_enum, RT5658_ASRC_3, RT5658_AD_MONO_R_T_SFT, 0x7,
	rt5658_asrc_clk_src, rt5658_asrc_clk_map_values);

static int rt5658_clk_sel_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int u_bit = 0, p_bit = 0;
	struct soc_enum *em =
		(struct soc_enum *)kcontrol->private_value;

	pr_debug("%s\n", __FUNCTION__);

	switch (em->reg) {
	case RT5658_ASRC_2:
		switch (em->shift_l) {
		case RT5658_AD_STO1_T_SFT:
			u_bit = RT5658_ADC_STO1_ASRC_MASK;
			p_bit = RT5658_PWR_ADC_S1F;
			break;
		case RT5658_DA_MONO_R_T_SFT:
			u_bit = RT5658_DAC_MONO_R_ASRC_MASK;
			p_bit = RT5658_PWR_DAC_MF_R;
			break;
		case RT5658_DA_MONO_L_T_SFT:
			u_bit = RT5658_DAC_MONO_L_ASRC_MASK;
			p_bit = RT5658_PWR_DAC_MF_L;
			break;
		case RT5658_DA_STO_T_SFT:
			u_bit = RT5658_DAC_STO_ASRC_MASK;
			p_bit = RT5658_PWR_DAC_S1F;
			break;
		}
		break;
	case RT5658_ASRC_3:
		switch (em->shift_l) {
		case RT5658_AD_MONO_R_T_SFT:
			u_bit = RT5658_ADC_MONO_R_ASRC_MASK;
			p_bit = RT5658_PWR_ADC_MF_R;
			break;
		case RT5658_AD_MONO_L_T_SFT:
			u_bit = RT5658_ADC_MONO_L_ASRC_MASK;
			p_bit = RT5658_PWR_ADC_MF_L;
			break;
		}
		break;
	}

	if (u_bit || p_bit) {
		switch (ucontrol->value.integer.value[0]) {
		case 1: /*enable*/
		case 2:
		case 3:
		case 4:
			if (snd_soc_read(codec, RT5658_PWR_DIG_2) & p_bit)
				snd_soc_update_bits(codec,
					RT5658_ASRC_1, u_bit, u_bit);
			break;
		default: /*disable*/
			snd_soc_update_bits(codec, RT5658_ASRC_1, u_bit, 0);
			break;
		}
	}

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

/**
 * rt5658_headset_detect - Detect headset.
 * @codec: SoC audio codec device.
 * @jack_insert: Jack insert or not.
 *
 * Detect whether is headset or not when jack inserted.
 *
 * Returns detect status.
 */

int rt5658_headset_detect(struct snd_soc_codec *codec, int jack_insert)
{
	int val, i = 0, sleep_time[5] = {30, 50, 100, 150, 300};

	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);

	if (jack_insert) {
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_1, RT5658_PWR_VREF2 |
			RT5658_PWR_MB, RT5658_PWR_VREF2 | RT5658_PWR_MB);
		msleep(20);
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_1, RT5658_PWR_FV2,
			RT5658_PWR_FV2);

		snd_soc_write(codec, RT5658_EJD_CTRL_1, 0xa880);
		snd_soc_write(codec, RT5658_EJD_CTRL_1, 0xa8a0);
		msleep(30);
		while(i < 5){
			msleep(sleep_time[i]);
			val = snd_soc_read(codec, RT5658_EJD_CTRL_2) & 0x0003;
			pr_debug("%s: %d MX-0C val=%d sleep %d\n", __func__, i,
				val, sleep_time[i]);
			i++;
			if (val == 0x1 || val == 0x2 || val == 0x3)
				break;
		}

		switch(val)
		{
		case 1:
			rt5658->jack_type = SND_JACK_HEADSET;
			snd_soc_update_bits(codec, RT5658_IL_CMD_2, 0x8000,
				0x8000);
			snd_soc_read(codec, RT5658_IL_CMD_1);
			/* for headset mic record*/
			snd_soc_write(codec, RT5658_EJD_CTRL_1,	0x60c0);
			break;
		case 2:
			rt5658->jack_type = SND_JACK_HEADPHONE;
			break;
		default:
			rt5658->jack_type = SND_JACK_HEADPHONE;
			break;
		}
	}
	else {
		rt5658->jack_type = 0;
		snd_soc_update_bits(codec, RT5658_IL_CMD_2, 0x8000, 0);
	}

	pr_debug("jack_type = %d\n", rt5658->jack_type);
	return rt5658->jack_type;
}
EXPORT_SYMBOL(rt5658_headset_detect);

// TODO: need check push button
int rt5658_button_detect(struct snd_soc_codec *codec)
{
	int btn_type, val;

	val = snd_soc_read(codec, RT5658_IL_CMD_1);
	btn_type = val & 0xff80;
	snd_soc_write(codec, RT5658_IL_CMD_1, val);
	if (btn_type != 0) {
		msleep(20);
		val = snd_soc_read(codec, RT5658_IL_CMD_1);
		snd_soc_write(codec, RT5658_IL_CMD_1, val);
	}
	return btn_type;
}

EXPORT_SYMBOL(rt5658_button_detect);

static const char *rt5658_push_btn_mode[] = {
	"Disable", "Read"
};

static const SOC_ENUM_SINGLE_DECL(rt5658_push_btn_enum, 0, 0,
	rt5658_push_btn_mode);

static int rt5658_push_btn_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int rt5658_push_btn_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	printk(KERN_INFO "ret=0x%x\n", rt5658_button_detect(codec));

	return 0;
}

static const char *rt5658_jack_type_mode[] = {
	"Disable", "Read"
};

static const SOC_ENUM_SINGLE_DECL(rt5658_jack_type_enum, 0, 0,
	rt5658_jack_type_mode);

static int rt5658_jack_type_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int rt5658_jack_type_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int jack_insert = ucontrol->value.integer.value[0];

	printk(KERN_INFO "ret=0x%x\n",
		rt5658_headset_detect(codec, jack_insert));

	return 0;
}

static const struct snd_kcontrol_new rt5658_snd_controls[] = {
	/* Speaker Output Volume */
	SOC_DOUBLE_TLV("Speaker Playback Volume", RT5658_SPO_VOL,
		RT5658_L_VOL_SFT, RT5658_R_VOL_SFT, 39, 1, out_vol_tlv),

	/* Headphone Output Volume */
	SOC_DOUBLE_R_TLV("HP Playback Volume", RT5658_HPL_GAIN, RT5658_HPR_GAIN,
		RT5658_G_HP_SFT, 31, 1, hp_vol_tlv),

	/* Mono Output Volume */
	SOC_SINGLE_TLV("Mono Playback Volume", RT5658_MONO_OUT,
		RT5658_L_VOL_SFT, 39, 1, out_vol_tlv),

	/* Output Volume */
	SOC_DOUBLE_TLV("OUT Playback Volume", RT5658_LOUT,
		RT5658_L_VOL_SFT, RT5658_R_VOL_SFT, 39, 1, out_vol_tlv),

	/* DAC Digital Volume */
	SOC_DOUBLE_TLV("DAC1 Playback Volume", RT5658_DAC1_DIG_VOL,
		RT5658_L_VOL_SFT, RT5658_R_VOL_SFT, 175, 0, dac_vol_tlv),
	SOC_DOUBLE("DAC1 Playback Switch", RT5658_AD_DA_MIXER,
		RT5658_M_DAC1_L_SFT, RT5658_M_DAC1_R_SFT, 1, 1),

	SOC_DOUBLE_TLV("DAC2 Playback Volume", RT5658_DAC2_DIG_VOL,
		RT5658_L_VOL_SFT, RT5658_R_VOL_SFT, 175, 0, dac_vol_tlv),
	SOC_DOUBLE("DAC2 Playback Switch", RT5658_DAC_CTRL,
		RT5658_M_DAC2_L_VOL_SFT, RT5658_M_DAC2_R_VOL_SFT, 1, 1),

	/* IN1/IN2/IN3/IN4 Volume */
	SOC_SINGLE_TLV("IN1 Boost Volume", RT5658_IN1_IN2,
		RT5658_BST1_SFT, 69, 0, in_bst_tlv),
	SOC_SINGLE_TLV("IN2 Boost Volume", RT5658_IN1_IN2,
		RT5658_BST2_SFT, 69, 0, in_bst_tlv),
	SOC_SINGLE_TLV("IN3 Boost Volume", RT5658_IN3_IN4,
		RT5658_BST3_SFT, 69, 0, in_bst_tlv),
	SOC_SINGLE_TLV("IN4 Boost Volume", RT5658_IN3_IN4,
		RT5658_BST4_SFT, 69, 0, in_bst_tlv),

	/* INL/INR Volume Control */
	SOC_DOUBLE_TLV("IN Capture Volume", RT5658_INL1_INR1_VOL,
		RT5658_INL_VOL_SFT, RT5658_INR_VOL_SFT, 31, 1, in_vol_tlv),

	/* ADC Digital Volume Control */
	SOC_DOUBLE("STO1 ADC Capture Switch", RT5658_STO1_ADC_DIG_VOL,
		RT5658_L_MUTE_SFT, RT5658_R_MUTE_SFT, 1, 1),
	SOC_DOUBLE_TLV("STO1 ADC Capture Volume", RT5658_STO1_ADC_DIG_VOL,
		RT5658_L_VOL_SFT, RT5658_R_VOL_SFT, 127, 0, adc_vol_tlv),
	SOC_DOUBLE("Mono ADC Capture Switch", RT5658_MONO_ADC_DIG_VOL,
		RT5658_L_MUTE_SFT, RT5658_R_MUTE_SFT, 1, 1),
	SOC_DOUBLE_TLV("Mono ADC Capture Volume", RT5658_MONO_ADC_DIG_VOL,
		RT5658_L_VOL_SFT, RT5658_R_VOL_SFT, 127, 0, adc_vol_tlv),
	SOC_DOUBLE("STO2 ADC Capture Switch", RT5658_STO2_ADC_DIG_VOL,
		RT5658_L_MUTE_SFT, RT5658_R_MUTE_SFT, 1, 1),
	SOC_DOUBLE_TLV("STO2 ADC Capture Volume", RT5658_STO2_ADC_DIG_VOL,
		RT5658_L_VOL_SFT, RT5658_R_VOL_SFT, 127, 0, adc_vol_tlv),

	/* ADC Boost Volume Control */
	SOC_DOUBLE_TLV("STO1 ADC Boost Gain Volume", RT5658_STO1_BOOST,
		RT5658_STO1_ADC_L_BST_SFT, RT5658_STO1_ADC_R_BST_SFT,
		3, 0, adc_bst_tlv),

	SOC_DOUBLE_TLV("Mono ADC Boost Gain Volume", RT5658_MONO_BOOST,
		RT5658_MONO_ADC_L_BST_SFT, RT5658_MONO_ADC_R_BST_SFT,
		3, 0, adc_bst_tlv),

	SOC_DOUBLE_TLV("STO2 ADC Boost Gain Volume", RT5658_STO2_BOOST,
		RT5658_STO2_ADC_L_BST_SFT, RT5658_STO2_ADC_R_BST_SFT,
		3, 0, adc_bst_tlv),

	SOC_ENUM("ADC IF2 Data Switch", rt5658_if2_adc_enum),
	SOC_ENUM("DAC IF2 Data Switch", rt5658_if2_dac_enum),
	SOC_ENUM("ADC IF3 Data Switch", rt5658_if3_adc_enum),
	SOC_ENUM("DAC IF3 Data Switch", rt5658_if3_dac_enum),

	SOC_ENUM_EXT("DA STO Clk Sel", rt5658_da_sto_asrc_enum,
		snd_soc_get_enum_double, rt5658_clk_sel_put),
	SOC_ENUM_EXT("DA MONOL Clk Sel", rt5658_da_monol_asrc_enum,
		snd_soc_get_enum_double, rt5658_clk_sel_put),
	SOC_ENUM_EXT("DA MONOR Clk Sel", rt5658_da_monor_asrc_enum,
		snd_soc_get_enum_double, rt5658_clk_sel_put),
	SOC_ENUM_EXT("AD STO1 Clk Sel", rt5658_ad_sto1_asrc_enum,
		snd_soc_get_enum_double, rt5658_clk_sel_put),
	SOC_ENUM_EXT("AD STO2 Clk Sel", rt5658_ad_sto2_asrc_enum,
		     snd_soc_get_enum_double, rt5658_clk_sel_put),
	SOC_ENUM_EXT("AD MONOL Clk Sel", rt5658_ad_monol_asrc_enum,
		snd_soc_get_enum_double, rt5658_clk_sel_put),
	SOC_ENUM_EXT("AD MONOR Clk Sel", rt5658_ad_monor_asrc_enum,
		snd_soc_get_enum_double, rt5658_clk_sel_put),
	SOC_ENUM_EXT("push button", rt5658_push_btn_enum,
		rt5658_push_btn_get, rt5658_push_btn_put),
	SOC_ENUM_EXT("jack type", rt5658_jack_type_enum,
		rt5658_jack_type_get, rt5658_jack_type_put),
};

/**
 * set_dmic_clk - Set parameter of dmic.
 *
 * @w: DAPM widget.
 * @kcontrol: The kcontrol of this widget.
 * @event: Event id.
 *
 * Choose dmic clock between 1MHz and 3MHz.
 * It is better for clock to approximate 3MHz.
 */
static int set_dmic_clk(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);
	int div[] = { 2, 3, 4, 6, 8, 12 }, idx =
	    -EINVAL, i, rate, red, bound, temp;

	pr_debug("%s\n", __FUNCTION__);

	rate = rt5658->lrck[RT5658_AIF1] << 8;
	red = 3000000 * 12;
	for (i = 0; i < ARRAY_SIZE(div); i++) {
		bound = div[i] * 3000000;
		if (rate > bound)
			continue;
		temp = bound - rate;
		if (temp < red) {
			red = temp;
			idx = i;
		}
	}
	if (idx < 0)
		dev_err(codec->dev, "Failed to set DMIC clock\n");
	else {
		snd_soc_update_bits(codec, RT5658_DMIC_CTRL_1,
			RT5658_DMIC_CLK_MASK, idx << RT5658_DMIC_CLK_SFT);
	}
	return idx;
}

static int set_adc_clk(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	pr_debug("%s\n", __FUNCTION__);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_update_bits(codec, RT5658_CHOP_ADC,
			RT5658_CKXEN_ADCC_MASK | RT5658_CKGEN_ADCC_MASK,
			RT5658_CKXEN_ADCC_MASK | RT5658_CKGEN_ADCC_MASK);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_update_bits(codec, RT5658_CHOP_ADC,
			RT5658_CKXEN_ADCC_MASK | RT5658_CKGEN_ADCC_MASK, 0);
		break;

	default:
		return 0;
	}

	return 0;

}

static int rt5658_charge_pump_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	pr_debug("%s\n", __FUNCTION__);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Depop */
		snd_soc_write(codec, RT5658_DEPOP_1, 0x0009);
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_write(codec, RT5658_DEPOP_1, 0x0004);
		break;
	default:
		return 0;
	}

	return 0;
}

static int is_sys_clk_from_pll(struct snd_soc_dapm_widget *w,
			 struct snd_soc_dapm_widget *sink)
{
	unsigned int val;
	struct snd_soc_codec *codec = w->codec;

	pr_debug("%s\n", __FUNCTION__);

	val = snd_soc_read(codec, RT5658_GLB_CLK);
	val &= RT5658_SCLK_SRC_MASK;
	if (val == RT5658_SCLK_SRC_PLL1)
		return 1;
	else
		return 0;
}

static int is_using_asrc(struct snd_soc_dapm_widget *w,
			 struct snd_soc_dapm_widget *sink)
{
	unsigned int reg, shift, val;
	struct snd_soc_codec *codec = w->codec;

	pr_debug("%s\n", __FUNCTION__);

	switch (w->shift) {
	case RT5658_ADC_MONO_R_ASRC_SFT:
		reg = RT5658_ASRC_3;
		shift = RT5658_AD_MONO_R_T_SFT;
		break;
	case RT5658_ADC_MONO_L_ASRC_SFT:
		reg = RT5658_ASRC_3;
		shift = RT5658_AD_MONO_L_T_SFT;
		break;
	case RT5658_ADC_STO1_ASRC_SFT:
		reg = RT5658_ASRC_2;
		shift = RT5658_AD_STO1_T_SFT;
		break;
	case RT5658_DAC_MONO_R_ASRC_SFT:
		reg = RT5658_ASRC_2;
		shift = RT5658_DA_MONO_R_T_SFT;
		break;
	case RT5658_DAC_MONO_L_ASRC_SFT:
		reg = RT5658_ASRC_2;
		shift = RT5658_DA_MONO_L_T_SFT;
		break;
	case RT5658_DAC_STO_ASRC_SFT:
		reg = RT5658_ASRC_2;
		shift = RT5658_DA_STO_T_SFT;
		break;
	default:
		return 0;
	}

	val = (snd_soc_read(codec, reg) >> shift) & 0xf;
	switch (val) {
	case 1:
	case 2:
	case 3:
		/* I2S_Pre_Div1 should be 1 in asrc mode */
		snd_soc_update_bits(codec, RT5658_ADDA_CLK_1,
			RT5658_I2S_PD1_MASK, RT5658_I2S_PD1_2);
		return 1;
	default:
		return 0;
	}

}

/* Digital Mixer */
static const struct snd_kcontrol_new rt5658_sto1_adc_l_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5658_STO1_ADC_MIXER,
			RT5658_M_STO1_ADC_L1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5658_STO1_ADC_MIXER,
			RT5658_M_STO1_ADC_L2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_sto1_adc_r_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5658_STO1_ADC_MIXER,
			RT5658_M_STO1_ADC_R1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5658_STO1_ADC_MIXER,
			RT5658_M_STO1_ADC_R2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_mono_adc_l_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5658_MONO_ADC_MIXER,
			RT5658_M_MONO_ADC_L1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5658_MONO_ADC_MIXER,
			RT5658_M_MONO_ADC_L2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_mono_adc_r_mix[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", RT5658_MONO_ADC_MIXER,
			RT5658_M_MONO_ADC_R1_SFT, 1, 1),
	SOC_DAPM_SINGLE("ADC2 Switch", RT5658_MONO_ADC_MIXER,
			RT5658_M_MONO_ADC_R2_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_dac_l_mix[] = {
	SOC_DAPM_SINGLE("Stereo ADC Switch", RT5658_AD_DA_MIXER,
			RT5658_M_ADCMIX_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 Switch", RT5658_AD_DA_MIXER,
			RT5658_M_DAC1_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_dac_r_mix[] = {
	SOC_DAPM_SINGLE("Stereo ADC Switch", RT5658_AD_DA_MIXER,
			RT5658_M_ADCMIX_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC1 Switch", RT5658_AD_DA_MIXER,
			RT5658_M_DAC1_R_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_sto_dac_l_mix[] = {
	SOC_DAPM_SINGLE("DAC L1 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_L1_STO_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R1 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_R1_STO_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_L2_STO_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_R2_STO_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_sto_dac_r_mix[] = {
	SOC_DAPM_SINGLE("DAC L1 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_L1_STO_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R1 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_R1_STO_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_L2_STO_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_STO_DAC_MIXER,
			RT5658_M_DAC_R2_STO_R_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_mono_dac_l_mix[] = {
	SOC_DAPM_SINGLE("DAC L1 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_L1_MONO_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R1 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_R1_MONO_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_L2_MONO_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_R2_MONO_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_mono_dac_r_mix[] = {
	SOC_DAPM_SINGLE("DAC L1 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_L1_MONO_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R1 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_R1_MONO_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_L2_MONO_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_MONO_DAC_MIXER,
			RT5658_M_DAC_R2_MONO_R_SFT, 1, 1),
};

/* Analog Input Mixer */
static const struct snd_kcontrol_new rt5658_rec1_l_mix[] = {
	SOC_DAPM_SINGLE("SPKVOLL Switch", RT5658_REC1_L2_MIXER,
			RT5658_M_SPKVOLL_RM1_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("INL Switch", RT5658_REC1_L2_MIXER,
			RT5658_M_INL_RM1_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST4 Switch", RT5658_REC1_L2_MIXER,
			RT5658_M_BST4_RM1_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_REC1_L2_MIXER,
			RT5658_M_BST3_RM1_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST2 Switch", RT5658_REC1_L2_MIXER,
			RT5658_M_BST2_RM1_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST1 Switch", RT5658_REC1_L2_MIXER,
			RT5658_M_BST1_RM1_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_rec1_r_mix[] = {
	SOC_DAPM_SINGLE("HPOVOLR Switch", RT5658_REC1_L2_MIXER,
			RT5658_M_HPOVOLR_RM1_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("INR Switch", RT5658_REC1_R2_MIXER,
			RT5658_M_INR_RM1_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST4 Switch", RT5658_REC1_R2_MIXER,
			RT5658_M_BST4_RM1_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_REC1_R2_MIXER,
			RT5658_M_BST3_RM1_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST2 Switch", RT5658_REC1_R2_MIXER,
			RT5658_M_BST2_RM1_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST1 Switch", RT5658_REC1_R2_MIXER,
			RT5658_M_BST1_RM1_R_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_rec2_l_mix[] = {
	SOC_DAPM_SINGLE("SPKVOLL Switch", RT5658_REC2_L2_MIXER,
			RT5658_M_SPKVOL_RM2_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("OUTVOLL Switch", RT5658_REC2_L2_MIXER,
			RT5658_M_OUTVOLL_RM2_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST4 Switch", RT5658_REC2_L2_MIXER,
			RT5658_M_BST4_RM2_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_REC2_L2_MIXER,
			RT5658_M_BST3_RM2_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST2 Switch", RT5658_REC2_L2_MIXER,
			RT5658_M_BST2_RM2_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST1 Switch", RT5658_REC2_L2_MIXER,
			RT5658_M_BST1_RM2_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_rec2_r_mix[] = {
	SOC_DAPM_SINGLE("MONOVOL Switch", RT5658_REC2_R2_MIXER,
			RT5658_M_MONOVOL_RM2_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("OUTVOLR Switch", RT5658_REC2_R2_MIXER,
			RT5658_M_OUTVOLR_RM2_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST4 Switch", RT5658_REC2_R2_MIXER,
			RT5658_M_BST4_RM2_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_REC2_R2_MIXER,
			RT5658_M_BST3_RM2_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST2 Switch", RT5658_REC2_R2_MIXER,
			RT5658_M_BST2_RM2_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST1 Switch", RT5658_REC2_R2_MIXER,
			RT5658_M_BST1_RM2_R_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_spk_l_mix[] = {
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_SPK_L_MIXER,
			RT5658_M_DAC_L2_SM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST1 Switch", RT5658_SPK_L_MIXER,
			RT5658_M_BST1_SM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("INL Switch", RT5658_SPK_L_MIXER,
			RT5658_M_IN_L_SM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("INR Switch", RT5658_SPK_L_MIXER,
			RT5658_M_IN_R_SM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_SPK_L_MIXER,
			RT5658_M_BST3_SM_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_spk_r_mix[] = {
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_SPK_R_MIXER,
			RT5658_M_DAC_R2_SM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST4 Switch", RT5658_SPK_R_MIXER,
			RT5658_M_BST4_SM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("INL Switch", RT5658_SPK_R_MIXER,
			RT5658_M_IN_L_SM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("INR Switch", RT5658_SPK_R_MIXER,
			RT5658_M_IN_R_SM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_SPK_R_MIXER,
			RT5658_M_BST3_SM_R_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_monovol_mix[] = {
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_MONOMIX_IN_GAIN,
			RT5658_M_DAC_L2_MM_SFT, 1, 1),
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_MONOMIX_IN_GAIN,
			RT5658_M_DAC_R2_MM_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST1 Switch", RT5658_MONOMIX_IN_GAIN,
			RT5658_M_BST1_MM_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST2 Switch", RT5658_MONOMIX_IN_GAIN,
			RT5658_M_BST2_MM_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_MONOMIX_IN_GAIN,
			RT5658_M_BST3_MM_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_out_l_mix[] = {
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_OUT_L_MIXER,
			RT5658_M_DAC_L2_OM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("INL Switch", RT5658_OUT_L_MIXER,
			RT5658_M_IN_L_OM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST1 Switch", RT5658_OUT_L_MIXER,
			RT5658_M_BST1_OM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST2 Switch", RT5658_OUT_L_MIXER,
			RT5658_M_BST2_OM_L_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_OUT_L_MIXER,
			RT5658_M_BST3_OM_L_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_out_r_mix[] = {
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_OUT_R_MIXER,
			RT5658_M_DAC_R2_OM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("INR Switch", RT5658_OUT_R_MIXER,
			RT5658_M_IN_R_OM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST2 Switch", RT5658_OUT_R_MIXER,
			RT5658_M_BST2_OM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST3 Switch", RT5658_OUT_R_MIXER,
			RT5658_M_BST3_OM_R_SFT, 1, 1),
	SOC_DAPM_SINGLE("BST4 Switch", RT5658_OUT_R_MIXER,
			RT5658_M_BST4_OM_R_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_spo_l_mix[] = {
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_SPO_AMP_GAIN,
			RT5658_M_DAC_L2_SPKOMIX_SFT, 1, 0),
	SOC_DAPM_SINGLE("SPKVOL L Switch", RT5658_SPO_AMP_GAIN,
			RT5658_M_SPKVOLL_SPKOMIX_SFT, 1, 0),
};

static const struct snd_kcontrol_new rt5658_spo_r_mix[] = {
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_SPO_AMP_GAIN,
			RT5658_M_DAC_R2_SPKOMIX_SFT, 1, 0),
	SOC_DAPM_SINGLE("SPKVOL R Switch", RT5658_SPO_AMP_GAIN,
			RT5658_M_SPKVOLR_SPKOMIX_SFT, 1, 0),
};

static const struct snd_kcontrol_new rt5658_mono_mix[] = {
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_MONOMIX_IN_GAIN,
			RT5658_M_DAC_L2_MA_SFT, 1, 1),
	SOC_DAPM_SINGLE("MONOVOL Switch", RT5658_MONOMIX_IN_GAIN,
			RT5658_M_MONOVOL_MA_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_lout_l_mix[] = {
	SOC_DAPM_SINGLE("DAC L2 Switch", RT5658_LOUT_MIXER,
			RT5658_M_DAC_L2_LM_SFT, 1, 1),
	SOC_DAPM_SINGLE("OUTVOL L Switch", RT5658_LOUT_MIXER,
			RT5658_M_OV_L_LM_SFT, 1, 1),
};

static const struct snd_kcontrol_new rt5658_lout_r_mix[] = {
	SOC_DAPM_SINGLE("DAC R2 Switch", RT5658_LOUT_MIXER,
			RT5658_M_DAC_R2_LM_SFT, 1, 1),
	SOC_DAPM_SINGLE("OUTVOL R Switch", RT5658_LOUT_MIXER,
			RT5658_M_OV_R_LM_SFT, 1, 1),
};

/*DAC L2, DAC R2*/
/*MX-1B [6:4], MX-1B [2:0]*/
static const char * const rt5658_dac2_src[] = {
	"IF1 DAC2", "IF2 DAC", "IF3 DAC", "Mono ADC MIX"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_dac_l2_enum, RT5658_DAC_CTRL,
	RT5658_DAC_L2_SEL_SFT, rt5658_dac2_src);

static const struct snd_kcontrol_new rt5658_dac_l2_mux =
	SOC_DAPM_ENUM("DAC L2 Source", rt5658_dac_l2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5658_dac_r2_enum, RT5658_DAC_CTRL,
	RT5658_DAC_R2_SEL_SFT, rt5658_dac2_src);

static const struct snd_kcontrol_new rt5658_dac_r2_mux =
	SOC_DAPM_ENUM("DAC R2 Source", rt5658_dac_r2_enum);


/* STO1 ADC1 Source */
/* MX-26 [13] */
static const char * const rt5658_sto1_adc1_src[] = {
	"DAC MIX", "ADC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_sto1_adc1_enum, RT5658_STO1_ADC_MIXER,
	RT5658_STO1_ADC1_SRC_SFT, rt5658_sto1_adc1_src);

static const struct snd_kcontrol_new rt5658_sto1_adc1_mux =
	SOC_DAPM_ENUM("Stereo1 ADC1 Source", rt5658_sto1_adc1_enum);

/* STO1 ADC Source */
/* MX-26 [12] */
static const char * const rt5658_sto1_adc_src[] = {
	"ADC1", "ADC2"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_sto1_adc_enum, RT5658_STO1_ADC_MIXER,
	RT5658_STO1_ADC_SRC_SFT, rt5658_sto1_adc_src);

static const struct snd_kcontrol_new rt5658_sto1_adc_mux =
	SOC_DAPM_ENUM("Stereo1 ADC Source", rt5658_sto1_adc_enum);

/* STO1 ADC2 Source */
/* MX-26 [11] */
static const char * const rt5658_sto1_adc2_src[] = {
	"DAC MIX", "DMIC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_sto1_adc2_enum, RT5658_STO1_ADC_MIXER,
	RT5658_STO1_ADC2_SRC_SFT, rt5658_sto1_adc2_src);

static const struct snd_kcontrol_new rt5658_sto1_adc2_mux =
	SOC_DAPM_ENUM("Stereo1 ADC2 Source", rt5658_sto1_adc2_enum);

/* STO1 DMIC Source */
/* MX-26 [8] */
static const char * const rt5658_sto1_dmic_src[] = {
	"DMIC1", "DMIC2"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_sto1_dmic_enum, RT5658_STO1_ADC_MIXER,
	RT5658_STO1_DMIC_SRC_SFT, rt5658_sto1_dmic_src);

static const struct snd_kcontrol_new rt5658_sto1_dmic_mux =
	SOC_DAPM_ENUM("Stereo1 DMIC Source", rt5658_sto1_dmic_enum);


/* MONO ADC L2 Source */
/* MX-27 [12] */
static const char * const rt5658_mono_adc_l2_src[] = {
	"Mono DAC MIXL", "DMIC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_adc_l2_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_ADC_L2_SRC_SFT, rt5658_mono_adc_l2_src);

static const struct snd_kcontrol_new rt5658_mono_adc_l2_mux =
	SOC_DAPM_ENUM("Mono ADC L2 Source", rt5658_mono_adc_l2_enum);


/* MONO ADC L1 Source */
/* MX-27 [11] */
static const char * const rt5658_mono_adc_l1_src[] = {
	"Mono DAC MIXL", "ADC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_adc_l1_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_ADC_L1_SRC_SFT, rt5658_mono_adc_l1_src);

static const struct snd_kcontrol_new rt5658_mono_adc_l1_mux =
	SOC_DAPM_ENUM("Mono ADC L1 Source", rt5658_mono_adc_l1_enum);

/* MONO ADC L Source, MONO ADC R Source*/
/* MX-27 [10:9], MX-27 [2:1] */
static const char * const rt5658_mono_adc_src[] = {
	"ADC1 L", "ADC1 R", "ADC2 L", "ADC2 R"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_adc_l_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_ADC_L_SRC_SFT, rt5658_mono_adc_src);

static const struct snd_kcontrol_new rt5658_mono_adc_l_mux =
	SOC_DAPM_ENUM("Mono ADC L Source", rt5658_mono_adc_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_adcr_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_ADC_R_SRC_SFT, rt5658_mono_adc_src);

static const struct snd_kcontrol_new rt5658_mono_adc_r_mux =
	SOC_DAPM_ENUM("Mono ADC R Source", rt5658_mono_adcr_enum);

/* MONO DMIC L Source */
/* MX-27 [8] */
static const char * const rt5658_mono_dmic_l_src[] = {
	"DMIC1 L", "DMIC2 L"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_dmic_l_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_DMIC_L_SRC_SFT, rt5658_mono_dmic_l_src);

static const struct snd_kcontrol_new rt5658_mono_dmic_l_mux =
	SOC_DAPM_ENUM("Mono DMIC L Source", rt5658_mono_dmic_l_enum);

/* MONO ADC R2 Source */
/* MX-27 [4] */
static const char * const rt5658_mono_adc_r2_src[] = {
	"Mono DAC MIXR", "DMIC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_adc_r2_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_ADC_R2_SRC_SFT, rt5658_mono_adc_r2_src);

static const struct snd_kcontrol_new rt5658_mono_adc_r2_mux =
	SOC_DAPM_ENUM("Mono ADC R2 Source", rt5658_mono_adc_r2_enum);

/* MONO ADC R1 Source */
/* MX-27 [3] */
static const char * const rt5658_mono_adc_r1_src[] = {
	"Mono DAC MIXR", "ADC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_adc_r1_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_ADC_R1_SRC_SFT, rt5658_mono_adc_r1_src);

static const struct snd_kcontrol_new rt5658_mono_adc_r1_mux =
	SOC_DAPM_ENUM("Mono ADC R1 Source", rt5658_mono_adc_r1_enum);

/* MONO DMIC R Source */
/* MX-27 [0] */
static const char * const rt5658_mono_dmic_r_src[] = {
	"DMIC1 R", "DMIC2 R"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_mono_dmic_r_enum, RT5658_MONO_ADC_MIXER,
	RT5658_MONO_DMIC_R_SRC_SFT, rt5658_mono_dmic_r_src);

static const struct snd_kcontrol_new rt5658_mono_dmic_r_mux =
	SOC_DAPM_ENUM("Mono DMIC R Source", rt5658_mono_dmic_r_enum);


/* DAC R1 Source, DAC L1 Source*/
/* MX-29 [11:10], MX-29 [9:8]*/
static const char * const rt5658_dac1_src[] = {
	"IF1 DAC1", "IF2 DAC", "IF3 DAC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_dac_r1_enum, RT5658_AD_DA_MIXER,
	RT5658_DAC1_R_SEL_SFT, rt5658_dac1_src);

static const struct snd_kcontrol_new rt5658_dac_r1_mux =
	SOC_DAPM_ENUM("DAC R1 Source", rt5658_dac_r1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5658_dac_l1_enum, RT5658_AD_DA_MIXER,
	RT5658_DAC1_L_SEL_SFT, rt5658_dac1_src);

static const struct snd_kcontrol_new rt5658_dac_l1_mux =
	SOC_DAPM_ENUM("DAC L1 Source", rt5658_dac_l1_enum);

/* DAC Digital Mixer L Source, DAC Digital Mixer R Source*/
/* MX-2C [6], MX-2C [4]*/
static const char * const rt5658_dig_dac_mix_src[] = {
	"Stereo DAC Mixer", "Mono DAC Mixer"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_dig_dac_mixl_enum, RT5658_DIG_MIXER,
	RT5658_DAC_MIX_L_SFT, rt5658_dig_dac_mix_src);

static const struct snd_kcontrol_new rt5658_dig_dac_mixl_mux =
	SOC_DAPM_ENUM("DAC Digital Mixer L Source", rt5658_dig_dac_mixl_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5658_dig_dac_mixr_enum, RT5658_DIG_MIXER,
	RT5658_DAC_MIX_R_SFT, rt5658_dig_dac_mix_src);

static const struct snd_kcontrol_new rt5658_dig_dac_mixr_mux =
	SOC_DAPM_ENUM("DAC Digital Mixer R Source", rt5658_dig_dac_mixr_enum);

/* Analog DAC L1 Source, Analog DAC R1 Source*/
/* MX-2D [3], MX-2D [2]*/
static const char * const rt5658_alg_dac1_src[] = {
	"DAC", "Stereo DAC Mixer"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_alg_dac_l1_enum, RT5658_A_DAC_MUX,
	RT5658_A_DACL1_SFT, rt5658_alg_dac1_src);

static const struct snd_kcontrol_new rt5658_alg_dac_l1_mux =
	SOC_DAPM_ENUM("Analog DACL1 Source", rt5658_alg_dac_l1_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5658_alg_dac_r1_enum, RT5658_A_DAC_MUX,
	RT5658_A_DACR1_SFT, rt5658_alg_dac1_src);

static const struct snd_kcontrol_new rt5658_alg_dac_r1_mux =
	SOC_DAPM_ENUM("Analog DACR1 Source", rt5658_alg_dac_r1_enum);

/* Analog DAC LR Source, Analog DAC R2 Source*/
/* MX-2D [1], MX-2D [0]*/
static const char * const rt5658_alg_dac2_src[] = {
	"Stereo DAC Mixer", "Mono DAC Mixer"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_alg_dac_l2_enum, RT5658_A_DAC_MUX,
	RT5658_A_DACL2_SFT, rt5658_alg_dac2_src);

static const struct snd_kcontrol_new rt5658_alg_dac_l2_mux =
	SOC_DAPM_ENUM("Analog DAC L2 Source", rt5658_alg_dac_l2_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5658_alg_dac_r2_enum, RT5658_A_DAC_MUX,
	RT5658_A_DACR2_SFT, rt5658_alg_dac2_src);

static const struct snd_kcontrol_new rt5658_alg_dac_r2_mux =
	SOC_DAPM_ENUM("Analog DAC R2 Source", rt5658_alg_dac_r2_enum);

/* Interface2 ADC Data Input*/
/* MX-2F [13:12] */
static const char * const rt5658_if2_adc_in_src[] = {
	"IF_ADC1", "IF_ADC2", "DAC_REF", "IF_ADC3"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_if2_adc_in_enum, RT5658_DIG_INF23_DATA,
	RT5658_IF2_ADC_IN_SFT, rt5658_if2_adc_in_src);

static const struct snd_kcontrol_new rt5658_if2_adc_in_mux =
	SOC_DAPM_ENUM("IF2 ADC IN Source", rt5658_if2_adc_in_enum);

/* Interface3 ADC Data Input*/
/* MX-2F [1:0] */
static const char * const rt5658_if3_adc_in_src[] = {
	"IF_ADC1", "IF_ADC2", "DAC_REF", "Stereo2_ADC_L/R"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_if3_adc_in_enum, RT5658_DIG_INF23_DATA,
	RT5658_IF3_ADC_IN_SFT, rt5658_if3_adc_in_src);

static const struct snd_kcontrol_new rt5658_if3_adc_in_mux =
	SOC_DAPM_ENUM("IF3 ADC IN Source", rt5658_if3_adc_in_enum);

/* PDM 1 L/R*/
/* MX-31 [15] [13] */
static const char * const rt5658_pdm_src[] = {
	"Mono DAC", "Stereo DAC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_pdm_l_enum, RT5658_PDM_OUT_CTRL,
	RT5658_PDM1_L_SFT, rt5658_pdm_src);

static const struct snd_kcontrol_new rt5658_pdm_l_mux =
	SOC_DAPM_ENUM("PDM L Source", rt5658_pdm_l_enum);

static const SOC_ENUM_SINGLE_DECL(
	rt5658_pdm_r_enum, RT5658_PDM_OUT_CTRL,
	RT5658_PDM1_R_SFT, rt5658_pdm_src);

static const struct snd_kcontrol_new rt5658_pdm_r_mux =
	SOC_DAPM_ENUM("PDM R Source", rt5658_pdm_r_enum);

/* SPDIF Output source*/
/* MX-36 [1:0] */
static const char * const rt5658_spdif_src[] = {
	"IF1_DAC1", "IF1_DAC2", "IF2_DAC", "IF3_DAC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_spdif_enum, RT5658_SPDIF_CTRL,
	RT5658_SPDIF_SEL_SFT, rt5658_spdif_src);

static const struct snd_kcontrol_new rt5658_spdif_mux =
	SOC_DAPM_ENUM("SPDIF Source", rt5658_spdif_enum);

/* I2S1 TDM ADCDAT Source */
/* MX-78[4:0] */
static const char * const rt5658_rx_adc_data_src[] = {
	"AD1:AD2:DAC:NUL", "AD1:AD2:NUL:DAC", "AD1:DAC:AD2:NUL",
	"AD1:DAC:NUL:AD2", "AD1:NUL:DAC:AD2", "AD1:NUL:AD2:DAC",
	"AD2:AD1:DAC:NUL", "AD2:AD1:NUL:DAC", "AD2:DAC:AD1:NUL",
	"AD2:DAC:NUL:AD1", "AD2:NUL:DAC:AD1", "AD1:NUL:AD1:DAC",
	"DAC:AD1:AD2:NUL", "DAC:AD1:NUL:AD2", "DAC:AD2:AD1:NUL",
	"DAC:AD2:NUL:AD1", "DAC:NUL:DAC:AD2", "DAC:NUL:AD2:DAC",
	"NUL:AD1:AD2:DAC", "NUL:AD1:DAC:AD2", "NUL:AD2:AD1:DAC",
	"NUL:AD2:DAC:AD1", "NUL:DAC:DAC:AD2", "NUL:DAC:AD2:DAC"
};

static const SOC_ENUM_SINGLE_DECL(
	rt5658_rx_adc_data_enum, RT5658_TDM_CTRL_2,
	RT5658_ADCDAT_SRC_SFT, rt5658_rx_adc_data_src);

static const struct snd_kcontrol_new rt5658_rx_adc_dac_mux =
	SOC_DAPM_ENUM("TDM ADCDAT Source", rt5658_rx_adc_data_enum);

/* Out Volume Switch */
static const struct snd_kcontrol_new spkvol_l_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_SPO_VOL, RT5658_VOL_L_SFT, 1, 1);

static const struct snd_kcontrol_new spkvol_r_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_SPO_VOL, RT5658_VOL_R_SFT, 1, 1);

static const struct snd_kcontrol_new monovol_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_MONO_OUT, RT5658_VOL_L_SFT, 1, 1);

static const struct snd_kcontrol_new outvol_l_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_LOUT, RT5658_VOL_L_SFT, 1, 1);

static const struct snd_kcontrol_new outvol_r_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_LOUT, RT5658_VOL_R_SFT, 1, 1);

/* Out Switch */
static const struct snd_kcontrol_new spo_l_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_CLASSD_2, RT5658_M_RF_DIG_SFT, 1, 1);

static const struct snd_kcontrol_new spo_r_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_CLASSD_2, RT5658_M_RF_DIG_SFT, 1, 1);

static const struct snd_kcontrol_new mono_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_MONO_OUT, RT5658_L_MUTE_SFT, 1, 1);

static const struct snd_kcontrol_new hpo_l_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_HP_VOL, RT5658_L_MUTE_SFT, 1, 1);

static const struct snd_kcontrol_new hpo_r_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_HP_VOL, RT5658_R_MUTE_SFT, 1, 1);

static const struct snd_kcontrol_new lout_l_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_LOUT, RT5658_L_MUTE_SFT, 1, 1);

static const struct snd_kcontrol_new lout_r_switch =
	SOC_DAPM_SINGLE("Switch", RT5658_LOUT, RT5658_R_MUTE_SFT, 1, 1);

static int rt5658_spk_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	pr_debug("%s\n", __FUNCTION__);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_write(codec, RT5658_CLASSD_2, 0x0550);
		snd_soc_write(codec, RT5658_CLASSD_1, 0x0803);
		snd_soc_write(codec, RT5658_CLASSD_8, 0x1028);
		snd_soc_write(codec, RT5658_CLASSD_10, 0x7cea);
		snd_soc_update_bits(codec, RT5658_PWR_DIG_1, RT5658_PWR_CLS_D,
			RT5658_PWR_CLS_D);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_write(codec, RT5658_CLASSD_0, 0x2021);
		snd_soc_write(codec, RT5658_CLASSD_1, 0x0011);
		snd_soc_write(codec, RT5658_CLASSD_2, 0x0150);
		snd_soc_update_bits(codec, RT5658_PWR_DIG_1, RT5658_PWR_CLS_D,
			0);
		break;

	default:
		return 0;
	}

	return 0;

}

static int rt5658_hp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	pr_debug("%s\n", __FUNCTION__);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_write(codec, RT5658_DEPOP_1, 0x0019);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_write(codec, RT5658_DEPOP_1, 0x0009);
		break;

	default:
		return 0;
	}

	return 0;
}

static int set_bst1_power(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_2,
			RT5658_PWR_BST1_P | RT5658_PWR_BST1,
			RT5658_PWR_BST1_P | RT5658_PWR_BST1);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_2,
			RT5658_PWR_BST1_P| RT5658_PWR_BST1, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int set_bst2_power(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_2,
			RT5658_PWR_BST2_P | RT5658_PWR_BST2,
			RT5658_PWR_BST2_P | RT5658_PWR_BST2);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_2,
			RT5658_PWR_BST2_P | RT5658_PWR_BST2, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int set_bst3_power(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_2,
			RT5658_PWR_BST3_P | RT5658_PWR_BST3,
			RT5658_PWR_BST3_P | RT5658_PWR_BST3);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_3,
			RT5658_PWR_BST3_P | RT5658_PWR_BST3, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int set_bst4_power(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_2,
			RT5658_PWR_BST4_P | RT5658_PWR_BST4,
			RT5658_PWR_BST4_P | RT5658_PWR_BST4);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_3,
			RT5658_PWR_BST4_P | RT5658_PWR_BST4, 0);
		break;

	default:
		return 0;
	}

	return 0;
}

static int set_dmic_power(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		pr_debug("%s\n", __FUNCTION__);
		/*Add delay to avoid pop noise*/
		msleep(dmic_power_delay);
		break;

	default:
		return 0;
	}

	return 0;
}

static const struct snd_soc_dapm_widget rt5658_dapm_widgets[] = {
	SND_SOC_DAPM_SUPPLY("LDO2", RT5658_PWR_ANLG_3, RT5658_PWR_LDO2_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_SUPPLY("PLL", RT5658_PWR_ANLG_3, RT5658_PWR_PLL_BIT, 0,
		NULL, 0),

	/* ASRC */
	SND_SOC_DAPM_SUPPLY_S("I2S1 ASRC", 1, RT5658_ASRC_1,
		RT5658_I2S1_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("I2S2 ASRC", 1, RT5658_ASRC_1,
		RT5658_I2S2_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("I2S3 ASRC", 1, RT5658_ASRC_1,
		RT5658_I2S3_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC STO ASRC", 1, RT5658_ASRC_1,
		RT5658_DAC_STO_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC Mono L ASRC", 1, RT5658_ASRC_1,
		RT5658_DAC_MONO_L_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC Mono R ASRC", 1, RT5658_ASRC_1,
		RT5658_DAC_MONO_R_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC STO1 ASRC", 1, RT5658_ASRC_1,
		RT5658_ADC_STO1_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC Mono L ASRC", 1, RT5658_ASRC_1,
		RT5658_ADC_MONO_L_ASRC_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ADC Mono R ASRC", 1, RT5658_ASRC_1,
		RT5658_ADC_MONO_R_ASRC_SFT, 0, NULL, 0),

	/* Input Side */
	/* micbias */
	SND_SOC_DAPM_MICBIAS("MICBIAS1", RT5658_PWR_ANLG_2, RT5658_PWR_MB1_BIT,
		0),
	SND_SOC_DAPM_MICBIAS("MICBIAS2", RT5658_PWR_ANLG_2, RT5658_PWR_MB2_BIT,
		0),
	SND_SOC_DAPM_MICBIAS("MICBIAS3", RT5658_PWR_ANLG_2, RT5658_PWR_MB3_BIT,
		0),

	/* Input Lines */
	SND_SOC_DAPM_INPUT("DMIC L1"),
	SND_SOC_DAPM_INPUT("DMIC R1"),
	SND_SOC_DAPM_INPUT("DMIC L2"),
	SND_SOC_DAPM_INPUT("DMIC R2"),

	SND_SOC_DAPM_INPUT("IN1P"),
	SND_SOC_DAPM_INPUT("IN1N"),
	SND_SOC_DAPM_INPUT("IN2P"),
	SND_SOC_DAPM_INPUT("IN2N"),
	SND_SOC_DAPM_INPUT("IN3P"),
	SND_SOC_DAPM_INPUT("IN3N"),
	SND_SOC_DAPM_INPUT("IN4P"),
	SND_SOC_DAPM_INPUT("IN4N"),

	SND_SOC_DAPM_PGA("DMIC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DMIC2", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("DMIC CLK", SND_SOC_NOPM, 0, 0,
		set_dmic_clk, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_SUPPLY("DMIC1 Power", RT5658_DMIC_CTRL_1,
		RT5658_DMIC_1_EN_SFT, 0, set_dmic_power, SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("DMIC2 Power", RT5658_DMIC_CTRL_1,
		RT5658_DMIC_2_EN_SFT, 0, set_dmic_power, SND_SOC_DAPM_POST_PMU),

	/* Boost */
	SND_SOC_DAPM_PGA("BST1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("BST2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("BST3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("BST4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("BST1 Power", SND_SOC_NOPM, 0, 0, set_bst1_power,
		SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("BST2 Power", SND_SOC_NOPM, 0, 0, set_bst2_power,
		SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("BST3 Power", SND_SOC_NOPM, 0, 0, set_bst3_power,
		SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("BST4 Power", SND_SOC_NOPM, 0, 0, set_bst4_power,
		SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),

	/* Input Volume */
	SND_SOC_DAPM_PGA("INL VOL", RT5658_PWR_VOL, RT5658_PWR_IN_L_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_PGA("INR VOL", RT5658_PWR_VOL, RT5658_PWR_IN_R_BIT,
		0, NULL, 0),

	/* REC Mixer */
	SND_SOC_DAPM_MIXER("RECMIX1L", RT5658_PWR_MIXER, RT5658_PWR_RM1_L_BIT,
		0, rt5658_rec1_l_mix, ARRAY_SIZE(rt5658_rec1_l_mix)),
	SND_SOC_DAPM_MIXER("RECMIX1R", RT5658_PWR_MIXER, RT5658_PWR_RM1_R_BIT,
		0, rt5658_rec1_r_mix, ARRAY_SIZE(rt5658_rec1_r_mix)),
	SND_SOC_DAPM_MIXER("RECMIX2L", RT5658_PWR_MIXER, RT5658_PWR_RM2_L_BIT,
		0, rt5658_rec2_l_mix, ARRAY_SIZE(rt5658_rec2_l_mix)),
	SND_SOC_DAPM_MIXER("RECMIX2R", RT5658_PWR_MIXER, RT5658_PWR_RM2_R_BIT,
		0, rt5658_rec2_r_mix, ARRAY_SIZE(rt5658_rec2_r_mix)),

	/* ADCs */
	SND_SOC_DAPM_ADC("ADC1 L", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC1 R", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC2 L", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC2 R", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SUPPLY("ADC1 L Power", RT5658_PWR_DIG_1,
		RT5658_PWR_ADC_L1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC1 R Power", RT5658_PWR_DIG_1,
		RT5658_PWR_ADC_R1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC2 L Power", RT5658_PWR_DIG_2,
		RT5658_PWR_ADC_L2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC2 R Power", RT5658_PWR_DIG_2,
		RT5658_PWR_ADC_R2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC1 clock", SND_SOC_NOPM, 0, 0, set_adc_clk,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_SUPPLY("ADC2 clock", SND_SOC_NOPM, 0, 0, set_adc_clk,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	/* ADC Mux */
	SND_SOC_DAPM_MUX("Stereo1 DMIC L Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_dmic_mux),
	SND_SOC_DAPM_MUX("Stereo1 DMIC R Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_dmic_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC L1 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_adc1_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC R1 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_adc1_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC L2 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_adc2_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC R2 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_adc2_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC L Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_adc_mux),
	SND_SOC_DAPM_MUX("Stereo1 ADC R Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_sto1_adc_mux),
	SND_SOC_DAPM_MUX("Mono ADC L2 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_adc_l2_mux),
	SND_SOC_DAPM_MUX("Mono ADC R2 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_adc_r2_mux),
	SND_SOC_DAPM_MUX("Mono ADC L1 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_adc_l1_mux),
	SND_SOC_DAPM_MUX("Mono ADC R1 Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_adc_r1_mux),
	SND_SOC_DAPM_MUX("Mono DMIC L Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_dmic_l_mux),
	SND_SOC_DAPM_MUX("Mono DMIC R Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_dmic_r_mux),
	SND_SOC_DAPM_MUX("Mono ADC L Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_adc_l_mux),
	SND_SOC_DAPM_MUX("Mono ADC R Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_mono_adc_r_mux),
	/* ADC Mixer */
	SND_SOC_DAPM_SUPPLY("ADC Stereo1 Filter", RT5658_PWR_DIG_2,
		RT5658_PWR_ADC_S1F_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC Stereo2 Filter", RT5658_PWR_DIG_2,
		RT5658_PWR_ADC_S2F_BIT, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Stereo1 ADC MIXL", SND_SOC_NOPM,
		0, 0, rt5658_sto1_adc_l_mix,
		ARRAY_SIZE(rt5658_sto1_adc_l_mix)),
	SND_SOC_DAPM_MIXER("Stereo1 ADC MIXR", SND_SOC_NOPM,
		0, 0, rt5658_sto1_adc_r_mix,
		ARRAY_SIZE(rt5658_sto1_adc_r_mix)),
	SND_SOC_DAPM_SUPPLY("ADC Mono Left Filter", RT5658_PWR_DIG_2,
		RT5658_PWR_ADC_MF_L_BIT, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Mono ADC MIXL", RT5658_MONO_ADC_DIG_VOL,
		RT5658_L_MUTE_SFT, 1, rt5658_mono_adc_l_mix,
		ARRAY_SIZE(rt5658_mono_adc_l_mix)),
	SND_SOC_DAPM_SUPPLY("ADC Mono Right Filter", RT5658_PWR_DIG_2,
		RT5658_PWR_ADC_MF_R_BIT, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Mono ADC MIXR", RT5658_MONO_ADC_DIG_VOL,
		RT5658_R_MUTE_SFT, 1, rt5658_mono_adc_r_mix,
		ARRAY_SIZE(rt5658_mono_adc_r_mix)),

	/* ADC PGA */
	SND_SOC_DAPM_PGA("IF_ADC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF_ADC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF_ADC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1_ADC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1_ADC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1_ADC3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1_ADC4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Stereo2 ADC LR", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_PGA("Stereo1 ADC Volume L", RT5658_STO1_ADC_DIG_VOL,
		RT5658_L_MUTE_SFT, 1, NULL, 0),
	SND_SOC_DAPM_PGA("Stereo1 ADC Volume R", RT5658_STO1_ADC_DIG_VOL,
		RT5658_R_MUTE_SFT, 1, NULL, 0),

	/* Digital Interface */
	SND_SOC_DAPM_SUPPLY("I2S1", RT5658_PWR_DIG_1, RT5658_PWR_I2S1_BIT,
		0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC1 L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC1 R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC2 L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 DAC2 R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF1 ADC R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("I2S2", RT5658_PWR_DIG_1, RT5658_PWR_I2S2_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 DAC R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF2 ADC2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("I2S3", RT5658_PWR_DIG_1, RT5658_PWR_I2S3_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_PGA("IF3 DAC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 DAC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 DAC R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 ADC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 ADC L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IF3 ADC R", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* Digital Interface Select */
	SND_SOC_DAPM_PGA("TDM AD1:AD2:DAC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("TDM AD2:DAC", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MUX("TDM Data Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_rx_adc_dac_mux),
	SND_SOC_DAPM_MUX("IF2 ADC Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_if2_adc_in_mux),
	SND_SOC_DAPM_MUX("IF3 ADC Mux", SND_SOC_NOPM, 0, 0,
		&rt5658_if3_adc_in_mux),

	/* Audio Interface */
	SND_SOC_DAPM_AIF_IN("AIF1RX", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF1TX", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AIF2RX", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF2TX", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AIF3RX", "AIF3 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF3TX", "AIF3 Capture", 0, SND_SOC_NOPM, 0, 0),

	/* Output Side */
	/* DAC mixer before sound effect  */
	SND_SOC_DAPM_MIXER("DAC1 MIXL", SND_SOC_NOPM, 0, 0,
		rt5658_dac_l_mix, ARRAY_SIZE(rt5658_dac_l_mix)),
	SND_SOC_DAPM_MIXER("DAC1 MIXR", SND_SOC_NOPM, 0, 0,
		rt5658_dac_r_mix, ARRAY_SIZE(rt5658_dac_r_mix)),

	/* DAC channel Mux */
	SND_SOC_DAPM_MUX("DAC L1 Mux", SND_SOC_NOPM, 0, 0, &rt5658_dac_l1_mux),
	SND_SOC_DAPM_MUX("DAC R1 Mux", SND_SOC_NOPM, 0, 0, &rt5658_dac_r1_mux),
	SND_SOC_DAPM_MUX("DAC L2 Mux", SND_SOC_NOPM, 0, 0, &rt5658_dac_l2_mux),
	SND_SOC_DAPM_MUX("DAC R2 Mux", SND_SOC_NOPM, 0, 0, &rt5658_dac_r2_mux),
	SND_SOC_DAPM_PGA("DAC L2 Volume", RT5658_PWR_DIG_1,
		RT5658_PWR_DAC_L2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DAC R2 Volume", RT5658_PWR_DIG_1,
		RT5658_PWR_DAC_R2_BIT, 0, NULL, 0),

	SND_SOC_DAPM_MUX("DAC L1 Source", SND_SOC_NOPM, 0, 0,
		&rt5658_alg_dac_l1_mux),
	SND_SOC_DAPM_MUX("DAC R1 Source", SND_SOC_NOPM, 0, 0,
		&rt5658_alg_dac_r1_mux),
	SND_SOC_DAPM_MUX("DAC L2 Source", SND_SOC_NOPM, 0, 0,
		&rt5658_alg_dac_l2_mux),
	SND_SOC_DAPM_MUX("DAC R2 Source", SND_SOC_NOPM, 0, 0,
		&rt5658_alg_dac_r2_mux),

	/* DAC Mixer */
	SND_SOC_DAPM_SUPPLY("DAC Stereo1 Filter", RT5658_PWR_DIG_2,
		RT5658_PWR_DAC_S1F_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DAC Mono Left Filter", RT5658_PWR_DIG_2,
		RT5658_PWR_DAC_MF_L_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DAC Mono Right Filter", RT5658_PWR_DIG_2,
		RT5658_PWR_DAC_MF_R_BIT, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Stereo DAC MIXL", SND_SOC_NOPM, 0, 0,
		rt5658_sto_dac_l_mix, ARRAY_SIZE(rt5658_sto_dac_l_mix)),
	SND_SOC_DAPM_MIXER("Stereo DAC MIXR", SND_SOC_NOPM, 0, 0,
		rt5658_sto_dac_r_mix, ARRAY_SIZE(rt5658_sto_dac_r_mix)),
	SND_SOC_DAPM_MIXER("Mono DAC MIXL", SND_SOC_NOPM, 0, 0,
		rt5658_mono_dac_l_mix, ARRAY_SIZE(rt5658_mono_dac_l_mix)),
	SND_SOC_DAPM_MIXER("Mono DAC MIXR", SND_SOC_NOPM, 0, 0,
		rt5658_mono_dac_r_mix, ARRAY_SIZE(rt5658_mono_dac_r_mix)),
	SND_SOC_DAPM_MUX("DAC MIXL", SND_SOC_NOPM, 0, 0,
		&rt5658_dig_dac_mixl_mux),
	SND_SOC_DAPM_MUX("DAC MIXR", SND_SOC_NOPM, 0, 0,
		&rt5658_dig_dac_mixr_mux),

	/* DACs */
	SND_SOC_DAPM_SUPPLY_S("DAC L1 Power", 1, RT5658_PWR_DIG_1,
		RT5658_PWR_DAC_L1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DAC R1 Power", 1, RT5658_PWR_DIG_1,
		RT5658_PWR_DAC_R1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_DAC("DAC L1", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC R1", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SUPPLY("DAC L2 Power", RT5658_PWR_DIG_1,
		RT5658_PWR_DAC_L2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DAC R2 Power", RT5658_PWR_DIG_1,
		RT5658_PWR_DAC_R2_BIT, 0, NULL, 0),
	SND_SOC_DAPM_DAC("DAC L2", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC R2", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_PGA("DAC_REF", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* OUT Mixer */
	SND_SOC_DAPM_MIXER("SPK MIXL", RT5658_PWR_MIXER, RT5658_PWR_SM_L_BIT,
		0, rt5658_spk_l_mix, ARRAY_SIZE(rt5658_spk_l_mix)),
	SND_SOC_DAPM_MIXER("SPK MIXR", RT5658_PWR_MIXER, RT5658_PWR_SM_L_BIT,
		0, rt5658_spk_r_mix, ARRAY_SIZE(rt5658_spk_r_mix)),
	SND_SOC_DAPM_MIXER("MONOVOL MIX", RT5658_PWR_MIXER, RT5658_PWR_MM_BIT,
		0, rt5658_monovol_mix, ARRAY_SIZE(rt5658_monovol_mix)),
	SND_SOC_DAPM_MIXER("OUT MIXL", RT5658_PWR_MIXER, RT5658_PWR_OM_L_BIT,
		0, rt5658_out_l_mix, ARRAY_SIZE(rt5658_out_l_mix)),
	SND_SOC_DAPM_MIXER("OUT MIXR", RT5658_PWR_MIXER, RT5658_PWR_OM_R_BIT,
		0, rt5658_out_r_mix, ARRAY_SIZE(rt5658_out_r_mix)),

	/* Ouput Volume */
	SND_SOC_DAPM_SWITCH("SPKVOL L", RT5658_PWR_VOL, RT5658_PWR_SV_L_BIT, 0,
		&spkvol_l_switch),
	SND_SOC_DAPM_SWITCH("SPKVOL R", RT5658_PWR_VOL, RT5658_PWR_SV_R_BIT, 0,
		&spkvol_r_switch),
	SND_SOC_DAPM_SWITCH("MONOVOL", RT5658_PWR_VOL, RT5658_PWR_MV_BIT, 0,
		&monovol_switch),
	SND_SOC_DAPM_SWITCH("OUTVOL L", RT5658_PWR_VOL, RT5658_PWR_OV_L_BIT, 0,
		&outvol_l_switch),
	SND_SOC_DAPM_SWITCH("OUTVOL R", RT5658_PWR_VOL, RT5658_PWR_OV_R_BIT, 0,
		&outvol_r_switch),

	/* SPO/MONO/HPO/LOUT */
	SND_SOC_DAPM_MIXER("SPO L MIX", SND_SOC_NOPM, 0, 0, rt5658_spo_l_mix,
		ARRAY_SIZE(rt5658_spo_l_mix)),
	SND_SOC_DAPM_MIXER("SPO R MIX", SND_SOC_NOPM, 0, 0, rt5658_spo_r_mix,
		ARRAY_SIZE(rt5658_spo_r_mix)),
	SND_SOC_DAPM_MIXER("Mono MIX", SND_SOC_NOPM, 0,	0, rt5658_mono_mix,
		ARRAY_SIZE(rt5658_mono_mix)),
	SND_SOC_DAPM_MIXER("LOUT L MIX", SND_SOC_NOPM, 0, 0, rt5658_lout_l_mix,
		ARRAY_SIZE(rt5658_lout_l_mix)),
	SND_SOC_DAPM_MIXER("LOUT R MIX", SND_SOC_NOPM, 0, 0, rt5658_lout_r_mix,
		ARRAY_SIZE(rt5658_lout_r_mix)),

	SND_SOC_DAPM_PGA_S("SPK Amp", 2, SND_SOC_NOPM, 0, 0, rt5658_spk_event,
		SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_PGA("Mono Amp", RT5658_PWR_ANLG_1,
		RT5658_PWR_MA_BIT, 0, NULL, 0),
	SND_SOC_DAPM_PGA_S("HP Amp", 2, SND_SOC_NOPM, 0, 0, rt5658_hp_event,
		SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_PGA("LOUT Amp", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("Charge Pump", SND_SOC_NOPM, 0, 0,
		rt5658_charge_pump_event, SND_SOC_DAPM_PRE_PMU |
		SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SWITCH("SPO L Playback", SND_SOC_NOPM, 0, 0,
		&spo_l_switch),
	SND_SOC_DAPM_SWITCH("SPO R Playback", SND_SOC_NOPM, 0, 0,
		&spo_r_switch),
	SND_SOC_DAPM_SWITCH("Mono Playback", SND_SOC_NOPM, 0, 0,
		&mono_switch),
	SND_SOC_DAPM_SWITCH("HPO L Playback", SND_SOC_NOPM, 0, 0,
		&hpo_l_switch),
	SND_SOC_DAPM_SWITCH("HPO R Playback", SND_SOC_NOPM, 0, 0,
		&hpo_r_switch),
	SND_SOC_DAPM_SWITCH("LOUT L Playback", SND_SOC_NOPM, 0, 0,
		&lout_l_switch),
	SND_SOC_DAPM_SWITCH("LOUT R Playback", SND_SOC_NOPM, 0, 0,
		&lout_r_switch),

	/* PDM */
	SND_SOC_DAPM_SUPPLY("PDM Power", RT5658_PWR_DIG_2,
		RT5658_PWR_PDM1_BIT, 0, NULL, 0),
	SND_SOC_DAPM_MUX("PDM L Mux", RT5658_PDM_OUT_CTRL,
		RT5658_M_PDM1_L_SFT, 1, &rt5658_pdm_l_mux),
	SND_SOC_DAPM_MUX("PDM R Mux", RT5658_PDM_OUT_CTRL,
		RT5658_M_PDM1_R_SFT, 1, &rt5658_pdm_r_mux),

	/* SPDIF */
	SND_SOC_DAPM_MUX("SPDIF Mux", SND_SOC_NOPM, 0, 0, &rt5658_spdif_mux),

	/* Output Lines */
	SND_SOC_DAPM_OUTPUT("HPOL"),
	SND_SOC_DAPM_OUTPUT("HPOR"),
	SND_SOC_DAPM_OUTPUT("SPOL"),
	SND_SOC_DAPM_OUTPUT("SPOR"),
	SND_SOC_DAPM_OUTPUT("LOUTL"),
	SND_SOC_DAPM_OUTPUT("LOUTR"),
	SND_SOC_DAPM_OUTPUT("MONOOUT"),
	SND_SOC_DAPM_OUTPUT("PDML"),
	SND_SOC_DAPM_OUTPUT("PDMR"),
	SND_SOC_DAPM_OUTPUT("SPDIF"),
};

static const struct snd_soc_dapm_route rt5658_dapm_routes[] = {
	/*PLL*/
	{ "ADC Stereo1 Filter", NULL, "PLL", is_sys_clk_from_pll },
	{ "ADC Stereo2 Filter", NULL, "PLL", is_sys_clk_from_pll },
	{ "ADC Mono Left Filter", NULL, "PLL", is_sys_clk_from_pll },
	{ "ADC Mono Right Filter", NULL, "PLL", is_sys_clk_from_pll },
	{ "DAC Stereo1 Filter", NULL, "PLL", is_sys_clk_from_pll },
	{ "DAC Mono Left Filter", NULL, "PLL", is_sys_clk_from_pll },
	{ "DAC Mono Right Filter", NULL, "PLL", is_sys_clk_from_pll },

	/*ASRC*/
	{ "ADC Stereo1 Filter", NULL, "ADC STO1 ASRC", is_using_asrc },
	{ "ADC Mono Left Filter", NULL, "ADC Mono L ASRC", is_using_asrc },
	{ "ADC Mono Right Filter", NULL, "ADC Mono R ASRC", is_using_asrc },
	{ "DAC Mono Left Filter", NULL, "DAC Mono L ASRC", is_using_asrc },
	{ "DAC Mono Right Filter", NULL, "DAC Mono R ASRC", is_using_asrc },
	{ "DAC Stereo1 Filter", NULL, "DAC STO ASRC", is_using_asrc },

	{ "I2S1", NULL, "I2S1 ASRC" },
	{ "I2S2", NULL, "I2S2 ASRC" },
	{ "I2S3", NULL, "I2S3 ASRC" },

	{ "IN1P", NULL, "LDO2" },
	{ "IN2P", NULL, "LDO2" },
	{ "IN3P", NULL, "LDO2" },
	{ "IN4P", NULL, "LDO2" },

	{ "DMIC1", NULL, "DMIC L1" },
	{ "DMIC1", NULL, "DMIC R1" },
	{ "DMIC2", NULL, "DMIC L2" },
	{ "DMIC2", NULL, "DMIC R2" },

	{ "BST1", NULL, "IN1P" },
	{ "BST1", NULL, "IN1N" },
	{ "BST1", NULL, "BST1 Power" },
	{ "BST2", NULL, "IN2P" },
	{ "BST2", NULL, "IN2N" },
	{ "BST2", NULL, "BST2 Power" },
	{ "BST3", NULL, "IN3P" },
	{ "BST3", NULL, "IN3N" },
	{ "BST3", NULL, "BST3 Power" },
	{ "BST4", NULL, "IN4P" },
	{ "BST4", NULL, "IN4N" },
	{ "BST4", NULL, "BST4 Power" },

	{ "INL VOL", NULL, "IN2P" },
	{ "INR VOL", NULL, "IN2N" },

	{ "RECMIX1L", "SPKVOLL Switch", "SPKVOL L" },
	{ "RECMIX1L", "INL Switch", "INL VOL" },
	{ "RECMIX1L", "BST4 Switch", "BST4" },
	{ "RECMIX1L", "BST3 Switch", "BST3" },
	{ "RECMIX1L", "BST2 Switch", "BST2" },
	{ "RECMIX1L", "BST1 Switch", "BST1" },

	{ "RECMIX1R", "HPOVOLR Switch", "HPO R Playback" },
	{ "RECMIX1R", "INR Switch", "INR VOL" },
	{ "RECMIX1R", "BST4 Switch", "BST4" },
	{ "RECMIX1R", "BST3 Switch", "BST3" },
	{ "RECMIX1R", "BST2 Switch", "BST2" },
	{ "RECMIX1R", "BST1 Switch", "BST1" },

	{ "RECMIX2L", "SPKVOLL Switch", "SPKVOL L" },
	{ "RECMIX2L", "OUTVOLL Switch", "OUTVOL L" },
	{ "RECMIX2L", "BST4 Switch", "BST4" },
	{ "RECMIX2L", "BST3 Switch", "BST3" },
	{ "RECMIX2L", "BST2 Switch", "BST2" },
	{ "RECMIX2L", "BST1 Switch", "BST1" },

	{ "RECMIX2R", "MONOVOL Switch", "MONOVOL" },
	{ "RECMIX2R", "OUTVOLR Switch", "OUTVOL R" },
	{ "RECMIX2R", "BST4 Switch", "BST4" },
	{ "RECMIX2R", "BST3 Switch", "BST3" },
	{ "RECMIX2R", "BST2 Switch", "BST2" },
	{ "RECMIX2R", "BST1 Switch", "BST1" },

	{ "ADC1 L", NULL, "RECMIX1L" },
	{ "ADC1 L", NULL, "ADC1 L Power" },
	{ "ADC1 L", NULL, "ADC1 clock" },
	{ "ADC1 R", NULL, "RECMIX1R" },
	{ "ADC1 R", NULL, "ADC1 R Power" },
	{ "ADC1 R", NULL, "ADC1 clock" },

	{ "ADC2 L", NULL, "RECMIX2L" },
	{ "ADC2 L", NULL, "ADC2 L Power" },
	{ "ADC2 L", NULL, "ADC2 clock" },
	{ "ADC2 R", NULL, "RECMIX2R" },
	{ "ADC2 R", NULL, "ADC2 R Power" },
	{ "ADC2 R", NULL, "ADC2 clock" },

	{ "DMIC L1", NULL, "DMIC CLK" },
	{ "DMIC L1", NULL, "DMIC1 Power" },
	{ "DMIC R1", NULL, "DMIC CLK" },
	{ "DMIC R1", NULL, "DMIC1 Power" },
	{ "DMIC L2", NULL, "DMIC CLK" },
	{ "DMIC L2", NULL, "DMIC2 Power" },
	{ "DMIC R2", NULL, "DMIC CLK" },
	{ "DMIC R2", NULL, "DMIC2 Power" },

	{ "Stereo1 DMIC L Mux", "DMIC1", "DMIC L1" },
	{ "Stereo1 DMIC L Mux", "DMIC2", "DMIC L2" },

	{ "Stereo1 DMIC R Mux", "DMIC1", "DMIC R1" },
	{ "Stereo1 DMIC R Mux", "DMIC2", "DMIC R2" },

	{ "Mono DMIC L Mux", "DMIC1 L", "DMIC L1" },
	{ "Mono DMIC L Mux", "DMIC2 L", "DMIC L2" },

	{ "Mono DMIC R Mux", "DMIC1 R", "DMIC R1" },
	{ "Mono DMIC R Mux", "DMIC2 R", "DMIC R2" },

	{ "Stereo1 ADC L Mux", "ADC1", "ADC1 L" },
	{ "Stereo1 ADC L Mux", "ADC2", "ADC2 L" },
	{ "Stereo1 ADC R Mux", "ADC1", "ADC1 R" },
	{ "Stereo1 ADC R Mux", "ADC2", "ADC2 R" },

	{ "Stereo1 ADC L1 Mux", "ADC", "Stereo1 ADC L Mux" },
	{ "Stereo1 ADC L1 Mux", "DAC MIX", "DAC MIXL" },
	{ "Stereo1 ADC L2 Mux", "DMIC", "Stereo1 DMIC L Mux" },
	{ "Stereo1 ADC L2 Mux", "DAC MIX", "DAC MIXL" },

	{ "Stereo1 ADC R1 Mux", "ADC", "Stereo1 ADC R Mux" },
	{ "Stereo1 ADC R1 Mux", "DAC MIX", "DAC MIXR" },
	{ "Stereo1 ADC R2 Mux", "DMIC", "Stereo1 DMIC R Mux" },
	{ "Stereo1 ADC R2 Mux", "DAC MIX", "DAC MIXR" },

	{ "Mono ADC L Mux", "ADC1 L", "ADC1 L" },
	{ "Mono ADC L Mux", "ADC1 R", "ADC1 R" },
	{ "Mono ADC L Mux", "ADC2 L", "ADC2 L" },
	{ "Mono ADC L Mux", "ADC2 R", "ADC2 R" },

	{ "Mono ADC R Mux", "ADC1 L", "ADC1 L" },
	{ "Mono ADC R Mux", "ADC1 R", "ADC1 R" },
	{ "Mono ADC R Mux", "ADC2 L", "ADC2 L" },
	{ "Mono ADC R Mux", "ADC2 R", "ADC2 R" },

	{ "Mono ADC L2 Mux", "DMIC", "Mono DMIC L Mux" },
	{ "Mono ADC L2 Mux", "Mono DAC MIXL", "Mono DAC MIXL" },
	{ "Mono ADC L1 Mux", "Mono DAC MIXL", "Mono DAC MIXL" },
	{ "Mono ADC L1 Mux", "ADC",  "Mono ADC L Mux" },

	{ "Mono ADC R1 Mux", "Mono DAC MIXR", "Mono DAC MIXR" },
	{ "Mono ADC R1 Mux", "ADC", "Mono ADC R Mux" },
	{ "Mono ADC R2 Mux", "DMIC", "Mono DMIC R Mux" },
	{ "Mono ADC R2 Mux", "Mono DAC MIXR", "Mono DAC MIXR" },

	{ "Stereo1 ADC MIXL", "ADC1 Switch", "Stereo1 ADC L1 Mux" },
	{ "Stereo1 ADC MIXL", "ADC2 Switch", "Stereo1 ADC L2 Mux" },
	{ "Stereo1 ADC MIXL", NULL, "ADC Stereo1 Filter" },

	{ "Stereo1 ADC MIXR", "ADC1 Switch", "Stereo1 ADC R1 Mux" },
	{ "Stereo1 ADC MIXR", "ADC2 Switch", "Stereo1 ADC R2 Mux" },
	{ "Stereo1 ADC MIXR", NULL, "ADC Stereo1 Filter" },

	{ "Mono ADC MIXL", "ADC1 Switch", "Mono ADC L1 Mux" },
	{ "Mono ADC MIXL", "ADC2 Switch", "Mono ADC L2 Mux" },
	{ "Mono ADC MIXL", NULL, "ADC Mono Left Filter" },

	{ "Mono ADC MIXR", "ADC1 Switch", "Mono ADC R1 Mux" },
	{ "Mono ADC MIXR", "ADC2 Switch", "Mono ADC R2 Mux" },
	{ "Mono ADC MIXR", NULL, "ADC Mono Right Filter" },

	{ "Stereo1 ADC Volume L", NULL, "Stereo1 ADC MIXL" },
	{ "Stereo1 ADC Volume R", NULL, "Stereo1 ADC MIXR" },

	{ "IF_ADC1", NULL, "Stereo1 ADC Volume L" },
	{ "IF_ADC1", NULL, "Stereo1 ADC Volume R" },
	{ "IF_ADC2", NULL, "Mono ADC MIXL" },
	{ "IF_ADC2", NULL, "Mono ADC MIXR" },

	{ "TDM AD1:AD2:DAC", NULL, "IF_ADC1" },
	{ "TDM AD1:AD2:DAC", NULL, "IF_ADC2" },
	{ "TDM AD1:AD2:DAC", NULL, "DAC_REF" },
	{ "TDM AD2:DAC", NULL, "IF_ADC2" },
	{ "TDM AD2:DAC", NULL, "DAC_REF" },
	{ "TDM Data Mux", "AD1:AD2:DAC:NUL", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD1:AD2:NUL:DAC", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD1:DAC:AD2:NUL", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD1:DAC:NUL:AD2", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD1:NUL:DAC:AD2", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD1:NUL:AD2:DAC", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD2:AD1:DAC:NUL", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD2:AD1:NUL:DAC", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD2:DAC:AD1:NUL", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD2:DAC:NUL:AD1", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD2:NUL:DAC:AD1", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "AD1:NUL:AD1:DAC", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "DAC:AD1:AD2:NUL", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "DAC:AD1:NUL:AD2", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "DAC:AD2:AD1:NUL", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "DAC:AD2:NUL:AD1", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "DAC:NUL:DAC:AD2", "TDM AD2:DAC" },
	{ "TDM Data Mux", "DAC:NUL:AD2:DAC", "TDM AD2:DAC" },
	{ "TDM Data Mux", "NUL:AD1:AD2:DAC", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "NUL:AD1:DAC:AD2", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "NUL:AD2:AD1:DAC", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "NUL:AD2:DAC:AD1", "TDM AD1:AD2:DAC" },
	{ "TDM Data Mux", "NUL:DAC:DAC:AD2", "TDM AD2:DAC" },
	{ "TDM Data Mux", "NUL:DAC:AD2:DAC", "TDM AD2:DAC" },
	{ "IF1 ADC", NULL, "I2S1" },
	{ "IF1 ADC", NULL, "TDM Data Mux" },

	{ "IF2 ADC Mux", "IF_ADC1", "IF_ADC1" },
	{ "IF2 ADC Mux", "IF_ADC2", "IF_ADC2" },
	{ "IF2 ADC Mux", "IF_ADC3", "IF_ADC3" },
	{ "IF2 ADC Mux", "DAC_REF", "DAC_REF" },
	{ "IF2 ADC", NULL, "IF2 ADC Mux"},
	{ "IF2 ADC", NULL, "I2S2" },

	{ "IF3 ADC Mux", "IF_ADC1", "IF_ADC1" },
	{ "IF3 ADC Mux", "IF_ADC2", "IF_ADC2" },
	{ "IF3 ADC Mux", "Stereo2_ADC_L/R", "Stereo2 ADC LR" },
	{ "IF3 ADC Mux", "DAC_REF", "DAC_REF" },
	{ "IF3 ADC", NULL, "IF3 ADC Mux"},
	{ "IF3 ADC", NULL, "I2S3" },

	{ "AIF1TX", NULL, "IF1 ADC" },
	{ "AIF2TX", NULL, "IF2 ADC" },
	{ "AIF3TX", NULL, "IF3 ADC" },

	{ "IF1 DAC1", NULL, "AIF1RX" },
	{ "IF1 DAC2", NULL, "AIF1RX" },
	{ "IF2 DAC", NULL, "AIF2RX" },
	{ "IF3 DAC", NULL, "AIF3RX" },

	{ "IF1 DAC1", NULL, "I2S1" },
	{ "IF1 DAC2", NULL, "I2S1" },
	{ "IF2 DAC", NULL, "I2S2" },
	{ "IF3 DAC", NULL, "I2S3" },

	{ "IF1 DAC2 L", NULL, "IF1 DAC2" },
	{ "IF1 DAC2 R", NULL, "IF1 DAC2" },
	{ "IF1 DAC1 L", NULL, "IF1 DAC1" },
	{ "IF1 DAC1 R", NULL, "IF1 DAC1" },
	{ "IF2 DAC L", NULL, "IF2 DAC" },
	{ "IF2 DAC R", NULL, "IF2 DAC" },
	{ "IF3 DAC L", NULL, "IF3 DAC" },
	{ "IF3 DAC R", NULL, "IF3 DAC" },

	{ "DAC L1 Mux", "IF1 DAC1", "IF1 DAC1 L" },
	{ "DAC L1 Mux", "IF2 DAC", "IF2 DAC L" },
	{ "DAC L1 Mux", "IF3 DAC", "IF3 DAC L" },
	{ "DAC L1 Mux", NULL, "DAC Stereo1 Filter" },

	{ "DAC R1 Mux", "IF1 DAC1", "IF1 DAC1 R" },
	{ "DAC R1 Mux", "IF2 DAC", "IF2 DAC R" },
	{ "DAC R1 Mux", "IF3 DAC", "IF3 DAC R" },
	{ "DAC R1 Mux", NULL, "DAC Stereo1 Filter" },

	{ "DAC1 MIXL", "Stereo ADC Switch", "Stereo1 ADC Volume L" },
	{ "DAC1 MIXL", "DAC1 Switch", "DAC L1 Mux" },
	{ "DAC1 MIXR", "Stereo ADC Switch", "Stereo1 ADC Volume R" },
	{ "DAC1 MIXR", "DAC1 Switch", "DAC R1 Mux" },

	{ "DAC_REF", NULL, "DAC L1" },
	{ "DAC_REF", NULL, "DAC R1" },

	{ "DAC L2 Mux", "IF1 DAC2", "IF1 DAC2 L" },
	{ "DAC L2 Mux", "IF2 DAC", "IF2 DAC L" },
	{ "DAC L2 Mux", "IF3 DAC", "IF3 DAC L" },
	{ "DAC L2 Mux", "Mono ADC MIX", "Mono ADC MIXL" },
	{ "DAC L2 Volume", NULL, "DAC L2 Mux" },
	{ "DAC L2 Volume", NULL, "DAC Mono Left Filter" },

	{ "DAC R2 Mux", "IF1 DAC2", "IF1 DAC2 R" },
	{ "DAC R2 Mux", "IF2 DAC", "IF2 DAC R" },
	{ "DAC R2 Mux", "IF3 DAC", "IF3 DAC R" },
	{ "DAC R2 Mux", "Mono ADC MIX", "Mono ADC MIXR" },
	{ "DAC R2 Volume", NULL, "DAC R2 Mux" },
	{ "DAC R2 Volume", NULL, "DAC Mono Right Filter" },

	{ "Stereo DAC MIXL", "DAC L1 Switch", "DAC1 MIXL" },
	{ "Stereo DAC MIXL", "DAC R1 Switch", "DAC1 MIXR" },
	{ "Stereo DAC MIXL", "DAC L2 Switch", "DAC L2 Volume" },
	{ "Stereo DAC MIXL", "DAC R2 Switch", "DAC R2 Volume" },

	{ "Stereo DAC MIXR", "DAC R1 Switch", "DAC1 MIXR" },
	{ "Stereo DAC MIXR", "DAC L1 Switch", "DAC1 MIXL" },
	{ "Stereo DAC MIXR", "DAC L2 Switch", "DAC L2 Volume" },
	{ "Stereo DAC MIXR", "DAC R2 Switch", "DAC R2 Volume" },

	{ "Mono DAC MIXL", "DAC L1 Switch", "DAC1 MIXL" },
	{ "Mono DAC MIXL", "DAC R1 Switch", "DAC1 MIXR" },
	{ "Mono DAC MIXL", "DAC L2 Switch", "DAC L2 Volume" },
	{ "Mono DAC MIXL", "DAC R2 Switch", "DAC R2 Volume" },
	{ "Mono DAC MIXR", "DAC L1 Switch", "DAC1 MIXL" },
	{ "Mono DAC MIXR", "DAC R1 Switch", "DAC1 MIXR" },
	{ "Mono DAC MIXR", "DAC R2 Switch", "DAC R2 Volume" },
	{ "Mono DAC MIXR", "DAC L2 Switch", "DAC L2 Volume" },

	{ "DAC MIXL", "Stereo DAC Mixer", "Stereo DAC MIXL" },
	{ "DAC MIXL", "Mono DAC Mixer", "Mono DAC MIXL" },
	{ "DAC MIXR", "Stereo DAC Mixer", "Stereo DAC MIXR" },
	{ "DAC MIXR", "Mono DAC Mixer", "Mono DAC MIXR" },

	{ "DAC L1 Source", NULL, "DAC L1 Power" },
	{ "DAC L1 Source", "DAC", "DAC1 MIXL" },
	{ "DAC L1 Source", "Stereo DAC Mixer", "Stereo DAC MIXL" },
	{ "DAC R1 Source", NULL, "DAC R1 Power" },
	{ "DAC R1 Source", "DAC", "DAC1 MIXR" },
	{ "DAC R1 Source", "Stereo DAC Mixer", "Stereo DAC MIXR" },
	{ "DAC L2 Source", "Stereo DAC Mixer", "Stereo DAC MIXL" },
	{ "DAC L2 Source", "Mono DAC Mixer", "Mono DAC MIXL" },
	{ "DAC L2 Source", NULL, "DAC L2 Power" },
	{ "DAC R2 Source", "Stereo DAC Mixer", "Stereo DAC MIXR" },
	{ "DAC R2 Source", "Mono DAC Mixer", "Mono DAC MIXR" },
	{ "DAC R2 Source", NULL, "DAC R2 Power" },

	{ "DAC L1", NULL, "DAC L1 Source" },
	{ "DAC R1", NULL, "DAC R1 Source" },
	{ "DAC L2", NULL, "DAC L2 Source" },
	{ "DAC R2", NULL, "DAC R2 Source" },

	{ "SPK MIXL", "DAC L2 Switch", "DAC L2" },
	{ "SPK MIXL", "BST1 Switch", "BST1" },
	{ "SPK MIXL", "INL Switch", "INL VOL" },
	{ "SPK MIXL", "INR Switch", "INR VOL" },
	{ "SPK MIXL", "BST3 Switch", "BST3" },
	{ "SPK MIXR", "DAC R2 Switch", "DAC R2" },
	{ "SPK MIXR", "BST4 Switch", "BST4" },
	{ "SPK MIXR", "INL Switch", "INL VOL" },
	{ "SPK MIXR", "INR Switch", "INR VOL" },
	{ "SPK MIXR", "BST3 Switch", "BST3" },

	{ "MONOVOL MIX", "DAC L2 Switch", "DAC L2" },
	{ "MONOVOL MIX", "DAC R2 Switch", "DAC R2" },
	{ "MONOVOL MIX", "BST1 Switch", "BST1" },
	{ "MONOVOL MIX", "BST2 Switch", "BST2" },
	{ "MONOVOL MIX", "BST3 Switch", "BST3" },

	{ "OUT MIXL", "DAC L2 Switch", "DAC L2" },
	{ "OUT MIXL", "INL Switch", "INL VOL" },
	{ "OUT MIXL", "BST1 Switch", "BST1" },
	{ "OUT MIXL", "BST2 Switch", "BST2" },
	{ "OUT MIXL", "BST3 Switch", "BST3" },
	{ "OUT MIXR", "DAC R2 Switch", "DAC R2" },
	{ "OUT MIXR", "INR Switch", "INR VOL" },
	{ "OUT MIXR", "BST2 Switch", "BST2" },
	{ "OUT MIXR", "BST3 Switch", "BST3" },
	{ "OUT MIXR", "BST4 Switch", "BST4" },

	{ "SPKVOL L", "Switch", "SPK MIXL" },
	{ "SPKVOL R", "Switch", "SPK MIXR" },
	{ "SPO L MIX", "DAC L2 Switch", "DAC L2" },
	{ "SPO L MIX", "SPKVOL L Switch", "SPKVOL L" },
	{ "SPO R MIX", "DAC R2 Switch", "DAC R2" },
	{ "SPO R MIX", "SPKVOL R Switch", "SPKVOL R" },
	{ "SPK Amp", NULL, "SPO L MIX" },
	{ "SPK Amp", NULL, "SPO R MIX" },
	{ "SPO L Playback", "Switch", "SPK Amp" },
	{ "SPO R Playback", "Switch", "SPK Amp" },
	{ "SPOL", NULL, "SPO L Playback" },
	{ "SPOR", NULL, "SPO R Playback" },

	{ "MONOVOL", "Switch", "MONOVOL MIX" },
	{ "Mono MIX", "DAC L2 Switch", "DAC L2" },
	{ "Mono MIX", "MONOVOL Switch", "MONOVOL" },
	{ "Mono Amp", NULL, "Mono MIX" },
	{ "Mono Playback", "Switch", "Mono Amp" },
	{ "MONOOUT", NULL, "Mono Playback" },

	{ "HP Amp", NULL, "DAC L1" },
	{ "HP Amp", NULL, "DAC R1" },
	{ "HP Amp", NULL, "Charge Pump" },
	{ "HPO L Playback", "Switch", "HP Amp"},
	{ "HPO R Playback", "Switch", "HP Amp"},
	{ "HPOL", NULL, "HPO L Playback" },
	{ "HPOR", NULL, "HPO R Playback" },

	{ "OUTVOL L", "Switch", "OUT MIXL" },
	{ "OUTVOL R", "Switch", "OUT MIXR" },
	{ "LOUT L MIX", "DAC L2 Switch", "DAC L2" },
	{ "LOUT L MIX", "OUTVOL L Switch", "OUTVOL L" },
	{ "LOUT R MIX", "DAC R2 Switch", "DAC R2" },
	{ "LOUT R MIX", "OUTVOL R Switch", "OUTVOL R" },
	{ "LOUT Amp", NULL, "LOUT L MIX" },
	{ "LOUT Amp", NULL, "LOUT R MIX" },
	{ "LOUT L Playback", "Switch", "LOUT Amp" },
	{ "LOUT R Playback", "Switch", "LOUT Amp" },
	{ "LOUTL", NULL, "LOUT L Playback" },
	{ "LOUTR", NULL, "LOUT R Playback" },

	{ "PDM L Mux", "Mono DAC", "Mono DAC MIXL" },
	{ "PDM L Mux", "Stereo DAC", "Stereo DAC MIXL" },
	{ "PDM L Mux", NULL, "PDM Power" },
	{ "PDM R Mux", "Mono DAC", "Mono DAC MIXR" },
	{ "PDM R Mux", "Stereo DAC", "Stereo DAC MIXR" },
	{ "PDM R Mux", NULL, "PDM Power" },
	{ "PDML", NULL, "PDM L Mux" },
	{ "PDMR", NULL, "PDM R Mux" },

	{ "SPDIF Mux", "IF3_DAC", "IF3 DAC" },
	{ "SPDIF Mux", "IF2_DAC", "IF2 DAC" },
	{ "SPDIF Mux", "IF1_DAC2", "IF1 DAC2" },
	{ "SPDIF Mux", "IF1_DAC1", "IF1 DAC1" },
	{ "SPDIF", NULL, "SPDIF Mux" },
};

static int get_clk_info(int sclk, int rate)
{
	int i, pd[] = { 1, 2, 3, 4, 6, 8, 12, 16 };

	if (sclk <= 0 || rate <= 0)
		return -EINVAL;

	rate = rate << 8;
	for (i = 0; i < ARRAY_SIZE(pd); i++)
		if (sclk == rate * pd[i])
			return i;

	return -EINVAL;
}

static int rt5658_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);
	unsigned int val_len = 0, val_clk, mask_clk;
	int pre_div, bclk_ms, frame_size;

	pr_debug("%s\n", __FUNCTION__);

	rt5658->lrck[dai->id] = params_rate(params);
	pre_div = get_clk_info(rt5658->sysclk, rt5658->lrck[dai->id]);
	if (pre_div < 0) {
		dev_err(codec->dev, "Unsupported clock setting %d for DAI %d\n",
			rt5658->lrck[dai->id], dai->id);
		return -EINVAL;
	}
	frame_size = snd_soc_params_to_frame_size(params);
	if (frame_size < 0) {
		dev_err(codec->dev, "Unsupported frame size: %d\n", frame_size);
		return -EINVAL;
	}
	bclk_ms = frame_size > 32 ? 1 : 0; //Bard: TODO: Maybe bclk_ms should be set by set_bclk_ratio
	rt5658->bclk[dai->id] = rt5658->lrck[dai->id] * (32 << bclk_ms);

	dev_dbg(dai->dev, "bclk is %dHz and lrck is %dHz\n",
		rt5658->bclk[dai->id], rt5658->lrck[dai->id]);
	dev_dbg(dai->dev, "bclk_ms is %d and pre_div is %d for iis %d\n",
				bclk_ms, pre_div, dai->id);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		val_len |= RT5658_I2S_DL_20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		val_len |= RT5658_I2S_DL_24;
		break;
	case SNDRV_PCM_FORMAT_S8:
		val_len |= RT5658_I2S_DL_8;
		break;
	default:
		return -EINVAL;
	}

	switch (dai->id) {
	case RT5658_AIF1:
		mask_clk = RT5658_I2S_PD1_MASK;
		val_clk = pre_div << RT5658_I2S_PD1_SFT;
		//mask_clk = (0x1<<15) |RT5658_I2S_PD1_MASK;
		//val_clk = bclk_ms <<15 |pre_div << RT5658_I2S_PD1_SFT;
		snd_soc_update_bits(codec, RT5658_I2S1_SDP,
			RT5658_I2S_DL_MASK, val_len);
		break;
	case RT5658_AIF2:
		mask_clk = RT5658_I2S_BCLK_MS2_MASK | RT5658_I2S_PD2_MASK;
		val_clk = bclk_ms << RT5658_I2S_BCLK_MS2_SFT |
			pre_div << RT5658_I2S_PD2_SFT;
		snd_soc_update_bits(codec, RT5658_I2S2_SDP,
			RT5658_I2S_DL_MASK, val_len);
		break;
	case RT5658_AIF3:
		mask_clk = RT5658_I2S_BCLK_MS3_MASK | RT5658_I2S_PD3_MASK;
		val_clk = bclk_ms << RT5658_I2S_BCLK_MS3_SFT |
			pre_div << RT5658_I2S_PD3_SFT;
		snd_soc_update_bits(codec, RT5658_I2S3_SDP,
			RT5658_I2S_DL_MASK, val_len);
		break;
	default:
		dev_err(codec->dev, "Invalid dai->id: %d\n", dai->id);
		return -EINVAL;
	}
printk("mask_clk=%4x, val_clk=%4x", mask_clk, val_clk);
	snd_soc_update_bits(codec, RT5658_ADDA_CLK_1, mask_clk, val_clk);

	return 0;
}

static int rt5658_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val = 0;

	pr_debug("%s\n", __FUNCTION__);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		rt5658->master[dai->id] = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		reg_val |= RT5658_I2S_MS_S;
		rt5658->master[dai->id] = 0;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		reg_val |= RT5658_I2S_BP_INV;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		reg_val |= RT5658_I2S_DF_LEFT;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		reg_val |= RT5658_I2S_DF_PCM_A;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		reg_val |= RT5658_I2S_DF_PCM_B;
		break;
	default:
		return -EINVAL;
	}

	switch (dai->id) {
	case RT5658_AIF1:
		snd_soc_update_bits(codec, RT5658_I2S1_SDP,
			RT5658_I2S_MS_MASK | RT5658_I2S_BP_MASK |
			RT5658_I2S_DF_MASK, reg_val);
		break;
	case RT5658_AIF2:
		snd_soc_update_bits(codec, RT5658_I2S2_SDP,
			RT5658_I2S_MS_MASK | RT5658_I2S_BP_MASK |
			RT5658_I2S_DF_MASK, reg_val);
		break;
	case RT5658_AIF3:
		snd_soc_update_bits(codec, RT5658_I2S3_SDP,
			RT5658_I2S_MS_MASK | RT5658_I2S_BP_MASK |
			RT5658_I2S_DF_MASK, reg_val);
		break;
	default:
		dev_err(codec->dev, "Invalid dai->id: %d\n", dai->id);
		return -EINVAL;
	}
	return 0;
}

static int rt5658_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val = 0;

	pr_debug("%s\n", __FUNCTION__);

	if (freq == rt5658->sysclk && clk_id == rt5658->sysclk_src)
		return 0;

	switch (clk_id) {
	case RT5658_SCLK_S_MCLK:
		reg_val |= RT5658_SCLK_SRC_MCLK;
		break;
	case RT5658_SCLK_S_PLL1:
		reg_val |= RT5658_SCLK_SRC_PLL1;
		break;
	case RT5658_SCLK_S_RCCLK:
		reg_val |= RT5658_SCLK_SRC_RCCLK;
		break;
	default:
		dev_err(codec->dev, "Invalid clock id (%d)\n", clk_id);
		return -EINVAL;
	}
	snd_soc_update_bits(codec, RT5658_GLB_CLK,
		RT5658_SCLK_SRC_MASK, reg_val);
	rt5658->sysclk = freq;
	rt5658->sysclk_src = clk_id;

	dev_dbg(dai->dev, "Sysclk is %dHz and clock id is %d\n", freq, clk_id);

	return 0;
}

/**
 * rt5658_pll_calc - Calcualte PLL M/N/K code.
 * @freq_in: external clock provided to codec.
 * @freq_out: target clock which codec works on.
 * @pll_code: Pointer to structure with M, N, K and bypass flag.
 *
 * Calcualte M/N/K code to configure PLL for codec. And K is assigned to 2
 * which make calculation more efficiently.
 *
 * Returns 0 for success or negative error code.
 */
static int rt5658_pll_calc(const unsigned int freq_in,
			   const unsigned int freq_out,
			   struct rt5658_pll_code *pll_code)
{
	int max_n = RT5658_PLL_N_MAX, max_m = RT5658_PLL_M_MAX;
	int k, n = 0, m = 0, red, n_t, m_t = 0, in_t, out_t, red_t =
	    abs(freq_out - freq_in);
	bool bypass = false;

	pr_debug("%s\n", __FUNCTION__);

	if (RT5658_PLL_INP_MAX < freq_in || RT5658_PLL_INP_MIN > freq_in)
		return -EINVAL;

	k = 100000000 / freq_out - 2;
	if (k > 31)
		k = 31;
	for (n_t = 0; n_t <= max_n; n_t++) {
		in_t = (2 * freq_in / (k + 2)) + (freq_in / (k + 2)) * n_t;
		if (in_t < 0)
			continue;
		if (in_t == freq_out) {
			bypass = true;
			n = n_t;
			goto code_find;
		}
		red = abs(in_t - freq_out);	/*m bypass */
		if (red < red_t) {
			bypass = true;
			n = n_t;
			m = m_t;
			if (red == 0)
				goto code_find;
			red_t = red;
		}
		for (m_t = 0; m_t <= max_m; m_t++) {
			out_t = in_t / (m_t + 2);
			red = abs(out_t - freq_out);
			if (red < red_t) {
				bypass = false;
				n = n_t;
				m = m_t;
				if (red == 0)
					goto code_find;
				red_t = red;
			}
		}
	}
	pr_debug("Only get approximation about PLL\n");

code_find:

	pll_code->m_bp = bypass;
	pll_code->m_code = m;
	pll_code->n_code = n;
	pll_code->k_code = k;
	return 0;
}

static int rt5658_set_dai_pll(struct snd_soc_dai *dai, int pll_id, int Source,
			unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);
	struct rt5658_pll_code pll_code;
	int ret;

	pr_debug("%s\n", __FUNCTION__);

	if (Source == rt5658->pll_src && freq_in == rt5658->pll_in &&
	    freq_out == rt5658->pll_out)
		return 0;

	if (!freq_in || !freq_out) {
		dev_dbg(codec->dev, "PLL disabled\n");

		rt5658->pll_in = 0;
		rt5658->pll_out = 0;
		snd_soc_update_bits(codec, RT5658_GLB_CLK,
			RT5658_SCLK_SRC_MASK, RT5658_SCLK_SRC_MCLK);
		return 0;
	}

	switch (Source) {
	case RT5658_PLL1_S_MCLK:
		snd_soc_update_bits(codec, RT5658_GLB_CLK,
			RT5658_PLL1_SRC_MASK, RT5658_PLL1_SRC_MCLK);
		break;
	case RT5658_PLL1_S_BCLK1:
		snd_soc_update_bits(codec, RT5658_GLB_CLK,
				RT5658_PLL1_SRC_MASK, RT5658_PLL1_SRC_BCLK1);
		break;
	case RT5658_PLL1_S_BCLK2:
		snd_soc_update_bits(codec, RT5658_GLB_CLK,
				RT5658_PLL1_SRC_MASK, RT5658_PLL1_SRC_BCLK2);
		break;
	case RT5658_PLL1_S_BCLK3:
		snd_soc_update_bits(codec, RT5658_GLB_CLK,
				RT5658_PLL1_SRC_MASK, RT5658_PLL1_SRC_BCLK3);
		break;
	default:
		dev_err(codec->dev, "Unknown PLL Source %d\n", Source);
		return -EINVAL;
	}

	ret = rt5658_pll_calc(freq_in, freq_out, &pll_code);
	if (ret < 0) {
		dev_err(codec->dev, "Unsupport input clock %d\n", freq_in);
		return ret;
	}

	dev_dbg(codec->dev, "bypass=%d m=%d n=%d k=%d\n",
		pll_code.m_bp, (pll_code.m_bp ? 0 : pll_code.m_code),
		pll_code.n_code, pll_code.k_code);

	snd_soc_write(codec, RT5658_PLL_CTRL_1,
		pll_code.n_code << RT5658_PLL_N_SFT | pll_code.k_code);
	snd_soc_write(codec, RT5658_PLL_CTRL_2,
		(pll_code.m_bp ? 0 : pll_code.m_code) << RT5658_PLL_M_SFT |
		pll_code.m_bp << RT5658_PLL_M_BP_SFT);

	rt5658->pll_in = freq_in;
	rt5658->pll_out = freq_out;
	rt5658->pll_src = Source;

	return 0;
}

static int rt5658_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask,
			unsigned int rx_mask, int slots, int slot_width)
{
	struct snd_soc_codec *codec = dai->codec;
	unsigned int val = 0;
printk("%s\n", __func__);
	if (rx_mask || tx_mask)
		val |= (1 << 15);

	switch (slots) {
	case 4:
		val |= (1 << 10);
		val |= (1 << 8);
		break;
	case 6:
		val |= (2 << 10);
		val |= (2 << 8);
		break;
	case 8:
		val |= (3 << 10);
		val |= (3 << 8);
		break;
	case 2:
		break;
	default:
		return -EINVAL;
	}

	switch (slot_width) {
	case 20:
		val |= (1 << 6);
		val |= (1 << 4);
		break;
	case 24:
		val |= (2 << 6);
		val |= (2 << 4);
		break;
	case 32:
		val |= (3 << 6);
		val |= (3 << 4);
		break;
	case 16:
		break;
	default:
		return -EINVAL;
	}

	snd_soc_update_bits(codec, RT5658_TDM_CTRL_1, 0x8ff0, val);

	return 0;
}

#define RT5658_REG_DISP_LEN 23
static ssize_t rt5658_codec_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5658_priv *rt5658 = i2c_get_clientdata(client);
	struct snd_soc_codec *codec = rt5658->codec;
	unsigned int val;
	int cnt = 0, i;

	codec->cache_bypass = 1;
	for (i = 0; i <= RT5658_ADC_R_EQ_POST_VOL; i++) {
		if (cnt + RT5658_REG_DISP_LEN >= PAGE_SIZE)
			break;
		val = snd_soc_read(codec, i);
		if (!val)
			continue;
		cnt += snprintf(buf + cnt, RT5658_REG_DISP_LEN,
				 "%04x: %04x\n", i, val);
	}

	codec->cache_bypass = 0;
	if (cnt >= PAGE_SIZE)
		cnt = PAGE_SIZE - 1;

	return cnt;
}

static ssize_t rt5658_codec_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5658_priv *rt5658 = i2c_get_clientdata(client);
	struct snd_soc_codec *codec = rt5658->codec;
	unsigned int val = 0, addr = 0;
	int i;

	pr_debug("register \"%s\" count=%d\n", buf, count);
	for (i = 0; i < count; i++) {	/*address */
		if (*(buf + i) <= '9' && *(buf + i) >= '0')
			addr = (addr << 4) | (*(buf + i) - '0');
		else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
			addr = (addr << 4) | ((*(buf + i) - 'a') + 0xa);
		else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
			addr = (addr << 4) | ((*(buf + i) - 'A') + 0xa);
		else
			break;
	}

	for (i = i + 1; i < count; i++) {
		if (*(buf + i) <= '9' && *(buf + i) >= '0')
			val = (val << 4) | (*(buf + i) - '0');
		else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
			val = (val << 4) | ((*(buf + i) - 'a') + 0xa);
		else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
			val = (val << 4) | ((*(buf + i) - 'A') + 0xa);
		else
			break;
	}
	pr_debug("addr=0x%x val=0x%x\n", addr, val);
	if (addr > RT5658_ADC_R_EQ_POST_VOL || val > 0xffff || val < 0)
		return count;

	if (i == count) {
		pr_debug("0x%02x = 0x%04x\n", addr,
			 codec->hw_read(codec, addr));
	} else {
		snd_soc_write(codec, addr, val);
	}

	return count;
}

static DEVICE_ATTR(codec_reg, 0666, rt5658_codec_show, rt5658_codec_store);

static ssize_t rt5658_codec_adb_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5658_priv *rt5658 = i2c_get_clientdata(client);
	struct snd_soc_codec *codec = rt5658->codec;
	unsigned int val;
	int cnt = 0, i;

	for (i = 0; i < rt5658->adb_reg_num; i++) {
		if (cnt + RT5658_REG_DISP_LEN >= PAGE_SIZE)
			break;
		val = snd_soc_read(codec, rt5658->adb_reg_addr[i] & 0xffff);
		cnt += snprintf(buf + cnt, RT5658_REG_DISP_LEN, "%05x: %04x\n",
			rt5658->adb_reg_addr[i], val);
	}

	return cnt;
}

static ssize_t rt5658_codec_adb_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt5658_priv *rt5658 = i2c_get_clientdata(client);
	struct snd_soc_codec *codec = rt5658->codec;
	unsigned int value = 0;
	int i = 2, j = 0;

	if (buf[0] == 'R' || buf[0] == 'r') {
		while (j < 0x100 && i < count) {
			rt5658->adb_reg_addr[j] = 0;
			value = 0;
			for ( ; i < count; i++) {
				if (*(buf + i) <= '9' && *(buf + i) >= '0')
					value = (value << 4) | (*(buf + i) - '0');
				else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
					value = (value << 4) | ((*(buf + i) - 'a')+0xa);
				else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
					value = (value << 4) | ((*(buf + i) - 'A')+0xa);
				else
					break;
			}
			i++;

			rt5658->adb_reg_addr[j] = value;
			j++;
		}
		rt5658->adb_reg_num = j;
	} else if (buf[0] == 'W' || buf[0] == 'w') {
		while (j < 0x100 && i < count) {
			/* Get address */
			rt5658->adb_reg_addr[j] = 0;
			value = 0;
			for ( ; i < count; i++) {
				if (*(buf + i) <= '9' && *(buf + i) >= '0')
					value = (value << 4) | (*(buf + i) - '0');
				else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
					value = (value << 4) | ((*(buf + i) - 'a')+0xa);
				else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
					value = (value << 4) | ((*(buf + i) - 'A')+0xa);
				else
					break;
			}
			i++;
			rt5658->adb_reg_addr[j] = value;

			/* Get value */
			rt5658->adb_reg_value[j] = 0;
			value = 0;
			for ( ; i < count; i++) {
				if (*(buf + i) <= '9' && *(buf + i) >= '0')
					value = (value << 4) | (*(buf + i) - '0');
				else if (*(buf + i) <= 'f' && *(buf + i) >= 'a')
					value = (value << 4) | ((*(buf + i) - 'a')+0xa);
				else if (*(buf + i) <= 'F' && *(buf + i) >= 'A')
					value = (value << 4) | ((*(buf + i) - 'A')+0xa);
				else
					break;
			}
			i++;
			rt5658->adb_reg_value[j] = value;

			j++;
		}

		rt5658->adb_reg_num = j;

		for (i = 0; i < rt5658->adb_reg_num; i++) {
			snd_soc_write(codec,
				rt5658->adb_reg_addr[i] & 0xffff,
				rt5658->adb_reg_value[i]);
		}

	}

	return count;
}

static DEVICE_ATTR(codec_reg_adb, 0664, rt5658_codec_adb_show, rt5658_codec_adb_store);

static int rt5658_set_bias_level(struct snd_soc_codec *codec,
			enum snd_soc_bias_level level)
{
	pr_debug("%s: level = %d\n", __FUNCTION__, level);

	switch (level) {
	case SND_SOC_BIAS_PREPARE:
		snd_soc_update_bits(codec, RT5658_DIG_MISC,
			RT5658_DIG_GATE_CTRL, RT5658_DIG_GATE_CTRL);
		snd_soc_update_bits(codec, RT5658_PWR_DIG_1, RT5658_PWR_LDO,
			RT5658_PWR_LDO);
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_1, RT5658_PWR_MB |
			RT5658_PWR_VREF1 | RT5658_PWR_VREF2, RT5658_PWR_MB |
			RT5658_PWR_VREF1 | RT5658_PWR_VREF2);
		msleep(20);
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_1,
			RT5658_PWR_FV1 | RT5658_PWR_FV2,
			RT5658_PWR_FV1 | RT5658_PWR_FV2);
		break;

	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, RT5658_PWR_DIG_1, RT5658_PWR_LDO, 0);
		snd_soc_update_bits(codec, RT5658_PWR_ANLG_1, RT5658_PWR_MB |
			RT5658_PWR_VREF1 | RT5658_PWR_VREF2 | RT5658_PWR_FV1 |
			RT5658_PWR_FV2, 0);
		snd_soc_update_bits(codec, RT5658_DIG_MISC,
			RT5658_DIG_GATE_CTRL, 0);
		break;

	default:
		break;
	}
	codec->dapm.bias_level = level;

	return 0;
}

static int rt5658_reg_init(struct snd_soc_codec *codec)
{
	int i;

	for (i = 0; i < RT5658_INIT_REG_LEN; i++)
		snd_soc_write(codec, init_list[i].reg, init_list[i].def);

	return 0;
}

static int rt5658_probe(struct snd_soc_codec *codec)
{
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);
#ifndef FOR_LATEST_VERSION
	int ret;
#endif
	pr_debug("%s\n", __FUNCTION__);

	rt5658->codec = codec;

#ifndef FOR_LATEST_VERSION
	ret = snd_soc_codec_set_cache_io(codec, 16, 16, SND_SOC_I2C);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}
#endif
	rt5658_reg_init(codec);
	rt5658_set_bias_level(codec, SND_SOC_BIAS_OFF);
	ret = device_create_file(codec->dev, &dev_attr_codec_reg);
	if (ret != 0) {
		dev_err(codec->dev,
			"Failed to create codex_reg sysfs files: %d\n", ret);
		return ret;
	}

	ret = device_create_file(codec->dev, &dev_attr_codec_reg_adb);
	if (ret != 0) {
		dev_err(codec->dev,
			"Failed to create codec_reg_adb sysfs files: %d\n", ret);
		return ret;
	}

	return 0;
}

static int rt5658_remove(struct snd_soc_codec *codec)
{
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);

	regmap_write(rt5658->regmap, RT5658_RESET, 0);
	device_remove_file(codec->dev, &dev_attr_codec_reg);
	device_remove_file(codec->dev, &dev_attr_codec_reg_adb);

	return 0;
}

#ifdef CONFIG_PM
static int rt5658_suspend(struct snd_soc_codec *codec)
{
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);

	regcache_cache_only(rt5658->regmap, true);
	regcache_mark_dirty(rt5658->regmap);
	return 0;
}

static int rt5658_resume(struct snd_soc_codec *codec)
{
	struct rt5658_priv *rt5658 = snd_soc_codec_get_drvdata(codec);

	regcache_cache_only(rt5658->regmap, false);
	regcache_sync(rt5658->regmap);

	return 0;
}
#else
#define rt5658_suspend NULL
#define rt5658_resume NULL
#endif

#define RT5658_STEREO_RATES SNDRV_PCM_RATE_8000_96000
#define RT5658_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S8)

struct snd_soc_dai_ops rt5658_aif_dai_ops = {
	.hw_params = rt5658_hw_params,
	.set_fmt = rt5658_set_dai_fmt,
	.set_sysclk = rt5658_set_dai_sysclk,
	.set_tdm_slot = rt5658_set_tdm_slot,
	.set_pll = rt5658_set_dai_pll,
	//.set_bclk_ratio = rt5658_set_bclk_ratio, //Bard: TODO implement rt5658_set_bclk_ratio for MX0077 bit 13, and bclk_ms
};

struct snd_soc_dai_driver rt5658_dai[] = {
	{
		.name = "rt5658-aif1",
		.id = RT5658_AIF1,
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5658_STEREO_RATES,
			.formats = RT5658_FORMATS,
		},
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5658_STEREO_RATES,
			.formats = RT5658_FORMATS,
		},
		.ops = &rt5658_aif_dai_ops,
	},
	{
		.name = "rt5658-aif2",
		.id = RT5658_AIF2,
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5658_STEREO_RATES,
			.formats = RT5658_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5658_STEREO_RATES,
			.formats = RT5658_FORMATS,
		},
		.ops = &rt5658_aif_dai_ops,
	},
	{
		.name = "rt5658-aif3",
		.id = RT5658_AIF3,
		.playback = {
			.stream_name = "AIF3 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5658_STEREO_RATES,
			.formats = RT5658_FORMATS,
		},
		.capture = {
			.stream_name = "AIF3 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5658_STEREO_RATES,
			.formats = RT5658_FORMATS,
		},
		.ops = &rt5658_aif_dai_ops,
	},
};

static struct snd_soc_codec_driver soc_codec_dev_rt5658 = {
	.probe = rt5658_probe,
	.remove = rt5658_remove,
	.suspend = rt5658_suspend,
	.resume = rt5658_resume,
	.set_bias_level = rt5658_set_bias_level,
	.idle_bias_off = true,
	.controls = rt5658_snd_controls,
	.num_controls = ARRAY_SIZE(rt5658_snd_controls),
	.dapm_widgets = rt5658_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(rt5658_dapm_widgets),
	.dapm_routes = rt5658_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(rt5658_dapm_routes),
};


static const struct regmap_config rt5658_regmap = {
	.reg_bits = 16,
	.val_bits = 16,
	.max_register = 0x0400,
	.volatile_reg = rt5658_volatile_register,
	.readable_reg = rt5658_readable_register,
	.cache_type = REGCACHE_RBTREE,
	.reg_defaults = rt5658_reg,
	.num_reg_defaults = ARRAY_SIZE(rt5658_reg),
};

static const struct i2c_device_id rt5658_i2c_id[] = {
	{ "rt5658", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, rt5658_i2c_id);

static int rt5658_i2c_probe(struct i2c_client *i2c,
		    const struct i2c_device_id *id)
{
	struct rt5658_platform_data *pdata = dev_get_platdata(&i2c->dev);
	struct rt5658_priv *rt5658;
	int ret;
	unsigned int val;

	pr_info("Codec driver version %s\n", VERSION);

	rt5658 = devm_kzalloc(&i2c->dev, sizeof(struct rt5658_priv),
		GFP_KERNEL);

	if (NULL == rt5658)
		return -ENOMEM;

	rt5658->i2c = i2c;
	i2c_set_clientdata(i2c, rt5658);

	if (pdata)
		rt5658->pdata = *pdata;

	rt5658->regmap = devm_regmap_init_i2c(i2c, &rt5658_regmap);
	if (IS_ERR(rt5658->regmap)) {
		ret = PTR_ERR(rt5658->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	regmap_read(rt5658->regmap, RT5658_DEVICE_ID, &val);
	if (val != DEVICE_ID) {
		dev_err(&i2c->dev,
			"Device with ID register %x is not rt5658\n", val);
		return -ENODEV;
	}

	regmap_write(rt5658->regmap, RT5658_RESET, 0);

	/* SPK init */
	regmap_write(rt5658->regmap, RT5658_MICBIAS_2, 0x0380); /*RC 25MHz turn on*/
	regmap_write(rt5658->regmap, RT5658_CLASSD_0, 0x2021);
	regmap_write(rt5658->regmap, RT5658_DIG_MISC, 0x0001);
	regmap_write(rt5658->regmap, RT5658_CLASSD_CTRL_1, 0x0260); /*Powen on Class D debounece clock*/
	regmap_write(rt5658->regmap, RT5658_PWR_DIG_1, 0x8380);
	regmap_write(rt5658->regmap, RT5658_PWR_DIG_2, 0x0400);
	regmap_write(rt5658->regmap, RT5658_PWR_ANLG_1, 0xa23e);
	msleep(20);
	regmap_write(rt5658->regmap, RT5658_PWR_ANLG_1, 0xf23e);
	regmap_write(rt5658->regmap, RT5658_MONO_GAIN, 0x0103);
	regmap_write(rt5658->regmap, RT5658_CLASSD_1, 0x0803);
	regmap_write(rt5658->regmap, RT5658_CLASSD_2, 0x0554); /*Slow Vcm*/
	regmap_write(rt5658->regmap, RT5658_CLASSD_8, 0x1028);
	regmap_write(rt5658->regmap, RT5658_CLASSD_10, 0x7cea);
	/* Headphone init*/
	regmap_write(rt5658->regmap, RT5658_HP_CHARGE_PUMP_2, 0x0073);
	regmap_write(rt5658->regmap, RT5658_BIAS_CUR_CTRL_8, 0xa702);
	regmap_write(rt5658->regmap, RT5658_HP_CHARGE_PUMP_1, 0x0e16);
	regmap_write(rt5658->regmap, RT5658_DEPOP_1, 0x0009);
	regmap_write(rt5658->regmap, RT5658_PWR_DIG_1, 0x8f80); /*for spk and hp*/
	/* Enalbe K ADC Power and Clock (hp and spk)*/
	regmap_write(rt5658->regmap, RT5658_CAL_REC, 0x0909);
	regmap_write(rt5658->regmap, RT5658_PWR_ANLG_3, 0x0180);
	regmap_write(rt5658->regmap, RT5658_CALIB_ADC_CTRL, 0x3c05);
	regmap_update_bits(rt5658->regmap, RT5658_HP_CALIB_CTRL_2, 0x0001,
		0x0001);
	/* Manual K ADC Offset (hp)*/
	regmap_write(rt5658->regmap, RT5658_HP_CALIB_CTRL_1, 0x4900);
	regmap_write(rt5658->regmap, RT5658_HP_CALIB_CTRL_7, 0x0016);
	regmap_update_bits(rt5658->regmap, RT5658_HP_CALIB_CTRL_1, 0x8000,
		0x8000);
	msleep(100);
	/* Manual K Internal Path Offset (hp)*/
	regmap_write(rt5658->regmap, RT5658_HP_VOL, 0x0000);
	regmap_write(rt5658->regmap, RT5658_HP_CALIB_CTRL_1, 0x4500);
	regmap_write(rt5658->regmap, RT5658_HP_CALIB_CTRL_7, 0x001f);
	regmap_update_bits(rt5658->regmap, RT5658_HP_CALIB_CTRL_1, 0x8000,
		0x8000);
	/* Start Calibration (spk)*/
	regmap_write(rt5658->regmap, RT5658_SPK_DC_CAILB_CTRL_3, 0x0000);
	regmap_write(rt5658->regmap, RT5658_CLASSD_0, 0x0021);
	regmap_update_bits(rt5658->regmap, RT5658_SPK_DC_CAILB_CTRL_1, 0x8000,
		0x8000);
	msleep(600);
	/* Power off K ADC (hp and spk)*/
	regmap_write(rt5658->regmap, RT5658_CAL_REC, 0x0808);
	regmap_write(rt5658->regmap, RT5658_PWR_ANLG_3, 0x0000);
	regmap_write(rt5658->regmap, RT5658_CALIB_ADC_CTRL, 0x2005);
	regmap_write(rt5658->regmap, RT5658_HP_CALIB_CTRL_7, 0x0000);
	regmap_update_bits(rt5658->regmap, RT5658_HP_CALIB_CTRL_2, 0x0001,
		0x0000);

	pr_debug("%s: dmic1_data_pin = %d, dmic2_data_pin =%d",	__FUNCTION__,
		rt5658->pdata.dmic1_data_pin, rt5658->pdata.dmic2_data_pin);

	/* line in diff mode*/
	if (rt5658->pdata.in1_diff)
		regmap_update_bits(rt5658->regmap, RT5658_IN1_IN2,
			RT5658_IN1_DF_MASK, RT5658_IN1_DF_MASK);
	if (rt5658->pdata.in3_diff)
		regmap_update_bits(rt5658->regmap, RT5658_IN3_IN4,
			RT5658_IN3_DF_MASK, RT5658_IN3_DF_MASK);
	if (rt5658->pdata.in4_diff)
		regmap_update_bits(rt5658->regmap, RT5658_IN3_IN4,
			RT5658_IN4_DF_MASK, RT5658_IN4_DF_MASK);

	/* DMIC pin*/
	if (rt5658->pdata.dmic1_data_pin != RT5658_DMIC1_NULL ||
		rt5658->pdata.dmic2_data_pin != RT5658_DMIC2_NULL) {
		regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
			RT5658_GP2_PIN_MASK, RT5658_GP2_PIN_DMIC1_SCL);

		switch (rt5658->pdata.dmic1_data_pin) {
		case RT5658_DMIC1_DATA_IN2N:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_1_DP_MASK, RT5658_DMIC_1_DP_IN2N);
			break;

		case RT5658_DMIC1_DATA_GPIO5:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_1_DP_MASK, RT5658_DMIC_1_DP_GPIO5);
			regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
				RT5658_GP5_PIN_MASK, RT5658_GP5_PIN_DMIC1_SDA);
			break;

		case RT5658_DMIC1_DATA_GPIO9:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_1_DP_MASK, RT5658_DMIC_1_DP_GPIO9);
			regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
				RT5658_GP9_PIN_MASK, RT5658_GP9_PIN_DMIC1_SDA);
			break;

		case RT5658_DMIC1_DATA_GPIO11:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_1_DP_MASK, RT5658_DMIC_1_DP_GPIO11);
			regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
				RT5658_GP11_PIN_MASK,
				RT5658_GP11_PIN_DMIC1_SDA);
			break;

		default:
			pr_debug("no DMIC1\n");
			break;
		}

		switch (rt5658->pdata.dmic2_data_pin) {
		case RT5658_DMIC2_DATA_IN2P:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_2_DP_MASK, RT5658_DMIC_2_DP_IN2P);
			break;

		case RT5658_DMIC2_DATA_GPIO6:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_2_DP_MASK, RT5658_DMIC_2_DP_GPIO6);
			regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
				RT5658_GP6_PIN_MASK, RT5658_GP6_PIN_DMIC2_SDA);
			break;

		case RT5658_DMIC2_DATA_GPIO10:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_2_DP_MASK, RT5658_DMIC_2_DP_GPIO10);
			regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
				RT5658_GP10_PIN_MASK, RT5658_GP10_PIN_DMIC2_SDA);
			break;

		case RT5658_DMIC2_DATA_GPIO12:
			regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
				RT5658_DMIC_2_DP_MASK, RT5658_DMIC_2_DP_GPIO12);
			regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
				RT5658_GP12_PIN_MASK,
				RT5658_GP12_PIN_DMIC2_SDA);
			break;

		default:
			pr_debug("no DMIC2\n");
			break;

		}
	}
	else {
		regmap_update_bits(rt5658->regmap, RT5658_GPIO_CTRL_1,
			RT5658_GP2_PIN_MASK | RT5658_GP5_PIN_MASK |
			RT5658_GP9_PIN_MASK | RT5658_GP11_PIN_MASK |
			RT5658_GP6_PIN_MASK | RT5658_GP10_PIN_MASK |
			RT5658_GP12_PIN_MASK,
			RT5658_GP2_PIN_GPIO2 | RT5658_GP5_PIN_GPIO5 |
			RT5658_GP9_PIN_GPIO9 | RT5658_GP11_PIN_GPIO11 |
			RT5658_GP6_PIN_GPIO6 | RT5658_GP10_PIN_GPIO10 |
			RT5658_GP12_PIN_GPIO12);
		regmap_update_bits(rt5658->regmap, RT5658_DMIC_CTRL_1,
			RT5658_DMIC_1_DP_MASK | RT5658_DMIC_2_DP_MASK,
			RT5658_DMIC_1_DP_IN2N | RT5658_DMIC_2_DP_IN2P);
	}

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_rt5658,
			rt5658_dai, ARRAY_SIZE(rt5658_dai));
	if (ret < 0)
		kfree(rt5658);

	return ret;
}

static int rt5658_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);

	return 0;
}

struct i2c_driver rt5658_i2c_driver = {
	.driver = {
		.name = "rt5658",
		.owner = THIS_MODULE,
	},
	.probe = rt5658_i2c_probe,
	.remove   = rt5658_i2c_remove,
	.id_table = rt5658_i2c_id,
};
static int __init rt5658_modinit(void)
{
	return i2c_add_driver(&rt5658_i2c_driver);
}

module_init(rt5658_modinit);

static void __exit rt5658_modexit(void)
{
	i2c_del_driver(&rt5658_i2c_driver);
}

module_exit(rt5658_modexit);


MODULE_DESCRIPTION("ASoC RT5658 driver");
MODULE_AUTHOR("Bard Liao <bardliao@realtek.com>");
MODULE_LICENSE("GPL v2");
