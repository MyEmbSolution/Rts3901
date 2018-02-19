#ifndef __RTS_CAM_H
#define __RTS_CAM_H

#include <stdint.h>
#include <rtsavapi.h>


#ifdef __cplusplus
extern "C"
{
#endif

#define MD_BLOCK_NUM (5)
#define MASK_BLOCK_NUM (5)
#define OSD_BUFFER_SIZE (256)
#define OSD_BLOCK_NUM (6)
#define MAX_AUDIO_SAMPLE_RATE_NUM (5)
#define MAX_AUDIO_BITRATE_NUM (20)
#define MAX_ENCODER_NUM (4)

enum {
	CAM_DOMAIN = 0x04000000,
	ATTR_NOT_SUPPORTED = CAM_DOMAIN | 0x000001, /*operation not supported*/
	ERR_VAL_OUT_OF_RANGE,
	ERR_UNKNOWN_WDR_MODE,
	ERR_LIB_CAM_OPEN = CAM_DOMAIN | 0x010001, /*open camera failed*/
	ERR_LIB_CAM_LOCK, /*lock camera failed*/
	ERR_LIB_CAM_UNLOCK,/*unlock camera failed*/
	ERR_LIB_CAM_GET_ENC_FMT, /*get supported encoder format failed*/
	ERR_LIB_CAM_GET_RES_FMT_FPS, /*get supported format/res/fps failed*/
	ERR_LIB_CAM_GET_CUR_RES_FMT_FPS, /*get current format/res/fps failed*/
	ERR_LIB_CAM_GET_CUR_OUT_FMT, /*get current output format failed*/
	ERR_LIB_CAM_GFMT, /*get format failed*/
	ERR_LIB_CAM_SFMT, /*set format failed*/
	ERR_LIB_CAM_GPARAM, /*get param failed*/
	ERR_LIB_CAM_SPARAM, /*set param failed*/
	ERR_LIB_CAM_SENC_OUT_FMT, /*set video encoder output format*/
	ERR_CAM_ISP_GBRIGHTNESS = CAM_DOMAIN | 0x020001, /*get brightness failed*/
	ERR_CAM_ISP_SBRIGHTNESS, /*set brightness failed*/
	ERR_CAM_ISP_GCONTRAST, /*get contrast failed*/
	ERR_CAM_ISP_SCONTRAST, /*set contrast failed*/
	ERR_CAM_ISP_GSATURATION, /*get saturation failed*/
	ERR_CAM_ISP_SSATURATION, /*set saturation failed*/
	ERR_CAM_ISP_GSHARPNESS, /*get sharpness failed*/
	ERR_CAM_ISP_SSHARPNESS, /*set sharpness failed*/
	ERR_CAM_ISP_GGAMMA, /*get gamma failed*/
	ERR_CAM_ISP_SGAMMA, /*set gamma failed*/
	ERR_CAM_ISP_GAWB, /*get awb failed*/
	ERR_CAM_ISP_SAWB, /*get awb failed*/
	ERR_CAM_ISP_GTEMPERATURE, /*get temperature failed*/
	ERR_CAM_ISP_STEMPERATURE, /*set temperature failed*/
	ERR_CAM_ISP_GPWRLINEFREQ, /*get power line frequency*/
	ERR_CAM_ISP_SPWRLINEFREQ, /*set power line frequency*/
	ERR_CAM_ISP_GMIRROR, /*get mirror failed*/
	ERR_CAM_ISP_SMIRROR, /*set mirror failed*/
	ERR_CAM_ISP_GFLIP, /*get flip failed*/
	ERR_CAM_ISP_SFLIP, /*set flip failed*/
	ERR_CAM_ISP_GDEHAZE, /*get dehaze failed*/
	ERR_CAM_ISP_SDEHAZE, /*get dehaze failed*/
	ERR_CAM_ISP_GLDC, /*get ldc failed*/
	ERR_CAM_ISP_SLDC, /*set ldc failed*/
	ERR_CAM_ISP_G3DNR, /*get temporal denoise failed*/
	ERR_CAM_ISP_S3DNR, /*set temporal denoise failed*/
	ERR_CAM_ISP_GROTATE, /*get rotate attribute failed*/
	ERR_CAM_ISP_SROTATE, /*set rotate attribute failed*/
	ERR_CAM_ISP_GAUTO_WDR, /*get auto wdr attribute failed*/
	ERR_CAM_ISP_SAUTO_WDR, /*set auto wdr attribute failed*/
	ERR_CAM_ISP_GMANUAL_WDR, /*get manual wdr attribute failed*/
	ERR_CAM_ISP_SMANUAL_WDR, /*set manual wdr attribute failed*/
	ERR_CAM_OSD_GATTR = CAM_DOMAIN | 0x030001, /*get osd attribute failed*/
	ERR_CAM_OSD_SATTR, /*set osd attribute failed*/
	ERR_CAM_MD_GATTR = CAM_DOMAIN | 0x040001, /*get motion detect attr failed*/
	ERR_CAM_MD_SATTR, /*set motion detect attr failed*/
	ERR_CAM_MASK_GATTR = CAM_DOMAIN | 0x050001, /*get mask attr failed*/
	ERR_CAM_MASK_SATTR, /*set mask attr failed*/
	ERR_CAM_VENC_GATTR = CAM_DOMAIN | 0x060001, /*get video encoder attr failed*/
	ERR_CAM_VENC_SATTR, /*set video encoder attr failed*/
	ERR_CAM_AENC_GATTR = CAM_DOMAIN | 0x070001, /*get audio encoder attr failed*/
	ERR_CAM_AENC_SATTR, /*set audio encoder attr failed*/
	ERR_CAM_EVENT_MD = CAM_DOMAIN | 0x080001, /*get motion detect event failed*/
	ERR_CAM_H264_GPROFILE = CAM_DOMAIN | 0x090001, /*get h264 profile failed*/
	ERR_CAM_H264_SPROFILE, /*set h264 profile failed*/
	ERR_CAM_H264_GLEVEL, /*get h264 level failed*/
	ERR_CAM_H264_SLEVEL, /*set h264 level failed*/
	ERR_CAM_ISP_GIR, /*get ir failed*/
	ERR_CAM_ISP_SIR, /*set ir failed*/
};

enum {
	WDR_DISABLE	= 0,
	WDR_AUTO_WEAK	= 1,
	WDR_AUTO_MED	= 2,
	WDR_AUTO_STRONG = 3,
	WDR_MANUAL	= 4,

};

enum {
	/*donot change the order*/
	VIDEO_ROTATE_NORMAL = 0,
	VIDEO_ROTATE_RIGHT,
	VIDEO_ROTATE_LEFT,
};

enum {
	H264_PROFILE_NONE = 0,
	H264_BASELINE_PROFILE = 1,
	H264_MAIN_PROFILE,
	H264_HIGH_PROFILE,
	H264_LEVEL_NONE = 0,
};

enum {
	/* attribute ID */
	ATTR_ID_VIDEO		= 1,
	ATTR_ID_AUDIO		= 2,
	ATTR_ID_STREAM		= 3,
	ATTR_ID_MOTION_DETECT	= 4,
	ATTR_ID_MASK		= 5,
	ATTR_ID_H264		= 6,
	ATTR_ID_OSD		= 7,
	ATTR_ID_MJPG		= 8,
};

enum video_frmsizetypes {
	VIDEO_FRMSIZE_TYPE_DISCRETE	= 1,
	VIDEO_FRMSIZE_TYPE_CONTINUOUS	= 2,
	VIDEO_FRMSIZE_TYPE_STEPWISE	= 3,
};

enum video_frmivaltypes {
	VIDEO_FRMIVAL_TYPE_DISCRETE	= 1,
	VIDEO_FRMIVAL_TYPE_CONTINUOUS	= 2,
	VIDEO_FRMIVAL_TYPE_STEPWISE	= 3,
};

struct video_frmival_stepwise {
	struct rts_fract min;	/* Minimum frame interval [s] */
	struct rts_fract max;	/* Maximum frame interval [s] */
	struct rts_fract step;	/* Frame interval step size [s] */
};

struct video_frmival {
	uint32_t type; /* enum */

	union {
		struct rts_fract		discrete;
		struct video_frmival_stepwise	stepwise;
	};
};

struct video_resolution_discrete {
	uint32_t width;		/* Frame width [pixel] */
	uint32_t height;	/* Frame height [pixel] */
};

struct video_resolution_stepwise {
	uint32_t min_width;	/* Minimum frame width [pixel] */
	uint32_t max_width;	/* Maximum frame width [pixel] */
	uint32_t step_width;	/* Frame width step size [pixel] */
	uint32_t min_height;	/* Minimum frame height [pixel] */
	uint32_t max_height;	/* Maximum frame height [pixel] */
	uint32_t step_height;	/* Frame height step size [pixel] */
};

struct video_resolution {
	uint32_t type;  /* enum */
	union {
		struct video_resolution_discrete discrete;
		struct video_resolution_stepwise stepwise;
	};

	int number; /* count of fps */
	/* define fps as integrate ? */
	struct video_frmival *frmivals;
};

struct video_format {
	uint32_t format;
	int number; /* count of resolution */
	struct video_resolution *resolutions;
};

struct video_enc_capa {
	uint32_t support_vencoder[MAX_ENCODER_NUM];
	uint8_t len;
};

struct video_ctrl {
	uint32_t	type;		/* enum video_ctrl_type */
	uint32_t	supported;	/* is this ctrl supported */
	int8_t		name[32];	/* Whatever */
	int32_t		minimum;	/* Note signedness */
	int32_t		maximum;
	int32_t		step;
	int32_t		default_val;
	int32_t		current;
	uint32_t	flags;
	uint32_t	reserved[2];
};

typedef struct video_ctrl h264_profile_ctl;
typedef struct video_ctrl h264_level_ctl;
typedef struct video_ctrl rotate_ctl;

struct attr_stream {
	uint32_t attr_id;

	struct video_enc_capa venc_capability;

	int number; /* count of format */
	struct video_format *formats;

	struct {
		uint32_t format;
		uint32_t output_format;
		struct video_resolution resolution;
		struct video_frmival frmival;
	} current;

	h264_profile_ctl h264_profile;
	h264_level_ctl h264_level;
	rotate_ctl rotate;
	uint32_t reserved[4];
};

struct stream_ctrl {
	uint32_t format;
	struct video_resolution resolution;
	struct video_frmival frmival;
	h264_profile_ctl h264_profile;
	h264_level_ctl h264_level;
	rotate_ctl rotate;
};

enum video_ctrl_type {
	VIDEO_CTRL_TYPE_INTEGER		= 1,
	VIDEO_CTRL_TYPE_BOOLEAN		= 2,
	VIDEO_CTRL_TYPE_MENU		= 3,
	VIDEO_CTRL_TYPE_BUTTON		= 4,
	VIDEO_CTRL_TYPE_INTEGER64	= 5,
	VIDEO_CTRL_TYPE_CTRL_CLASS	= 6,
	VIDEO_CTRL_TYPE_STRING		= 7,
	VIDEO_CTRL_TYPE_BITMASK		= 8,
	VIDEO_CTRL_TYPE_INTEGER_MENU	= 9,
};

struct video_rect {
	uint32_t left;
	uint32_t top;
	uint32_t right;
	uint32_t bottom;
};

enum enum_video_ctrl_id {
	VIDEO_CTRL_ID_BRIGHTNESS = 1,
	VIDEO_CTRL_ID_CONTRAST,
	VIDEO_CTRL_ID_HUE,
	VIDEO_CTRL_ID_SATURATION,
	VIDEO_CTRL_ID_SHARPNESS,
	VIDEO_CTRL_ID_GAMMA,
	VIDEO_CTRL_ID_AWB_CTRL,
	VIDEO_CTRL_ID_WB_TEMPERATURE,
	VIDEO_CTRL_ID_BLC,
	VIDEO_CTRL_ID_GAIN,
	VIDEO_CTRL_ID_PWR_FREQUENCY,
	VIDEO_CTRL_ID_EXPOSURE_MODE,
	VIDEO_CTRL_ID_EXPOSURE_PRIORITY,
	VIDEO_CTRL_ID_EXPOSURE_TIME,
	VIDEO_CTRL_ID_AF,
	VIDEO_CTRL_ID_FOCUS,
	VIDEO_CTRL_ID_ZOOM,
	VIDEO_CTRL_ID_PAN_TILT,
	VIDEO_CTRL_ID_ROLL,
	VIDEO_CTRL_ID_FLIP,
	VIDEO_CTRL_ID_MIRROR,
	VIDEO_CTRL_ID_ROTATE,
	VIDEO_CTRL_ID_DEHAZE,
	VIDEO_CTRL_ID_LDC,
	VIDEO_CTRL_ID_IR,
	VIDEO_CTRL_ID_GRAY_MODE,
	VIDEO_CTRL_ID_AUTO_WDR,
	VIDEO_CTRL_ID_MANUAL_WDR,
	VIDEO_CTRL_ID_TEMPORAL_DENOISE,
	VIDEO_CTRL_ID_ISP_SPECIAL_EFFECT,
	VIDEO_CTRL_ID_EV_COMPENSATE,
	VIDEO_CTRL_ID_COLOR_TEMPERATURE_ESTIMATION,
	VIDEO_CTRL_ID_AE_LOCK,
	VIDEO_CTRL_ID_AWB_LOCK,
	VIDEO_CTRL_ID_AF_LOCK,
	VIDEO_CTRL_ID_LED_TOUCH_MODE,
	VIDEO_CTRL_ID_LED_FLASH_MODE,
	VIDEO_CTRL_ID_ISO,
	VIDEO_CTRL_ID_SCENE_MODE,
	VIDEO_CTRL_ID_ROI_MODE,
	VIDEO_CTRL_ID_3A_STATUS,
	VIDEO_CTRL_ID_IDEA_EYE_SENSITIVITY,
	VIDEO_CTRL_ID_IDEA_EYE_STATUS,
	VIDEO_CTRL_ID_IEDA_EYE_MODE,
	VIDEO_CTRL_MAX_ID
};

enum enum_gpio_ctrl_id {
	GPIO_CTRL_IR_MODE_ON = 1,
	GPIO_CTRL_IR_MODE_OFF,
};

enum enum_stream_ctrl_id {
	STREAM_CTRL_ID_RESOLUTION = 1,
	STREAM_CTRL_ID_FPS,
	STREAM_CTRL_ID_H264_PROFILE,
	STREAM_CTRL_ID_H264_LEVEL,
	STREAM_CTRL_ID_ROTATE,
};

typedef struct video_ctrl brightness_ctl;
typedef struct video_ctrl contrast_ctl;
typedef struct video_ctrl hue_ctl;
typedef struct video_ctrl saturation_ctl;
typedef struct video_ctrl sharpness_ctl;
typedef struct video_ctrl gamma_ctl;
typedef struct video_ctrl auto_white_balance_ctl;
typedef struct video_ctrl white_balance_temperature_ctl;
typedef struct video_ctrl backlight_compensate_ctl;
typedef struct video_ctrl gain_ctl;
typedef struct video_ctrl power_line_frequency_ctl;
typedef struct video_ctrl exposure_mode_ctl;
typedef struct video_ctrl exposure_priority_ctl;
typedef struct video_ctrl exposure_time_ctl;
typedef struct video_ctrl auto_focus_ctl;
typedef struct video_ctrl focus_ctl;
typedef struct video_ctrl zoom_ctl;
typedef struct video_ctrl pan_tilt_ctl;
typedef struct video_ctrl roll_ctl;
typedef struct video_ctrl flip_ctl;
typedef struct video_ctrl mirror_ctl;
typedef struct video_ctrl isp_special_effect_ctl;
typedef struct video_ctrl ev_compensate_ctl;
typedef struct video_ctrl color_temperature_estimation_ctl;
typedef struct video_ctrl ae_lock_ctl;
typedef struct video_ctrl awb_lock_ctl;
typedef struct video_ctrl af_lock_ctl;
typedef struct video_ctrl led_touch_mode_ctl;
typedef struct video_ctrl led_flash_mode_ctl;
typedef struct video_ctrl iso_ctl;
typedef struct video_ctrl scene_mode_ctl;
typedef struct video_ctrl roi_mode_ctl;
typedef struct video_ctrl ae_awb_af_status_ctl;
typedef struct video_ctrl idea_eye_sensitivity_ctl;
typedef struct video_ctrl idea_eye_status_ctl;
typedef struct video_ctrl idea_eye_mode_ctl;
typedef struct video_ctrl dehaze_ctl;
typedef struct video_ctrl ldc_ctl;
typedef struct video_ctrl temporal_denoise_ctl;
typedef struct video_ctrl auto_wdr_ctl;
typedef struct video_ctrl manual_wdr_ctl;
typedef struct video_ctrl ir_ctl;

struct attr_video {
	uint32_t attr_id;

	brightness_ctl brightness;
	contrast_ctl contrast;
	hue_ctl hue;
	saturation_ctl saturation;
	sharpness_ctl sharpness;
	gamma_ctl gamma;
	auto_white_balance_ctl auto_white_balance;
	white_balance_temperature_ctl white_balance_temperature;
	backlight_compensate_ctl backlight_compensate;
	gain_ctl gain;
	power_line_frequency_ctl power_line_frequency;
	exposure_mode_ctl exposure_mode;
	exposure_priority_ctl exposure_priority;
	exposure_time_ctl exposure_time;
	auto_focus_ctl auto_focus;
	focus_ctl focus;
	zoom_ctl zoom;
	pan_tilt_ctl pan_tilt;
	roll_ctl roll;
	flip_ctl flip;
	mirror_ctl mirror;
	isp_special_effect_ctl isp_special_effect;
	ev_compensate_ctl ev_compensate;
	color_temperature_estimation_ctl color_temperature_estimation;
	ae_lock_ctl ae_lock;
	awb_lock_ctl awb_lock;
	af_lock_ctl af_lock;
	led_touch_mode_ctl led_touch_mode;
	led_flash_mode_ctl led_flash_mode;
	iso_ctl iso;
	scene_mode_ctl scene_mode;
	roi_mode_ctl roi_mode;
	ae_awb_af_status_ctl ae_awb_af_status;
	idea_eye_sensitivity_ctl idea_eye_sensitivity;
	idea_eye_status_ctl idea_eye_status;
	idea_eye_mode_ctl idea_eye_mode;
	dehaze_ctl dehaze;
	ldc_ctl ldc;
	temporal_denoise_ctl temporal_denoise;
	auto_wdr_ctl auto_wdr;
	manual_wdr_ctl manual_wdr;
	ir_ctl ir;

	/* roi */
	struct video_rect	roi;

	uint32_t		reserved[4];
};

enum {
	/* motion detect block type */
	MD_BLOCK_TYPE_RECT	= 1,
	MD_BLOCK_TYPE_GRID,

	MD_MAX_BITMAP_SIZE	= 150,

	MASK_BLOCK_TYPE_RECT	= 1,
	MASK_BLOCK_TYPE_GRID,
};

struct grid_unit {
	uint32_t width;
	uint32_t height;
};

struct video_grid {
	uint32_t left;
	uint32_t top;

	struct grid_unit cur;
	uint32_t rows;
	uint32_t columns;
	int	length;
	uint8_t bitmap[MD_MAX_BITMAP_SIZE];

	struct grid_unit min;
	struct grid_unit step;
};

struct md_block {
	uint32_t type;
	int enable;
	uint8_t sensitivity;
	uint8_t percentage;

	union {
		struct video_rect rect;
		struct video_grid grid;
	};
	uint32_t res_width;
	uint32_t res_height;
};

struct attr_md {
	uint32_t attr_id;
	int number;	/* default 5 */
	struct md_block blocks[MD_BLOCK_NUM];

	uint32_t reserved[4];
};

struct mask_block {
	uint32_t type;
	int enable;
	uint32_t color;

	union {
		struct video_rect rect;
		struct video_grid grid;
	};

	uint32_t res_width;
	uint32_t res_height;
};

struct attr_mask {
	uint32_t attr_id;
	int number;	/* default 5 */
	struct mask_block blocks[MASK_BLOCK_NUM];

	uint32_t reserved[4];
};

enum {
	OSD_SINGLE_CHAR = 0,
	OSD_DOUBLE_CHAR,
	OSD_BITMAP,
	OSD_OTHERS,
	OSD_ROTATE_0 = 0,
	OSD_ROTATE_90,
	OSD_ROTATE_180,
	OSD_ROTATE_270,
};

struct osd_block {
	uint8_t enable;
	struct video_rect rect;

	/*uint8_t addressing_mode;*/	/* 0: direct addressing, 1: indirect addressing*/
	uint8_t fg_mode;	/* single char, double char, or bitmap*/
	/*uint32_t start_addr;*/	/* addr should set by this lib*/
	uint8_t flick_enable;
	uint32_t flick_speed;
	uint8_t fg_transparent_enable;
	uint32_t fg_transparency; /* 0-100*/
	uint32_t fg_color;
	uint8_t bg_enable;
	uint32_t bg_color;
	/*char stroke*/
	uint8_t stroke_enable;
	uint8_t stroke_direct; /* 0:minus inc, 1:add inc */
	uint8_t stroke_extent;
	uint8_t each_line_wrap_enable;
	uint32_t each_line_wrap_num;
	uint32_t osd_font_size;
	uint32_t osd_width;	/* direct addressing, char dot array or bitmap width*/
	uint32_t osd_height;	/* direct addressing, char dot array or bitmap height*/
	uint8_t osd_rotate_mode;	/* 90/180/270*/
	uint32_t osd_up_shift;
	uint32_t osd_left_shift;
	uint8_t osd_buffer[OSD_BUFFER_SIZE];	/* char or bitmap info*/
};

struct attr_osd {
	uint32_t attr_id;
	int number; /* default 6 */
	struct osd_block blocks[OSD_BLOCK_NUM];
	uint8_t count_of_supported_font_type;
	/*uint32_t double_width_char_addr;*/	/* addr should set by this lib */
	/*uint32_t time_info_addr; */	/* addr should set by this lib */
	uint8_t time_am_pm_enable;	/* only for soc */
	uint8_t date_display_enable;	/* only for soc */
	uint8_t time_display_enable;	/* only for soc */
	/*uint32_t date_info_addr; */	/* addr should set by this lib */
	/*uint32_t config_info_addr;*/	/* this is fw const var*/

	uint32_t reserved[4];
};

enum bitrate_mode {
	BITRATE_MODE_CBR	= (1 << 1),
	BITRATE_MODE_VBR	= (1 << 2),
	BITRATE_MODE_C_VBR	= (1 << 3),
};

struct attr_h264 {
	uint32_t support_bitrate_mode;
	uint32_t bitrate_mode;

	uint32_t bitrate;
	uint32_t max_bitrate;
	uint32_t min_bitrate;

	uint32_t qp;
	uint32_t max_qp;
	uint32_t min_qp;

	uint32_t gop;
	uint32_t max_gop;
	uint32_t min_gop;

};

struct attr_mjpg {


};

/*video encoder's params to tune'*/
struct attr_video_encoder {
	uint32_t attr_id;
	uint32_t codec_id;
	/*the following params will be delete in future*/
	uint32_t support_bitrate_mode;
	uint32_t bitrate_mode;

	uint32_t bitrate;
	uint32_t max_bitrate;
	uint32_t min_bitrate;

	uint32_t qp;
	uint32_t max_qp;
	uint32_t min_qp;

	uint32_t gop;
	uint32_t max_gop;
	uint32_t min_gop;

	union {
		struct attr_h264 h264;
		struct attr_mjpg mjpg;
	} cur_vec;

	uint32_t reserved[4];
};

enum {
	AUDIO_CODEC_ID_G711	= 0,
	AUDIO_CODEC_NUM,
};

struct audio_encoder_options {
	uint32_t codec_id;
	uint32_t support_bitrate_mode;
	uint32_t bitrate_mode;

	uint32_t sample_rate_num;
	uint32_t sample_rate[MAX_AUDIO_SAMPLE_RATE_NUM];

	uint32_t bitrate_num;
	uint32_t bitrate[MAX_AUDIO_BITRATE_NUM];
};

struct attr_audio_encoder {
	uint32_t attr_id;

	int number;
	struct audio_encoder_options options[AUDIO_CODEC_NUM];

	struct {
		uint32_t codec_id;
		uint32_t sample_rate;
		uint32_t bitrate;
	} current;

	uint32_t reserved[4];
};

enum event_mode {
	EVENT_MOTION_DETECT = (1 << 1),
	EVENT_VOICE_DETECT = (1 << 2),
};

int rts_cam_open(const char *name);
void rts_cam_close(int fd);

int rts_cam_lock(int fd, int time_out);
int rts_cam_unlock(int fd);

int rts_cam_get_stream_attr(int fd, struct attr_stream **attr);
int rts_cam_set_stream_attr(int fd, struct attr_stream *attr);

int rts_cam_get_stream_ctrl(int fd, uint32_t id, struct stream_ctrl *ctrl);
int rts_cam_set_stream_ctrl(int fd, uint32_t id, struct stream_ctrl *ctrl);

int rts_cam_get_video_attr(int fd, struct attr_video **attr);
int rts_cam_set_video_attr(int fd, struct attr_video *attr);

int rts_cam_get_video_ctrl(int fd, uint32_t id, void *ctrl);
int rts_cam_set_video_ctrl(int fd, uint32_t id, void *ctrl);

int rts_cam_get_md_attr(int fd, struct attr_md **attr);
int rts_cam_set_md_attr(int fd, struct attr_md *attr);

int rts_cam_get_mask_attr(int fd, struct attr_mask **attr);
int rts_cam_set_mask_attr(int fd, struct attr_mask *attr);

int rts_cam_get_osd_attr(int fd, struct attr_osd **attr);
int rts_cam_set_osd_attr(int fd, struct attr_osd *attr);

int rts_cam_get_video_encoder_attr(int fd, struct attr_video_encoder **attr);
int rts_cam_set_video_encoder_attr(int fd, struct attr_video_encoder *attr);

int rts_cam_get_audio_encoder_attr(int fd, struct attr_audio_encoder **attr);
int rts_cam_set_audio_encoder_attr(int fd, struct attr_audio_encoder *attr);

int rts_cam_get_events(int fd, int event_id, uint32_t *property);

void rts_cam_free_attr(void *attr);

int rts_cam_set_gpio_ctrl(int fd, uint32_t id);

#ifdef __cplusplus
}
#endif

#endif
