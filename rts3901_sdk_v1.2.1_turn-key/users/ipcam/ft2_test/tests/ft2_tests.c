/*
 * Realtek Semiconductor Corp.
 *
 * ft2_test_tests.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>

#include "tester.h"
#include "ft2errno.h"
#include "ft2log.h"

#include "ft2_test_fake.h"
#include "ft2_test_isp.h"
#include "ft2_test_h264.h"
#include "ft2_test_jpeg.h"
#include "ft2_test_sd.h"
#include "ft2_test_gpio.h"
#include "ft2_test_i2c.h"
#include "ft2_test_uart02.h"
#include "ft2_test_rtc.h"
#include "ft2_test_audio.h"
#include "ft2_test_aes.h"
#include "ft2_test_usb.h"
#include "ft2_test_stop.h"
#include "ft2_test_disk.h"

struct ft2_test_item g_testitems[] = {
	{
		.test_name = "fake",
		.test_id = 0xff,
		.check = ft2_fake_check,
		.runonce = ft2_fake_runonce,
	},
	{
		.test_name = "stop",
		.test_id = 0x00,
		.check = ft2_stop_check,
		.runonce = ft2_stop_runonce,
	},
	{
		.test_name = "isp ",
		.test_id = 1,
		.check = ft2_isp_check,
		.runonce = ft2_isp_runonce,
		.cleanup = ft2_isp_cleanup,
	},
	{
		.test_name = "h264",
		.test_id = 2,
		.check = ft2_h264_check,
		.runonce = ft2_h264_runonce,
		.cleanup = ft2_h264_cleanup,
	},
	{
                .test_name = "jpeg",
                .test_id = 3,
                .check = ft2_jpeg_check,
                .runonce = ft2_jpeg_runonce,
                .cleanup = ft2_jpeg_cleanup,
        },
	{
		.test_name = "aud ",
		.test_id = 4,
		.check = ft2_audio_check,
		.runonce = ft2_audio_runonce,
		.cleanup = ft2_audio_cleanup,
	},
	{
		.test_name = "sdc ",
		.test_id = 5,
		.check = ft2_sd_check,
		.runonce = ft2_sd_runonce,
	},
	{
		.test_name = "usb ",
		.test_id = 6,
		.check = ft2_usb_check,
		.runonce = ft2_usb_runonce,
	},
	{
		.test_name = "aes ",
		.test_id = 7,
		.check = ft2_aes_check,
		.runonce = ft2_aes_runonce,
	},
	{
		.test_name = "gpio",
		.test_id = 8,
		.check = ft2_gpio_check,
		.runonce = ft2_gpio_runonce,
	},
	{
		.test_name = "uart",
		.test_id = 9,
		.check = ft2_uart02_check,
		.runonce = ft2_uart02_runonce,
	},
	{
		.test_name = "i2c ",
		.test_id = 10,
		.check = ft2_i2c_check,
		.runonce = ft2_i2c_runonce,
	},
	{
		.test_name = "rtc ",
		.test_id = 11,
		.check = ft2_rtc_check,
		.runonce = ft2_rtc_runonce,
	},
	{
		.test_name = "disk",
		.test_id = 12,
		.check = ft2_disk_check,
		.runonce = ft2_disk_runonce,
	},
};

int add_test_items(FT2TesterInst inst)
{
	if (!inst)
		return -FT2_NULL_POINT;

	return ft2_register_test_item(inst, g_testitems,
			sizeof(g_testitems)/sizeof(struct ft2_test_item));
}
