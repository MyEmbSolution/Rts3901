/*
 *	rtscam.c -- USB rtscam gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#define _STILL_IMAGE_METHOD_1_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/usb/video.h>
#include "f_uvc.h"
#include "android_uvc.h"

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */

int rlx_uvc_debug = RLX_TRACE_DEFAULT;
EXPORT_SYMBOL_GPL(rlx_uvc_debug);
module_param(rlx_uvc_debug, int, 0644);
MODULE_PARM_DESC(rlx_uvc_debug, "activates debug info");

/* string IDs are assigned dynamically */

#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

struct format_setting {
	u32 type;
	u16 width;
	u16 height;
	u32 fps;
	u32 fileindex;
};

struct format_setting_list {
	struct list_head list;
	struct format_setting fts;
};

struct desc_entry {
	struct list_head list;
	struct uvc_descriptor_header *desc;
	u8 malloc;
};

int dwc_otg_pcd_wait_in_nak(void);

static struct usb_configuration *uvcconf;
static struct usb_composite_dev *uvccdev;
static LIST_HEAD(frame_desc_list);
static LIST_HEAD(uvc_desc_list);

static u8 fpsbitmap[16] = {
	SENSOR_FPS_120,
	SENSOR_FPS_60,
	SENSOR_FPS_30,
	SENSOR_FPS_25,
	SENSOR_FPS_24,
	SENSOR_FPS_23,
	SENSOR_FPS_20,
	SENSOR_FPS_15,
	SENSOR_FPS_12,
	SENSOR_FPS_11,
	SENSOR_FPS_10,
	SENSOR_FPS_9,
	SENSOR_FPS_8,
	SENSOR_FPS_7,
	SENSOR_FPS_5,
	SENSOR_FPS_3,
};

static u32 formatmap[3];

DECLARE_UVC_HEADER_DESCRIPTOR(1);

const struct UVC_HEADER_DESCRIPTOR (1)
uvc_control_header = {
	.bLength = UVC_DT_HEADER_SIZE(1),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_HEADER,
#ifndef CONFIG_USB_RTSX_UVC_15
	.bcdUVC = cpu_to_le16(0x0100),
#else
	.bcdUVC = cpu_to_le16(0x0150),
#endif
	.wTotalLength = 0,	/* dynamic */
	.dwClockFrequency = cpu_to_le32(DEV_CLOCK_FRQ),
	.bInCollection = 1,	/* dynamic */
	.baInterfaceNr[0] = 1,	/* dynamic */
};

static struct uvc_camera_terminal_descriptor
uvc_camera_terminal = {
	.bLength = UVC_DT_CAMERA_TERMINAL_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_INPUT_TERMINAL,
	.bTerminalID = ENT_ID_CAMERA_IT,
	.wTerminalType = cpu_to_le16(0x0201),
	.bAssocTerminal = 0,
	.iTerminal = 0,
	.wObjectiveFocalLengthMin = cpu_to_le16(0),
	.wObjectiveFocalLengthMax = cpu_to_le16(0),
	.wOcularFocalLength = cpu_to_le16(0),
	.bControlSize = 3,
	.bmControls[0] = 0x0e,
	.bmControls[1] = 0x2a,
	.bmControls[2] = 0x00,
};

#ifndef CONFIG_USB_RTSX_UVC_15

static struct uvc_processing_unit_descriptor uvc_processing_unit = {
	.bLength = UVC_DT_PROCESSING_UNIT_SIZE(2),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_PROCESSING_UNIT,
	.bUnitID = ENT_ID_PROCESSING_UNIT,
	.bSourceID = SRC_ID_PROCESSING_UNIT,
	.wMaxMultiplier = 0,
	.bControlSize = 2,
	.bmControls[0] = 0x7f,
	.bmControls[1] = 0x15,
	.iProcessing = 0,
	.bmVideoStandards = 0,
};

#else

static struct uvc_processing_unit_descriptor uvc_processing_unit = {
	.bLength = UVC_DT_PROCESSING_UNIT_SIZE(4),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_PROCESSING_UNIT,
	.bUnitID = ENT_ID_PROCESSING_UNIT,
	.bSourceID = SRC_ID_PROCESSING_UNIT,
	.wMaxMultiplier = 0,
	.bControlSize = 3,
	.bmControls[0] = 0x7f,
	.bmControls[1] = 0x15,
	.bmControls[2] = 0,
	.iProcessing = 0,
	.bmVideoStandards = 0,
};
#endif

