/*
 * svr_cam_utils.h
 * Author: yafei meng (yafei_meng@realsil.com.cn)
 */
#ifndef __SVR_CAM_UTILS__H
#define __SVR_CAM_UTILS__H

#include "rts_cam.h"
#include "cli_cam.h"


int get_isp_settings_from_file(uint32_t id, void *arg);
int save_isp_settings_to_file(uint32_t id, void *arg);
int get_isp_attr(uint32_t id, uint8_t *args);
int set_isp_attr(uint32_t id, uint8_t *args);
int get_stream_attr(const char *dev_path, uint8_t *args);
int get_stream_resolution(const char *dev_path, uint8_t *args);
int get_stream_fps(const char *dev_path, uint8_t *args);
int get_md_status(const char *dev_path, uint32_t *property);
int get_md_attr(const char *dev_path, struct attr_md *attr);
int set_md_attr(const char *dev_path, struct attr_md *attr);
int get_mask_attr(const char *dev_path, struct attr_mask *attr);
int set_mask_attr(const char *dev_path, struct attr_mask *attr);
int parse_video_encoder_attr_from_peacock_config(char *dev_path,
		uint32_t *gop, uint32_t *bitrate, uint32_t *bitrate_mode,
		uint32_t *bitrate_max, uint32_t *bitrate_min);
int get_video_encoder_attr_range(uint32_t *min_gop, uint32_t *max_gop,
		uint32_t *min_qp, uint32_t *max_qp,
		uint32_t *min_bitrate, uint32_t *max_bitrate);

int get_current_audio_encoder_attr(uint32_t *sample_rate, uint32_t *bitrate);

int get_audio_encoder_attr_options(uint32_t *sample_rate_num,
		uint32_t *sample_rate, uint32_t *bitrate_num,
		uint32_t *bitrate);

int update_peacock_config(char *dev_path, uint32_t *denominator,
		uint32_t *width, uint32_t *height, char *pixformat,
		uint32_t *gop, uint32_t *bitrate, uint32_t *bitrate_mode,
		uint32_t *bitrate_max, uint32_t *bitrate_min);

int svr_get_venc_capability(void *arg);
int svr_get_venc_output_format(const char *dev_name, void *arg);
int svr_set_venc_output_format(const char *dev_name, void *arg);
int svr_get_ldc_attr(const char *dev_name, void *args);
int svr_set_ldc_attr(const char *dev_name, void *args);
int svr_get_video_rotate_attr(const char *dev_path, void *args);
int svr_set_video_rotate_attr(const char *dev_path, void *args);
int svr_get_h264_profile_attr(const char *dev_path, void *args);
int svr_set_h264_profile_attr(const char *dev_path, void *args);
int svr_get_h264_level_attr(const char *dev_path, void *args);
int svr_set_h264_level_attr(const char *dev_path, void *args);
int svr_reset_preview(const char *dev_path, void *args);
int svr_reset_osd(const char *dev_path, void *args);
int svr_set_ir_gpio(uint8_t *args);
#endif
