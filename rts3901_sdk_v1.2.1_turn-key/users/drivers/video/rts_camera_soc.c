/*
 * Camera driver for Realtek soc camera
 *
 * rts_soc_dev.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 * Ming Qian, Realsil Software Engineering, <ming_qian@realsil.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <asm/unaligned.h>
#include <media/videobuf2-memops.h>
#include <linux/firmware.h>

#include <linux/rts_sysctl.h>

#include <linux/platform_data/camera-rtsoc.h>
#include <linux/regulator/consumer.h>

#include "linux/rts_camera_soc.h"
#include "rts_camera.h"
#include "rts_camera_soc.h"
#include "rts_camera_mem.h"
#include "rts-dma-contig.h"
#include "rts_hw_id.h"

#define RTS_SOC_CAMERA_DRV_NAME		"rts_soc_camera"

#define RTS_SOC_MEMORY_DEV_NAME		"rtsmem"
#define RTS_SOC_CAM_DEV_NAME		"rtscam"
#define RTS_SOC_CTRL_DEV_NAME		"rtscamctrl"

#define RTS_SOC_CLK_ON_NEED		0


#define RTS_MCU_FW_MASK_DDR		0x2
#define RTS_MCU_FW_MASK_USER		0x4

#if defined CONFIG_RTS_MCU_FW_CUSTOM
#define RTS_MCU_FW_FLAG		(RTS_MCU_FW_MASK_DDR | RTS_MCU_FW_MASK_USER)
#elif defined CONFIG_RTS_MCU_FW_IN_DDR
#define RTS_MCU_FW_FLAG		RTS_MCU_FW_MASK_DDR
#else
#define RTS_MCU_FW_FLAG		RTS_MCU_FW_MASK_DDR
#endif

#if !(RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_USER)
#include <bspchip.h>
#include <generated/mtd_partition_def.h>
#endif

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_USER)
#define FIRMWARE_RTS_ISP   "isp.fw"
MODULE_FIRMWARE(FIRMWARE_RTS_ISP);
#endif

#ifdef RTS_MTD_MCU_FW_SIZE
#define RTS_MCU_FW_SIZE		RTS_MTD_MCU_FW_SIZE
#else
#define RTS_MCU_FW_SIZE		131072
#endif

#define RTS_MCU_IQTABLE_OFFSET		0x7000
#define RTS_MCU_IQTABLE_HEADER_SIZE	8

struct rtscam_soc_dma_buffer {
	struct list_head	list;
	struct device		*dev;
	void			*vaddr;
	unsigned long		size;
	dma_addr_t		phy_addr;
	int			initialized;
	u32			index;

	/*MMAP related*/
	struct vb2_vmarea_handler handler;
	atomic_t refcount;
};

struct rtscam_soc_slot_info {
	u8 slot_index;
	u8 slot_num;
	struct rtscam_video_buffer *slots[RTSCAM_SOC_HW_SLOT_NUM];
};

struct rtscam_soc_skip_info {
	int m;
	int n;
	int flag;
	int count;
	int index;
};

#ifdef CONFIG_REGULATOR_RTP
struct rtscam_soc_pwr {
	struct regulator *pwr;
	int voltage;
	char name[64];
	int flag;
};
#endif

struct rtscam_soc_dev {
	struct device		*dev;
	struct clk		*clk;
	void __iomem	*base;

	unsigned long iostart;
	unsigned int iosize;

	int initialized;
	atomic_t init_count;

	atomic_t mcu_count;
	int mcu_state;

	struct completion	cmd_completion;
	struct mutex		cmd_lock;

	struct vb2_alloc_ctx    *alloc_ctx;
	const struct vb2_mem_ops *mem_ops;

	struct rtscam_soc_device_descriptor dev_desc;
	struct rtscam_soc_unit_descriptor entities[3];
	struct rtscam_soc_hclk_descriptor hclk_desc;

	struct rtscam_soc_api_version api_version;

	u32 sensor_frmival;

	struct rtscam_video_device rvdev;

	struct rtscam_soc_slot_info slot_info[RTSCAM_SOC_MAX_STREAM_NUM];
	struct rtscam_soc_skip_info skip_info[RTSCAM_SOC_MAX_STREAM_NUM];

	struct rtscam_soc_dma_buffer isp_headers;

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
	struct rtscam_soc_dma_buffer fw_buffer;
	int fw_update;
#endif

	unsigned long mtd_status;

	struct list_head user_memorys;

	struct rtscam_ge_device *mem_dev;
	struct rtscam_ge_device *cam_dev;
	struct rtscam_ge_device *ctrl_dev;

	struct rtscam_soc_fw_version_t fw_version;

	struct rtscam_soc_dma_buffer td_buf;
	int td_enable;

	struct rtscam_soc_dma_buffer ldc_buf;
	int ldc_enable;

#ifdef CONFIG_REGULATOR_RTP
	atomic_t pwr_use;
	struct rtscam_soc_pwr pio;
	struct rtscam_soc_pwr panalog;
	struct rtscam_soc_pwr pcore;

	struct rtscam_soc_pwr *pwrs[RTSCAM_SOC_SNR_POWER_COUNT];
	int pwr_wait[RTSCAM_SOC_SNR_POWER_COUNT - 1];
#endif

	struct rtscam_soc_pdata *pdata;
	u32 isp_capibility;
	struct rtscam_mem_info rtsmem;

	u32 mcu_timeout;

	char name[PLATFORM_NAME_SIZE];
	kernel_ulong_t devtype;
	unsigned long support;
};

struct rtscam_soc_snr_fmt {
	u16 width;
	u16 height;
	u8 snr_fmt;
};

static struct rtscam_soc_dev *m_rsocdev;

static struct rtscam_video_format_xlate m_rtscam_soc_formats[] = {
	{
		.index = 0,
		.name = "SEMIPLANAR YCBCR 4:2:0",
		.fourcc = V4L2_PIX_FMT_NV12,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.field = V4L2_FIELD_NONE,
		.bpp = 12,
		.is_yuv = true,
		.rts_code = RTSCAM_SOC_FORMAT_TYPE_YUV420_SEMIPLANAR,
	},
	{
		.index = 1,
		.name = "MJPEG",
		.fourcc = V4L2_PIX_FMT_MJPEG,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.field = V4L2_FIELD_NONE,
		.bpp = 12,
		.is_yuv = false,
		.rts_code = RTSCAM_SOC_FORMAT_TYPE_MJPG,
	},
	{
		.index = 2,
		.name = "SEMIPLANAR YCBCR 4:2:2",
		.fourcc = V4L2_PIX_FMT_NV16,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.field = V4L2_FIELD_NONE,
		.bpp = 16,
		.is_yuv = true,
		.rts_code = RTSCAM_SOC_FORMAT_TYPE_YUV422_SEMIPLANAR,
	},
};

static u32 m_rtscam_soc_max_fps = 300;
static u32 m_rtscam_soc_step_fps = 5;

static struct rtscam_video_ctrl_menu exposure_auto_controls[] = {
	{ 8, "Auto Mode" },
	{ 1, "Manual Mode" },
};