static const struct uvc_output_terminal_descriptor
uvc_output_terminal = {
	.bLength = UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID = ENT_ID_OUTPUT_TRM,
	.wTerminalType = cpu_to_le16(0x0101),
	.bAssocTerminal = 0,
	.bSourceID = SRC_ID_OUTPUT_TRM,
	.iTerminal = 0,
};

#ifdef CONFIG_USB_RTSX_UVC_15
static const struct uvc_encode_unit_descriptor
uvc_h264_encode_unit = {
	.bLength = 13,
	.bDescriptorType = 0x24,
	.bDescriptorSubType = 0x07,
	.bUnitID = ENT_ID_H264_ENCODING_UNIT,
	.bSourceID = SRC_ID_H264_ENCODING_UNIT,
	.iEncoding = 0x00,
	.bControlSize = 0x03,
	.bmControls[0] = 0x60,
	.bmControls[1] = 0x02,
	.bmControls[2] = 0x00,
	.bmControlsRuntime[0] = 0x60,
	.bmControlsRuntime[1] = 0x02,
	.bmControlsRuntime[2] = 0x00,
};
#endif

DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2);

static const struct UVC_EXTENSION_UNIT_DESCRIPTOR (1, 2)
uvc_extension_dbg_unit = {
	.bLength = 0x1B,
	.bDescriptorType = 0x24,
	.bDescriptorSubType = 0x06,
	.bUnitID = ENT_ID_EXTENSION_UNIT_DBG,
	.guidExtensionCode = {
		0x8C, 0xA7, 0x29, 0x12, 0xB4, 0x47, 0x94, 0x40, 0xB0, 0xCE,
		0xDB, 0x07, 0x38, 0x6F, 0xB9, 0x38,
	},
	.bNumControls = 0x02,
	.bNrInPins = 0x01,
	.baSourceID[0] = SRC_ID_EXTENSION_UNIT_DBG,
	.bControlSize = 0x02,
	.bmControls[0] = 0x00,
	.bmControls[1] = 0x06,
	.iExtension = 0x00,
};

/*
DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1, 4);

static struct UVC_EXTENSION_UNIT_DESCRIPTOR (1, 4)
uvc_rtkextension_unit = {
	.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1, 4),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_EXTENSION_UNIT,
	.bUnitID = ENT_ID_RTK_EXTENDED_CTL_UNIT,
	.guidExtensionCode = {
		0x5A, 0x10, 0xB8, 0x26, 0x13, 0x07, 0x70, 0x48, 0x97, 0x9D,
		0xDA, 0x79, 0x44, 0x4B, 0xB6, 0x8E,
	},
	.bNumControls = 0x08,
	.bNrInPins = 0x01,
	.baSourceID[0] = SRC_ID_RTK_EXTENDED_CTL_UNIT,
	.bControlSize =	0x04,
	.bmControls[0] = 0x3f,
	.bmControls[1] = 0x0,
	.bmControls[2] = 0x14,
	.bmControls[3] = 0x0,
	.iExtension = 0,
};
*/
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(3, 1);

static struct UVC_INPUT_HEADER_DESCRIPTOR (3, 1)
uvc_input_header = {
	.bLength = UVC_DT_INPUT_HEADER_SIZE(3, 1) - 3,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_INPUT_HEADER,
	.bNumFormats = 1,
	.wTotalLength = 0,	/* dynamic */
	.bEndpointAddress = 0,	/* dynamic */
	.bmInfo = 0,
	.bTerminalLink = ENT_ID_OUTPUT_TRM,
#ifdef _STILL_IMAGE_METHOD_1_
	.bStillCaptureMethod = 1,
#else
	.bStillCaptureMethod = 2,
#endif
	.bTriggerSupport = 0,
	.bTriggerUsage = 0,
	.bControlSize = 1,
	.bmaControls[0][0] = 0,
	.bmaControls[0][1] = 0,
	.bmaControls[0][2] = 0,
};

static struct uvc_format_mjpeg uvc_format_mjpg = {
	.bLength = UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_MJPEG,
	.bFormatIndex = 2,
	.bNumFrameDescriptors = 2,
	.bmFlags = 1,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
};

