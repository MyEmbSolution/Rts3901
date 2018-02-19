/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_soc.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _RTS_CAMERA_SOC_H
#define _RTS_CAMERA_SOC_H

#include "rts_camera_soc_regs.h"

#define RTSCAM_SOC_VIDEO_NR_B			51

#define RTSCAM_SOC_CMD_TIMEOUT_MIN		300
#define RTSCAM_SOC_CMD_TIMEOUT_MAX		5000
#define RTSCAM_SOC_CMD_TIMEOUT			500

#define RTSCAM_SOC_CMD_DATA_REG_NUM		8
#define RTSCAM_SOC_CMD_DIRECTION_MASK		0x80

#define RTSCAM_SOC_CMD_STATUS_STATUS_BIT	31
#define RTSCAM_SOC_CMD_STATUS_RESULT_BIT	30
#define RTSCAM_SOC_CMD_STATUS_ERROR_BIT		16
#define RTSCAM_SOC_CMD_STATUS_ERROR_MASK	0xff0000

#define RTSCAM_SOC_MAX_STREAM_NUM		5
#define RTSCAM_SOC_MJPEG_STRM_IDX		(RTSCAM_SOC_MAX_STREAM_NUM - 1)
#define RTSCAM_SOC_TD_INDEX			5
#define RTSCAM_SOC_LDC_INDEX			6

#define RTSCAM_SOC_MAX_SLOT_NUM			4
#define RTSCAM_SOC_HW_SLOT_NUM			2

#define RTSCAM_SOC_FPS_UNIT			10000000

#define RTSCAM_SOC_MIN_W			80
#define RTSCAM_SOC_MIN_H			60
#define RTSCAM_SOC_STEP_W			2
#define RTSCAM_SOC_STEP_H			2

#define RTSCAM_SOC_MTD_NUM			5

#define RTSCAM_SOC_FRAME_HEADER_LENGTH		8

#define RTSCAM_SOC_ISP_FUNC_TD_BIT		0
#define RTSCAM_SOC_ISP_FUNC_LDC_BIT		1

enum rtscam_soc_cmd_data_dir {
	host_to_device = 0,
	device_to_host = 1
};

enum RTS_CPU_LOCK_MCU_MASK {
	CPU_LOCK_MCU_REQUEST = 0x1,
	CPU_LOCK_MCU_DONE = 0x2,
	CPU_LOCK_MCU_RELEASE = 0x4,
	CPU_LOCK_MCU_CACHE_MISS = 0x8,
};

enum hclk_src_type {
	HCLK_SRC_480M = 0x00,
	HCLK_SRC_288M = 0x01,
	HCLK_SRC_324M = 0x02,
	HCLK_SRC_297M = 0x03,
	HCLK_SRC_216M = 0x04,
};

#define RTS_CMDCODE_GET_DEVICE_DES			((0x01 << 8) | 0x81)
#define RTS_CMDCODE_GET_FRAME_INTERVAL_DES		((0x01 << 8) | 0x82)
#define RTS_CMDCODE_GET_VS_FMT_DES			((0x01 << 8) | 0x83)
#define RTS_CMDCODE_GET_ISP_PROCESS_DES			((0x01 << 8) | 0x84)
#define RTS_CMDCODE_GET_CAMERA_DES			((0x01 << 8) | 0x85)
#define RTS_CMDCODE_GET_RTK_EXT_CTL_DES			((0x01 << 8) | 0x86)
#define RTS_CMDCODE_GET_HCLK_CFG_DES			((0x01 << 8) | 0x87)
#define RTS_CMDCODE_GET_FW_VERSION			((0x01 << 8) | 0x88)
#define RTS_CMDCODE_GET_SNR_POWER			((0x01 << 8) | 0x89)
#define RTS_CMDCODE_GET_API_VERSION			((0x01 << 8) | 0x8a)
#define RTS_CMDCODE_GET_SNR_POWER_SEQ			((0x01 << 8) | 0x8b)

#define RTS_CMDCODE_SET_FPS				((0x02 << 8) | 0x01)
#define RTS_CMDCODE_START_PREVIEW			((0x02 << 8) | 0x02)
#define RTS_CMDCODE_STOP_PREVIEW			((0x02 << 8) | 0x03)
#define RTS_CMDCODE_PAUSE_VIDEO				((0x02 << 8) | 0x05)
#define RTS_CMDCODE_RESUME_VIDEO			((0x02 << 8) | 0x06)
#define RTS_CMDCODE_SET_FORMAT				((0x02 << 8) | 0x07)
#define RTS_CMDCODE_SNR_PWRON_BEF			((0x02 << 8) | 0x09)

#define RTS_CMDCODE_GET_SNR_FMT				((0x06 << 8) | 0x81)
#define RTS_CMDCODE_SET_ISP_FUNC			((0x0a << 8) | 0x01)
#define RTS_CMDCODE_GET_ISP_FUNC			((0x0a << 8) | 0x81)