static struct rtscam_video_ctrl_info m_rtscam_soc_ctrls[] = {
	{
		.index = 0,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_BRIGHTNESS_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 1,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_CONTRAST_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 2,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_HUE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 3,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_SATURATION_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 4,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_SHARPNESS_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 5,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_GAMMA_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 6,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 7,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
		.size = 6,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 8,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_BACKLIGHT_COMPENSATION_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 9,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_GAIN_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 10,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_POWER_LINE_FREQUENCY_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 11,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_HUE_AUTO_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_CUR |
		RTS_CTRL_FLAG_GET_DEF |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 12,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 13,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_CUR |
		RTS_CTRL_FLAG_GET_DEF |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 14,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_DIGITAL_MULTIPLIER_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 15,
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 0,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_SCANNING_MODE_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_CUR |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 1,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_AE_MODE_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_CUR |
		RTS_CTRL_FLAG_GET_DEF |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 2,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_AE_PRIORITY_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_CUR |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 3,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
		.size = 4,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 4,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_EXPOSURE_TIME_RELATIVE_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_CUR |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 5,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_FOCUS_ABSOLUTE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 6,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_FOCUS_RELATIVE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 7,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_IRIS_ABSOLUTE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 8,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_IRIS_RELATIVE_CONTROL,
		.size = 1,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_CUR |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 9,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_ZOOM_ABSOLUTE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 10,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_ZOOM_RELATIVE_CONTROL,
		.size = 3,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 11,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_PANTILT_ABSOLUTE_CONTROL,
		.size = 8,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 12,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_PANTILT_RELATIVE_CONTROL,
		.size = 4,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 13,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_ROLL_ABSOLUTE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 14,
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_ROLL_RELATIVE_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
	{
		.index = 30,
		.unit = RTSCAM_SOC_RTK_EXT_CTL_TYPE,
		.selector = RTSCAM_SOC_RTK_EXT_GAIN_CONTROL,
		.size = 2,
		.flags = RTS_CTRL_FLAG_SET_CUR |
		RTS_CTRL_FLAG_GET_RANGE |
		RTS_CTRL_FLAG_GET_INFO,
	},
};

static struct rtscam_video_ctrl_mapping m_rtscam_soc_ctrl_mappings[] = {
	{
		.id = V4L2_CID_BRIGHTNESS,
		.name = "Brightness",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_BRIGHTNESS_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id = V4L2_CID_CONTRAST,
		.name = "Contrast",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_CONTRAST_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_HUE,
		.name = "Hue",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_HUE_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id = V4L2_CID_SATURATION,
		.name = "Saturation",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_SATURATION_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_SHARPNESS,
		.name = "Sharpness",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_SHARPNESS_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_GAMMA,
		.name = "Gamma",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_GAMMA_CONTROL,
		.size  = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.name = "White Balance Temperature",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_RED_BALANCE,
		.name = "White Balance Red Component",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector =  RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = RTS_V4L2_CID_GREEN_BALANCE,
		.name = "White Balance Green Component",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector =  RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
		.size = 16,
		.offset = 16,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_BLUE_BALANCE,
		.name = "White Balance Blue Component",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector =  RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
		.size = 16,
		.offset = 32,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_BACKLIGHT_COMPENSATION,
		.name = "Backlight Compensation",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector =  RTSCAM_SOC_PU_BACKLIGHT_COMPENSATION_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_GAIN,
		.name = "Gain",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_GAIN_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_POWER_LINE_FREQUENCY,
		.name = "Power Line Frequency",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_POWER_LINE_FREQUENCY_CONTROL,
		.size = 2,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_HUE_AUTO,
		.name = "Hue, Auto",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_HUE_AUTO_CONTROL,
		.size = 1,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_BOOLEAN,
		.data_type = RTS_CTRL_DATA_TYPE_BOOLEAN,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.name = "White Balance Temperature, Auto",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL,
		.size = 8,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = RTS_V4L2_CID_AUTO_WHITE_BALANCE_COMPONENT,
		.name = "White Balance Component, Auto",
		.unit = RTSCAM_SOC_ISP_PROCESSING_TYPE,
		.selector = RTSCAM_SOC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL,
		.size = 1,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_BOOLEAN,
		.data_type = RTS_CTRL_DATA_TYPE_BOOLEAN,
	},
	/* Digital Multiplier, not defined */
	/* Digital Multiplier Limit, not defined */
	/* Scanning Mode, not defined */
	{
		.id = V4L2_CID_EXPOSURE_AUTO,
		.name = "Exposure, Auto",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_AE_MODE_CONTROL,
		.size = 4,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_MENU,
		.data_type = RTS_CTRL_DATA_TYPE_BITMASK,
		.menu_info = exposure_auto_controls,
		.menu_count = ARRAY_SIZE(exposure_auto_controls),
	},
	{
		.id = V4L2_CID_EXPOSURE_AUTO_PRIORITY,
		.name = "Exposure, Auto Priority",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_AE_PRIORITY_CONTROL,
		.size = 1,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_BOOLEAN,
		.data_type = RTS_CTRL_DATA_TYPE_BOOLEAN,
	},
	{
		.id = V4L2_CID_EXPOSURE_ABSOLUTE,
		.name = "Exposure (Absolute)",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
		.size = 32,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	/* Exposure Time (Relative) */
	{
		.id = V4L2_CID_FOCUS_ABSOLUTE,
		.name = "Focus (absolute)",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_FOCUS_ABSOLUTE_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	/* Focus (Relative) */
	{
		.id = V4L2_CID_IRIS_ABSOLUTE,
		.name = "Iris, Absolute",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_IRIS_ABSOLUTE_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id = V4L2_CID_IRIS_RELATIVE,
		.name = "Iris, Relative",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_IRIS_RELATIVE_CONTROL,
		.size = 8,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id = V4L2_CID_ZOOM_ABSOLUTE,
		.name = "Zoom, Absolute",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_ZOOM_ABSOLUTE_CONTROL,
		.size = 16,
		.offset = 0 ,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
	/* Zoom (Relative) */
	{
		.id = V4L2_CID_PAN_ABSOLUTE,
		.name = "Pan (Absolute)",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_PANTILT_ABSOLUTE_CONTROL,
		.size = 32,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id = V4L2_CID_TILT_ABSOLUTE,
		.name = "Tilt (Absolute)",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_PANTILT_ABSOLUTE_CONTROL,
		.size = 32,
		.offset = 32,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_SIGNED,
	},
	/* PanTilt (Relative) */
	{
		.id = RTS_V4L2_CID_ROLL_ABSOLUTE,
		.name = "Roll (Absolute)",
		.unit = RTSCAM_SOC_CAMERA_TYPE,
		.selector = RTSCAM_SOC_CT_ROLL_ABSOLUTE_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id = RTS_V4L2_CID_AE_GAIN,
		.name = "AE Gain",
		.unit = RTSCAM_SOC_RTK_EXT_CTL_TYPE,
		.selector = RTSCAM_SOC_RTK_EXT_GAIN_CONTROL,
		.size = 16,
		.offset = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = RTS_CTRL_DATA_TYPE_UNSIGNED,
	},
};

enum {
	SOC_ISP_SUPPORT_MIPI = (1 << 0),
	SOC_ISP_SUPPORT_WDOG = (1 << 1),
};

static unsigned long __get_soc_camera_support(unsigned long hw_id)
{
	unsigned long support = SOC_ISP_SUPPORT_MIPI | SOC_ISP_SUPPORT_WDOG;

	switch (hw_id) {
	case TYPE_RLE0745:
		support = 0;
		break;
	default:
		break;
	}

	return support;;
}

int rtscam_soc_attach(struct rtscam_soc_dev *rsocdev);
int rtscam_soc_detach(struct rtscam_soc_dev *rsocdev);

static u32 rts_read_reg(struct rtscam_soc_dev *rsocdev, off_t reg)
{
	u32 value;
	value = ioread32(rsocdev->base + reg);

	rtsprintk(RTS_TRACE_REG, "[Read]reg = 0x%x, value = 0x%x\n",
	          (unsigned int)(rsocdev->base + reg), value);

	return le32_to_cpu(value);
}

static void rts_write_reg(struct rtscam_soc_dev *rsocdev,
                          u32 value, off_t reg)
{
	rtsprintk(RTS_TRACE_REG, "[Write]reg = 0x%x, value = 0x%x\n",
	          (unsigned int)(rsocdev->base + reg), value);

	iowrite32(cpu_to_le32(value), rsocdev->base + reg);
}

static u32 rtscam_read_reg(struct rtscam_video_stream *stream, off_t reg)
{
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	return rts_read_reg(rsocdev, reg);
}

static void rtscam_write_reg(struct rtscam_video_stream *stream,
                             u32 value, off_t reg)
{
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	return rts_write_reg(rsocdev, value, reg);
}

static u32 rtscam_soc_get_mcu_start_addr(struct rtscam_soc_dev *rsocdev)
{
	u32 mcu_start_addr = 0;

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
	if (rsocdev->fw_buffer.initialized) {
#if (RTS_MCU_FW_FLAG == RTS_MCU_FW_MASK_DDR)
		mcu_start_addr = BSP_SPI_FLASH_PADDR + RTS_MTD_MCU_FW_OFFSET;
		if (rsocdev->fw_update) {
			rsocdev->fw_update = 0;
			rts_dma_copy(rsocdev->fw_buffer.phy_addr,
			             mcu_start_addr,
			             RTS_MTD_MCU_FW_SIZE);
			rtsprintk(RTS_TRACE_INFO, "copy fw from flash\n");
		}
#endif
		if (rsocdev->fw_update) {
			rtsprintk(RTS_TRACE_ERROR, "pls update fw first\n");
			return 0;
		}
		return rsocdev->fw_buffer.phy_addr;
	}
#endif

	return mcu_start_addr;
}

static void rtscam_soc_dma_free_coherent(struct rtscam_soc_dev *rsocdev,
        struct rtscam_soc_dma_buffer *buffer)
{
	if (!buffer->initialized)
		return;

	/*
	dma_free_coherent(rsocdev->dev, buffer->size,
			buffer->vaddr, buffer->phy_addr);
			*/
	rtscam_mem_free(&rsocdev->rtsmem, buffer->size,
	                buffer->vaddr, buffer->phy_addr);
	put_device(buffer->dev);
	buffer->initialized = 0;
}

static int rtscam_soc_dma_alloc_coherent(struct rtscam_soc_dev *rsocdev,
        struct rtscam_soc_dma_buffer *buffer, gfp_t gfp)
{
	unsigned long size = PAGE_ALIGN(buffer->size);

	if (buffer->initialized)
		return 0;

	if (buffer->size == 0)
		return -EINVAL;

	/*
	buffer->vaddr = dma_alloc_coherent(rsocdev->dev, size,
			&buffer->phy_addr, gfp);
			*/
	buffer->vaddr = rtscam_mem_alloc(&rsocdev->rtsmem, size, &buffer->phy_addr);
	if (!buffer->vaddr) {
		rtsprintk(RTS_TRACE_ERROR, "rtscam_mem_alloc fail\n");
		return -ENOMEM;
	}

	buffer->size = size;
	buffer->dev = get_device(rsocdev->dev);
	buffer->initialized = 1;

	return 0;
}

static u8 __get_cmd_data_direction(struct rtscam_soc_cmd_stru *cmd)
{
	if (cmd->cmdcode & RTSCAM_SOC_CMD_DIRECTION_MASK)
		return device_to_host;
	else
		return host_to_device;
}

static int __check_mcu_cmd_status(u32 status)
{
	return test_bit(RTSCAM_SOC_CMD_STATUS_STATUS_BIT, (void *)&status);
}

static int __check_mcu_cmd_result(u32 status)
{
	return test_bit(RTSCAM_SOC_CMD_STATUS_RESULT_BIT, (void *)&status);
}

static u32 __get_mcu_cmd_error(u32 status)
{
	u32 error_type = status & RTSCAM_SOC_CMD_STATUS_ERROR_MASK;

	error_type = error_type >> RTSCAM_SOC_CMD_STATUS_ERROR_BIT;

	return error_type;
}

static u32 m_host_to_device_data_regs[RTSCAM_SOC_CMD_DATA_REG_NUM] = {
	RTS_REG_DATA0_HOST_TO_MCU,
	RTS_REG_DATA1_HOST_TO_MCU,
	RTS_REG_DATA2_HOST_TO_MCU,
	RTS_REG_DATA3_HOST_TO_MCU,
	RTS_REG_DATA4_HOST_TO_MCU,
	RTS_REG_DATA5_HOST_TO_MCU,
	RTS_REG_DATA6_HOST_TO_MCU,
	RTS_REG_DATA7_HOST_TO_MCU
};

static u32 m_device_to_host_data_regs[RTSCAM_SOC_CMD_DATA_REG_NUM] = {
	RTS_REG_DATA0_MCU_TO_HOST,
	RTS_REG_DATA1_MCU_TO_HOST,
	RTS_REG_DATA2_MCU_TO_HOST,
	RTS_REG_DATA3_MCU_TO_HOST,
	RTS_REG_DATA4_MCU_TO_HOST,
	RTS_REG_DATA5_MCU_TO_HOST,
	RTS_REG_DATA6_MCU_TO_HOST,
	RTS_REG_DATA7_MCU_TO_HOST
};

static int __rtscam_soc_exec_command(struct rtscam_soc_dev *rsocdev,
                                     struct rtscam_soc_cmd_stru *cmd, u32 timeout)
{
	u8 data_dir;
	int loop = 0;
	u32 status;
	u32 value;
	u32 cmd0;
	u32 cmd1;
	int ret = 0;

	if (NULL == rsocdev || NULL == cmd)
		return -EINVAL;

	mutex_lock(&rsocdev->cmd_lock);

	data_dir = __get_cmd_data_direction(cmd);
	if (cmd->length > RTSCAM_SOC_CMD_DATA_REG_NUM * 4) {
		rtsprintk(RTS_TRACE_ERROR,
		          "data length(%d) is out of range(%d)\n",
		          cmd->length, RTSCAM_SOC_CMD_DATA_REG_NUM * 4);
		ret = -EINVAL;
		goto exit;
	}
	if (cmd->length > 0 && NULL == cmd->buf) {
		rtsprintk(RTS_TRACE_ERROR, "buf is NULL\n");
		ret = -EINVAL;
		goto exit;
	}

	/*check mcu status*/
	for (loop = 0; loop < 300; loop++) {
		status = rts_read_reg(rsocdev, RTS_REG_MCU_CMD_STATUS);
		if (!__check_mcu_cmd_status(status))
			break;
		usleep_range(9900, 10000);
	}
	if (__check_mcu_cmd_status(status)) {
		rtsprintk(RTS_TRACE_ERROR,
		          "MCU is busy now.(0x%08x), cmd(0x%04x) fail\n",
		          status, cmd->cmdcode);
		ret = -EINVAL;
		goto exit;
	}

	if (host_to_device == data_dir) {
		/*write data*/
		for (loop = 0; loop * 4 < cmd->length; loop++) {
			int i;
			int size = cmd->length - loop * 4;
			if (size > 4)
				size = 4;
			value = 0;
			for (i = 0; i < size; i++)
				value |= (cmd->buf[loop * 4 + i] << (8 * i));
			rts_write_reg(rsocdev, value,
			              m_host_to_device_data_regs[loop]);
		}
	}
	/*write cmd1*/
	cmd1 = (cmd->param << 16) | cmd->addr;
	rts_write_reg(rsocdev, cmd1, RTS_REG_CMD1_HOST_TO_MCU);

	cmd0 = (cmd->cmdcode << 16) | (cmd->index << 8) | cmd->length;
	rts_write_reg(rsocdev, cmd0, RTS_REG_CMD0_HOST_TO_MCU);

	init_completion(&rsocdev->cmd_completion);
	wait_for_completion_timeout(&rsocdev->cmd_completion,
	                            timeout * HZ / 1000);

	status = rts_read_reg(rsocdev, RTS_REG_MCU_CMD_STATUS);
	if (__check_mcu_cmd_status(status)) {
		rtsprintk(RTS_TRACE_ERROR,
		          "exec cmd(0x%04x) timeout, status = 0x%08x\n",
		          cmd->cmdcode, status);
		ret = -EINVAL;
		goto exit;
	}

	if (__check_mcu_cmd_result(status)) {
		cmd->error_type = __get_mcu_cmd_error(status);
		rtsprintk(RTS_TRACE_ERROR,
		          "exec cmd(0x%04x) fail, error type = %d\n",
		          cmd->cmdcode, cmd->error_type);
		ret = -EINVAL;
		goto exit;
	} else {
		cmd->error_type = 0;
	}

	if (device_to_host == data_dir) {
		/* read data */
		for (loop = 0; loop * 4 < cmd->length; loop++) {
			int i;
			int size = cmd->length - loop * 4;
			if (size > 4)
				size = 4;
			value = rts_read_reg(rsocdev,
			                     m_device_to_host_data_regs[loop]);

			for (i = 0; i < size; i++)
				cmd->buf[loop * 4 + i] = (value >> (i * 8)) & 0xff;
		}
	}

	ret = 0;
exit:
	mutex_unlock(&rsocdev->cmd_lock);
	return ret;
}

static int rtscam_soc_exec_command(struct rtscam_soc_dev *rsocdev,
                                   struct rtscam_soc_cmd_stru *cmd)
{
	rtsprintk(RTS_TRACE_COMMAND,
	          "soc exec command %04x %02x %02x %04x %04x\n",
	          cmd->cmdcode, cmd->index, cmd->length,
	          cmd->param, cmd->addr);
	return __rtscam_soc_exec_command(rsocdev, cmd, rsocdev->mcu_timeout);
}

int rtscam_soc_get_device_des(struct rtscam_soc_dev *rsocdev,
                              struct rtscam_soc_device_descriptor *desc)
{
	u8 buf[8] = {0};
	int ret;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_DEVICE_DES,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (NULL == desc)
		return -EINVAL;

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (0 == ret) {
		desc->length = buf[0];
		desc->type = buf[1];
		desc->hwversion = get_unaligned_le16(&buf[2]);
		desc->fwversion = get_unaligned_le16(&buf[4]);
		desc->streamnum = buf[6];
		desc->frmivalnum = buf[7];
	}

	return ret;

}

int rtscam_soc_get_frmival_desc(struct rtscam_soc_dev *rsocdev,
                                struct rtscam_soc_frmival_descriptor *desc)
{
	u8 buf[RTSCAM_SOC_CMD_DATA_REG_NUM * 4];
	int index = 0;
	int num = 0;
	int ret = -EINVAL;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_FRAME_INTERVAL_DES,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (NULL == desc)
		return -EINVAL;

	while (index < desc->frmivalnum) {
		if (desc->frmivalnum - index > RTSCAM_SOC_CMD_DATA_REG_NUM)
			num = RTSCAM_SOC_CMD_DATA_REG_NUM;
		else
			num = desc->frmivalnum - index;
		rcmd.length = 4 * num;
		rcmd.addr = 4 * index;

		ret = rtscam_soc_exec_command(rsocdev, &rcmd);
		if (0 != ret)
			break;

		if (desc->frmivals) {
			int i;
			u32 frmival;
			for (i = 0; i < num; i++) {
				frmival = get_unaligned_le32(&buf[4 * i]);
				rtsprintk(RTS_TRACE_DEBUG,
				          "frmival : %d\n", frmival);
				*(desc->frmivals + index + i) = frmival;
			}
		}

		index += num;
	}

	return ret;
}

int rtscam_soc_get_vs_format_desc(struct rtscam_soc_dev *rsocdev,
                                  struct rtscam_soc_stream_format_descriptor *desc)
{
	u8 buf[7];
	int ret = 0;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_VS_FMT_DES,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (NULL == desc)
		return -EINVAL;

	rcmd.index = desc->streamid;
	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (0 == ret) {
		desc->length = buf[0];
		desc->type = buf[1];
		desc->format = buf[2];
		desc->width = get_unaligned_le16(&buf[3]);
		desc->height =  get_unaligned_le16(&buf[5]);
	}

	return ret;
}

static int rtscam_soc_get_unit_desc(struct rtscam_soc_dev *rsocdev,
                                    u16 cmdcode, struct rtscam_soc_unit_descriptor *desc)
{
	u8 buf[32];
	int ret = 0;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = cmdcode,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (NULL == desc)
		return -EINVAL;

	if (RTS_CMDCODE_GET_ISP_PROCESS_DES == cmdcode ||
	    RTS_CMDCODE_GET_CAMERA_DES == cmdcode)
		rcmd.length = 6;

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (0 == ret) {
		desc->length = buf[0];
		desc->type = buf[1];
		desc->controlsize = buf[2];
		if (desc->controlsize > 0)
			memcpy(desc->bmcontrols, buf + 3, desc->controlsize);
	}

	return ret;
}

int rtscam_soc_get_isp_processing_desc(struct rtscam_soc_dev *rsocdev,
                                       struct rtscam_soc_unit_descriptor *desc)
{
	u16 cmdcode = RTS_CMDCODE_GET_ISP_PROCESS_DES;

	return rtscam_soc_get_unit_desc(rsocdev, cmdcode, desc);
}

int rtscam_soc_get_camera_desc(struct rtscam_soc_dev *rsocdev,
                               struct rtscam_soc_unit_descriptor *desc)
{
	u16 cmdcode = RTS_CMDCODE_GET_CAMERA_DES;

	return rtscam_soc_get_unit_desc(rsocdev, cmdcode, desc);
}

int rtscam_soc_get_ext_controls_desc(struct rtscam_soc_dev *rsocdev,
                                     struct rtscam_soc_unit_descriptor *desc)
{
	u16 cmdcode = RTS_CMDCODE_GET_RTK_EXT_CTL_DES;

	return rtscam_soc_get_unit_desc(rsocdev, cmdcode, desc);
}

int rtscam_soc_get_hclk_desc(struct rtscam_soc_dev *rsocdev,
                             struct rtscam_soc_hclk_descriptor *desc)
{
	u8 buf[3];
	int ret = 0;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_HCLK_CFG_DES,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (NULL == desc)
		return -EINVAL;

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (0 == ret) {
		desc->length = buf[0];
		desc->type = buf[1];
		desc->src_type = buf[2] & 0x7f;
		desc->ssc_flag = (buf[2] >> 7) & 1;
	}

	return ret;
}

int rtscam_soc_get_fw_version(struct rtscam_soc_dev *rsocdev,
                              struct rtscam_soc_fw_version_t *version)
{
	u8 buf[32];
	int ret = 0;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_FW_VERSION,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (NULL == version)
		return -EINVAL;

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (0 == ret) {
		version->header = buf[0];
		version->length = buf[1];
		version->magictag = get_unaligned_be32(buf + 2);
		version->ic_name = get_unaligned_be16(buf + 6);
		version->vid = get_unaligned_be16(buf + 8);
		version->pid = get_unaligned_be16(buf + 10);
		if ((version->header >> 4) == 3) {
			version->fw_ver = get_unaligned_be32(buf + 12);
			version->cus_ver = get_unaligned_be32(buf + 16);
			memcpy(version->reserved, buf + 20, 12);
		} else if ((version->header >> 4) == 2) {
			version->fw_ver = get_unaligned_be16(buf + 12);
			version->cus_ver = get_unaligned_be16(buf + 14);
			memcpy(version->reserved, buf + 16, 16);
		}
	}

	return ret;
}

int rtscam_soc_get_snr_power(struct rtscam_soc_dev *rsocdev,
                             struct rtscam_soc_snr_power *power)
{
	u8 buf[3];
	int ret = 0;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_SNR_POWER,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (!power)
		return -EINVAL;

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (0 == ret) {
		power->power_io = buf[0];
		power->power_analog = buf[1];
		power->power_core = buf[2];
	}

	return ret;
}

int rtscam_soc_get_api_version(struct rtscam_soc_dev *rsocdev,
                               struct rtscam_soc_api_version *api_version)
{
	u8 buf[2];
	int ret = 0;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_API_VERSION,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	if (!api_version)
		return -EINVAL;

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (0 == ret) {
		api_version->main = buf[0];
		api_version->sub = buf[1];
	}

	return ret;
}

int rtscam_soc_get_snr_power_seq(struct rtscam_soc_dev *rsocdev,
                                 struct rtscam_soc_snr_power_seq *on,
                                 struct rtscam_soc_snr_power_seq *off)
{
	u8 buf[10];
	int ret = 0;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_SNR_POWER_SEQ,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (ret)
		return ret;

	if (on) {
		on->seqs[0] = buf[0] & 0x3;
		on->seqs[1] = (buf[0] >> 2) & 0x3;
		on->seqs[2] = (buf[0] >> 4) & 0x3;
		on->wait[0] = get_unaligned_le16(buf + 1);
		on->wait[1] = get_unaligned_le16(buf + 3);
	}
	if (off) {
		off->seqs[0] = buf[5] & 0x3;
		off->seqs[1] = (buf[5] >> 2) & 0x3;
		off->seqs[2] = (buf[5] >> 4) & 0x3;
		off->wait[0] = get_unaligned_le16(buf + 6);
		off->wait[1] = get_unaligned_le16(buf + 8);
	}

	return 0;
}

int rtscam_soc_prepare_snr_pwron(struct rtscam_soc_dev *rsocdev)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_SNR_PWRON_BEF,
		.index = 0,
		.length = 0,
		.param = 0,
		.addr = 0,
		.buf = NULL,
		.error_type = 0
	};

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_set_fps(struct rtscam_soc_dev *rsocdev, u32 frmival)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_SET_FPS,
		.index = 0,
		.length = 0,
		.param = RTSCAM_H_WORD(frmival),
		.addr = RTSCAM_L_WORD(frmival),
		.buf = NULL,
		.error_type = 0
	};

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_start_preview(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_START_PREVIEW,
		.index = stream->streamid,
		.length = 0,
		.param = stream->user_width,
		.addr = stream->user_height,
		.buf = NULL,
		.error_type = 0
	};
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_stop_preview(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_STOP_PREVIEW,
		.index = stream->streamid,
		.length = 0,
		.param = 0,
		.addr = 0,
		.buf = NULL,
		.error_type = 0
	};
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_pause_video(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_PAUSE_VIDEO,
		.index = stream->streamid,
		.length = 0,
		.param = 0,
		.addr = 0,
		.buf = NULL,
		.error_type = 0
	};
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_resume_video(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_RESUME_VIDEO,
		.index = stream->streamid,
		.length = 0,
		.param = 0,
		.addr = 0,
		.buf = NULL,
		.error_type = 0
	};
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_set_format(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_SET_FORMAT,
		.index = stream->streamid,
		.length = 0,
		.param = stream->rts_code,
		.addr = 0,
		.buf = NULL,
		.error_type = 0
	};
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_ctrl_query_ctrl(struct rtscam_soc_dev *rsocdev,
                               u8 unit, u8 query, u8 selector, u8 length, u8 *data)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = (unit << 8) | query,
		.index = selector,
		.length = length,
		.param = 0,
		.addr = 0,
		.buf = data,
		.error_type = 0
	};

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_get_snr_fmt(struct rtscam_soc_dev *rsocdev,
                           struct rtscam_soc_snr_fmt *sfmt)
{
	u8 buf[5];
	int ret;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_SNR_FMT,
		.index = 0,
		.length = sizeof(buf),
		.param = 0,
		.addr = 0,
		.buf = buf,
		.error_type = 0
	};

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (!ret && sfmt) {
		sfmt->width = get_unaligned_le16(buf);
		sfmt->height = get_unaligned_le16(buf + 2);
		sfmt->snr_fmt = buf[4];
	}

	return ret;
}

int rtscam_soc_get_isp_func(struct rtscam_soc_dev *rsocdev, u8 *func)
{
	u8 val;
	int ret;
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_ISP_FUNC,
		.index = 0,
		.length = 1,
		.param = 0,
		.addr = 0,
		.buf = &val,
		.error_type = 0
	};

	ret = rtscam_soc_exec_command(rsocdev, &rcmd);
	if (!ret && func)
		*func = val;

	return ret;
}