static struct uvc_format_uncompressed uvc_format_yuv = {
	.bLength = UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex = 1,
	.bNumFrameDescriptors = 2,
	.guidFormat = {
		       'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00,
		       0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel = 16,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
};

static const struct uvc_color_matching_descriptor
 uvc_color_matching = {
	.bLength = UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_COLORFORMAT,
	.bColorPrimaries = 1,
	.bTransferCharacteristics = 1,
	.bMatrixCoefficients = 4,
};

struct uvc_format_h264 uvc_format_h264 = {
	.bLength = UVC_DT_FORMAT_H264_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VS_FORMAT_H264,
	.bFormatIndex = 0x01,
	.bNumFrameDescriptors = 0x0,
	.bDefaultFrameIndex = 0x01,
	.bMaxCodecConfigDelay = 0x01,
	.bmSupportedSliceModes = 0x00,
	.bmSupportedSyncFrameTypes = 0x03,
	.bResolutionScaling = 0x03,
	.Reserved1 = 0x00,
	.bmSupportedRateControlModes = H264_RATE_CONTROL_MODE,
	.wMaxMBperSecOneResolutionNoScalability = 0x6C00,
	.wMaxMBperSecTwoResolutionsNoScalability = 0x0000,
	.wMaxMBperSecThreeResolutionsNoScalability = 0x0000,
	.wMaxMBperSecFourResolutionsNoScalability = 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalability = 0x6C00,
	.wMaxMBperSecTwoResolutionsTemporalScalablility = 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalScalability = 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability = 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability = 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability = 0x0000,
	.wMaxMBperSecOneResolutionTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecOneResolutionFullScalability = 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability = 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability = 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability = 0x0000,
};

struct uvc_format_yuv420 uvc_format_yuv420 = {
	.bLength = UVC_DT_FORMAT_YUV420_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VS_FORMAT_VENDOR,
	.bFormatIndex = 0x01,
	.bNumFrameDescriptors = 0x01,
	.guidMajorFormat = {
			    'Y', '4', '2', '0', 0x39, 0xb4, 0x41, 0x48, 0x8E,
			    0xE4, 0x0B, 0x36, 0x2D, 0x7D, 0x40, 0x59,
			    },
	.guidSubFormat = {
			  'Y', 'U', 'V', '2', 0x39, 0xb4, 0x41, 0x48, 0x8E,
			  0xE4, 0x0B, 0x36, 0x2D, 0x7D, 0x40, 0x59,
			  },
	.guidSpecifier = {
			  'Y', 'U', 'V', '3', 0x39, 0xb4, 0x41, 0x48, 0x8E,
			  0xE4, 0x0B, 0x36, 0x2D, 0x7D, 0x40, 0x59,
			  },
	.bPayloadClass = 0x00,
	.bDefaultFrameIndex = 0x01,
	.bCopyProtect = 0x00,
};

struct uvc_descriptor_header *uvc_fs_control_cls[] = {
	(struct uvc_descriptor_header *)&uvc_control_header,
	(struct uvc_descriptor_header *)&uvc_camera_terminal,
	(struct uvc_descriptor_header *)&uvc_processing_unit,
	(struct uvc_descriptor_header *)&uvc_output_terminal,
#ifdef CONFIG_USB_RTSX_UVC_15
	(struct uvc_descriptor_header *)&uvc_h264_encode_unit,
#endif
	(struct uvc_descriptor_header *)&uvc_extension_dbg_unit,
/*
	(struct uvc_descriptor_header *)&uvc_rtkextension_unit,
*/
	NULL,
};

#include "f_uvc.c"

static unsigned bitconts(unsigned u)
{
	unsigned ret = 0;

	while (u) {
		u = (u & (u - 1));
		ret++;
	}

	return ret;
}

static void init_frame_desc(struct uvc_frame_common *desc,
			    u8 format,
			    struct format_setting *fts, u8 frameindex)
{
	u32 pixels;
	u8 bFrameIntervalNum;
	u8 minfps, maxfps;
	u32 fps;
	int i;

	rlxprintk(RLX_TRACE_DEBUG, "init_frame_desc:%c%c%c%c %d %d 0x%x\n",
		  ((char *)&(fts->type))[0],
		  ((char *)&(fts->type))[1],
		  ((char *)&(fts->type))[2],
		  ((char *)&(fts->type))[3], fts->width, fts->height, fts->fps);

	fps = fts->fps;

