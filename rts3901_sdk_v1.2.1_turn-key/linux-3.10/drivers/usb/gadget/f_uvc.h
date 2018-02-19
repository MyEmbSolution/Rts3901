/*
 *	f_uvc.h  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#ifndef _F_UVC_H_
#define _F_UVC_H_

#include <linux/usb/composite.h>
#include <linux/usb/video.h>

/* --------------------------------------------------------------------------
 * USB configuration
 */

#define FORMAT_TYPE_NC		0x00
#define FORMAT_TYPE_YUY2	0x11
#define FORMAT_TYPE_MJPG	0x22
#define FORMAT_TYPE_YUV420	0x33
#define FORMAT_TYPE_RAW	0x44
#define FORMAT_TYPE_M420	0x55
#define FORMAT_TYPE_RSDH	0x66
#define FORMAT_TYPE_H264	0x77

#define VS_UNDEFINED		0x00
#define VS_INPUT_HEADER	0x01
#define VS_OUTPUT_HEADER	0x02
#define VS_STILL_IMAGE_FRAME	0x03
#define VS_FORMAT_UNCOMPRESSED	0x04
#define VS_FRAME_UNCOMPRESSED		0x05
#define VS_FORMAT_MJPEG	0x06
#define VS_FRAME_MJPEG		0x07
#define VS_FORMAT_MPEG2TS	0x0A
#define VS_FORMAT_DV		0x0C
#define VS_COLORFORMAT		0x0D
#define VS_FORMAT_VENDOR	0x0E
#define VS_FRAME_VENDOR	0x0F
#define VS_FORMAT_FRAME_BASED	0x10
#define VS_FRAME_FRAME_BASED	0x11
#define VS_FORMAT_STREAM_BASED	0x12
#define VS_FORMAT_H264		0x13
#define VS_FRAME_H264		0x14

#define RATE_CONTROL_VBR_MODE				0x01
#define RATE_CONTROL_CBR_MODE				0x02
#define RATE_CONTROL_CONSTANT_QP_MODE		0x03
#define RATE_CONTROL_GVBR_MODE			0x04
#define RATE_CONTROL_VBRN_MODE			0x05
#define RATE_CONTROL_GVBRN_MODE			0x06

#define H264_INTER_QP_16		0x10
#define H264_INTRA_QP_16		0x10
#define H264_INTER_QP_40		0x28
#define H264_INTRA_QP_38		0x26
#define H264_BASELINE_PROFILE		0x42
#define H264_LEVEL_31			0x1F
#define H264_ENC_CAPBILITY		0x21

#define H264_RATE_CONTROL_MODE ((1<<(RATE_CONTROL_VBR_MODE-1)) \
	|(1<<(RATE_CONTROL_CONSTANT_QP_MODE-1)))

#define ENT_ID_CAMERA_IT		0x01
#define ENT_ID_PROCESSING_UNIT	0x02
#define ENT_ID_EXTENSION_UNIT_DBG	0x04
#define ENT_ID_OUTPUT_TRM		0x03
#define ENT_ID_RTK_EXTENDED_CTL_UNIT	0x06
#define ENT_ID_H264_ENCODING_UNIT	0x5

#define	SRC_ID_OUTPUT_TRM		ENT_ID_EXTENSION_UNIT_DBG
#define SRC_ID_PROCESSING_UNIT	ENT_ID_CAMERA_IT
#ifdef CONFIG_USB_RTSX_UVC_15
#define SRC_ID_EXTENSION_UNIT_DBG	ENT_ID_H264_ENCODING_UNIT
#else
#define SRC_ID_EXTENSION_UNIT_DBG	ENT_ID_PROCESSING_UNIT
#endif
#define SRC_ID_RTK_EXTENDED_CTL_UNIT	ENT_ID_EXTENSION_UNIT_DBG
#define	SRC_ID_H264_ENCODING_UNIT	ENT_ID_PROCESSING_UNIT

#define PU_CONTROL_UNDEFINED 0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL 0x01
#define PU_BRIGHTNESS_CONTROL 0x02
#define PU_CONTRAST_CONTROL 0x03
#define PU_GAIN_CONTROL 0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL 0x05
#define PU_HUE_CONTROL 0x06
#define PU_SATURATION_CONTROL 0x07
#define PU_SHARPNESS_CONTROL 0x08
#define PU_GAMMA_CONTROL 0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0C
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0D
#define PU_DIGITAL_MULTIPLIER_CONTROL 0x0E
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL 0x0F
#define PU_HUE_AUTO_CONTROL 0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL 0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL 0x12