int rtscam_soc_set_isp_func(struct rtscam_soc_dev *rsocdev, u8 func)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_SET_ISP_FUNC,
		.index = 0,
		.length = 1,
		.param = 0,
		.addr = 0,
		.buf = &func,
		.error_type = 0
	};

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_get_gpio_use_status(struct rtscam_soc_dev *rsocdev,
                                   u8 idx, u8 *status)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_GPIO_USE_STATUS,
		.index = idx,
		.length = 1,
		.param = 0,
		.addr = 0,
		.buf = status,
		.error_type = 0
	};

	if (idx >= RTS_MCU_GPIO_NUM)
		return -EINVAL;

	if (!status)
		return -EINVAL;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_get_gpio_direction(struct rtscam_soc_dev *rsocdev,
                                  u8 idx, u8 *direction)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_GPIO_DIR,
		.index = idx,
		.length = 1,
		.param = 0,
		.addr = 0,
		.buf = direction,
		.error_type = 0
	};

	if (idx >= RTS_MCU_GPIO_NUM)
		return -EINVAL;

	if (!direction)
		return -EINVAL;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_set_gpio_direction(struct rtscam_soc_dev *rsocdev,
                                  u8 idx, u8 direction)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_SET_GPIO_DIR,
		.index = idx,
		.length = 1,
		.param = 0,
		.addr = 0,
		.buf = &direction,
		.error_type = 0
	};

	if (idx >= RTS_MCU_GPIO_NUM)
		return -EINVAL;

	if (direction)
		direction = RTS_MCU_GPIO_OUTPUT;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_get_gpio_value(struct rtscam_soc_dev *rsocdev,
                              u8 idx, u8 *value)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_GET_GPIO_VALUE,
		.index = idx,
		.length = 1,
		.param = 0,
		.addr = 0,
		.buf = value,
		.error_type = 0
	};

	if (idx >= RTS_MCU_GPIO_NUM)
		return -EINVAL;

	if (!value)
		return -EINVAL;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_set_gpio_value(struct rtscam_soc_dev *rsocdev,
                              u8 idx, u8 value)
{
	struct rtscam_soc_cmd_stru rcmd = {
		.cmdcode = RTS_CMDCODE_SET_GPIO_VALUE,
		.index = idx,
		.length = 1,
		.param = 0,
		.addr = 0,
		.buf = &value,
		.error_type = 0
	};

	if (idx >= RTS_MCU_GPIO_NUM)
		return -EINVAL;

	if (value)
		value = RTS_MCU_GPIO_HIGH;

	return rtscam_soc_exec_command(rsocdev, &rcmd);
}

int rtscam_soc_request_mcu_gpio(int gpio)
{
	struct rtscam_soc_dev *rsocdev = m_rsocdev;
	u8 status;
	int ret;

	if (!rsocdev)
		return -EINVAL;

	if (!rsocdev->initialized)
		return -EINVAL;

	if (RTSCAM_STATE_ACTIVE != rsocdev->mcu_state)
		return -EINVAL;

	ret = rtscam_soc_get_gpio_use_status(rsocdev, gpio, &status);
	if (ret)
		return ret;

	if (status != RTS_MCU_GPIO_IDLE)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_soc_request_mcu_gpio);

int rtscam_soc_free_mcu_gpio(int gpio)
{
	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_soc_free_mcu_gpio);

int rtscam_soc_set_mcu_gpio_direction(int gpio, int dir)
{
	struct rtscam_soc_dev *rsocdev = m_rsocdev;

	if (!rsocdev)
		return -EINVAL;

	if (!rsocdev->initialized)
		return -EINVAL;

	if (RTSCAM_STATE_ACTIVE != rsocdev->mcu_state)
		return -EINVAL;

	return rtscam_soc_set_gpio_direction(rsocdev, gpio, dir);
}
EXPORT_SYMBOL_GPL(rtscam_soc_set_mcu_gpio_direction);

int rtscam_soc_get_mcu_gpio_direction(int gpio)
{
	struct rtscam_soc_dev *rsocdev = m_rsocdev;
	u8 direction;
	int ret;

	if (!rsocdev)
		return -EINVAL;

	if (!rsocdev->initialized)
		return -EINVAL;

	if (RTSCAM_STATE_ACTIVE != rsocdev->mcu_state)
		return -EINVAL;

	ret = rtscam_soc_get_gpio_direction(rsocdev, gpio, &direction);
	if (ret)
		return ret;

	return direction;
}
EXPORT_SYMBOL_GPL(rtscam_soc_get_mcu_gpio_direction);

int rtscam_soc_set_mcu_gpio_value(int gpio, int val)
{
	struct rtscam_soc_dev *rsocdev = m_rsocdev;

	if (!rsocdev)
		return -EINVAL;

	if (!rsocdev->initialized)
		return -EINVAL;

	if (RTSCAM_STATE_ACTIVE != rsocdev->mcu_state)
		return -EINVAL;

	return rtscam_soc_set_gpio_value(rsocdev, gpio, val);
}
EXPORT_SYMBOL_GPL(rtscam_soc_set_mcu_gpio_value);

int rtscam_soc_get_mcu_gpio_value(int gpio)
{
	struct rtscam_soc_dev *rsocdev = m_rsocdev;
	u8 val;
	int ret;

	if (!rsocdev)
		return -EINVAL;

	if (!rsocdev->initialized)
		return -EINVAL;

	if (RTSCAM_STATE_ACTIVE != rsocdev->mcu_state)
		return -EINVAL;

	ret = rtscam_soc_get_gpio_value(rsocdev, gpio, &val);
	if (ret)
		return ret;

	return val;
}
EXPORT_SYMBOL_GPL(rtscam_soc_get_mcu_gpio_value);

static u8 __get_stream_reg_index(struct rtscam_video_stream *stream)
{
	u8 idx;

	idx = stream->streamid;
	if (RTSCAM_SOC_FORMAT_TYPE_MJPG == stream->rts_code)
		idx = RTSCAM_SOC_MJPEG_STRM_IDX;

	return idx;
}

static struct rtscam_video_stream *__get_stream_from_reg_index(
    struct rtscam_soc_dev *rsocdev, int index)
{
	if (index < 0 || index > RTSCAM_SOC_MJPEG_STRM_IDX)
		return NULL;

	if (rsocdev->rvdev.streamnum == 0 || NULL == rsocdev->rvdev.streams)
		return NULL;

	if (RTSCAM_SOC_MJPEG_STRM_IDX == index)
		index = rsocdev->rvdev.streamnum - 1;

	return rsocdev->rvdev.streams + index;
}

static void rtscam_soc_isp_control(struct rtscam_soc_dev *rsocdev, u8 idx,
                                   int enable)
{
	u32 value;

	if (idx > 6)
		return;

	value = rts_read_reg(rsocdev, RTS_REG_ISP_CONTROL);

	if (enable)
		set_bit(idx + 8, (void *)&value);
	else
		clear_bit(idx + 8, (void *)&value);

	rts_write_reg(rsocdev, value, RTS_REG_ISP_CONTROL);
}

static void rtscam_soc_reset_stream(struct rtscam_video_stream *stream)
{
	int idx;
	u32 value;

	idx = __get_stream_reg_index(stream);
	value = rtscam_read_reg(stream, RTS_REG_ISP_CONTROL);

	set_bit(idx, (void *)&value);
	rtscam_write_reg(stream, value, RTS_REG_ISP_CONTROL);
}

static int rtscam_soc_get_mtd_status(struct rtscam_soc_dev *rsocdev, int idx)
{
	return test_and_clear_bit(idx, &rsocdev->mtd_status);
}

static void rtscam_soc_set_mtd_status(struct rtscam_soc_dev *rsocdev, int idx)
{
	set_bit(idx, &rsocdev->mtd_status);
}

static void *rtscam_soc_get_isp_header(struct rtscam_video_stream *stream,
                                       int frameid)
{
	u8 idx = __get_stream_reg_index(stream);
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;
	int offset;

	if (idx > RTSCAM_SOC_MJPEG_STRM_IDX)
		return NULL;

	if (frameid < 0 || frameid >= RTSCAM_SOC_HW_SLOT_NUM)
		return NULL;

	if (!rsocdev->isp_headers.initialized)
		return NULL;

	offset = idx * RTSCAM_SOC_MAX_SLOT_NUM + frameid;
	offset = offset * RTSCAM_SOC_FRAME_HEADER_LENGTH;

	return rsocdev->isp_headers.vaddr + offset;
}

static int __calc_coprime(int *a, int *b)
{
	int m = *a;
	int n = *b;

	if (m < 0 || n < 0)
		return 0;
	if (m == 0)
		return n;
	if (n == 0)
		return m;

	while (n != 0) {
		int tmp = m % n;
		m = n;
		n = tmp;
	}

	*a = (*a) / m;
	*b = (*b) / m;

	return m;
}

static void rtscam_soc_init_skip_info(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;
	struct rtscam_soc_skip_info *skip = NULL;

	if (stream->streamid >= rsocdev->dev_desc.streamnum)
		return;

	skip = &rsocdev->skip_info[stream->streamid];

	skip->m = RTSCAM_SOC_FPS_UNIT / rsocdev->sensor_frmival;
	skip->n = stream->user_denominator / stream->user_numerator;

	__calc_coprime(&skip->m, &skip->n);

	if (skip->m > 2 * skip->n) {
		skip->flag = 1;
	} else {
		skip->flag = 0;
		skip->n = skip->m - skip->n;
	}
	skip->count = 0;
	skip->index = 0;
}

static int rtscam_soc_skip_frame(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;
	struct rtscam_soc_skip_info *skip_info = NULL;
	int skip = 0;

	if (stream->streamid >= rsocdev->dev_desc.streamnum)
		return 0;

	skip_info = &rsocdev->skip_info[stream->streamid];

	rtsprintk(RTS_TRACE_VIDEO, "%d %d %d %d %d\n",
	          skip_info->m, skip_info->n, skip_info->flag,
	          skip_info->count, skip_info->index);

	if (0 == skip_info->n)
		return skip_info->flag;

	if ((skip_info->n - skip_info->count) * skip_info->m >=
	    (skip_info->m - skip_info->index) * skip_info->n) {
		skip = 1 - skip_info->flag;
		skip_info->count++;
	} else {
		skip = skip_info->flag;
	}

	skip_info->index++;
	if (skip_info->index % skip_info->m == 0) {
		skip_info->count = 0;
		skip_info->index = 0;
	}

	return skip;
}

static void rtscam_soc_inc_overflow(struct rtscam_soc_dev *rsocdev, int i)
{
	struct rtscam_video_stream *stream = NULL;

	stream = __get_stream_from_reg_index(rsocdev, i);
	if (stream)
		stream->overflow++;
}


static int rtscam_soc_process_mcu_irq(struct rtscam_soc_dev *rsocdev)
{
	u32 status;
	int MCU_INT_BIT_CMD = 0;
	int MCU_INT_BIT_CACHEMISS = 1;
	int MCU_INT_BIT_WATCHDOG = 2;
	int MCU_INT_BIT_MTD_BASE = 16;
	int i = 0;
	int index;
	const off_t reg = RTS_REG_INT_FLAG_MCU_TO_HOST;
	u32 mask;

	status = rts_read_reg(rsocdev, reg);

	mask = 1 << MCU_INT_BIT_CMD;
	if (status & mask) {
		rtsprintk(RTS_TRACE_COMMAND, "mcu cmd done\n");
		rts_write_reg(rsocdev, mask, reg);
		complete(&rsocdev->cmd_completion);
		return 1;
	}

	mask = 1 << MCU_INT_BIT_CACHEMISS;
	if (status & mask) {
		rtsprintk(RTS_TRACE_WARNING, "cache miss fail\n");
		rts_write_reg(rsocdev, mask, reg);
		return 1;
	}

	if (SOC_ISP_SUPPORT_WDOG & rsocdev->support) {
		mask = 1 << MCU_INT_BIT_WATCHDOG;
		if (status & mask) {
			rtsprintk(RTS_TRACE_WARNING, "mcu watchdog rst\n");
			rts_write_reg(rsocdev, mask, reg);
			return 1;
		}
	}

	/*mtd*/
	for (i = 0; i < RTSCAM_SOC_MTD_NUM; i++) {
		index = MCU_INT_BIT_MTD_BASE + i;
		mask = 1 << index;
		if (status & mask) {
			rtscam_soc_set_mtd_status(rsocdev, i);
			rts_write_reg(rsocdev, mask, reg);
			return 1;
		}
	}

	if (status) {
		rts_write_reg(rsocdev, status, reg);
		return 1;
	}

	return 0;
}

static int rtscam_soc_process_frame(struct rtscam_video_stream *stream,
                                    int frameid)
{
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;
	struct rtscam_soc_slot_info *info;
	struct rtscam_video_buffer *rbuf;
	u32 reg;
	u32 status;
	u32 mask;
	u8 idx = __get_stream_reg_index(stream);
	void *isp_header;
	unsigned long bytesused;
	int skip;

	info = &rsocdev->slot_info[stream->streamid];

	if (frameid < 0 || frameid >= info->slot_num)
		return -EINVAL;

	reg = RTS_REG_YUV_FRAME_BUFFER_STATUS_BASE + 4 * idx;
	status = rts_read_reg(rsocdev, reg);

	mask = 0xc << (4 * frameid);
	if (!(mask & status))
		return 0;

	mask = 1 << (4 * frameid + 2);
	if (mask & status) {
		rbuf = info->slots[frameid];
		info->slots[frameid] = NULL;
		rtscam_submit_buffer(stream, rbuf);

		rtsprintk(RTS_TRACE_WARNING,
		          "frame error:stream<%d> frame<%d> : 0x%08x\n",
		          stream->streamid, frameid, status);
		return -EINVAL;
	}

	rbuf = info->slots[frameid];
	if (NULL == rbuf) {
		rtsprintk(RTS_TRACE_WARNING,
		          "frameid[%d] is invalid\n", frameid);
		return -EINVAL;
	}
	info->slots[frameid] = NULL;

	skip = rtscam_soc_skip_frame(stream);
	rtsprintk(RTS_TRACE_VIDEO, "frameid = %d, skip = %d\n", frameid, skip);
	if (skip) {
		rtscam_submit_buffer(stream, rbuf);
		return 0;
	}

	isp_header = rtscam_soc_get_isp_header(stream, frameid);
	if (isp_header)
		rbuf->pts = get_unaligned_le32(isp_header + 4);

	if (RTSCAM_SOC_FORMAT_TYPE_MJPG == stream->rts_code)
		bytesused = rtscam_read_reg(stream,
		                            RTS_REG_MJPEG_FRAME_RECVED_LENGTH + 4 * frameid);
	else
		bytesused = stream->sizeimage;

	rtscam_buffer_done(stream, rbuf, bytesused);
	rtscam_submit_buffer(stream, NULL);

	return 0;
}

