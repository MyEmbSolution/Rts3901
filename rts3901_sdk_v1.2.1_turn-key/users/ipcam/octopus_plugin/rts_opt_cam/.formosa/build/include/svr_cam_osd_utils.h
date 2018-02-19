/*
 * svr_cam_osd_utils.h
 * Author: yafei meng (yafei_meng@realsil.com.cn)
 */
#ifndef __SVR_CAM_OSD_UTILS_H
#define __SVR_CAM_OSD_UTILS_H

#include <rtsosd.h>
int release_osd_plugin(int *fd_osd_ctl, void **p_fd_osd, int len);
int get_osd_plugin(int *fd_osd_ctl, void **p_fd_osd,
		struct rts_osd_lib_info *osd_char_info, int len);
int set_osd_attr(const char *dev_path, void *args);
int get_osd_attr(const char *dev_path, void *args);
int init_osd(const char *dev_path);

#endif