#define CT_CONTROL_UNDEFINED 0x00
#define CT_SCANNING_MODE_CONTROL 0x01
#define CT_AE_MODE_CONTROL 0x02
#define CT_AE_PRIORITY_CONTROL 0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define CT_FOCUS_ABSOLUTE_CONTROL 0x06
#define CT_FOCUS_RELATIVE_CONTROL 0x07
#define CT_FOCUS_AUTO_CONTROL 0x08
#define CT_IRIS_ABSOLUTE_CONTROL 0x09
#define CT_IRIS_RELATIVE_CONTROL 0x0A
#define CT_ZOOM_ABSOLUTE_CONTROL 0x0B
#define CT_ZOOM_RELATIVE_CONTROL 0x0C
#define CT_PANTILT_ABSOLUTE_CONTROL 0x0D
#define CT_PANTILT_RELATIVE_CONTROL 0x0E
#define CT_ROLL_ABSOLUTE_CONTROL 0x0F
#define CT_ROLL_RELATIVE_CONTROL 0x10
#define CT_PRIVACY_CONTROL 0x11
#define CT_ROI_CONTROL		0x14

#define F_SEL_640_480    (0)
#define F_SEL_160_120    (1)
#define F_SEL_176_144    (2)
#define F_SEL_320_180	(3)
#define F_SEL_320_200    (4)
#define F_SEL_320_240    (5)
#define F_SEL_352_288    (6)
#define F_SEL_424_240	(7)
#define F_SEL_640_360	(8)
#define F_SEL_640_400    (9)
#define F_SEL_800_600    (10)
#define F_SEL_848_480	(11)
#define F_SEL_960_540	 (12)
#define F_SEL_1024_768   (13)
#define F_SEL_1280_720   (14)
#define F_SEL_1280_800   (15)
#define F_SEL_1280_960   (16)
#define F_SEL_1280_1024  (17)
#define F_SEL_1600_1200  (18)
#define F_SEL_1920_1080  (19)
#define F_SEL_1600_900	 (20)
#define F_SEL_2048_1536  (21)
#define F_SEL_2592_1944  (22)
#define F_SEL_3264_2448  (23)
#define F_SEL_NONE           0xff

#define FPS_120    (0x0001<<0)
#define FPS_60   (0x0001<<1)
#define FPS_30    (0x0001<<2)
#define FPS_25    (0x0001<<3)
#define FPS_24    (0x0001<<4)
#define FPS_23    (0x0001<<5)
#define FPS_20    (0x0001<<6)
#define FPS_15    (0x0001<<7)
#define FPS_12    (0x0001<<8)
#define FPS_11    (0x0001<<9)
#define FPS_10    (0x0001<<10)
#define FPS_9      (0x0001<<11)
#define FPS_8      (0x0001<<12)
#define FPS_7      (0x0001<<13)
#define FPS_5      (0x0001<<14)
#define FPS_3      (0x0001<<15)

#define SENSOR_FPS_120   120
#define SENSOR_FPS_60   60
#define SENSOR_FPS_40   40
#define SENSOR_FPS_30   30
#define SENSOR_FPS_25   25
#define SENSOR_FPS_24   24
#define SENSOR_FPS_23   23
#define SENSOR_FPS_20   20
#define SENSOR_FPS_15   15
#define SENSOR_FPS_12   12
#define SENSOR_FPS_11   11
#define SENSOR_FPS_10   10
#define SENSOR_FPS_9      9
#define SENSOR_FPS_8      8
#define SENSOR_FPS_7      7
#define SENSOR_FPS_5      5
#define SENSOR_FPS_3      3

#define CTL_MIN_CT_WHITBALANCETEMPAUTO (0)
#define CTL_MAX_CT_WHITBALANCETEMPAUTO (1)

#define CTL_MIN_CT_EXPOSURE_TIME_ABSOLUTE (50)
#define CTL_MAX_CT_EXPOSURE_TIME_ABSOLUTE (10000)

#define CTL_MIN_PU_PAN (-16)
#define CTL_MAX_PU_PAN (16)

#define CTL_MIN_PU_TILT (-12)
#define CTL_MAX_PU_TILT (12)

#define	REQUEST_TYPE		0x60
#define	STANDARD_REQUEST	0x00
#define	CLASS_REQUEST		0x20
#define	VENDOR_REQUEST		0x40

#define RC_UNDEFINED 0x00
#define SET_CUR 0x01
#define GET_CUR 0x81
#define GET_MIN 0x82
#define GET_MAX 0x83
#define GET_RES 0x84
#define GET_LEN 0x85
#define GET_INFO 0x86
#define GET_DEF 0x87

#define IF_IDX_VIDEOCONTROL 0x00
#define IF_IDX_VIDEOSTREAMING 0X01

#define VS_ERR_NOERROR (0)
#define VS_ERR_CNTPROTECT (1)
#define VS_ERR_IBUFUNDERRUN (2)
#define VS_ERR_DATDISCONT (3)
#define VS_ERR_OBUFUNDERRUN (4)
#define VS_ERR_OBUFOVERRUN (5)
#define VS_ERR_FMTCHANGE (6)
#define VS_ERR_STLIMGCAPERR (7)
#define VS_ERR_UNKNOWN (8)

#define DEV_CLOCK_FRQ 12000000