static int rtscam_soc_process_isp_irq(struct rtscam_soc_dev *rsocdev)
{
	const u32 reg_isp = RTS_REG_INT_FLAG_ISP_HOST;
	u32 status;
	u32 mask = 0;
	int overflow_flag = 0;
	int i;
	struct rtscam_video_stream *stream = NULL;

	status = rts_read_reg(rsocdev, reg_isp);

#define RTSCAM_SOC_TD_ISP_OVERFLOW_BIT		20
#define RTSCAM_SOC_TD_DDR_OVERFLOW_BIT		21
#define RTSCAM_SOC_ISP_HOST_RESERVED_BIT	22

	/* clear reserved isp_host interrupt */
	mask = (0xffffffff << RTSCAM_SOC_ISP_HOST_RESERVED_BIT) & 0xffffffff;
	if (status & mask) {
		rts_write_reg(rsocdev, mask, reg_isp);
		return IRQ_HANDLED;
	}

	/* check td overflow */
	mask = 1 << RTSCAM_SOC_TD_ISP_OVERFLOW_BIT;
	if (status & mask) {
		rts_write_reg(rsocdev, mask, reg_isp);
		overflow_flag = 1;
	}
	mask = 1 << RTSCAM_SOC_TD_DDR_OVERFLOW_BIT;
	if (status & mask) {
		rts_write_reg(rsocdev, mask, reg_isp);
		overflow_flag = 1;
	}

	if (overflow_flag) {
		rtsprintk(RTS_TRACE_WARNING, "td overflow : 0x%08x\n", status);
		return IRQ_HANDLED;
	}

	/*check isp overflow*/
	for (i = 0; i < RTSCAM_SOC_MAX_STREAM_NUM; i++) {
		mask = 0x7 << (4 * i);
		if (status & mask) {
			rtsprintk(RTS_TRACE_WARNING, "isp overflow : 0x%08x\n",
			          status);
			rts_write_reg(rsocdev, mask, reg_isp);
			overflow_flag = 1;
			rtscam_soc_inc_overflow(rsocdev, i);
			return IRQ_HANDLED;
		}
	}

	for (i = 0; i < RTSCAM_SOC_MAX_STREAM_NUM; i++) {
		mask = 1 << (4 * i + 3);
		if (status & mask) {
			rts_write_reg(rsocdev, mask, reg_isp);
			break;
		}
	}

	stream = __get_stream_from_reg_index(rsocdev, i);
	if (NULL == stream) {
		rtsprintk(RTS_TRACE_WARNING, "no stream found\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < RTSCAM_SOC_HW_SLOT_NUM; i++) {
		spin_lock(&stream->lock);
		rtscam_soc_process_frame(stream, i);
		spin_unlock(&stream->lock);
	}

	return IRQ_HANDLED;
}

static irqreturn_t rtscam_soc_irq(int irq, void *data)
{
	struct rtscam_soc_dev *rsocdev = data;

	if (rtscam_soc_process_mcu_irq(rsocdev))
		return IRQ_HANDLED;

	return rtscam_soc_process_isp_irq(rsocdev);

}

static void rtscam_soc_enable_interrupt(struct rtscam_soc_dev *rsocdev,
                                        int enable)
{
	u32 int_en;

	if (enable)
		int_en = 0xffffffff;
	else
		int_en = 0;

	/* MCU CMD done interrupt enable. */
	rts_write_reg(rsocdev, int_en, RTS_REG_INT_EN_MCU_TO_HOST);
	rts_write_reg(rsocdev, 0xffffffff, RTS_REG_INT_FLAG_MCU_TO_HOST);

	/* MCU interrupt to Host enable */
	rts_write_reg(rsocdev, int_en, RTS_REG_INT_EN_ISP_TO_HOST);
	rts_write_reg(rsocdev, 0xffffffff, RTS_REG_INT_FLAG_ISP_HOST);
}

static int rtscam_soc_enable_mcu(struct rtscam_soc_dev *rsocdev,
                                 int enable)
{
	struct clk *clk = rsocdev->clk;
	u32 lock_mcu_done;
	int i;

	if (enable) {
		int ret;
		u32 mcu_fw_addr;

		if (RTSCAM_STATE_ACTIVE == rsocdev->mcu_state) {
			atomic_inc(&rsocdev->mcu_count);
			return 0;
		}

		if (atomic_inc_return(&rsocdev->mcu_count) != 1)
			return 0;

		mcu_fw_addr = rtscam_soc_get_mcu_start_addr(rsocdev);
		if (!mcu_fw_addr) {
			rtsprintk(RTS_TRACE_ERROR, "can't get fw\n");
			atomic_dec(&rsocdev->mcu_count);
			return -EINVAL;
		}

		rtsprintk(RTS_TRACE_DEBUG, "mcu run in 0x%08x\n", mcu_fw_addr);

		if (TYPE_RLE0745 == RTS_SOC_HW_ID(rsocdev->devtype)) {
			rts_write_reg(rsocdev, 0x01, RTS_REG_ISP_OCP_IF_DUMMY);
			udelay(1);
			rts_write_reg(rsocdev, 0x00, RTS_REG_ISP_OCP_IF_DUMMY);
		} else {
			rts_sys_force_reset(FORCE_RESET_VIDEO);
			udelay(1);
		}

		rts_sys_force_reset(FORCE_RESET_MCU_PREPARE);

		/* set MCU code base address in serial flash */
		rts_write_reg(rsocdev, mcu_fw_addr, RTS_REG_MCU_SPI_BASE_ADDR);
		for (i = 0; i < 3; i++) {
			u32 temp = rts_read_reg(rsocdev,
			                        RTS_REG_MCU_SPI_BASE_ADDR);
			if (temp != mcu_fw_addr) {
				rtsprintk(RTS_TRACE_ERROR,
				          "mcu addr error[%d]: 0x%08x/0x%08x\n",
				          i, temp, mcu_fw_addr);
				rts_write_reg(rsocdev, mcu_fw_addr,
				              RTS_REG_MCU_SPI_BASE_ADDR);
			} else {
				break;
			}
		}
		if (i >= 3) {
			ret = -EINVAL;
			atomic_dec(&rsocdev->mcu_count);
			rtsprintk(RTS_TRACE_ERROR, "set mcu addr fail\n");
			return ret;
		}

		//rts_sys_force_reset(FORCE_RESET_MCU);

		ret = clk_prepare_enable(clk);
		if (ret) {
			atomic_dec(&rsocdev->mcu_count);
			rtsprintk(RTS_TRACE_ERROR, "enable mcu clk fail\n");
			return ret;
		}

		rts_write_reg(rsocdev,
		              CPU_LOCK_MCU_CACHE_MISS, RTS_REG_CPU_LOCK_MCU);

		if (SOC_ISP_SUPPORT_MIPI & rsocdev->support) {
			rts_sys_force_reset(FORCE_RESET_MIPI);
			udelay(1);
		}

		rts_sys_force_reset(FORCE_RESET_ISP);
		udelay(1);
		rts_sys_force_reset(FORCE_RESET_MCU_DONE);

		/*release lock mcu*/
		rts_write_reg(rsocdev,
		              CPU_LOCK_MCU_RELEASE, RTS_REG_CPU_LOCK_MCU);

		rtscam_soc_enable_interrupt(rsocdev, 1);

		init_completion(&rsocdev->cmd_completion);
		ret = wait_for_completion_timeout(&rsocdev->cmd_completion,
		                                  rsocdev->mcu_timeout * HZ / 1000);
		if (ret > 0) {
			rsocdev->mcu_state = RTSCAM_STATE_ACTIVE;
			rtsprintk(RTS_TRACE_DEBUG, "mcu initialized\n");
		} else {
			u32 temp = rts_read_reg(rsocdev,
			                        RTS_REG_MCU_SPI_BASE_ADDR);
			rtsprintk(RTS_TRACE_ERROR,
			          "mcu addr error : 0x%08x/0x%08x\n",
			          temp, mcu_fw_addr);

			atomic_dec(&rsocdev->mcu_count);
			rtsprintk(RTS_TRACE_ERROR, "enable mcu fail\n");
			/*request lock mcu*/
			rts_write_reg(rsocdev,
			              CPU_LOCK_MCU_REQUEST, RTS_REG_CPU_LOCK_MCU);
			for (i = 0; i < 300; i++) {
				lock_mcu_done = rts_read_reg(rsocdev,
				                             RTS_REG_CPU_LOCK_MCU);
				if (lock_mcu_done & CPU_LOCK_MCU_DONE) {
					rtsprintk(RTS_TRACE_DEBUG,
					          "lock mcu done\n");
					break;
				}
				usleep_range(9900, 10000);
			}

			if (i >= 300)
				rtsprintk(RTS_TRACE_ERROR,
				          "request mcu lock fail\n");

			clk_disable_unprepare(clk);

			return -EINVAL;
		}
	} else {
		if (RTSCAM_STATE_PASSIVE == rsocdev->mcu_state)
			return 0;

		if (atomic_dec_return(&rsocdev->mcu_count) != 0)
			return 0;

		rtscam_soc_enable_interrupt(rsocdev, 0);

		/*request lock mcu*/
		rts_write_reg(rsocdev,
		              CPU_LOCK_MCU_REQUEST, RTS_REG_CPU_LOCK_MCU);
		for (i = 0; i < 300; i++) {
			lock_mcu_done = rts_read_reg(rsocdev,
			                             RTS_REG_CPU_LOCK_MCU);
			if (lock_mcu_done & CPU_LOCK_MCU_DONE) {
				rtsprintk(RTS_TRACE_DEBUG, "lock mcu done\n");
				break;
			}
			usleep_range(9900, 10000);
		}

		if (i >= 300) {
			atomic_inc(&rsocdev->mcu_count);
			return -EINVAL;
		}

		clk_disable_unprepare(clk);

		atomic_set(&rsocdev->mcu_count, 0);
		rsocdev->mcu_state = RTSCAM_STATE_PASSIVE;
		rtsprintk(RTS_TRACE_DEBUG, "disable mcu\n");
	}

	return 0;
}

static int rtscam_soc_enable_hclk(struct rtscam_soc_dev *rsocdev, int enable)
{
	const unsigned long M = 1000000;
	unsigned long m;
	unsigned long n;
	unsigned long f;
	struct clk *clk1;
	struct clk *clk2;
	struct clk *clk3;

	if (HCLK_SRC_480M == rsocdev->hclk_desc.src_type)
		return 0;

	if (enable) {
		switch (rsocdev->hclk_desc.src_type) {
		case HCLK_SRC_288M:
			m = 1400 * M;
			n = 1400 * M / 5;
			f = 287998047;
			break;
		case HCLK_SRC_324M:
			m = 1575 * M;
			n = 1575 * M / 5;
			f = 323997803;
			break;
		case HCLK_SRC_297M:
			m = 1450 * M;
			n = 1450 * M / 5;
			f = 297000428;
			break;
		case HCLK_SRC_216M:
			m = 1075 * M;
			n = 1075 * M / 5;
			f = 216000000;
			break;
		default:
			return 0;
		}
	} else {
		switch (rsocdev->hclk_desc.src_type) {
		case HCLK_SRC_288M:
		case HCLK_SRC_324M:
		case HCLK_SRC_297M:
		case HCLK_SRC_216M:
			m = 0;
			n = 0;
			f = 0;
			break;
		default:
			return 0;
		}
	}

	clk1 = clk_get(rsocdev->dev, "sys_pll4_m");
	clk2 = clk_get(rsocdev->dev, "sys_pll4_n");
	clk3 = clk_get(rsocdev->dev, "sys_pll4_f");

	if (IS_ERR(clk1)) {
		rtsprintk(RTS_TRACE_ERROR, "get pll4 m fail\n");
		return -EINVAL;
	}
	if (IS_ERR(clk2)) {
		rtsprintk(RTS_TRACE_ERROR, "get pll4 n fail\n");
		return -EINVAL;
	}
	if (IS_ERR(clk3)) {
		rtsprintk(RTS_TRACE_ERROR, "get pll4 f fail\n");
		return -EINVAL;
	}

	if (enable) {
		clk_set_rate(clk1, m);
		clk_set_rate(clk2, n);
		clk_set_rate(clk3, f);

		clk_prepare_enable(clk3);
	} else {
		clk_disable_unprepare(clk3);
	}

	return 0;
}

static int rtscam_soc_config_hw_slot_num(struct rtscam_video_stream *stream,
        int count)
{
	u32 reg = RTS_REG_FRAME_BUFFER_COUNT;
	u32 value;
	u8 idx = __get_stream_reg_index(stream);

	if (count < 0 || count >= 8)
		return -EINVAL;

	value = rtscam_read_reg(stream, reg);

	clear_bit(4 * idx, (void *)&value);
	clear_bit(4 * idx + 1, (void *)&value);
	clear_bit(4 * idx + 2, (void *)&value);
	clear_bit(4 * idx + 3, (void *)&value);

	value |= (count << (4 * idx));

	rtscam_write_reg(stream, value, reg);

	return 0;
}

static int rtscam_soc_config_isp_buffer(struct rtscam_soc_dev *rsocdev)
{
	u32 isp_chanel_config[RTSCAM_SOC_MAX_STREAM_NUM][4] = {
		/*y0 start, y0 size, uv0 start, uv0 size*/
		{0, 0x100, 0x100, 0x100},
		/*y1 start, y1 size, uv1 start, uv1 size*/
		{0x200, 0x80, 0x280, 0x80},
		/*y2 start, y2 size, uv2 start, uv2 size*/
		{0x300, 0x40, 0x340, 0x40},
		/*y3 start, y3 size, uv3 start, uv3 size*/
		{0x380, 0x40, 0x3c0, 0x40},
		/*mjpeg start, mjpeg size, td start. td size*/
		{0, 0x100, 0x100, 0x100},
	};
	u8 idx = 0;
	struct rtscam_video_stream *stream;
	int i;
	u32 reg;
	u32 value;

	if (!rsocdev)
		return -EINVAL;

	for (i = 0; i < rsocdev->rvdev.streamnum; i++) {
		stream = rsocdev->rvdev.streams + i;
		idx = __get_stream_reg_index(stream);

		/*yuv: y start&size; mjpeg : mjpeg start&size*/
		reg = RTS_REG_YUV_ISP_BUF_CONFIG_BASE + 8 * idx;
		value = (isp_chanel_config[idx][0] << 3) +
		        (isp_chanel_config[idx][1] << 19);
		rts_write_reg(rsocdev, value, reg);

		/*yuv: uv start&size; mjpeg : td start&size*/
		reg = reg + 4;
		value = (isp_chanel_config[idx][2] << 3) +
		        (isp_chanel_config[idx][3] << 19);
		rts_write_reg(rsocdev, value, reg);
	}

	/*if no mjpeg, config td alone*/
	if (idx < RTSCAM_SOC_MJPEG_STRM_IDX) {
		idx = RTSCAM_SOC_MJPEG_STRM_IDX;

		reg = RTS_REG_YUV_ISP_BUF_CONFIG_BASE + 8 * idx + 4;
		value = (isp_chanel_config[idx][2] << 3) +
		        (isp_chanel_config[idx][3] << 19);
		rts_write_reg(rsocdev, value, reg);
	}

	return 0;
}

static int rtscam_soc_config_isp_header(struct rtscam_soc_dev *rsocdev);
static int rtscam_soc_config_isp_buffer(struct rtscam_soc_dev *rsocdev);
int rtscam_soc_start_clock(struct rtscam_video_device *icd)
{
	struct rtscam_soc_dev *rsocdev = icd->priv;
#if RTS_SOC_CLK_ON_NEED
	int ret;
	u8 func = 0;

	ret = rtscam_soc_enable_mcu(rsocdev, 1);
	if (ret)
		return ret;
	rtscam_soc_config_isp_header(rsocdev);
	rtscam_soc_config_isp_buffer(rsocdev);
	if (rsocdev->td_enable) {
		rts_write_reg(rsocdev, rsocdev->td_buf.phy_addr,
		              RTS_REG_TD_BUFFER_START_ADDRESS);
		rts_write_reg(rsocdev, rsocdev->td_buf.size,
		              RTS_REG_TD_BUFFER_LENGTH);
		rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_TD_INDEX, 1);
		ret = rtscam_soc_get_isp_func(rsocdev, &func);
		if (ret) {
			rtsprintk(RTS_TRACE_ERROR,
			          "<%s, %d>get isp func fail\n",
			          __func__, __LINE__);
		} else {
			func |= (1 << RTSCAM_SOC_ISP_FUNC_TD_BIT);
			rtscam_soc_set_isp_func(rsocdev, func);
		}
	}
	if (rsocdev->ldc_enable) {
		rts_write_reg(rsocdev, rsocdev->ldc_buf.phy_addr,
		              RTS_REG_LDC_MAP_TABLE_START);
		rts_write_reg(rsocdev, rsocdev->ldc_buf.size,
		              RTS_REG_LDC_MAP_TABLE_SIZE);
		rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_LDC_INDEX, 1);
		ret = rtscam_soc_get_isp_func(rsocdev, &func);
		if (ret) {
			rtsprintk(RTS_TRACE_ERROR,
			          "<%s, %d>get isp func fail\n",
			          __func__, __LINE__);
		} else {
			func |= (1 << RTSCAM_SOC_ISP_FUNC_LDC_BIT);
			rtscam_soc_set_isp_func(rsocdev, func);
		}
	}
#endif
	rtscam_soc_enable_hclk(rsocdev, 1);

	rtscam_soc_set_fps(rsocdev, rsocdev->sensor_frmival);

	return 0;
}

int rtscam_soc_stop_clock(struct rtscam_video_device *icd)
{
	struct rtscam_soc_dev *rsocdev = icd->priv;

	rtscam_soc_enable_hclk(rsocdev, 0);
#if RTS_SOC_CLK_ON_NEED
	rtscam_soc_enable_mcu(rsocdev, 0);
#endif

	return 0;
}

int rtscam_soc_get_ctrl(struct rtscam_video_device *icd,
                        struct rtscam_video_ctrl **ctrl, int index)
{
	struct rtscam_soc_dev *rsocdev = icd->priv;
	int ret = -EINVAL;
	int i;
	int count = 0;
	struct rtscam_soc_unit_descriptor *entity;

	if (index < 0)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(rsocdev->entities); i++) {
		int idx = index - count;

		entity = rsocdev->entities + i;
		if (idx < entity->ncontrols) {
			*ctrl = entity->controls + idx;
			ret = 0;
			break;
		}
		count += entity->ncontrols;
	}

	return ret;
}

int rtscam_soc_put_ctrl(struct rtscam_video_device *icd,
                        struct rtscam_video_ctrl *ctrl)
{
	return 0;
}

int rtscam_soc_query_ctrl(struct rtscam_video_device *icd,
                          u8 unit, u8 query, u8 selector, u8 length, u8 *data)
{
	struct rtscam_soc_dev *rsocdev = icd->priv;
	int ret;

	ret = rtscam_soc_ctrl_query_ctrl(rsocdev, unit, query, selector,
	                                 length, data);

	rtsprintk((ret ? RTS_TRACE_ERROR : RTS_TRACE_CTRL),
	          "%s : %d 0x%02x 0x%02x, %d, ret = %d\n",
	          __func__, unit, query, selector, length, ret);

	return ret;
}

int rtscam_soc_init_capture_buffers(struct rtscam_video_stream *stream)
{
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;
	struct rtscam_soc_slot_info *info;
	int i;

	info = &rsocdev->slot_info[stream->streamid];
	info->slot_index = 0;
	for (i = 0; i < RTSCAM_SOC_HW_SLOT_NUM; i++)
		info->slots[i] = NULL;

	if (stream->vb2_vidp.num_buffers < RTSCAM_SOC_HW_SLOT_NUM)
		info->slot_num = stream->vb2_vidp.num_buffers;
	else
		info->slot_num = RTSCAM_SOC_HW_SLOT_NUM;

	rtscam_soc_config_hw_slot_num(stream, info->slot_num);

	return 0;
}

static int rtscam_soc_is_yuv(u8 rts_code)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(m_rtscam_soc_formats); i++) {
		if (m_rtscam_soc_formats[i].rts_code == rts_code)
			return m_rtscam_soc_formats[i].is_yuv;
	}

	return false;
}

int rtscam_soc_submit_buffer(struct rtscam_video_stream *stream,
                             struct rtscam_video_buffer *rbuf)
{
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;
	struct rtscam_soc_slot_info *info;
	int i;
	int nr;
	dma_addr_t phy_addr;
	u32 reg_addr;
	u32 reg_index; /* yuv:uv addr, mjpeg:length */
	u32 reg_status;
	u32 status;
	int ret;

	info = &rsocdev->slot_info[stream->streamid];

	for (i = 0; i < info->slot_num; i++) {
		if (info->slots[i] == rbuf)
			return -EINVAL;
	}

	for (i = 0; i < info->slot_num; i++) {
		nr = (i + info->slot_index) % info->slot_num;
		if (info->slots[nr] == NULL) {
			info->slot_index = (nr + 1) % info->slot_num;
			break;
		}
	}

	if (i == info->slot_num) {
		rtsprintk(RTS_TRACE_WARNING, "there is no free slot now\n");
		return -EINVAL;
	}

