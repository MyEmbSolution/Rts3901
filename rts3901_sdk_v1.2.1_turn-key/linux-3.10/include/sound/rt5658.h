/*
 * linux/sound/rt5658.h -- Platform data for RT5658
 *
 * Copyright 2013 Realtek Microelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_SND_RT5658_H
#define __LINUX_SND_RT5658_H

enum rt5658_dmic1_data_pin {
	RT5658_DMIC1_NULL,
	RT5658_DMIC1_DATA_IN2N,
	RT5658_DMIC1_DATA_GPIO5,
	RT5658_DMIC1_DATA_GPIO9,
	RT5658_DMIC1_DATA_GPIO11,
};

enum rt5658_dmic2_data_pin {
	RT5658_DMIC2_NULL,
	RT5658_DMIC2_DATA_IN2P,
	RT5658_DMIC2_DATA_GPIO6,
	RT5658_DMIC2_DATA_GPIO10,
	RT5658_DMIC2_DATA_GPIO12,
};

struct rt5658_platform_data {
	bool in1_diff;
	bool in3_diff;
	bool in4_diff;

	enum rt5658_dmic1_data_pin dmic1_data_pin;
	enum rt5658_dmic2_data_pin dmic2_data_pin;
};

#endif

