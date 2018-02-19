#ifndef _RTS_VIDEO_H_
#define _RTS_VIDEO_H_

/* --------------------------------------------------------------------------
 * UVC constants
 */

/* A.2. Video Interface Subclass Codes */

/* A.3. Video Interface Protocol Codes */
#define UVC_PC_PROTOCOL_15						0x01

/* A.5. Video Class-Specific VC Interface Descriptor Subtypes */
#define UVC_VC_ENCODING_UINT						0x07

/* A.6. Video Class-Specific VS Interface Descriptor Subtypes */
#define UVC_VS_FORMAT_H264						0x13
#define UVC_VS_FRAME_H264						0x14
#define UVC_VS_FORMAT_H264_SIMULCAST					0x15
#define UVC_VS_FORMAT_VP8						0x16
#define UVC_VS_FRAME_VP8						0x17
#define UVC_VS_FORMAT_VP8_SIMULCAST					0x18

/* A.7. Video Class-Specific Endpoint Descriptor Subtypes */

/* A.8. Video Class-Specific Request Codes */
#define UVC_SET_CUR_ALL							0x11
#define UVC_GET_CUR_ALL							0x91
#define UVC_GET_MIN_ALL							0x92
#define UVC_GET_MAX_ALL							0x93
#define UVC_GET_RES_ALL							0x94
#define UVC_GET_DEF_ALL							0x97

/* A.9.1. VideoControl Interface Control Selectors */

/* A.9.2. Terminal Control Selectors */

/* A.9.3. Selector Unit Control Selectors */

/* A.9.4. Camera Terminal Control Selectors */
#define UVC_CT_FOCUS_SIMPLE_CONTROL					0x12
#define UVC_CT_WINDOW_CONTROL						0x13
#define UVC_CT_REGION_OF_INTEREST_CONTROL				0x14

/* A.9.5. Processing Unit Control Selectors */
#define UVC_PU_CONTRAST_AUTO_CONTROL					0x13

/* A.9.6. Encoding Unit Control Selectors */
#define UVC_EU_CONTROL_UNDEFINED					0x00
#define UVC_EU_SELECT_LAYER_CONTROL					0x01
#define UVC_EU_PROFILE_TOOLSET_CONTROL					0x02
#define UVC_EU_VIDEO_RESOLUTION_CONTROL					0x03
#define UVC_EU_MIN_FRAME_INTERVAL_CONTROL				0x04
#define UVC_EU_SLICE_MODE_CONTROL					0x05
#define UVC_EU_RATE_CONTROL_MODE_CONTROL				0x06
#define UVC_EU_AVERAGE_BITRATE_CONTROL					0x07
#define UVC_EU_CPB_SIZE_CONTROL						0x08
#define UVC_EU_PEAK_BIT_RATE_CONTROL					0x09
#define UVC_EU_QUANTIZATION_PARAMS_CONTROL				0x0a
#define UVC_EU_SYNC_REF_FRAME_CONTROL					0x0b
#define UVC_EU_LTR_BUFFER_CONTROL					0x0c
#define UVC_EU_LTR_PICTURE_CONTROL					0x0d
#define UVC_EU_LTR_VALIDATION_CONTROL					0x0e
#define UVC_EU_LEVEL_IDC_LIMIT_CONTROL					0x0f
#define UVC_EU_SEI_PAYLOADTYPE_CONTROL					0x10
#define UVC_EU_QP_RANGE_CONTROL						0x11
#define UVC_EU_PRIORITY_CONTROL						0x12
#define UVC_EU_START_OR_STOP_LAYER_CONTROL				0x13
#define UVC_EU_ERROR_RESILIENCY_CONTROL					0x14

/* A.9.7 Extension Unit Control Selectors */
#define UVC_XU_CONTROL_UNDEFINED					0x00

/* A.9.8. VideoStreaming Interface Control Selectors */


/* 4.3.1.1. Video Probe and Commit Controls */
struct uvc_streaming_control_ex {
	__u16 bmHint;
	__u8  bFormatIndex;
	__u8  bFrameIndex;
	__u32 dwFrameInterval;
	__u16 wKeyFrameRate;
	__u16 wPFrameRate;
	__u16 wCompQuality;
	__u16 wCompWindowSize;
	__u16 wDelay;
	__u32 dwMaxVideoFrameSize;
	__u32 dwMaxPayloadTransferSize;
	__u32 dwClockFrequency;
	__u8  bmFramingInfo;
	__u8  bPreferedVersion;
	__u8  bMinVersion;
	__u8  bMaxVersion;
	__u8  bUsage;
	__u8  bBitDepthLuma;
	__u8  bmSettings;
	__u8  bMaxNumberOfRefFramesPlus1;
	__u16 bmRateControlModes;
	__u64  bmLayoutPerStream;
} __attribute__((__packed__));

#endif