	ret = rtscam_get_phy_addr(&rbuf->vb, &phy_addr);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "get buffer phy addr fail\n");
		return ret;
	}

	info->slots[nr] = rbuf;
	if (RTSCAM_SOC_FORMAT_TYPE_MJPG == stream->rts_code) {
		reg_addr = RTS_REG_MJPEG_FRAME_BUFFER_START_ADDRESS_BASE +
		           4 * nr;
		reg_index = RTS_REG_MJPEG_FRAME_BUFFER_LENGTH;
		reg_status = RTS_REG_MJPEG_STREAM_FRAME_BUFFER_STATUS;

		rts_write_reg(rsocdev, phy_addr, reg_addr);
		rts_write_reg(rsocdev, stream->sizeimage, reg_index);
	} else if (rtscam_soc_is_yuv(stream->rts_code)) {
		u32 offset =
		    (stream->streamid * RTSCAM_SOC_MAX_SLOT_NUM + nr) * 4;
		dma_addr_t phy_addr_uv = phy_addr +
		                         stream->user_width * stream->user_height;

		reg_addr = RTS_REG_YUV_FRAME_Y_START_ADDRESS_BASE + offset;
		reg_index = RTS_REG_YUV_FRAME_UV_START_ADDRESS_BASE + offset;
		reg_status = RTS_REG_YUV_FRAME_BUFFER_STATUS_BASE +
		             stream->streamid * 4;

		rts_write_reg(rsocdev, phy_addr, reg_addr);
		rts_write_reg(rsocdev, phy_addr_uv, reg_index);
	} else {
		rtsprintk(RTS_TRACE_ERROR,
		          "invalid stream format (%d)\n", stream->rts_code);
		return -EINVAL;
	}

	status = 0xc << (4 * nr);
	rts_write_reg(rsocdev, status, reg_status);
	status = rts_read_reg(rsocdev, reg_status);
	rtsprintk(RTS_TRACE_DEBUG,
	          "clear frame status, reg = 0x%08x, value = 0x%08x\n",
	          reg_status, status);

	return 0;
}

#ifdef CONFIG_REGULATOR_RTP
static int __compare_api_version(struct rtscam_soc_api_version *version,
                                 u8 main, u8 sub)
{
	if (version->main < main)
		return -1;
	else if (version->main > main)
		return 1;
	else if (version->sub < sub)
		return -1;
	else if (version->sub > sub)
		return 1;
	else
		return 0;
}

static int __enable_power(struct rtscam_soc_pwr *pwr)
{
	int ret;

	if (!pwr || !pwr->pwr)
		return -EINVAL;

	if (pwr->flag)
		return 0;

	if (!pwr->voltage)
		return -EINVAL;

	regulator_set_voltage(pwr->pwr, pwr->voltage, pwr->voltage);
	ret = regulator_enable(pwr->pwr);
	rtsprintk(ret ? RTS_TRACE_ERROR : RTS_TRACE_POWER,
	          "set %s power %d, ret = %d\n",
	          pwr->name, pwr->voltage, ret);
	if (ret)
		return ret;

	pwr->flag = 1;

	return 0;
}

static int __disable_power(struct rtscam_soc_pwr *pwr)
{
	if (!pwr || !pwr->pwr)
		return -EINVAL;

	if (!pwr->flag)
		return 0;

	regulator_disable(pwr->pwr);
	pwr->flag = 0;
	rtsprintk(RTS_TRACE_POWER, "set power %s off\n", pwr->name);

	return 0;
}

static int __get_regulator(struct rtscam_soc_dev *rsocdev)
{
	int ret = 0;
	struct rtscam_soc_snr_power power;
	const int power_unit = 100000;

	ret = rtscam_soc_get_snr_power(rsocdev, &power);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "get snr power fail\n");
		return ret;
	}

	if (!power.power_io && !power.power_analog && !power.power_core)
		return 0;

	if (NULL == rsocdev->pio.pwr && power.power_io) {
		rsocdev->pio.pwr = regulator_get(NULL, "LDO1");
		if (IS_ERR(rsocdev->pio.pwr)) {
			rtsprintk(RTS_TRACE_ERROR, "get regulator io fail\n");
			rsocdev->pio.pwr = NULL;
		}
		rsocdev->pio.voltage = power.power_io * power_unit;
		sprintf(rsocdev->pio.name, "io");
		rsocdev->pio.flag = 0;
	}
	if (NULL == rsocdev->panalog.pwr && power.power_analog) {
		rsocdev->panalog.pwr = regulator_get(NULL, "LDO2");
		if (IS_ERR(rsocdev->panalog.pwr)) {
			rtsprintk(RTS_TRACE_ERROR, "get regulator analog fail\n");
			rsocdev->panalog.pwr = NULL;
		}
		rsocdev->panalog.voltage = power.power_analog * power_unit;
		sprintf(rsocdev->panalog.name, "analog");
		rsocdev->panalog.flag = 0;
	}
	if (NULL == rsocdev->pcore.pwr && power.power_core) {
		rsocdev->pcore.pwr = regulator_get(NULL, "SWR_OUT_RSV");
		if (IS_ERR(rsocdev->pcore.pwr)) {
			rtsprintk(RTS_TRACE_ERROR, "get regulator core fail\n");
			rsocdev->pcore.pwr = NULL;
		}
		rsocdev->pcore.voltage = power.power_core * power_unit;
		sprintf(rsocdev->pcore.name, "core");
		rsocdev->pcore.flag = 0;
	}

	return 0;
}

static int __put_regulator(struct rtscam_soc_dev *rsocdev)
{
	int i;

	if (rsocdev->pio.pwr) {
		__disable_power(&rsocdev->pio);
		regulator_put(rsocdev->pio.pwr);
		rsocdev->pio.pwr = NULL;
		rsocdev->pio.voltage = 0;
	}
	if (rsocdev->panalog.pwr) {
		__disable_power(&rsocdev->panalog);
		regulator_put(rsocdev->panalog.pwr);
		rsocdev->panalog.pwr = NULL;
		rsocdev->panalog.voltage = 0;
	}
	if (rsocdev->pcore.pwr) {
		__disable_power(&rsocdev->pcore);
		regulator_put(rsocdev->pcore.pwr);
		rsocdev->pcore.pwr = NULL;
		rsocdev->pcore.voltage = 0;
	}

	for (i = 0; i < RTSCAM_SOC_SNR_POWER_COUNT; i++) {
		rsocdev->pwrs[i] = NULL;
		if (i < RTSCAM_SOC_SNR_POWER_COUNT - 1)
			rsocdev->pwr_wait[i] = 0;
	}
	return 0;
}

static struct rtscam_soc_pwr *__get_snr_power_by_index(
    struct rtscam_soc_dev *rsocdev, int index)
{
	struct rtscam_soc_pwr *result = NULL;

	switch (index) {
	case 0:
		result = &rsocdev->pio;
		break;
	case 1:
		result = &rsocdev->panalog;
		break;
	case 2:
		result = &rsocdev->pcore;
		break;
	default:
		break;
	}

	return result;
}

static int __get_snr_power_seq(struct rtscam_soc_dev *rsocdev, int pwron)
{
	int ret = 0;
	int i;
	struct rtscam_soc_snr_power_seq on;
	struct rtscam_soc_snr_power_seq off;
	struct rtscam_soc_snr_power_seq *seq = NULL;

	if (!rsocdev->pio.pwr && !rsocdev->panalog.pwr && !rsocdev->pcore.pwr)
		return -EINVAL;

	rsocdev->pwrs[0] = &rsocdev->pio;
	rsocdev->pwrs[1] = &rsocdev->panalog;
	rsocdev->pwrs[2] = &rsocdev->pcore;
	rsocdev->pwr_wait[0] = 0;
	rsocdev->pwr_wait[1] = 0;

	if (__compare_api_version(&rsocdev->api_version, 1, 3) < 0)
		return 0;

	ret = rtscam_soc_get_snr_power_seq(rsocdev, &on, &off);
	if (ret)
		return ret;

	rtsprintk(RTS_TRACE_DEBUG, "power on seq : %d %d %d\n",
	          on.seqs[0], on.seqs[1], on.seqs[2]);
	rtsprintk(RTS_TRACE_DEBUG, "power off seq : %d %d %d\n",
	          off.seqs[0], off.seqs[1], off.seqs[2]);

	if (pwron)
		seq = &on;
	else
		seq = &off;

	for (i = 0; i < RTSCAM_SOC_SNR_POWER_COUNT; i++) {
		rsocdev->pwrs[i] = __get_snr_power_by_index(rsocdev,
		                   seq->seqs[i]);
		if (!rsocdev->pwrs[i])
			rtsprintk(RTS_TRACE_ERROR, "get power %s %d fail\n",
			          pwron ? "on" : "off", seq->seqs[i]);
		if (i < RTSCAM_SOC_SNR_POWER_COUNT - 1)
			rsocdev->pwr_wait[i] = seq->wait[i];
	}

	if (pwron)
		rtscam_soc_prepare_snr_pwron(rsocdev);

	return 0;
}

static int rtscam_soc_set_snr_power(struct rtscam_soc_dev *rsocdev, int enable)
{
	int i;
	struct rtscam_soc_pwr *pwr;

	if (enable)
		__get_regulator(rsocdev);

	__get_snr_power_seq(rsocdev, enable);
	for (i = 0; i < RTSCAM_SOC_SNR_POWER_COUNT; i++) {
		pwr = rsocdev->pwrs[i];
		if (!pwr || !pwr->pwr)
			continue;
		if (enable)
			__enable_power(pwr);
		else
			__disable_power(pwr);
		if (i < RTSCAM_SOC_SNR_POWER_COUNT - 1 && rsocdev->pwr_wait[i])
			udelay(rsocdev->pwr_wait[i]);
	}

	if (!enable)
		__put_regulator(rsocdev);

	return 0;
}
#endif

int rtscam_soc_s_stream(struct rtscam_video_stream *stream, int enable)
{
	int ret = 0;
	u8 idx;
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;

	rtsprintk(RTS_TRACE_VIDEO, "set stream %s\n", enable ? "on" : "off");

	if (!stream || !stream->icd || !stream->icd->priv)
		return -EINVAL;

	idx = __get_stream_reg_index(stream);

	if (enable) {
		stream->sequence = 0;
		stream->overflow = 0;

		rtscam_soc_set_format(stream);
		rtscam_soc_init_skip_info(stream);

		rtscam_soc_reset_stream(stream);
		usleep_range(9900, 10000);

#ifdef CONFIG_REGULATOR_RTP
		if (atomic_inc_return(&rsocdev->pwr_use) == 1)
			rtscam_soc_set_snr_power(rsocdev, 1);
		udelay(2000);
#endif
		rtscam_soc_isp_control(rsocdev, idx, 1);

		ret = rtscam_soc_start_preview(stream);
#ifdef CONFIG_REGULATOR_RTP
		if (ret) {
			rtscam_soc_isp_control(rsocdev, idx, 0);
			if (atomic_dec_return(&rsocdev->pwr_use) == 0)
				rtscam_soc_set_snr_power(rsocdev, 0);
		}
#endif
	} else {
		rtscam_soc_stop_preview(stream);
		msleep(100);

		rtscam_soc_isp_control(rsocdev, idx, 0);
#ifdef CONFIG_REGULATOR_RTP
		if (atomic_dec_return(&rsocdev->pwr_use) == 0)
			rtscam_soc_set_snr_power(rsocdev, 0);
#endif
	}

	return ret;
}

static int rtscam_soc_get_td_buf(struct rtscam_soc_dev *rsocdev)
{
	unsigned long size;
	struct rtscam_soc_snr_fmt sfmt;
	int ret;

	if (rsocdev->td_buf.initialized)
		return 0;

	ret = rtscam_soc_get_snr_fmt(rsocdev, &sfmt);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:get sensor fmt fail\n", __func__);
		return ret;
	}

	size = sfmt.width * sfmt.height / 4;
	rsocdev->td_buf.size = size;
	ret = rtscam_soc_dma_alloc_coherent(rsocdev,
	                                    &rsocdev->td_buf, GFP_KERNEL);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:alloc td buffer fail\n", __func__);
		return ret;
	}
	rts_write_reg(rsocdev, rsocdev->td_buf.phy_addr,
	              RTS_REG_TD_BUFFER_START_ADDRESS);
	rts_write_reg(rsocdev, rsocdev->td_buf.size, RTS_REG_TD_BUFFER_LENGTH);

	return 0;
}

static void rtscam_soc_put_td_buf(struct rtscam_soc_dev *rsocdev)
{
	rtscam_soc_dma_free_coherent(rsocdev, &rsocdev->td_buf);
}

static int rtscam_soc_disable_td(struct rtscam_soc_dev *rsocdev)
{
	int ret;
	u8 func;
	const u8 td_off_mask = ~(1 << RTSCAM_SOC_ISP_FUNC_TD_BIT);

	if (!rsocdev->td_enable)
		return 0;

	rsocdev->td_enable = 0;

	ret = rtscam_soc_get_isp_func(rsocdev, &func);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:get td isp func enable fail\n", __func__);
		return ret;
	}
	func &= td_off_mask;
	ret = rtscam_soc_set_isp_func(rsocdev, func);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:set td isp func enable fail\n", __func__);
		return ret;
	}
	rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_TD_INDEX, 0);
	rtscam_soc_put_td_buf(rsocdev);


	return 0;
}

static int rtscam_soc_enable_td(struct rtscam_soc_dev *rsocdev)
{
	int ret;
	u8 func;
	const u8 td_on_mask = 1 << RTSCAM_SOC_ISP_FUNC_TD_BIT;

	if (rsocdev->td_enable)
		return 0;

	ret = rtscam_soc_get_isp_func(rsocdev, &func);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:get td isp func enable fail\n", __func__);
		return ret;
	}
	rtscam_soc_get_td_buf(rsocdev);
	if (!rsocdev->td_buf.initialized) {
		rtsprintk(RTS_TRACE_ERROR, "please init td buffer first\n");
		return -EINVAL;
	}

	rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_TD_INDEX, 1);
	func |= td_on_mask;
	ret = rtscam_soc_set_isp_func(rsocdev, func);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:set td isp func enable fail\n", __func__);
		rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_TD_INDEX, 0);
		rtscam_soc_put_td_buf(rsocdev);
		return ret;
	}

	rsocdev->td_enable = 1;

	return 0;
}

static int rtscam_soc_s_td(struct rtscam_soc_dev *rsocdev, int enable)
{
	int ret;

	if (!(rsocdev->isp_capibility & RTSCAM_SOC_CAP_FLAG_TDNR))
		return -EINVAL;

	if (enable)
		ret = rtscam_soc_enable_td(rsocdev);
	else
		ret = rtscam_soc_disable_td(rsocdev);

	rtsprintk(RTS_TRACE_DEBUG, "%s temporal denoise\n",
	          enable ? "enable" : "disable");
	return ret;
}

static void __pause_stream(struct rtscam_video_device *icd, int resume)
{
	unsigned i;

	for (i = 0; i < icd->streamnum; i++) {
		struct rtscam_video_stream *stream = icd->streams + i;
		if (vb2_is_streaming(&stream->vb2_vidp))
			rtscam_soc_s_stream(stream, resume);
	}
}

static int rtscam_soc_enable_ldc(struct rtscam_soc_dev *rsocdev,
                                 struct rtscam_soc_ldc_stru *pldc)
{
	int ret;
	u8 func;
	const u8 ldc_on_mask = (1 << RTSCAM_SOC_ISP_FUNC_LDC_BIT);
	unsigned int size;
	struct rtscam_soc_snr_fmt sfmt;

	if (!(rsocdev->isp_capibility & RTSCAM_SOC_CAP_FLAG_LDC))
		return -EINVAL;

	if (rsocdev->ldc_enable)
		return 0;

	if (!pldc || !pldc->ptable)
		return -EINVAL;

	ret = rtscam_soc_get_snr_fmt(rsocdev, &sfmt);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:get sensor fmt fail\n", __func__);
		return ret;
	}

	size = (14 + 4 + 2 * (sfmt.width - 2) + 7) / 8;
	size = (size + 3) & (~3);
	size = (size * sfmt.height + 127) & (~127);

	if (pldc->length < size) {
		rtsprintk(RTS_TRACE_ERROR,
		          "invalid ldc table, length is not enough\n");
		return -EINVAL;
	}

	ret = rtscam_soc_get_isp_func(rsocdev, &func);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:get td isp func enable fail\n", __func__);
		return ret;
	}

	rsocdev->ldc_buf.size = size;
	ret = rtscam_soc_dma_alloc_coherent(rsocdev,
	                                    &rsocdev->ldc_buf, GFP_KERNEL);
	if (ret || !rsocdev->ldc_buf.initialized) {
		rtsprintk(RTS_TRACE_ERROR, "please init ldc buffer first\n");
		return -EINVAL;
	}

	copy_from_user(rsocdev->ldc_buf.vaddr,
	               (void __user *)pldc->ptable,
	               pldc->length);

	__pause_stream(&rsocdev->rvdev, 0);
	rts_write_reg(rsocdev, rsocdev->ldc_buf.phy_addr,
	              RTS_REG_LDC_MAP_TABLE_START);
	rts_write_reg(rsocdev, rsocdev->ldc_buf.size,
	              RTS_REG_LDC_MAP_TABLE_SIZE);

	rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_LDC_INDEX, 1);

	func |= ldc_on_mask;
	ret = rtscam_soc_set_isp_func(rsocdev, func);
	__pause_stream(&rsocdev->rvdev, 1);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:set td isp func enable fail\n", __func__);
		rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_LDC_INDEX, 0);
		rtscam_soc_dma_free_coherent(rsocdev, &rsocdev->ldc_buf);
		return ret;
	}
	rsocdev->ldc_enable = 1;

	return 0;
}

