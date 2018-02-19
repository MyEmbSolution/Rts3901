/*
 * rts_cam_utils.h
 * Author: yafei_meng (yafei_meng@realsil.com.cn)
 */
#ifndef __RTS_CAM_UTILS_H
#define __RTS_CAM_UTILS_H

int cam_set_ctrl(int fd, uint32_t cmd, int val);
int cam_get_ctrl(int fd, uint32_t cmd, int *val, int *max_val,
		 int *min_val, int *step_val, int *def_val);

void free_video_attr(void *attr);
void free_stream_attr(void *attr);
void free_h264_attr(void *attr);
void free_motion_detect_attr(void *attr);
void free_mask_attr(void *attr);
void free_osd_attr(void *attr);
void free_audio_attr(void *attr);

int get_brightness(int fd, void *param);
int set_brightness(int fd, void *param);
int get_contrast(int fd, void *param);
int set_contrast(int fd, void *param);
int set_auto_wdr(int fd, void *param);
int get_auto_wdr(int fd, void *param);
int set_manual_wdr(int fd, void *param);
int get_manual_wdr(int fd, void *param);
int set_dehaze(int fd, void *param);
int get_dehaze(int fd, void *param);
int set_ldc(int fd, void *param);
int get_ldc(int fd, void *param);
int set_temporal_denoise(int fd, void *param);
int get_temporal_denoise(int fd, void *param);
int set_h264_profile(int fd, void *param);
int get_h264_profile(int fd, void *param);
int set_h264_level(int fd, void *param);
int get_h264_level(int fd, void *param);
int set_video_rotate(int fd, void *param);
int get_video_rotate(int fd, void *param);
int get_hue(int fd, void *param);
int set_hue(int fd, void *param);
int get_saturation(int fd, void *param);
int set_saturation(int fd, void *param);
int get_sharpness(int fd, void *param);
int set_sharpness(int fd, void *param);
int get_gamma(int fd, void *param);
int set_gamma(int fd, void *param);
int get_awb_ctrl(int fd, void *param);
int set_awb_ctrl(int fd, void *param);
int get_wb_temperature(int fd, void *param);
int set_wb_temperature(int fd, void *param);
int get_blc(int fd, void *param);
int set_blc(int fd, void *param);
int get_gain(int fd, void *param);
int set_gain(int fd, void *param);
int get_power_line_frequency(int fd, void *param);
int set_power_line_frequency(int fd, void *param);
int get_exposue_mode(int fd, void *param);
int set_exposue_mode(int fd, void *param);
int get_exposue_priority(int fd, void *param);
int set_exposue_priority(int fd, void *param);
int get_exposue_time(int fd, void *param);
int set_exposue_time(int fd, void *param);
int get_af(int fd, void *param);
int set_af(int fd, void *param);
int get_focus(int fd, void *param);
int set_focus(int fd, void *param);
int get_zoom(int fd, void *param);
int set_zoom(int fd, void *param);
int get_pan_tilt(int fd, void *param);
int set_pan_tilt(int fd, void *param);
int get_roll(int fd, void *param);
int set_roll(int fd, void *param);
int get_flip(int fd, void *param);
int set_flip(int fd, void *param);
int get_mirror(int fd, void *param);
int set_mirror(int fd, void *param);
int get_ir(int fd, void *param);
int set_ir(int fd, void *param);
int set_gray(int fd, void *param);
int get_rotate(int fd, void *param);
int set_rotate(int fd, void *param);
int get_special_effect(int fd, void *param);
int set_special_effect(int fd, void *param);
int get_ev_compensate(int fd, void *param);
int set_ev_compensate(int fd, void *param);
int get_color_temperature_estimation(int fd, void *param);
int set_color_temperature_estimation(int fd, void *param);
int get_ae_lock(int fd, void *param);
int set_ae_lock(int fd, void *param);
int get_awb_lock(int fd, void *param);
int set_awb_lock(int fd, void *param);
int get_af_lock(int fd, void *param);
int set_af_lock(int fd, void *param);
int get_led_touch_mode(int fd, void *param);
int set_led_touch_mode(int fd, void *param);
int get_led_flash_mode(int fd, void *param);
int set_led_flash_mode(int fd, void *param);
int get_iso(int fd, void *param);
int set_iso(int fd, void *param);
int get_scene_mode(int fd, void *param);
int set_scene_mode(int fd, void *param);
int get_roi_mode(int fd, void *param);
int set_roi_mode(int fd, void *param);
int get_3a_status(int fd, void *param);
int set_3a_status(int fd, void *param);
int get_idea_eye_sensitivity(int fd, void *param);
int set_idea_eye_sensitivity(int fd, void *param);
int get_idea_eye_status(int fd, void *param);
int set_idea_eye_status(int fd, void *param);
int get_idea_eye_mode(int fd, void *param);
int set_idea_eye_mode(int fd, void *param);
int rst_preview(int fd, void *param);
int rst_osd(int fd, void *param);
int set_gpio(int fd, void *param);
#endif