	switch (format) {
	case FORMAT_TYPE_YUY2:
		format = VS_FRAME_UNCOMPRESSED;
		break;
	case FORMAT_TYPE_MJPG:
		format = VS_FRAME_MJPEG;
		break;
	case FORMAT_TYPE_YUV420:
		format = VS_FRAME_VENDOR;
		break;
	case FORMAT_TYPE_M420:
	case FORMAT_TYPE_RSDH:
	case FORMAT_TYPE_RAW:
	default:
		format = VS_FRAME_VENDOR;
		break;
	}

	bFrameIntervalNum = bitconts(fts->fps);
	pixels = fts->width * fts->height;
	minfps = fpsbitmap[fls(fts->fps) - 1];
	maxfps = fpsbitmap[ffs(fts->fps) - 1];

	rlxprintk(RLX_TRACE_DEBUG, "fps:%d %d %d 0x%x\n",
		  minfps, maxfps, bFrameIntervalNum, fps);

	desc->bLength = (bFrameIntervalNum << 2) + 26;
	desc->bDescriptorType = USB_DT_CS_INTERFACE;
	desc->bDescriptorSubType = format;
	desc->bFrameIndex = frameindex;
#ifdef _STILL_IMAGE_METHOD_1_
	desc->bmCapabilities = 1;
#else
	desc->bmCapabilities = 2;
#endif
	desc->wWidth = cpu_to_le16(fts->width);
	desc->wHeight = cpu_to_le16(fts->height);
	desc->dwMinBitRate = cpu_to_le32(pixels * minfps << 4);
	desc->dwMaxBitRate = cpu_to_le32(pixels * maxfps << 4);
	desc->dwMaxVideoFrameBufferSize = cpu_to_le32(pixels << 1);
	desc->dwDefaultFrameInterval = cpu_to_le32(10000000 / maxfps);
	desc->bFrameIntervalType = bFrameIntervalNum;
	for (i = 0; i < bFrameIntervalNum; i++) {
		desc->dwFrameInterval[i] =
		    cpu_to_le32(10000000 / (fpsbitmap[ffs(fps) - 1]));
		fps &= ~(1 << (ffs(fps) - 1));

	}

	return;
}

#ifdef CONFIG_USB_RTSX_UVC_15
static void init_h264_frame_desc(struct uvc_frame_h264 *desc,
				 u8 byFormatType,
				 struct format_setting *fts, u8 frameindex)
{
	u32 pixels;
	u8 bFrameIntervalNum;
	int i;
	u32 fps;
	u8 minfps, maxfps;
	rlxprintk(RLX_TRACE_DEBUG, "init_h264_frame_desc:%c%c%c%c %d %d 0x%x\n",
		  ((char *)&(fts->type))[0],
		  ((char *)&(fts->type))[1],
		  ((char *)&(fts->type))[2],
		  ((char *)&(fts->type))[3], fts->width, fts->height, fts->fps);

	fps = fts->fps;

	bFrameIntervalNum = bitconts(fts->fps);

	desc->bLength = (bFrameIntervalNum << 2) + 44;
	desc->bDescriptorType = USB_DT_CS_INTERFACE;
	desc->bDescriptorSubType = VS_FRAME_H264;

	desc->bFrameIndex = frameindex;
	desc->wWidth = cpu_to_le16(fts->width);
	desc->wHeight = cpu_to_le16(fts->height);

	desc->wSARwidth = 0x0001;
	desc->wSARheight = 0x0001;

	desc->wProfile = 0x6400;
	desc->bLevelIDC = 40;

	desc->wConstrainedToolset = 0;

	desc->bmSupportedUsages[0] = 0x01;
	desc->bmSupportedUsages[1] = 0x00;
	desc->bmSupportedUsages[2] = 0x01;
	desc->bmSupportedUsages[3] = 0x00;

	desc->bmCapabilities[0] = H264_ENC_CAPBILITY;
	desc->bmCapabilities[1] = 0x00;

	desc->bmSVCCapabilities[0] = 0;
	desc->bmSVCCapabilities[1] = 0;
	desc->bmSVCCapabilities[2] = 0;
	desc->bmSVCCapabilities[3] = 0;

	desc->bmMVCCapabilities[0] = 0;
	desc->bmMVCCapabilities[1] = 0;
	desc->bmMVCCapabilities[2] = 0;
	desc->bmMVCCapabilities[3] = 0;

	bFrameIntervalNum = bitconts(fts->fps);