static int rtscam_soc_disable_ldc(struct rtscam_soc_dev *rsocdev)
{
	int ret;
	u8 func;
	const u8 ldc_off_mask = ~(1 << RTSCAM_SOC_ISP_FUNC_LDC_BIT);

	if (!(rsocdev->isp_capibility & RTSCAM_SOC_CAP_FLAG_LDC))
		return -EINVAL;

	if (!rsocdev->ldc_enable)
		return 0;

	ret = rtscam_soc_get_isp_func(rsocdev, &func);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:get td isp func enable fail\n", __func__);
		return ret;
	}

	func &= ldc_off_mask;
	__pause_stream(&rsocdev->rvdev, 0);
	ret = rtscam_soc_set_isp_func(rsocdev, func);
	if (ret) {
		__pause_stream(&rsocdev->rvdev, 1);
		rtsprintk(RTS_TRACE_ERROR,
		          "%s:set td isp func enable fail\n", __func__);
		return ret;
	}
	rtscam_soc_isp_control(rsocdev, RTSCAM_SOC_LDC_INDEX, 0);
	rtscam_soc_dma_free_coherent(rsocdev, &rsocdev->ldc_buf);
	__pause_stream(&rsocdev->rvdev, 1);

	rsocdev->ldc_enable = 0;

	return 0;
}

static int __exec_isp_cmd(struct rtscam_soc_dev *rsocdev,
                          struct rtscam_soc_cmd_stru *pscmd)
{
	struct rtscam_soc_cmd_stru scmd;
	u8 data_dir;
	int ret;

	data_dir = __get_cmd_data_direction(pscmd);

	scmd.cmdcode = pscmd->cmdcode;
	scmd.index = pscmd->index;
	scmd.length = pscmd->length;
	scmd.param = pscmd->param;
	scmd.addr = pscmd->addr;

	if (scmd.length > 0) {
		scmd.buf = kzalloc(scmd.length, GFP_KERNEL);
		if (NULL == scmd.buf)
			return -ENOMEM;

		if (host_to_device == data_dir) {
			if (copy_from_user(scmd.buf,
			                   (void __user *)pscmd->buf,
			                   scmd.length)) {
				kfree(scmd.buf);
				return -EFAULT;
			}
		}
	} else {
		scmd.buf = NULL;
	}

	ret = rtscam_soc_exec_command(rsocdev, &scmd);
	if (0 == ret && device_to_host == data_dir) {
		if (copy_to_user((void __user *)pscmd->buf,
		                 scmd.buf, scmd.length)) {
			ret = -EFAULT;
		}
	}

	if (scmd.buf) {
		kfree(scmd.buf);
		scmd.buf = NULL;
	}

	return ret;
}

static long rtscam_soc_do_exec_vcmd(struct file *filp, unsigned int cmd,
                                    void *arg)
{
	struct rtscam_video_stream *stream =
	    (struct rtscam_video_stream *)filp;
	struct rtscam_soc_dev *rsocdev = stream->icd->priv;
	long ret = -EFAULT;

	switch (cmd) {
	case RTSOCIOC_ISPCMD:
		ret = __exec_isp_cmd(rsocdev, arg);
		break;
	case RTSOCIOC_G_HWOFFSET:
		*(unsigned long *)arg = rsocdev->iostart;
		ret = 0;
		break;
	case RTSOCIOC_G_HWIOSIZE:
		*(unsigned int *)arg = rsocdev->iosize;
		ret = 0;
		break;
	case RTSOCIOC_G_MTDSTATE: {
		struct rtscam_soc_mtd_state *mtd = arg;
		if (mtd->index < 0 || mtd->index >= RTSCAM_SOC_MTD_NUM)
			return -EINVAL;

		mtd->state = rtscam_soc_get_mtd_status(rsocdev, mtd->index);
		ret = 0;
		break;
	}
	case RTSOCIOC_G_STREAMID:
		*(int *)arg = stream->streamid;
		ret = 0;
		break;
	case RTSOCIOC_S_TD:
		ret = rtscam_soc_s_td(rsocdev, *(int *)arg);
		break;
	case RTSOCIOC_G_TD:
		*(int *)arg = rsocdev->td_enable;
		ret = 0;
		break;
	case RTSOCIOC_ENABLE_LDC:
		ret = rtscam_soc_enable_ldc(rsocdev, arg);
		break;
	case RTSOCIOC_DISABLE_LDC:
		ret = rtscam_soc_disable_ldc(rsocdev);
		break;
	case RTSOCIOC_G_LDC:
		*(int *)arg = rsocdev->ldc_enable;
		ret = 0;
		break;
	default:
		rtsprintk(RTS_TRACE_ERROR,
		          "Unknown rtscam soc cmd: 0x%x, '%c' 0x%x\n",
		          cmd,
		          _IOC_TYPE(cmd),
		          _IOC_NR(cmd));
		ret = -EINVAL;
		break;
	}

	return ret;
}

int rtscam_soc_exec_vcmd(struct rtscam_video_stream *stream,
                         struct rtscam_vcmd *pcmd)
{
	return rtscam_usercopy((struct file *)stream, pcmd->cmdcode,
	                       (unsigned long)pcmd->arg, rtscam_soc_do_exec_vcmd);
}

static struct rtscam_video_ops rtscam_soc_ops = {
	.owner			= THIS_MODULE,
	.start_clock		= rtscam_soc_start_clock,
	.stop_clock		= rtscam_soc_stop_clock,
	.get_ctrl		= rtscam_soc_get_ctrl,
	.put_ctrl		= rtscam_soc_put_ctrl,
	.query_ctrl		= rtscam_soc_query_ctrl,
	.init_capture_buffers	= rtscam_soc_init_capture_buffers,
	.submit_buffer		= rtscam_soc_submit_buffer,
	.s_stream		= rtscam_soc_s_stream,
	.exec_command		= rtscam_soc_exec_vcmd,
};

static void rtscam_soc_ctrl_init_ctrl(struct rtscam_soc_dev *rsocdev,
                                      struct rtscam_video_ctrl *ctrl)
{
	struct rtscam_video_ctrl_info *info;
	struct rtscam_video_ctrl_mapping *mapping;
	int i;

	for (i = 0; i < ARRAY_SIZE(m_rtscam_soc_ctrls); i++) {
		info = m_rtscam_soc_ctrls + i;
		if (ctrl->unit == info->unit && ctrl->index == info->index) {
			rtscam_ctrl_add_info(ctrl, info);
			break;
		}
	}

	if (!ctrl->initialized)
		return;

	for (i = 0; i < ARRAY_SIZE(m_rtscam_soc_ctrl_mappings); i++) {
		mapping = m_rtscam_soc_ctrl_mappings + i;
		if (ctrl->unit == mapping->unit &&
		    ctrl->info.selector == mapping->selector)
			rtscam_ctrl_add_mapping(ctrl, mapping);
	}
}

static int rtscam_soc_ctrl_init_device(struct rtscam_soc_dev *rsocdev)
{
	unsigned int ncontrols;
	int i;
	struct rtscam_soc_unit_descriptor *entity;
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(rsocdev->entities); i++) {
		struct rtscam_video_ctrl *ctrl;
		int j;

		entity = &rsocdev->entities[i];

		ncontrols = memweight(entity->bmcontrols, entity->controlsize);
		entity->controls = kcalloc(ncontrols,
		                           sizeof(struct rtscam_video_ctrl), GFP_KERNEL);
		if (NULL == entity->controls) {
			ret = -ENOMEM;
			goto error;
		}
		entity->ncontrols = ncontrols;

		/* Initialize all supported controls */
		ctrl = entity->controls;
		for (j = 0; j < entity->controlsize * 8; j++) {
			if (rts_test_bit(entity->bmcontrols, j) == 0)
				continue;
			ctrl->index = j;
			ctrl->unit = entity->type;
			rtscam_soc_ctrl_init_ctrl(rsocdev, ctrl);
			ctrl++;
		}
	}

	return 0;

error:
	for (i = 0; i < ARRAY_SIZE(rsocdev->entities); i++) {
		entity = &rsocdev->entities[i];
		if (entity->controls) {
			kfree(entity->controls);
			entity->controls = NULL;
		}
		entity->ncontrols = 0;
	}
	return ret;
}

static int rtscam_soc_config_isp_header(struct rtscam_soc_dev *rsocdev)
{
	int ret;
	u32 reg = RTS_REG_ISP_HEADER_ADDR;

	if (!rsocdev->isp_headers.initialized) {
		rsocdev->isp_headers.size = RTSCAM_SOC_MAX_STREAM_NUM *
		                            RTSCAM_SOC_MAX_SLOT_NUM *
		                            RTSCAM_SOC_FRAME_HEADER_LENGTH;

		ret = rtscam_soc_dma_alloc_coherent(rsocdev,
		                                    &rsocdev->isp_headers, GFP_KERNEL);
		if (ret) {
			rtsprintk(RTS_TRACE_ERROR,
			          "no memory for isp header\n");
			return ret;
		}
	}

	rts_write_reg(rsocdev, rsocdev->isp_headers.phy_addr, reg);

	return 0;
}

static int rtscam_soc_clear_isp_header(struct rtscam_soc_dev *rsocdev)
{
	rtscam_soc_dma_free_coherent(rsocdev, &rsocdev->isp_headers);
	return 0;
}

static struct rtscam_ge_device *__create_device(struct rtscam_soc_dev *rsocdev,
        const char *name, struct rtscam_ge_file_operations *fops)
{
	struct rtscam_ge_device *gdev;
	int ret;

	gdev = rtscam_ge_device_alloc();
	if (!gdev)
		return NULL;

	strlcpy(gdev->name, name, sizeof(gdev->name));
	gdev->parent = get_device(rsocdev->dev);
	gdev->release = rtscam_ge_device_release;
	gdev->fops = fops;

	rtscam_ge_set_drvdata(gdev, rsocdev);
	ret = rtscam_ge_register_device(gdev);
	if (ret) {
		rtscam_ge_device_release(gdev);
		return NULL;
	}

	return gdev;
}

static void __remove_device(struct rtscam_ge_device *gdev)
{
	if (!gdev)
		return;

	put_device(gdev->parent);
	rtscam_ge_unregister_device(gdev);
}

static struct rtscam_soc_dma_buffer *__find_dma_buffer(
    struct rtscam_soc_dev *rsocdev, dma_addr_t phy_addr)
{
	struct rtscam_soc_dma_buffer *buffer;

	list_for_each_entry(buffer, &rsocdev->user_memorys, list) {
		if (phy_addr == buffer->phy_addr)
			return buffer;
	}

	return NULL;
}

static struct rtscam_soc_dma_buffer *__find_dma_buffer_by_index(
    struct rtscam_soc_dev *rsocdev, u32 index)
{
	struct rtscam_soc_dma_buffer *buffer;

	list_for_each_entry(buffer, &rsocdev->user_memorys, list) {
		if (index == buffer->index)
			return buffer;
	}

	return NULL;
}

static u32 __calc_dma_buffer_index(struct rtscam_soc_dev *rsocdev)
{
	struct rtscam_soc_dma_buffer *buffer;
	u32 index = 0;
	int flag = 1;

	while (flag) {
		flag = 0;
		list_for_each_entry(buffer, &rsocdev->user_memorys, list) {
			if (buffer->index == index) {
				index++;
				flag = 1;
			}
		}
	}

	return index;
}

static void rtscam_buffer_put(void *buf_priv)
{
	struct rtscam_soc_dma_buffer *buffer = buf_priv;
	struct rtscam_video_device *icd;
	struct rtscam_soc_dev *rsocdev;

	if (!atomic_dec_and_test(&buffer->refcount))
		return;

	icd = to_rtscam_video_device(buffer->dev);
	rsocdev = container_of(icd, struct rtscam_soc_dev, rvdev);

	rtscam_soc_dma_free_coherent(rsocdev, buffer);
	list_del_init(&buffer->list);
	kfree(buffer);
}

static int __alloc_dma_buffer(struct rtscam_soc_dev *rsocdev,
                              struct rtscam_soc_dma_buf *pbuf)
{
	struct rtscam_soc_dma_buffer *buffer;
	int ret;

	if (!pbuf)
		return -EINVAL;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;
	atomic_set(&buffer->refcount, 0);

	buffer->size = pbuf->size;
	ret = rtscam_soc_dma_alloc_coherent(rsocdev, buffer, GFP_KERNEL);
	if (ret) {
		kfree(buffer);
		return ret;
	}
	buffer->index = __calc_dma_buffer_index(rsocdev);
	buffer->handler.refcount = &buffer->refcount;
	buffer->handler.put = rtscam_buffer_put;
	buffer->handler.arg = buffer;
	atomic_inc(&buffer->refcount);

	list_add_tail(&buffer->list, &rsocdev->user_memorys);

	pbuf->size = buffer->size;
	pbuf->dma_addr = buffer->phy_addr;
	pbuf->offset = buffer->index << PAGE_SHIFT;
	pbuf->vaddr = 0;

	return 0;
}

static int __free_dma_buffer(struct rtscam_soc_dev *rsocdev,
                             unsigned long dma_addr)
{
	struct rtscam_soc_dma_buffer *buffer;

	buffer = __find_dma_buffer(rsocdev, dma_addr);

	if (buffer)
		rtscam_buffer_put(buffer);

	return 0;
}

static int rtscam_soc_mem_open(struct file *filp)
{
	struct rtscam_ge_device *gdev = rtscam_devdata(filp);
	struct rtscam_soc_dev *rsocdev = rtscam_ge_get_drvdata(gdev);

	filp->private_data = rsocdev;

	return 0;
}

static int rtscam_soc_mem_close(struct file *filp)
{
	filp->private_data = NULL;

	return 0;
}

static long rtscam_soc_mem_do_ioctl(struct file *filp, unsigned int cmd,
                                    void *arg)
{
	struct rtscam_soc_dev *rsocdev = filp->private_data;
	long ret = -EINVAL;

	switch (cmd) {
	case RTSOCIOC_ALLOC_DMA: {
		struct rtscam_soc_dma_buf *pbuf = arg;
		ret = __alloc_dma_buffer(rsocdev, pbuf);
		break;
	}
	case RTSOCIOC_FREE_DMA: {
		unsigned long daddr = *(unsigned long *)arg;
		ret = __free_dma_buffer(rsocdev, daddr);
		break;
	}
	default:
		rtsprintk(RTS_TRACE_ERROR,
		          "unknown[rtsmem] ioctl 0x%08x, '%c' 0x%x\n",
		          cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static long rtscam_soc_mem_ioctl(struct file *filp, unsigned int cmd,
                                 unsigned long arg)
{
	return rtscam_usercopy(filp, cmd, arg, rtscam_soc_mem_do_ioctl);
}

static int rtscam_soc_mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct rtscam_soc_dev *rsocdev = filp->private_data;
	struct rtscam_soc_dma_buffer *buffer;
	u32 index = vma->vm_pgoff;
	int ret;

	buffer = __find_dma_buffer_by_index(rsocdev, index);
	if (!buffer)
		return -EINVAL;

	vma->vm_pgoff = 0;

	ret = dma_mmap_coherent(buffer->dev, vma, buffer->vaddr,
	                        buffer->phy_addr, buffer->size);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR,
		          "maping dma memory fail, error =  %d\n", ret);
		return ret;
	}
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = &buffer->handler;
	vma->vm_ops = &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	return 0;
}

static struct rtscam_ge_file_operations rtscam_soc_mem_fops = {
	.owner		= THIS_MODULE,
	.open		= rtscam_soc_mem_open,
	.release	= rtscam_soc_mem_close,
	.unlocked_ioctl	= rtscam_soc_mem_ioctl,
	.mmap		= rtscam_soc_mem_mmap,
};

static int rtscam_soc_create_memory_dev(struct rtscam_soc_dev *rsocdev)
{
	if (rsocdev->mem_dev)
		return 0;

	rsocdev->mem_dev = __create_device(rsocdev,
	                                   RTS_SOC_MEMORY_DEV_NAME, &rtscam_soc_mem_fops);
	if (!rsocdev->mem_dev)
		return -EINVAL;

	return 0;
}

static void rtscam_soc_remove_memory_dev(struct rtscam_soc_dev *rsocdev)
{
	__remove_device(rsocdev->mem_dev);
	rsocdev->mem_dev = NULL;
}

static int rtscam_soc_cam_open(struct file *filp)
{
	return 0;
}

static int rtscam_soc_cam_close(struct file *filp)
{
	return 0;
}

static int __detach_device(struct rtscam_soc_dev *rsocdev)
{
	int ret = -EINVAL;
	struct rtscam_video_device *icd = &rsocdev->rvdev;

	if (!rsocdev->initialized)
		return 0;

	if (0 == atomic_read(&icd->use_count))
		ret = rtscam_soc_detach(rsocdev);
	else
		rtsprintk(RTS_TRACE_ERROR,
		          "close device before detach camera\n");

	return ret;
}

static int __load_fw(struct rtscam_soc_dev *rsocdev, struct rtscam_soc_fw *fw)
{
	int ret = -EINVAL;
#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
	unsigned int length;
	struct rtscam_soc_dma_buffer *fw_buffer = NULL;

	if (!rsocdev || !fw)
		return -EINVAL;

	fw_buffer = &rsocdev->fw_buffer;

	ret = __detach_device(rsocdev);
	if (ret)
		return ret;

	length = fw->length;
	if (length > fw_buffer->size)
		length = fw_buffer->size;

	copy_from_user(fw_buffer->vaddr, (void __user *)fw->fw, length);
	rsocdev->fw_update = 0;

	ret = rtscam_soc_attach(rsocdev);
#endif

	return ret;
}

