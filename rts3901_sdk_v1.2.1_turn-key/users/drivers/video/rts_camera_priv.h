/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_priv.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _RTS_CAMERA_PRIV_H
#define _RTS_CAMERA_PRIV_H

#include "rts_camera.h"

#define RTSCAM_MAX_BUFFER_NUM			16

#define rtscam_call_video_op(icd, op, args...)		\
	(((icd)->ops->op) ? ((icd)->ops->op(args)) : -EINVAL)

void rtscam_get_timestamp(struct timeval *tv);

int rtscam_video_init_ctrl(struct rtscam_video_device *icd);
int rtscam_video_release_ctrl(struct rtscam_video_device *icd);

int rtscam_query_v4l2_ctrl(struct rtscam_video_device *icd,
		struct v4l2_queryctrl *v4l2_ctrl);
int rtscam_get_ctrl(struct rtscam_video_device *icd,
		struct v4l2_control *ctrl);
int rtscam_set_ctrl(struct rtscam_video_device *icd,
		struct v4l2_control *ctrl);
int rtscam_get_ext_ctrls(struct rtscam_video_device *icd,
		struct v4l2_ext_controls *ctrls);
int rtscam_set_ext_ctrls(struct rtscam_video_device *icd,
		struct v4l2_ext_controls *ctrls);
int rtscam_try_ext_ctrls(struct rtscam_video_device *icd,
		struct v4l2_ext_controls *ctrls);

struct rtscam_video_format *find_format_by_fourcc(
		struct rtscam_video_stream *stream, __u32 fourcc);
struct rtscam_video_frmival *get_video_frmival(
		struct rtscam_video_stream *stream, __u32 fourcc,
		__u32 width, __u32 height);
int rtscam_check_stream_format(struct rtscam_video_stream *stream);
int rtscam_check_user_format(struct rtscam_video_stream *stream,
		__u32 user_format, __u32 user_width, __u32 user_height,
		__u32 user_numerator, __u32 user_denominator);
int rtscam_try_user_format(struct rtscam_video_stream *stream,
		__u32 user_format, __u32 user_width, __u32 user_height);
int rtscam_set_user_format(struct rtscam_video_stream *stream,
		__u32 user_format, __u32 user_width, __u32 user_height);
int rtscam_set_user_frmival(struct rtscam_video_stream *stream,
		__u32 user_numerator, __u32 user_denominator);

#define v4l2pixfmtstr(x)	(x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, ((x) >> 24) & 0xff

#endif