	pixels = fts->width * fts->height;

	minfps = fpsbitmap[fls(fts->fps) - 1];
	maxfps = fpsbitmap[ffs(fts->fps) - 1];

	rlxprintk(RLX_TRACE_DEBUG, "fps:%d %d %d 0x%x\n",
		  minfps, maxfps, bFrameIntervalNum, fps);
	desc->dwMinBitRate = cpu_to_le32(pixels * minfps << 4);
	desc->dwMaxBitRate = cpu_to_le32(pixels * maxfps << 4);
	desc->dwDefaultFrameInterval = cpu_to_le32(10000000 / maxfps);
	desc->bNumFrameIntervals = bFrameIntervalNum;

	rlxprintk(RLX_TRACE_DEBUG, "fps:%d %d %d 0x%x\n",
		  minfps, maxfps, bFrameIntervalNum, fps);
	for (i = 0; i < bFrameIntervalNum; i++) {
		desc->dwFrameInterval[i] =
		    cpu_to_le32(10000000 / (fpsbitmap[ffs(fps) - 1]));
		rlxprintk(RLX_TRACE_DEBUG, "fps:0x%x %d\n", fps,
			  fpsbitmap[ffs(fps) - 1]);
		fps &= ~(1 << (ffs(fps) - 1));
	}