#define VC_CONTROL_UNDEFINED 0x00
#define VC_VIDEO_POWER_MODE_CONTROL 0x01
#define VC_REQUEST_ERROR_CODE_CONTROL 0x02

#define VC_ERR_NOERROR 0x00
#define VC_ERR_NOTRDY 0x01
#define VC_ERR_WRNGSTAT 0x02
#define VC_ERR_PWR 0x03
#define VC_ERR_OUTOFRANGE 0x04
#define VC_ERR_INVDUNIT 0x05
#define VC_ERR_INVDCTL 0x06
#define VC_ERR_INVDREQ 0x07
#define VC_ERR_UNKNOWN 0xFF

#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define CT_PANTILT_ABSOLUTE_CONTROL 0x0D
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B

#define SCMD_DATA_VLD	0x80
#define SCMD_DIR_MSK	0x40
#define SCMD_DIR_IN	0x40
#define SCMD_DIR_OUT	0x00

#define VCMD_DBG	0x00
#define EXU1_CMD_STS	0x0A

#define VSCMD_IMEM_R			(0x01|SCMD_DATA_VLD|SCMD_DIR_IN)
#define VSCMD_XMEM_W			(0x02|SCMD_DATA_VLD|SCMD_DIR_OUT)
#define VSCMD_XMEM_R			(0x02|SCMD_DATA_VLD|SCMD_DIR_IN)
#define VSCMD_CODE_R			(0x03|SCMD_DATA_VLD|SCMD_DIR_IN)
#define VSCMD_BAR_INFO_READ		(0x20|SCMD_DATA_VLD|SCMD_DIR_IN)
#define VSCMD_DEV_STS_R		(0x0d|SCMD_DATA_VLD|SCMD_DIR_IN)
#define VSCMD_DEV_QUALIFICATION_GET	(0x10|SCMD_DATA_VLD|SCMD_DIR_IN)
#define VSCMD_IR_MODE_GET		(0x3A|SCMD_DATA_VLD|SCMD_DIR_IN)

#define ANDROIDUVC_IOC_MAGIC	0x74
#define ANDROIDUVC_SETFORMAT	1
#define ANDROIDUVC_CLEARFORMAT	2
#define ANDROIDUVC_QUERYFORMAT	3
#define ANDROIDUVC_WAIT_NAK	4
#define ANDROIDUVC_GET_NUM	5
#define ANDROIDUVC_EN_NAK	6
#define ANDROIDUVC_PRODUCE_DESC 7

#define ANDROIDUVC_IOC_SETFORMAT _IOW(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_SETFORMAT, struct format_setting)
#define ANDROIDUVC_IOC_QUERYFORMAT	_IOWR(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_QUERYFORMAT, struct format_setting)
#define ANDROIDUVC_IOC_CLRFORMAT	_IO(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_CLEARFORMAT)
#define ANDROIDUVC_IOC_WAIT_NAK	_IOWR(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_WAIT_NAK, int)
#define ANDROIDUVC_IOC_GET_NUM	_IOWR(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_GET_NUM, int)
#define ANDROIDUVC_IOC_EN_NAK	_IOWR(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_EN_NAK, int)
#define ANDROIDUVC_IOC_EN_NAK	_IOWR(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_EN_NAK, int)
#define ANDROIDUVC_IOC_PRODUCE_DESC _IOWR(ANDROIDUVC_IOC_MAGIC, \
	ANDROIDUVC_PRODUCE_DESC, int)

#define RLX_TRACE_ERROR		(1 << 0)
#define RLX_TRACE_PROBE		(1 << 1)
#define RLX_TRACE_CONTROL	(1 << 2)
#define RLX_TRACE_IOCTL		(1 << 3)
#define RLX_TRACE_BUF		(1 << 4)
#define RLX_TRACE_VIDEO		(1 << 5)
#define RLX_TRACE_STATUS	(1 << 6)
#define RLX_TRACE_DEBUG		(1 << 7)
#define RLX_TRACE_INFO		(1 << 8)
#define RLX_TRACE_NOTICE	(1 << 9)
#define RLX_TRACE_WARNING	(1 << 10)
#define RLX_TRACE_REG		(1 << 11)

#define RLX_TRACE_DEFAULT	0x1

extern int rlx_uvc_debug;

#define rlxprintk(flag, arg...) \
	do { \
		if (rlx_uvc_debug & flag) \
			printk(KERN_DEBUG "rlx_uvc:"arg);\
	} while (0)

int uvc_bind_config(struct usb_configuration *c,
		    const struct uvc_descriptor_header *const *fs_control,
		    const struct uvc_descriptor_header *const *hs_control,
		    const struct uvc_descriptor_header *const *fs_streaming,
		    const struct uvc_descriptor_header *const *hs_streaming,
		    const struct uvc_descriptor_header *const *ss_streaming);
int uvc_outdata_setstall(void);
#endif /* _F_UVC_H_ */