#define RTS_CMDCODE_GET_GPIO_USE_STATUS			((0x0c << 8) | 0x81)
#define RTS_CMDCODE_SET_GPIO_DIR			((0x0c << 8) | 0x02)
#define RTS_CMDCODE_GET_GPIO_DIR			((0x0c << 8) | 0x82)
#define RTS_CMDCODE_SET_GPIO_VALUE			((0x0c << 8) | 0x03)
#define RTS_CMDCODE_GET_GPIO_VALUE			((0x0c << 8) | 0x83)

struct rtscam_soc_device_descriptor {
	u8 length;
	u8 type;
	u16 hwversion;
	u16 fwversion;
	u8 streamnum;
	u8 frmivalnum;
};

struct rtscam_soc_frmival_descriptor {
	u8 frmivalnum;
	u32 *frmivals;
};

struct rtscam_soc_stream_format_descriptor {
	u8 streamid;
	u8 length;
	u8 type;
	u8 format;
	u16 width;
	u16 height;
};

struct rtscam_soc_unit_descriptor {
	u8 length;
	u8 type;
	u8 controlsize;
	u8 bmcontrols[32];

	unsigned int ncontrols;
	struct rtscam_video_ctrl *controls;
};

struct rtscam_soc_hclk_descriptor {
	u8 length;
	u8 type;
	u8 src_type;
	u8 ssc_flag;
};

struct rtscam_soc_fw_version_t {
	u8 header;
	u8 length;
	u32 magictag;
	u16 ic_name;
	u16 vid;
	u16 pid;
	u32 fw_ver;
	u32 cus_ver;
	u8 reserved[16];
};

#define RTSCAM_SOC_SNR_POWER_COUNT		3
struct rtscam_soc_snr_power {
	u8 power_io;
	u8 power_analog;
	u8 power_core;
};

struct rtscam_soc_snr_power_seq {
	u8 seqs[RTSCAM_SOC_SNR_POWER_COUNT];
	u16 wait[RTSCAM_SOC_SNR_POWER_COUNT - 1];
};

struct rtscam_soc_api_version {
	u8 main;
	u8 sub;
};

/* A.1 Descriptor Types */
#define RTSCAM_SOC_UNDEFINED_TYPE		0x0
#define RTSCAM_SOC_DEVICE_TYPE			0x1
#define RTSCAM_SOC_VS_INTERFACE_TYPE		0x2
#define RTSCAM_SOC_ISP_PROCESSING_TYPE		0x3
#define RTSCAM_SOC_CAMERA_TYPE			0x4
#define RTSCAM_SOC_RTK_EXT_CTL_TYPE		0x5
#define RTSCAM_SOC_HCLK_CFG_TYPE		0x6

/* A.2 Video Stream Data Types */
#define RTSCAM_SOC_VS_UNDEFINED			0x0
#define RTSCAM_SOC_VS_FORMAT_UNCOMPRESSED	0x4
#define RTSCAM_SOC_VS_FRAME_UNCOMPRESSED	0x5
#define RTSCAM_SOC_VS_FORMAT_MJPEG		0x6
#define RTSCAM_SOC_VS_FRAME_MJPEG		0x7
#define RTSCAM_SOC_VS_FORMAT_H264		0x13
#define RTSCAM_SOC_VS_FRAME_H264		0x14

/* format type */
#define RTSCAM_SOC_FORMAT_TYPE_YUV420_SEMIPLANAR			0x01
#define RTSCAM_SOC_FORMAT_TYPE_YUV422_SEMIPLANAR			0x02
#define RTSCAM_SOC_FORMAT_TYPE_MJPG					0x04

/* A.4 ISP Processing Control Selectors */
#define RTSCAM_SOC_PU_CONTROL_UNDEFINED					0x00
#define RTSCAM_SOC_PU_BACKLIGHT_COMPENSATION_CONTROL			0x01
#define RTSCAM_SOC_PU_BRIGHTNESS_CONTROL				0x02
#define RTSCAM_SOC_PU_CONTRAST_CONTROL					0x03
#define RTSCAM_SOC_PU_GAIN_CONTROL					0x04
#define RTSCAM_SOC_PU_POWER_LINE_FREQUENCY_CONTROL			0x05
#define RTSCAM_SOC_PU_HUE_CONTROL					0x06
#define RTSCAM_SOC_PU_SATURATION_CONTROL				0x07
#define RTSCAM_SOC_PU_SHARPNESS_CONTROL					0x08
#define RTSCAM_SOC_PU_GAMMA_CONTROL					0x09
#define RTSCAM_SOC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL			0x0A
#define RTSCAM_SOC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL		0x0B
#define RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_CONTROL			0x0C
#define RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL		0x0D
#define RTSCAM_SOC_PU_DIGITAL_MULTIPLIER_CONTROL			0x0E
#define RTSCAM_SOC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL			0x0F
#define RTSCAM_SOC_PU_HUE_AUTO_CONTROL					0x10
#define RTSCAM_SOC_PU_ANALOG_VIDEO_STANDARD_CONTROL			0x11
#define RTSCAM_SOC_PU_ANALOG_LOCK_STATUS_CONTROL			0x12
#define RTSCAM_SOC_PU_CONTRAST_AUTO_CONTROL				0x13