	return;
}
#endif

void uvc_copy_stream_descs(u8 **mem, struct usb_descriptor_header ***dst)
{
	struct desc_entry *q;

	list_for_each_entry(q, &uvc_desc_list, list) {
		memcpy(*mem, q->desc, q->desc->bLength);
		**dst = (struct usb_descriptor_header *)*mem;
		*dst++;
		dst = dst;
		*mem += q->desc->bLength;
	}
}

int uvc_get_stream_descs_num(void)
{
	struct desc_entry *q;
	int count = 0;

	list_for_each_entry(q, &uvc_desc_list, list) {
		count++;
	}

	return count;
}

int uvc_get_stream_descs_len(void)
{
	struct desc_entry *q;
	int len = 0;

	list_for_each_entry(q, &uvc_desc_list, list) {
		len += q->desc->bLength;
	}
	return len;
}

#ifndef _STILL_IMAGE_METHOD_1_
static void init_still_desc(char *descbuf, int num, int type)
{
	unsigned char byoffet;
	unsigned short *wtmp;
	struct format_setting_list *q = 0;

	descbuf[0] = (num << 2) + 5 + 1;
	descbuf[1] = 0x24;
	descbuf[2] = 0x03;
	descbuf[3] = 0x00;
	descbuf[4] = num;

	byoffet = 5;

	list_for_each_entry(q, &frame_desc_list, list) {
		if (q->fts.type == type) {
			wtmp = (unsigned short *)&descbuf[byoffet];
			*wtmp = q->fts.width;
			byoffet += 2;
			wtmp = (unsigned short *)&descbuf[byoffet];
			*wtmp = q->fts.height;
			byoffet += 2;
		}
	}

	descbuf[byoffet] = 0;

	return;
}
#endif

static void rtscam_produce_desc(void)
{
	u8 yuvfm = 0, yuy2fm = 0, mjpegfm = 0;
#ifdef CONFIG_USB_RTSX_UVC_15
	u8 h264fm = 0;
#endif
	u8 frameindex = 1, count = 0;
	struct desc_entry *desc;
	u8 formatindex = 1;
	struct format_setting_list *q;
	int streamdesclen;

	desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
	desc->desc = (struct uvc_descriptor_header *)&uvc_input_header;
	list_add_tail(&desc->list, &uvc_desc_list);

	list_for_each_entry(q, &frame_desc_list, list) {
		if (q->fts.type == V4L2_PIX_FMT_MJPEG) {
			if (mjpegfm == 0) {
				uvc_input_header.bLength++;
				desc = kzalloc(sizeof(struct desc_entry),
					       GFP_KERNEL);
				desc->desc = (struct uvc_descriptor_header *)
				    &uvc_format_mjpg;
				uvc_format_mjpg.bFormatIndex = formatindex;
				formatindex++;
				list_add_tail(&desc->list, &uvc_desc_list);
				mjpegfm = 1;
				count++;
				streamdesclen += desc->desc->bLength;
			}
			desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
			desc->desc = kzalloc(sizeof(struct uvc_frame_mjpeg)
					     + (bitconts(q->fts.fps) << 2),
					     GFP_KERNEL);
			desc->malloc = 1;
			uvc_format_mjpg.bNumFrameDescriptors = frameindex;
			init_frame_desc((struct uvc_frame_common *)desc->desc,
					FORMAT_TYPE_MJPG,
					&(q->fts), frameindex++);
			streamdesclen += desc->desc->bLength;
			list_add_tail(&desc->list, &uvc_desc_list);
		}
	}

	if (mjpegfm) {
#ifndef _STILL_IMAGE_METHOD_1_
		desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
		desc->desc = kzalloc(sizeof(struct uvc_still_desc)
				     + ((frameindex - 1) << 2) + 1, GFP_KERNEL);
		desc->malloc = 1;
		init_still_desc(desc->desc, frameindex - 1, V4L2_PIX_FMT_MJPEG);
		list_add_tail(&desc->list, &uvc_desc_list);
		streamdesclen += desc->desc->bLength;
#endif
		desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
		desc->desc = (struct uvc_descriptor_header *)
		    &uvc_color_matching;
		list_add_tail(&desc->list, &uvc_desc_list);
		formatmap[formatindex - 2] = V4L2_PIX_FMT_MJPEG;
		streamdesclen += desc->desc->bLength;
	}

#ifdef CONFIG_USB_RTSX_UVC_15
	frameindex = 1;
	list_for_each_entry(q, &frame_desc_list, list) {
		if (q->fts.type == V4L2_PIX_FMT_H264) {
			if (h264fm == 0) {
				uvc_input_header.bLength++;
				desc = kzalloc(sizeof(struct desc_entry),
					       GFP_KERNEL);
				desc->desc = (struct uvc_descriptor_header *)
				    &uvc_format_h264;
				uvc_format_h264.bFormatIndex = formatindex;
				formatindex++;
				count++;

				list_add_tail(&desc->list, &uvc_desc_list);
				h264fm++;
				formatmap[formatindex - 2] = V4L2_PIX_FMT_H264;
				streamdesclen += desc->desc->bLength;

			}
			uvc_format_h264.bNumFrameDescriptors = frameindex;

			desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
			desc->desc = kzalloc(44 + (bitconts(q->fts.fps) << 2),
					     GFP_KERNEL);
			init_h264_frame_desc((struct uvc_frame_h264 *)
					     desc->desc, FORMAT_TYPE_H264,
					     &(q->fts), frameindex++);
			list_add_tail(&desc->list, &uvc_desc_list);
			streamdesclen += desc->desc->bLength;
		}
	}
#endif

	frameindex = 1;
	list_for_each_entry(q, &frame_desc_list, list) {
		if (q->fts.type == V4L2_PIX_FMT_NV12) {
			if (yuvfm == 0) {
				uvc_input_header.bLength++;
				desc = kzalloc(sizeof(struct desc_entry),
					       GFP_KERNEL);
				desc->desc = (struct uvc_descriptor_header *)
				    &uvc_format_yuv420;
				uvc_format_yuv420.bFormatIndex = formatindex;
				formatindex++;
				list_add_tail(&desc->list, &uvc_desc_list);
				yuvfm = 1;
				count++;
			}
			desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
			desc->desc =
			    kzalloc(sizeof(struct uvc_frame_common) +
				    (bitconts(q->fts.fps) << 2), GFP_KERNEL);
			uvc_format_yuv420.bNumFrameDescriptors = frameindex;

			init_frame_desc((struct uvc_frame_common *)desc->desc,
					FORMAT_TYPE_YUV420,
					&(q->fts), frameindex++);

			list_add_tail(&desc->list, &uvc_desc_list);
			streamdesclen += desc->desc->bLength;
		}
	}

	if (yuvfm) {
#ifndef _STILL_IMAGE_METHOD_1_
		desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
		desc->desc = kzalloc(sizeof(struct uvc_still_desc)
				     + ((frameindex - 1) << 2) + 1, GFP_KERNEL);
		desc->malloc = 1;
		init_still_desc(desc->desc, frameindex - 1, V4L2_PIX_FMT_NV12);
		list_add_tail(&desc->list, &uvc_desc_list);
		streamdesclen += desc->desc->bLength;
#endif

		desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
		desc->desc = (struct uvc_descriptor_header *)
		    &uvc_color_matching;
		list_add_tail(&desc->list, &uvc_desc_list);

		formatmap[formatindex - 2] = V4L2_PIX_FMT_NV12;
		streamdesclen += desc->desc->bLength;
	}

	frameindex = 1;
	list_for_each_entry(q, &frame_desc_list, list) {
		if (q->fts.type == V4L2_PIX_FMT_YUYV) {
			if (yuy2fm == 0) {
				uvc_input_header.bLength++;
				desc = kzalloc(sizeof(struct desc_entry),
					       GFP_KERNEL);
				desc->desc = (struct uvc_descriptor_header *)
				    &uvc_format_yuv;
				uvc_format_yuv.bFormatIndex = formatindex;
				formatindex++;
				list_add_tail(&desc->list, &uvc_desc_list);
				yuy2fm = 1;
				count++;
				streamdesclen += desc->desc->bLength;

			}
			desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
			desc->desc =
			    kzalloc(sizeof(struct uvc_frame_common) +
				    (bitconts(q->fts.fps) << 2), GFP_KERNEL);
			uvc_format_yuv.bNumFrameDescriptors = frameindex;

			init_frame_desc((struct uvc_frame_common *)desc->desc,
					FORMAT_TYPE_YUY2,
					&(q->fts), frameindex++);

			list_add_tail(&desc->list, &uvc_desc_list);
			streamdesclen += desc->desc->bLength;
		}
	}

	if (yuy2fm) {
#ifndef _STILL_IMAGE_METHOD_1_
		desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
		desc->desc = kzalloc(sizeof(struct uvc_still_desc)
				     + ((frameindex - 1) << 2) + 1, GFP_KERNEL);
		desc->malloc = 1;
		init_still_desc(desc->desc, frameindex - 1, V4L2_PIX_FMT_YUYV);
		list_add_tail(&desc->list, &uvc_desc_list);
		streamdesclen += desc->desc->bLength;
#endif
		desc = kzalloc(sizeof(struct desc_entry), GFP_KERNEL);
		desc->desc = (struct uvc_descriptor_header *)
		    &uvc_color_matching;
		list_add_tail(&desc->list, &uvc_desc_list);

		formatmap[formatindex - 2] = V4L2_PIX_FMT_YUYV;
		streamdesclen += desc->desc->bLength;

	}

	uvc_input_header.bLength = 13 + count;
	uvc_input_header.wTotalLength = streamdesclen + uvc_input_header.bLength;
	uvc_input_header.bNumFormats = count;

}

static int rtscam_query_desc(int fmtindex, int frmindex,
			     struct format_setting *fts)
{
	struct format_setting_list *q = 0;
	int i = 1;