static int __dump_fw(struct rtscam_soc_dev *rsocdev, struct rtscam_soc_fw *fw)
{
	int ret = -EINVAL;
#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
	unsigned int length;
	struct rtscam_soc_dma_buffer *fw_buffer = NULL;

	if (!rsocdev || !fw)
		return -EINVAL;

	fw_buffer = &rsocdev->fw_buffer;

	if (rsocdev->fw_update) {
		rtsprintk(RTS_TRACE_ERROR, "please load fw before dump\n");
		return -EINVAL;
	}

	length = fw->length;
	if (length > fw_buffer->size)
		length = fw_buffer->size;

	copy_to_user((void __user *)fw->fw, fw_buffer->vaddr, length);
	ret = 0;
#endif
	return ret;
}

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_USER)
static int __download_firmware(struct rtscam_soc_dev *rsocdev)
{
	int ret;
	const struct firmware *fw;
	unsigned int length;
	struct rtscam_soc_dma_buffer *fw_buffer = &rsocdev->fw_buffer;

	rtsprintk(RTS_TRACE_INFO,
	          "begin to load fw from %s\n", FIRMWARE_RTS_ISP);

	ret = request_firmware(&fw, FIRMWARE_RTS_ISP, rsocdev->dev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "Fail to load %s, %d.\n",
		          FIRMWARE_RTS_ISP, ret);
		return ret;
	}

	rtsprintk(RTS_TRACE_INFO, "Load firmware size : %d.\n", fw->size);

	length = fw->size;
	if (length > fw_buffer->size)
		length = fw_buffer->size;

	memcpy(fw_buffer->vaddr, fw->data, length);
	rsocdev->fw_update = 0;
	release_firmware(fw);

	return 0;
}

static int __load_firmware(struct rtscam_soc_dev *rsocdev)
{
	int ret = 0;

	if (!rsocdev) {
		rtsprintk(RTS_TRACE_ERROR, "NULL point\n");
		return -EINVAL;
	}

	ret = __detach_device(rsocdev);
	if (ret)
		return ret;

	ret = __download_firmware(rsocdev);
	if (ret)
		return ret;

	ret = rtscam_soc_attach(rsocdev);

	return ret;
}

static int __download_iq_table(struct rtscam_soc_dev *rsocdev, char *iqtable)
{
	int ret = 0;
	const struct firmware *iq;
	unsigned int length;
	struct rtscam_soc_dma_buffer *fw_buffer = NULL;
	unsigned int offset = RTS_MCU_IQTABLE_OFFSET;

	if (!rsocdev) {
		rtsprintk(RTS_TRACE_ERROR, "NULL point\n");
		return -EINVAL;
	}

	if (!iqtable)
		return -EINVAL;

	if (rsocdev->fw_update) {
		rtsprintk(RTS_TRACE_ERROR, "pls load fw before iq table\n");
		return -EINVAL;
	}

	fw_buffer = &rsocdev->fw_buffer;

	ret = request_firmware(&iq, iqtable, rsocdev->dev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "Fail to load %s, %d.\n",
		          iqtable, ret);
		return ret;
	}

	rtsprintk(RTS_TRACE_INFO, "Load iq table size : %d.\n", iq->size);

	if ((*(u8 *)(fw_buffer->vaddr + offset)) != (*(u8 *)iq->data)) {
		rtsprintk(RTS_TRACE_ERROR,
		          "fw's ver[%d] and iq table's ver[%d] is not match\n",
		          (*(u8 *)(fw_buffer->vaddr + offset)),
		          (*(u8 *)iq->data));
		ret = -EINVAL;
		goto exit;
	}
	length = get_unaligned_be16(iq->data + 1);
	if (iq->size < length + RTS_MCU_IQTABLE_HEADER_SIZE) {
		rtsprintk(RTS_TRACE_ERROR,
		          "iq table is size is not correct\n");
		ret = -EINVAL;
		goto exit;
	}

	memcpy(fw_buffer->vaddr + RTS_MCU_IQTABLE_OFFSET,
	       iq->data, length + RTS_MCU_IQTABLE_HEADER_SIZE);

exit:
	release_firmware(iq);
	return ret;
}

static int __load_iq_table(struct rtscam_soc_dev *rsocdev, char *iqtable)
{
	int ret = 0;

	if (!rsocdev || !iqtable) {
		rtsprintk(RTS_TRACE_ERROR, "NULL point\n");
		return -EINVAL;
	}

	ret = __detach_device(rsocdev);
	if (ret)
		return ret;

	ret = __download_iq_table(rsocdev, iqtable);
	if (ret)
		return ret;

	ret = rtscam_soc_attach(rsocdev);

	return ret;
}

static int __load_firmware_and_iq_table(struct rtscam_soc_dev *rsocdev,
                                        char *iqtable)
{
	int ret = 0;

	if (!rsocdev || !iqtable) {
		rtsprintk(RTS_TRACE_ERROR, "NULL point\n");
		return -EINVAL;
	}

	ret = __detach_device(rsocdev);
	if (ret)
		return ret;

	ret = __download_firmware(rsocdev);
	if (ret)
		return ret;

	ret = __download_iq_table(rsocdev, iqtable);
	if (ret)
		return ret;

	ret = rtscam_soc_attach(rsocdev);

	return ret;
}
#endif

static long rtscam_soc_cam_do_ioctl(struct file *filp, unsigned int cmd,
                                    void *arg)
{
	struct rtscam_soc_dev *rsocdev;
	struct rtscam_video_device *icd;
	int ret = 0;

	if (NULL == m_rsocdev)
		return -EINVAL;

	rsocdev = m_rsocdev;
	icd = &rsocdev->rvdev;

	mutex_lock(&icd->dev_lock);
	switch (cmd) {
	case RTSOCIOC_CAMERA_DETACH:
		ret = __detach_device(rsocdev);
		break;
	case RTSOCIOC_CAMERA_ATTACH:
		ret = rtscam_soc_attach(rsocdev);
		break;
#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
	case RTSOCIOC_CAMERA_LOAD_FW:
		ret = __load_fw(rsocdev, arg);
		break;
	case RTSOCIOC_CAMERA_DUMP_FW:
		ret = __dump_fw(rsocdev, arg);
		break;
	case RTSOCIOC_CAMERA_G_FW_SIZE:
		*(unsigned int *)arg = rsocdev->fw_buffer.size;
		break;
#endif
	default:
		rtsprintk(RTS_TRACE_ERROR,
		          "Unknown[rtscam] ioctl 0x%08x, type = '%c' nr = 0x%x\n",
		          cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		ret = -ENOTTY;
		break;
	}
	mutex_unlock(&icd->dev_lock);

	return ret;
}

static long rtscam_soc_cam_ioctl(struct file *filp, unsigned int cmd,
                                 unsigned long arg)
{
	return rtscam_usercopy(filp, cmd, arg, rtscam_soc_cam_do_ioctl);
}

static struct rtscam_ge_file_operations rtscam_soc_cam_fops = {
	.owner		= THIS_MODULE,
	.open		= rtscam_soc_cam_open,
	.release	= rtscam_soc_cam_close,
	.unlocked_ioctl	= rtscam_soc_cam_ioctl,
};

static int rtscam_soc_create_cam_dev(struct rtscam_soc_dev *rsocdev)
{
	if (rsocdev->cam_dev)
		return 0;

	rsocdev->cam_dev = __create_device(rsocdev,
	                                   RTS_SOC_CAM_DEV_NAME, &rtscam_soc_cam_fops);
	if (!rsocdev->cam_dev)
		return -EINVAL;

	return 0;
}

static void rtscam_soc_remove_cam_dev(struct rtscam_soc_dev *rsocdev)
{
	__remove_device(rsocdev->cam_dev);
	rsocdev->cam_dev = NULL;
}

static int rtscam_soc_ctrl_open(struct file *filp)
{
	struct rtscam_ge_device *gdev = rtscam_devdata(filp);
	struct rtscam_soc_dev *rsocdev = rtscam_ge_get_drvdata(gdev);
	struct rtscam_video_device *icd = &rsocdev->rvdev;
	int ret = 0;

	if (!rsocdev->initialized)
		return -EINVAL;

	if (mutex_lock_interruptible(&icd->dev_lock))
		return -ERESTARTSYS;

	if (atomic_inc_return(&icd->use_count) == 1) {
		ret = rtscam_soc_start_clock(icd);
		if (ret < 0) {
			rtsprintk(RTS_TRACE_ERROR,
			          "couldn't activate the mcu:%d\n",
			          ret);
			atomic_dec(&icd->use_count);
			mutex_unlock(&icd->dev_lock);
			return ret;
		}
	}

	mutex_unlock(&icd->dev_lock);

	filp->private_data = icd;

	return 0;
}

static int rtscam_soc_ctrl_close(struct file *filp)
{
	struct rtscam_video_device *icd = filp->private_data;

	mutex_lock(&icd->dev_lock);
	if (atomic_dec_return(&icd->use_count) == 0)
		rtscam_soc_stop_clock(icd);
	mutex_unlock(&icd->dev_lock);

	filp->private_data = NULL;

	return 0;
}

static long rtscam_soc_ctrl_do_ioctl(struct file *filp, unsigned int cmd,
                                     void *arg)
{
	struct rtscam_video_device *icd = filp->private_data;

	return rtscam_video_do_ctrl_ioctl(icd, cmd, arg);
}

static long rtscam_soc_ctrl_ioctl(struct file *filp, unsigned int cmd,
                                  unsigned long arg)
{
	return rtscam_usercopy(filp, cmd, arg, rtscam_soc_ctrl_do_ioctl);
}

static struct rtscam_ge_file_operations rtscam_soc_ctrl_fops = {
	.owner		= THIS_MODULE,
	.open		= rtscam_soc_ctrl_open,
	.release	= rtscam_soc_ctrl_close,
	.unlocked_ioctl	= rtscam_soc_ctrl_ioctl,
};

static int rtscam_soc_create_ctrl_dev(struct rtscam_soc_dev *rsocdev)
{
	if (rsocdev->ctrl_dev)
		return 0;

	rsocdev->ctrl_dev = __create_device(rsocdev,
	                                    RTS_SOC_CTRL_DEV_NAME, &rtscam_soc_ctrl_fops);
	if (!rsocdev->ctrl_dev)
		return -EINVAL;

	return 0;

}

static void rtscam_soc_remove_ctrl_dev(struct rtscam_soc_dev *rsocdev)
{
	__remove_device(rsocdev->ctrl_dev);
	rsocdev->ctrl_dev = NULL;
}

static u32 __get_next_small_fps(u32 fth)
{
	u32 a;

	if (fth > m_rtscam_soc_max_fps)
		return m_rtscam_soc_max_fps;
	if (fth == 2)
		return 1;
	if (fth == 1)
		return 0;

	a = (m_rtscam_soc_max_fps - fth) / m_rtscam_soc_step_fps;
	a = m_rtscam_soc_max_fps - a * m_rtscam_soc_step_fps;

	if (a > m_rtscam_soc_step_fps)
		return a - m_rtscam_soc_step_fps;
	return 2;
}

static int rtscam_soc_init_video_stream(struct rtscam_soc_dev *rsocdev,
                                        struct rtscam_video_stream *stream)
{
	struct rtscam_soc_stream_format_descriptor fmt_desc;
	struct rtscam_video_format_xlate *fmt = NULL;
	int i;
	u32 fps;
	struct rtscam_frame_size max;
	struct rtscam_frame_size min = {RTSCAM_SOC_MIN_W, RTSCAM_SOC_MIN_H};
	struct rtscam_frame_size step = {RTSCAM_SOC_STEP_W, RTSCAM_SOC_STEP_H};
	int ret;

	if (TYPE_RLE0745 == RTS_SOC_HW_ID(rsocdev->devtype) &&
	    0 == stream->streamid) {
		min.width = 800;
		min.height = 600;
	}

	fmt_desc.streamid = stream->streamid;

	ret = rtscam_soc_get_vs_format_desc(rsocdev, &fmt_desc);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "enum stream<%d> format fail\n",
		          stream->streamid);
		goto error;
	}

	for (i = 0; i < ARRAY_SIZE(m_rtscam_soc_formats); i++) {
		fmt = &m_rtscam_soc_formats[i];
		if (fmt->rts_code & fmt_desc.format) {
			ret = rtscam_register_format(stream, fmt);
			if (ret)
				goto error;

			max.width = fmt_desc.width;
			max.height = fmt_desc.height;

			ret = rtscam_register_frame_stepwise(stream,
			                                     fmt->fourcc, &max, &min, &step);
			if (ret)
				goto error;

			fps = RTSCAM_SOC_FPS_UNIT / rsocdev->sensor_frmival;
			if (fps > m_rtscam_soc_max_fps)
				fps = m_rtscam_soc_max_fps;

			while (fps > 0) {
				struct v4l2_fract ival;

				ival.numerator = 1;
				ival.denominator = fps;
				ret = rtscam_register_frmival_discrete(stream,
				                                       fmt->fourcc,
				                                       &max,
				                                       &ival);
				if (ret)
					goto error;
				fps = __get_next_small_fps(fps);
			}
		}
	}

	return 0;

error:
	rtscam_clr_format(stream);
	return ret;
}

static int rtscam_soc_release_video_stream(struct rtscam_soc_dev *rsocdev,
        struct rtscam_video_stream *stream)
{
	rtscam_clr_format(stream);
	return 0;
}

static int rtscam_soc_init_video_device(struct rtscam_soc_dev *rsocdev)
{
	struct rtscam_video_device *icd = &rsocdev->rvdev;
	int ret = 0;
	int i;
	struct rtscam_video_stream *streams = NULL;

	if (0 == rsocdev->dev_desc.streamnum)
		return -EINVAL;

	streams = kcalloc(rsocdev->dev_desc.streamnum,
	                  sizeof(struct rtscam_video_stream), GFP_KERNEL);
	if (NULL == streams)
		return -ENOMEM;

	for (i = 0; i < rsocdev->dev_desc.streamnum; i++) {
		struct rtscam_video_stream *stream = streams + i;

		stream->streamid = i;
		ret = rtscam_soc_init_video_stream(rsocdev, stream);
		if (ret) {
			rtsprintk(RTS_TRACE_ERROR,
			          "init stream<%d> fail\n", i);
			goto error;
		}

	}

	icd->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	icd->streams = streams;
	icd->streamnum = rsocdev->dev_desc.streamnum;

	icd->ops = &rtscam_soc_ops;
	icd->drv_name = RTS_SOC_CAMERA_DRV_NAME;
	icd->dev_name = rsocdev->name;
	icd->dev = rsocdev->dev;
	icd->video_nr = RTSCAM_SOC_VIDEO_NR_B;
	icd->alloc_ctx = rsocdev->alloc_ctx;
	icd->mem_ops = rsocdev->mem_ops;

	icd->priv = rsocdev;

	icd->initialized = 1;

	return 0;
error:
	for (i = 0; i < rsocdev->dev_desc.streamnum; i++) {
		struct rtscam_video_stream *stream = streams + i;

		rtscam_soc_release_video_stream(rsocdev, stream);
	}
	kfree(streams);
	streams = NULL;

	return ret;
}

static int rtscam_soc_release_video_deivce(struct rtscam_soc_dev *rsocdev)
{
	struct rtscam_video_device *icd = &rsocdev->rvdev;
	int i;

	for (i = 0; i < icd->streamnum; i++) {
		struct rtscam_video_stream *stream = icd->streams + i;

		rtscam_soc_release_video_stream(rsocdev, stream);
	}
	kfree(icd->streams);
	icd->streams = NULL;
	icd->streamnum = 0;

	icd->ops = NULL;
	icd->drv_name = NULL;
	icd->dev_name = NULL;
	icd->dev = NULL;
	icd->alloc_ctx = NULL;
	icd->mem_ops = NULL;

	icd->priv = NULL;

	icd->initialized = 0;

	return 0;
}

static int rtscam_soc_release(struct rtscam_soc_dev *rsocdev)
{
	int i;

	if (!rsocdev)
		return -EINVAL;

	rtscam_soc_release_video_deivce(rsocdev);

	rtscam_soc_clear_isp_header(rsocdev);

	memset(&rsocdev->hclk_desc, 0, sizeof(rsocdev->hclk_desc));

	for (i = 0; i < ARRAY_SIZE(rsocdev->entities); i++) {
		struct rtscam_soc_unit_descriptor *entity;
		int j;

		entity = &rsocdev->entities[i];
		for (j = 0; j < entity->ncontrols; j++) {
			struct rtscam_video_ctrl *ctrl = entity->controls + i;
			if (!ctrl->initialized)
				continue;
			rtscam_ctrl_clr_mapping(ctrl);
			rtscam_ctrl_clr_info(ctrl);
		}

		kfree(entity->controls);
		memset(entity, 0, sizeof(struct rtscam_soc_unit_descriptor));
	}

	memset(&rsocdev->dev_desc, 0, sizeof(rsocdev->dev_desc));
	return 0;
}