/* A.4 Camera Control Selectors */
#define RTSCAM_SOC_CT_CONTROL_UNDEFINED					0x00
#define RTSCAM_SOC_CT_SCANNING_MODE_CONTROL				0x01
#define RTSCAM_SOC_CT_AE_MODE_CONTROL					0x02
#define RTSCAM_SOC_CT_AE_PRIORITY_CONTROL				0x03
#define RTSCAM_SOC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL			0x04
#define RTSCAM_SOC_CT_EXPOSURE_TIME_RELATIVE_CONTROL			0x05
#define RTSCAM_SOC_CT_FOCUS_ABSOLUTE_CONTROL				0x06
#define RTSCAM_SOC_CT_FOCUS_RELATIVE_CONTROL				0x07
#define RTSCAM_SOC_CT_FOCUS_AUTO_CONTROL				0x08
#define RTSCAM_SOC_CT_IRIS_ABSOLUTE_CONTROL				0x09
#define RTSCAM_SOC_CT_IRIS_RELATIVE_CONTROL				0x0A
#define RTSCAM_SOC_CT_ZOOM_ABSOLUTE_CONTROL				0x0B
#define RTSCAM_SOC_CT_ZOOM_RELATIVE_CONTROL				0x0C
#define RTSCAM_SOC_CT_PANTILT_ABSOLUTE_CONTROL				0x0D
#define RTSCAM_SOC_CT_PANTILT_RELATIVE_CONTROL				0x0E
#define RTSCAM_SOC_CT_ROLL_ABSOLUTE_CONTROL				0x0F
#define RTSCAM_SOC_CT_ROLL_RELATIVE_CONTROL				0x10
#define RTSCAM_SOC_CT_PRIVACY_CONTROL					0x11
#define RTSCAM_SOC_CT_FOCUS_SIMPLE_CONTROL				0x12
#define RTSCAM_SOC_CT_WINDOW_CONTROL					0x13
#define RTSCAM_SOC_CT_REGION_OF_INTEREST_CONTROL			0x14

/* A.6 Rtk Extended Control Selectors */
#define RTSCAM_SOC_RTK_EXT_ISP_SPECIAL_EFFECT_CTL			0x01
#define RTSCAM_SOC_RTK_EXT_EVCOM_CTL					0x02
#define RTSCAM_SOC_RTK_EXT_CTE_CTL					0x03
#define RTSCAM_SOC_RTK_EXT_AE_LOCK_CTL					0x04
#define RTSCAM_SOC_RTK_EXT_AWB_LOCK_CTL					0x05
#define RTSCAM_SOC_RTK_EXT_AF_LOCK_CTL					0x06
#define RTSCAM_SOC_RTK_EXT_LED_TORCH_CTL				0x07
#define RTSCAM_SOC_RTK_EXT_LED_FLASH_CTL				0x08
#define RTSCAM_SOC_RTK_EXT_ISO_CTL					0x09
#define RTSCAM_SOC_RTK_EXT_PHOTO_SCENEMODE_CTL				0x0A
#define RTSCAM_SOC_RTK_EXT_ROI_MODE_CTL					0x13
/* CT_REGION_OF_INTEREST_CONTROL defined in UVC1.5 */
#define RTSCAM_SOC_RTK_EXT_ROI_CTL					0x14
#define RTSCAM_SOC_RTK_EXT_3ASTS_CTL					0x15
#define RTSCAM_SOC_RTK_EXT_BURSTMODE_CTL				0x16
#define RTSCAM_SOC_RTK_EXT_STILLMODE_CTL				0x17
#define RTSCAM_SOC_RTK_EXT_STILLSETTING_CTL				0x18
#define RTSCAM_SOC_RTK_EXT_IDEAEYE_SENSITIVITY_CTL			0x19
#define RTSCAM_SOC_RTK_EXT_IDEAEYE_STATUS_CTL				0x1A
#define RTSCAM_SOC_RTK_EXT_IDEAEYE_MODE_CTL				0x1B
#define RTSCAM_SOC_RTK_EXT_IQ_MODE_CTL					0x1d
#define RTSCAM_SOC_RTK_EXT_IQ_PARAMETER_CTL				0x1e
#define RTSCAM_SOC_RTK_EXT_GAIN_CONTROL					0x1f

#endif