	if ((fmtindex - 1) >= ARRAY_SIZE(formatmap))
		return -1;

	if (formatmap[fmtindex - 1] == 0)
		return -1;

	list_for_each_entry(q, &frame_desc_list, list) {
		if (q->fts.type == formatmap[fmtindex - 1]) {
			if (i == frmindex) {
				*fts = q->fts;
				rlxprintk(RLX_TRACE_DEBUG,
					  "query result: %c%c%c%c %d %d 0x%x\n",
					  ((char *)&(fts->type))[0],
					  ((char *)&(fts->type))[1],
					  ((char *)&(fts->type))[2],
					  ((char *)&(fts->type))[3],
					  fts->width, fts->height, fts->fps);
				return 0;
			}
			i++;
		}
	}

	return -1;

}

static dev_t android_uvc_devno = MKDEV(243, 0);

static int android_uvc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int android_uvc_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long android_uvc_ioctl(struct file *file,
			      unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	rlxprintk(RLX_TRACE_IOCTL, "ioctl:%x\n", cmd);

	switch (cmd) {
	case ANDROIDUVC_IOC_SETFORMAT:{
			struct format_setting_list *fsl;

			fsl = kzalloc(sizeof(struct format_setting_list),
				      GFP_KERNEL);
			if (fsl == NULL)
				return -ENOMEM;

			if (copy_from_user(&(fsl->fts), (char *)arg,
					   sizeof(struct format_setting))) {
				kfree(fsl);
				retval = -EFAULT;
			}

			rlxprintk(RLX_TRACE_DEBUG,
				  "set format:%c%c%c%c %d %d 0x%x\n",
				  ((char *)&(fsl->fts.type))[0],
				  ((char *)&(fsl->fts.type))[1],
				  ((char *)&(fsl->fts.type))[2],
				  ((char *)&(fsl->fts.type))[3], fsl->fts.width,
				  fsl->fts.height, fsl->fts.fps);

			list_add_tail(&fsl->list, &frame_desc_list);
			break;
		}
	case ANDROIDUVC_IOC_PRODUCE_DESC:
		rlxprintk(RLX_TRACE_DEBUG, "produce uvc desc\n");
#ifdef CONFIG_USB_G_RTSX_UVC
		rtscam_produce_desc();
		usb_add_config(uvccdev, uvcconf, rtsx_uvc_function_bind);
#endif
		break;

	case ANDROIDUVC_IOC_QUERYFORMAT:{
			struct format_setting fs;
			int ret;

			if (copy_from_user
			    (&fs, (char *)arg, sizeof(struct format_setting))) {
				retval = -EFAULT;
				break;
			}

			ret = rtscam_query_desc(fs.width, fs.height, &fs);
			if (ret)
				return -EINVAL;

			copy_to_user((char *)arg, &fs,
				     sizeof(struct format_setting));
			break;
		}
	case ANDROIDUVC_IOC_CLRFORMAT:{
			int ret;
			struct format_setting_list *fsl, *fsl_copy;

			ret = list_empty(&frame_desc_list);
			if (ret)
				break;

			list_for_each_entry_safe(fsl, fsl_copy,
						 &frame_desc_list, list) {
				list_del(&fsl->list);
				kfree(fsl);
			}

			memset(formatmap, 0, ARRAY_SIZE(formatmap));
			break;
		}
	case ANDROIDUVC_IOC_WAIT_NAK:{
			int epnum;
			epnum = dwc_otg_pcd_wait_in_nak();
			copy_to_user((char *)arg, &epnum, sizeof(epnum));
			break;
		}
	default:
		return -EINVAL;
	}

	return retval;
}

static const struct file_operations android_uvc_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = android_uvc_ioctl,
	.open = android_uvc_open,
	.release = android_uvc_release,
};