static int rtscam_soc_init(struct rtscam_soc_dev *rsocdev)
{
	int ret = 0;
	struct rtscam_soc_frmival_descriptor frmival_desc;
	int i;

	if (!rsocdev)
		return -EINVAL;

	ret = rtscam_soc_get_api_version(rsocdev, &rsocdev->api_version);
	if (0 == ret)
		rtsprintk(RTS_TRACE_INFO, "Found ISP %d.%03d device\n",
		          rsocdev->api_version.main,
		          rsocdev->api_version.sub);

	ret = rtscam_soc_get_device_des(rsocdev, &rsocdev->dev_desc);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "get device descriptor fail\n");
		goto error;
	}
	if (rsocdev->dev_desc.streamnum > RTSCAM_SOC_MAX_STREAM_NUM) {
		rtsprintk(RTS_TRACE_ERROR,
		          "cann't support %d streams, max %d\n",
		          rsocdev->dev_desc.streamnum,
		          RTSCAM_SOC_MAX_STREAM_NUM);
		ret = -EINVAL;
		goto error;
	}

	if (0 == rsocdev->dev_desc.frmivalnum) {
		ret = -EINVAL;
		goto error;
	}

	frmival_desc.frmivalnum = rsocdev->dev_desc.frmivalnum;
	frmival_desc.frmivals = kzalloc(frmival_desc.frmivalnum * sizeof(u32),
	                                GFP_KERNEL);
	if (!frmival_desc.frmivals) {
		ret = -ENOMEM;
		goto error;
	}

	ret = rtscam_soc_get_frmival_desc(rsocdev, &frmival_desc);
	if (ret) {
		kfree(frmival_desc.frmivals);
		rtsprintk(RTS_TRACE_ERROR, "get frmival desc fain\n");
		goto error;
	}
	rsocdev->sensor_frmival = *frmival_desc.frmivals;
	for (i = 1; i < frmival_desc.frmivalnum; i++) {
		u32 tmp = *(frmival_desc.frmivals + i);
		if (rsocdev->sensor_frmival > tmp)
			rsocdev->sensor_frmival = tmp;
	}

	kfree(frmival_desc.frmivals);

	ret = rtscam_soc_get_isp_processing_desc(rsocdev,
	        &rsocdev->entities[0]);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "get isp process desc fail\n");
		goto error;
	}

	ret = rtscam_soc_get_camera_desc(rsocdev, &rsocdev->entities[1]);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "get camera desc fail\n");
		goto error;
	}

	ret = rtscam_soc_get_ext_controls_desc(rsocdev, &rsocdev->entities[2]);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "get extended control desc fail\n");
		goto error;
	}

	ret = rtscam_soc_get_hclk_desc(rsocdev, &rsocdev->hclk_desc);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "get hclk desc fail\n");
		goto error;
	}

	rtscam_soc_get_fw_version(rsocdev, &rsocdev->fw_version);

	ret = rtscam_soc_ctrl_init_device(rsocdev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "init device ctrl fail\n");
		goto error;
	}

	ret = rtscam_soc_config_isp_header(rsocdev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "config isp header fail\n");
		goto error;
	}

	ret = rtscam_soc_init_video_device(rsocdev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "init rts video device fail\n");
		goto error;
	}

	ret = rtscam_soc_config_isp_buffer(rsocdev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "config isp buffer fail\n");
		goto error;
	}

	return 0;
error:
	rtscam_soc_release(rsocdev);
	return ret;
}

static ssize_t show_fwinfo(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
	struct rtscam_video_device *icd = to_rtscam_video_device(dev);
	struct rtscam_soc_dev *rsocdev = container_of(icd,
	                                 struct rtscam_soc_dev, rvdev);
	struct rtscam_soc_fw_version_t *version = &rsocdev->fw_version;
	int num = 0;

	num += scnprintf(buf, PAGE_SIZE,
	                 "header   = 0x%02x\n", version->header);
	num += scnprintf(buf + num, PAGE_SIZE,
	                 "length   = 0x%02x\n", version->length);
	num += scnprintf(buf + num, PAGE_SIZE,
	                 "magictag = 0x%08x\n", version->magictag);
	num += scnprintf(buf + num, PAGE_SIZE,
	                 "ic name  = 0x%04x\n", version->ic_name);
	num += scnprintf(buf + num, PAGE_SIZE,
	                 "vid      = 0x%04x\n", version->vid);
	num += scnprintf(buf + num, PAGE_SIZE,
	                 "pid      = 0x%04x\n", version->pid);
	num += scnprintf(buf + num, PAGE_SIZE,
	                 "fw ver   = 0x%08x\n", version->fw_ver);
	num += scnprintf(buf + num, PAGE_SIZE,
	                 "cus ver  = 0x%08x\n", version->cus_ver);

	return num;
}
DEVICE_ATTR(fwinfo, S_IRUGO, show_fwinfo, NULL);

static ssize_t show_meminfo(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
	struct rtscam_video_device *icd = to_rtscam_video_device(dev);
	struct rtscam_soc_dev *rsocdev = container_of(icd,
	                                 struct rtscam_soc_dev, rvdev);
	int num = 0;

	num += scnprintf(buf + num, PAGE_SIZE, "total : %ld\n",
	                 rtscam_mem_get_total_size(&rsocdev->rtsmem));
	num += scnprintf(buf + num, PAGE_SIZE, "used  : %ld\n",
	                 rtscam_mem_get_used_size(&rsocdev->rtsmem));
	num += scnprintf(buf + num, PAGE_SIZE, "left  : %ld\n",
	                 rtscam_mem_get_left_size(&rsocdev->rtsmem));

	return num;
}
DEVICE_ATTR(meminfo, S_IRUGO, show_meminfo, NULL);

static ssize_t show_apiversion(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
	struct rtscam_video_device *icd = to_rtscam_video_device(dev);
	struct rtscam_soc_dev *rsocdev = container_of(icd,
	                                 struct rtscam_soc_dev, rvdev);
	int num = 0;

	num += scnprintf(buf, PAGE_SIZE, "ISP %d.%03d\n",
	                 rsocdev->api_version.main, rsocdev->api_version.sub);

	return num;
}
DEVICE_ATTR(apiversion, S_IRUGO, show_apiversion, NULL);

static ssize_t show_overflow(struct device *dev,
                             struct device_attribute *attr, char *buf)
{
	struct rtscam_video_device *icd = to_rtscam_video_device(dev);
	struct rtscam_video_stream *stream = NULL;
	int num = 0;
	int i;

	for (i = 0; i < icd->streamnum; i++) {
		stream = icd->streams + i;
		num += scnprintf(buf + num, PAGE_SIZE,
		                 "stream[%d] overflow %ld\n",
		                 i, stream->overflow);
	}

	return num;
}
DEVICE_ATTR(overflow, S_IRUGO, show_overflow, NULL);

static ssize_t rtscam_soc_get_mcu_timeout(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	struct rtscam_video_device *icd = to_rtscam_video_device(dev);
	struct rtscam_soc_dev *rsocdev = container_of(icd,
	                                 struct rtscam_soc_dev, rvdev);
	int num = 0;

	num += scnprintf(buf, PAGE_SIZE, "%u\n", rsocdev->mcu_timeout);

	return num;
}

static ssize_t rtscam_soc_set_mcu_timeout(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct rtscam_video_device *icd = to_rtscam_video_device(dev);
	struct rtscam_soc_dev *rsocdev = container_of(icd,
	                                 struct rtscam_soc_dev, rvdev);
	u32 timeout = 0;

	sscanf(buf, "%u", &timeout);
	if (timeout < RTSCAM_SOC_CMD_TIMEOUT_MIN)
		rsocdev->mcu_timeout = RTSCAM_SOC_CMD_TIMEOUT_MIN;
	else if (timeout > RTSCAM_SOC_CMD_TIMEOUT_MAX)
		rsocdev->mcu_timeout = RTSCAM_SOC_CMD_TIMEOUT_MAX;
	else
		rsocdev->mcu_timeout = timeout;

	return count;
}
static DEVICE_ATTR(timeout, S_IRUGO | S_IWUGO,
                   rtscam_soc_get_mcu_timeout, rtscam_soc_set_mcu_timeout);

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_USER)
static ssize_t rtscam_soc_get_loadfw(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
	int num = 0;

	num += scnprintf(buf, PAGE_SIZE, "%u\n", 0);

	return num;
}

static ssize_t rtscam_soc_set_loadfw(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t count)
{
	struct rtscam_soc_dev *rsocdev = m_rsocdev;
	int load = 0;
	char iqtable[128] = {0};
	int ret = 0;

	if (!rsocdev) {
		rtsprintk(RTS_TRACE_ERROR, "rts soc dev is not init\n");
		return -EINVAL;
	}

	sscanf(buf, "%d %127s", &load, iqtable);

	mutex_lock(&rsocdev->rvdev.dev_lock);
	if (load && strlen(iqtable))
		ret = __load_firmware_and_iq_table(rsocdev, iqtable);
	else if (load)
		ret = __load_firmware(rsocdev);
	else if (strlen(iqtable))
		ret = __load_iq_table(rsocdev, iqtable);
	mutex_unlock(&rsocdev->rvdev.dev_lock);

	if (ret)
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(loadfw, S_IRUGO | S_IWUGO,
                   rtscam_soc_get_loadfw, rtscam_soc_set_loadfw);
#endif

int rtscam_soc_attach(struct rtscam_soc_dev *rsocdev)
{
	int ret = 0;
	struct rtscam_video_device *icd = NULL;

	if (!rsocdev)
		return -EINVAL;

	if (rsocdev->initialized)
		return 0;

	if (atomic_inc_return(&rsocdev->init_count) != 1) {
		atomic_dec(&rsocdev->init_count);
		return -EBUSY;
	}

	icd = &rsocdev->rvdev;

	ret = rtscam_soc_enable_mcu(rsocdev, 1);
	if (ret) {
#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
		rsocdev->fw_update = 1;
#endif
		goto eenable;
	}

	ret = rtscam_soc_init(rsocdev);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "init rts soc camera fail\n");
		goto einit;
	}

	ret = rtscam_video_register_device(icd);
	if (ret) {
		rtsprintk(RTS_TRACE_ERROR, "register rts video device fail\n");
		goto ereg;
	}

#if RTS_SOC_CLK_ON_NEED
	rtscam_soc_enable_mcu(rsocdev, 0);
#endif

	device_create_file(rsocdev->dev, &dev_attr_fwinfo);
	device_create_file(rsocdev->dev, &dev_attr_meminfo);
	device_create_file(rsocdev->dev, &dev_attr_timeout);
	device_create_file(rsocdev->dev, &dev_attr_apiversion);
	device_create_file(rsocdev->dev, &dev_attr_overflow);

	rsocdev->initialized = 1;

	rtsprintk(RTS_TRACE_INFO, "%s initialized\n",
	          rsocdev->name);

	return 0;

ereg:
	rtscam_soc_release(rsocdev);
einit:
	rtscam_soc_enable_mcu(rsocdev, 0);
eenable:
	if (ret)
		atomic_dec(&rsocdev->init_count);
	return ret;
}

int rtscam_soc_detach(struct rtscam_soc_dev *rsocdev)
{
	struct rtscam_video_device *icd = NULL;

	if (!rsocdev)
		return -EINVAL;

	if (!rsocdev->initialized)
		return 0;

	if (atomic_dec_return(&rsocdev->init_count) != 0) {
		atomic_inc(&rsocdev->init_count);
		return -EBUSY;
	}

	device_remove_file(rsocdev->dev, &dev_attr_fwinfo);
	device_remove_file(rsocdev->dev, &dev_attr_meminfo);
	device_remove_file(rsocdev->dev, &dev_attr_timeout);
	device_remove_file(rsocdev->dev, &dev_attr_apiversion);
	device_remove_file(rsocdev->dev, &dev_attr_overflow);

	icd = &rsocdev->rvdev;

	rtscam_video_unregister_device(icd);

	rtscam_soc_release(rsocdev);

	rtscam_soc_enable_mcu(rsocdev, 0);
#if (RTS_MCU_FW_FLAG == RTS_MCU_FW_MASK_DDR)
	rsocdev->fw_update = 1;
#endif
	rsocdev->initialized = 0;
	return 0;
}

int rtscam_soc_probe(struct platform_device *pdev)
{
	struct rtscam_soc_dev *rsocdev;
	struct resource *res;
	void __iomem *base;
	int irq;
	const struct platform_device_id *id_entry;
	int err = 0;

	rtsprintk(RTS_TRACE_INFO, "%s\n", __func__);

	id_entry = platform_get_device_id(pdev);
	if (!id_entry) {
		rtsprintk(RTS_TRACE_ERROR, "not support soc camera\n");
		return -EINVAL;
	}

	if (!pdev->dev.platform_data) {
		rtsprintk(RTS_TRACE_ERROR, "please isp platform_data\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);

	if (NULL == res || irq < 0) {
		rtsprintk(RTS_TRACE_ERROR, "Missing platform resource data\n");
		err = -ENODEV;
		goto out;
	}

	rsocdev = devm_kzalloc(&pdev->dev, sizeof(*rsocdev), GFP_KERNEL);
	if (NULL == rsocdev) {
		rtsprintk(RTS_TRACE_ERROR,
		          "Couldn't allocate rts camera soc object\n");
		err = ENOMEM;
		goto out;
	}

	rsocdev->dev = get_device(&pdev->dev);

	mutex_init(&rsocdev->cmd_lock);
	init_completion(&rsocdev->cmd_completion);
	atomic_set(&rsocdev->init_count, 0);
	atomic_set(&rsocdev->mcu_count, 0);
	rsocdev->mcu_state = RTSCAM_STATE_PASSIVE;
	INIT_LIST_HEAD(&rsocdev->user_memorys);
#ifdef CONFIG_REGULATOR_RTP
	atomic_set(&rsocdev->pwr_use, 0);
#endif
	mutex_init(&rsocdev->rvdev.dev_lock);
	mutex_init(&rsocdev->rvdev.ctrl_lock);

	strncpy(rsocdev->name, id_entry->name, PLATFORM_NAME_SIZE);
	rsocdev->devtype = id_entry->driver_data;
	rsocdev->support =
	    __get_soc_camera_support(RTS_SOC_HW_ID(rsocdev->devtype));

	rsocdev->pdata = pdev->dev.platform_data;
	rsocdev->isp_capibility = rsocdev->pdata->capibility;
	err = rtscam_mem_init(&rsocdev->rtsmem, rsocdev->dev,
	                      rsocdev->pdata->resvd_mem_base,
	                      rsocdev->pdata->resvd_mem_base,
	                      rsocdev->pdata->resvd_mem_size);
	if (err) {
		rtsprintk(RTS_TRACE_ERROR, "rtscam_mem_init fail : %d\n", err);
		goto out;
	}

	rsocdev->clk = devm_clk_get(rsocdev->dev, "mcu_ck");
	if (IS_ERR(rsocdev->clk)) {
		rtsprintk(RTS_TRACE_ERROR, "Get mcu clk failed\n");
		err = PTR_ERR(rsocdev->clk);
		goto out;
	}

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		rtsprintk(RTS_TRACE_ERROR, "Couldn't ioremap resource\n");
		err = PTR_ERR(base);
		goto out;
	}

	rsocdev->base = base;

	rsocdev->iostart = res->start;
	rsocdev->iosize = resource_size(res);

	err = devm_request_irq(rsocdev->dev, irq, rtscam_soc_irq,
	                       IRQF_SHARED, RTS_SOC_CAMERA_DRV_NAME, rsocdev);
	if (err) {
		rtsprintk(RTS_TRACE_ERROR, "request irq failed\n");
		goto out;
	}

	rsocdev->alloc_ctx = rts_dma_contig_init_ctx(rsocdev->dev,
	                     &rsocdev->rtsmem);
	if (IS_ERR(rsocdev->alloc_ctx)) {
		rtsprintk(RTS_TRACE_ERROR, "init vb2 ctx failed\n");
		err = PTR_ERR(rsocdev->alloc_ctx);
		goto out;
	}
	/*
	 * rsocdev->mem_ops = &vb2_dma_contig_memops;
	 */
	rsocdev->mem_ops = &rts_dma_contig_memops;

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
	rsocdev->fw_buffer.size = RTS_MCU_FW_SIZE;
	rtscam_soc_dma_alloc_coherent(rsocdev,
	                              &rsocdev->fw_buffer, GFP_KERNEL);
	rsocdev->fw_update = 1;
#endif
	rsocdev->mcu_timeout = RTSCAM_SOC_CMD_TIMEOUT;

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_USER)
	device_create_file(rsocdev->dev, &dev_attr_loadfw);
#else
	err = rtscam_soc_attach(rsocdev);
	if (err)
		goto out;
#endif

	m_rsocdev = rsocdev;
	rtscam_soc_create_memory_dev(rsocdev);
	rtscam_soc_create_cam_dev(rsocdev);
	rtscam_soc_create_ctrl_dev(rsocdev);

	return 0;
out:
	rtscam_mem_release(&rsocdev->rtsmem);
	m_rsocdev = NULL;
	return err;
}

int rtscam_soc_remove(struct platform_device *pdev)
{
	struct rtscam_video_device *icd = to_rtscam_video_device(&pdev->dev);
	struct rtscam_soc_dev *rsocdev = icd->priv;

	m_rsocdev = NULL;

	rtscam_soc_remove_ctrl_dev(rsocdev);
	rtscam_soc_remove_cam_dev(rsocdev);
	rtscam_soc_remove_memory_dev(rsocdev);

	rtscam_soc_detach(rsocdev);
#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_DDR)
	rtscam_soc_dma_free_coherent(rsocdev, &rsocdev->fw_buffer);
#endif
	rts_dma_contig_cleanup_ctx(rsocdev->alloc_ctx);

#if (RTS_MCU_FW_FLAG & RTS_MCU_FW_MASK_USER)
	device_remove_file(rsocdev->dev, &dev_attr_loadfw);
#endif
	rtscam_mem_release(&rsocdev->rtsmem);
	put_device(rsocdev->dev);
	rsocdev->dev = NULL;

	return 0;
}

static struct platform_device_id rtsx_soc_camera_devtypes[] = {
	{
		.name = "rle0745-fpga-isp",
		.driver_data = TYPE_RLE0745 | TYPE_FPGA
	},
	{
		.name = "rle0745-isp",
		.driver_data = TYPE_RLE0745
	},
	{
		.name = "rts3901-fpga-isp",
		.driver_data = TYPE_RTS3901 | TYPE_FPGA
	},
	{
		.name = "rts3901-isp",
		.driver_data = TYPE_RTS3901
	},
	{
		.name = "rts3903-fpga-isp",
		.driver_data = TYPE_RTS3903 | TYPE_FPGA
	},
	{
		.name = "rts3903-isp",
		.driver_data = TYPE_RTS3903
	},
	{}
};

static struct platform_driver rtscam_soc_driver = {
	.driver		= {
		.name	= RTS_SOC_CAMERA_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= rtscam_soc_probe,
	.remove		= rtscam_soc_remove,
	.id_table	= rtsx_soc_camera_devtypes,
};

module_platform_driver(rtscam_soc_driver);

MODULE_DESCRIPTION("Realsil Soc Camera device driver");
MODULE_AUTHOR("Ming Qian <ming_qian@realsil.com.cn>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.1");
MODULE_ALIAS("platform:" RTS_SOC_CAMERA_DRV_NAME);