static struct cdev android_uvc_cdev;

int rtsx_uvc_function_bind(struct usb_configuration *c)
{
	return uvc_bind_config(c, (const struct uvc_descriptor_header * const *)
			       uvc_fs_control_cls, NULL, NULL, NULL, NULL);
}
EXPORT_SYMBOL_GPL(rtsx_uvc_function_bind);

int rtsx_uvc_add_config(struct usb_composite_dev *cdev,
			struct usb_configuration *config,
			int (*bind) (struct usb_configuration *))
{

	uvcconf = config;
	uvccdev = cdev;
	return 0;
}
EXPORT_SYMBOL_GPL(rtsx_uvc_add_config);

int rtsx_uvc_function_init()
{
	int err;

	err = register_chrdev_region(android_uvc_devno, 1, "android_uvc");
	if (err) {
		rlxprintk(RLX_TRACE_ERROR, "register_chrdev_region fail\n");
		return -ENODEV;
	}

	cdev_init(&android_uvc_cdev, &android_uvc_fops);
	android_uvc_cdev.owner = THIS_MODULE;

	err = cdev_add(&android_uvc_cdev, android_uvc_devno, 1);
	if (err) {
		rlxprintk(RLX_TRACE_ERROR, "cdev_add fail\n");
		unregister_chrdev_region(android_uvc_devno, 1);
		return -ENODEV;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(rtsx_uvc_function_init);

void rtsx_uvc_function_cleanup(void)
{
	struct desc_entry *q;
	struct format_setting_list *fsl;

	cdev_del(&android_uvc_cdev);
	unregister_chrdev_region(android_uvc_devno, 1);

	list_for_each_entry(q, &uvc_desc_list, list) {
		list_del_init(&(q->list));

		if (q->malloc)
			kfree(q->desc);
		kfree(q);
	}

	list_for_each_entry(fsl, &frame_desc_list, list) {
		list_del_init(&(fsl->list));
		kfree(fsl);
	}

}
EXPORT_SYMBOL_GPL(rtsx_uvc_function_cleanup);

void rtsx_uvc_function_enable(void)
{
	rlxprintk(RLX_TRACE_ERROR, "uvc function enable\n");
#ifndef CONFIG_USB_G_RTSX_UVC
	rtscam_produce_desc();
#endif
}
EXPORT_SYMBOL_GPL(rtsx_uvc_function_enable);
