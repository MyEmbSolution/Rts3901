/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_common.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include "rts_camera_priv.h"

unsigned int rtscam_debug = RTS_TRACE_DEFAULT;
unsigned int rts_clock_param = CLOCK_MONOTONIC;

EXPORT_SYMBOL_GPL(rtscam_debug);
EXPORT_SYMBOL_GPL(rts_clock_param);

module_param(rtscam_debug, uint, 0644);
MODULE_PARM_DESC(rtscam_debug, "activates debug info");

static int rts_clock_param_get(char *buffer, struct kernel_param *kp)
{
	if (CLOCK_MONOTONIC == rts_clock_param)
		return sprintf(buffer, "CLOCK_MONOTONIC");
	else
		return sprintf(buffer, "CLOCK_REALTIME");
}

static int rts_clock_param_set(const char *val, struct kernel_param *kp)
{
	if (strncasecmp(val, "clock_", strlen("clock_")) == 0)
		val += strlen("clock_");

	if (strcasecmp(val, "monotonic") == 0)
		rts_clock_param = CLOCK_MONOTONIC;
	else if (strcasecmp(val, "realtime") == 0)
		rts_clock_param = CLOCK_REALTIME;
	else
		return -EINVAL;

	return 0;
}

module_param_call(clock, rts_clock_param_set, rts_clock_param_get,
		  &rts_clock_param, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(clock, "Video buffers timestamp clock");

void rtscam_get_timestamp(struct timeval *tv)
{
	struct timespec ts;

	if (CLOCK_MONOTONIC == rts_clock_param)
		ktime_get_ts(&ts);
	else
		ktime_get_real_ts(&ts);

	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;
}

struct rtscam_video_format *find_format_by_fourcc(
		struct rtscam_video_stream *stream, __u32 fourcc)
{
	struct rtscam_video_format *format = stream->user_formats;

	while (format) {
		if (fourcc == format->fourcc)
			return format;
		format = format->next;
	}

	return NULL;
}

int rtscam_register_format(struct rtscam_video_stream *stream,
		struct rtscam_video_format_xlate *pfmt)
{
	struct rtscam_video_format *format = NULL;
	struct rtscam_video_format *p = NULL;

	if (find_format_by_fourcc(stream, pfmt->fourcc)) {
		rtsprintk(RTS_TRACE_ERROR, "%c%c%c%c has been registered\n",
				v4l2pixfmtstr(pfmt->fourcc));
		return -EEXIST;
	}

	format = kzalloc(sizeof(*format), GFP_KERNEL);
	if (NULL == format)
		return -ENOMEM;

	format->type = pfmt->type;
	format->index = pfmt->index;
	format->colorspace = pfmt->colorspace;
	format->field = pfmt->field;
	format->fourcc = pfmt->fourcc;
	format->bpp = pfmt->bpp;
	format->is_yuv = pfmt->is_yuv;
	format->rts_code = pfmt->rts_code;
	memcpy((void*)format->name, (void*)pfmt->name, sizeof(format->name));

	format->next = NULL;

	if (!stream->user_format) {
		stream->rts_code = pfmt->rts_code;
		stream->user_format = pfmt->fourcc;
	}

	if (NULL == stream->user_formats) {
		stream->user_formats = format;
		return 0;
	}
	p = stream->user_formats;
	while (p->next)
		p = p->next;
	p->next = format;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_register_format);

struct rtscam_video_frame *find_frame(struct rtscam_video_format *format,
		__u32 width, __u32 height)
{
	struct rtscam_video_frame *frame = NULL;

	frame = format->discrete.frames;

	while (frame) {
		if (frame->size.width == width && frame->size.height == height)
			return frame;
		frame = frame->next;
	}

	return NULL;
}

int rtscam_register_frame_discrete(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *size)
{
	struct rtscam_video_format *format;
	struct rtscam_video_frame *frame = NULL;
	struct rtscam_video_frame *p;

	format = find_format_by_fourcc(stream, fourcc);

	if (NULL == format) {
		rtsprintk(RTS_TRACE_ERROR,
				"please register format(%c%c%c%c) first\n",
				v4l2pixfmtstr(fourcc));
		return -EINVAL;
	}
	if (format->initialized && RTSCAM_SIZE_DISCRETE != format->frame_type)
		return -EINVAL;

	if (find_frame(format, size->width, size->height))
		return -EEXIST;

	frame = kzalloc(sizeof(*frame), GFP_KERNEL);
	if (NULL == frame)
		return -ENOMEM;

	frame->size.width = size->width;
	frame->size.height = size->height;
	frame->next = NULL;

	if (stream->user_format == fourcc &&
			(!stream->user_width || !stream->user_height)) {
		stream->user_width = size->width;
		stream->user_height = size->height;
		stream->bytesperline = (stream->user_width * format->bpp) >> 3;
		stream->sizeimage = stream->user_height * stream->bytesperline;
	}

	if (format->discrete.frames == NULL) {
		format->discrete.frames = frame;
	} else {
		p = format->discrete.frames;
		while (p->next)
			p = p->next;
		p->next = frame;
	}

	format->frame_type = RTSCAM_SIZE_DISCRETE;
	format->initialized = 1;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_register_frame_discrete);

int rtscam_register_frame_stepwise(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *max,
		struct rtscam_frame_size *min, struct rtscam_frame_size *step)
{
	struct rtscam_video_format *format;

	format = find_format_by_fourcc(stream, fourcc);

	if (NULL == format) {
		rtsprintk(RTS_TRACE_ERROR,
				"please register format(%c%c%c%c) first\n",
				v4l2pixfmtstr(fourcc));
		return -EINVAL;
	}

	if (format->initialized &&
			RTSCAM_SIZE_STEPWISE != format->frame_type &&
			RTSCAM_SIZE_CONTINUOUS != format->frame_type)
		return -EINVAL;

	if (stream->user_format == fourcc &&
			(!stream->user_width || !stream->user_height)) {
		stream->user_width = max->width;
		stream->user_height = max->height;
		stream->bytesperline = (stream->user_width * format->bpp) >> 3;
		stream->sizeimage = stream->user_height * stream->bytesperline;
	}

	format->stepwise.max.width = max->width;
	format->stepwise.max.height = max->height;
	format->stepwise.min.width = min->width;
	format->stepwise.min.height = min->height;
	format->stepwise.step.width = step->width;
	format->stepwise.step.height = step->height;

	format->frame_type = RTSCAM_SIZE_STEPWISE;
	format->initialized = 1;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_register_frame_stepwise);

struct rtscam_video_frmival *get_video_frmival(
		struct rtscam_video_stream *stream, __u32 fourcc,
		__u32 width, __u32 height)
{
	struct rtscam_video_format *format;
	struct rtscam_video_frmival *frmival = NULL;

	format = find_format_by_fourcc(stream, fourcc);

	if (NULL == format) {
		rtsprintk(RTS_TRACE_ERROR,
				"please register format(%c%c%c%c) first\n",
				v4l2pixfmtstr(fourcc));
		return NULL;
	}

	if (RTSCAM_SIZE_STEPWISE == format->frame_type ||
			RTSCAM_SIZE_CONTINUOUS == format->frame_type) {
		if (width < format->stepwise.min.width)
			return NULL;
		if (width > format->stepwise.max.width)
			return NULL;
		if ((width - format->stepwise.min.width) %
				format->stepwise.step.width != 0)
			return NULL;
		if (height < format->stepwise.min.height)
			return NULL;
		if (height > format->stepwise.max.height)
			return NULL;
		if ((height - format->stepwise.min.height) %
				format->stepwise.step.height != 0)
			return NULL;
		frmival = &format->stepwise.frmival;
	} else if (RTSCAM_SIZE_DISCRETE == format->frame_type) {
		struct rtscam_video_frame *frame = find_frame(format,
				width, height);
		if (NULL == frame)
			return NULL;
		frmival = &frame->frmival;
	}

	return frmival;
}

int rtscam_register_frmival_discrete(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *size,
		struct v4l2_fract *ival)
{
	struct rtscam_video_frmival *frmival = NULL;
	struct rtscam_frame_frmival *p;
	struct rtscam_frame_frmival *fival;

	frmival = get_video_frmival(stream, fourcc, size->width, size->height);
	if (NULL == frmival)
		return -EINVAL;

	if (frmival->initialized &&
			RTSCAM_SIZE_DISCRETE != frmival->frmival_type)
		return -EINVAL;

	p = frmival->discrete.frmivals;
	while (p) {
		if (p->frmival.numerator == ival->numerator &&
				p->frmival.denominator == ival->denominator)
			return -EEXIST;
		p = p->next;
	}

	fival = kzalloc(sizeof(*fival), GFP_KERNEL);
	if (!fival)
		return -ENOMEM;

	fival->frmival.numerator = ival->numerator;
	fival->frmival.denominator = ival->denominator;
	fival->next = NULL;

	if (stream->user_format == fourcc &&
			stream->user_width == size->width &&
			stream->user_height == size->height) {
		if (!stream->user_numerator || !stream->user_denominator) {
			stream->user_numerator = ival->numerator;
			stream->user_denominator = ival->denominator;
		}
	}

	if (NULL == frmival->discrete.frmivals) {
		frmival->discrete.frmivals = fival;
	} else {
		p = frmival->discrete.frmivals;
		while (p->next)
			p = p->next;
		p->next = fival;
	}

	frmival->initialized = 1;
	frmival->frmival_type = RTSCAM_SIZE_DISCRETE;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_register_frmival_discrete);

int rtscam_register_frmival_stepwise(struct rtscam_video_stream *stream,
		__u32 fourcc, struct rtscam_frame_size *size,
		struct v4l2_fract *max, struct v4l2_fract *min,
		struct v4l2_fract *step)
{
	struct rtscam_video_frmival *frmival = NULL;

	frmival = get_video_frmival(stream, fourcc, size->width, size->height);
	if (NULL == frmival)
		return -EINVAL;

	if (frmival->initialized &&
			RTSCAM_SIZE_STEPWISE != frmival->frmival_type &&
			RTSCAM_SIZE_CONTINUOUS != frmival->frmival_type)
		return -EINVAL;

	if (stream->user_format == fourcc &&
			stream->user_width == size->width &&
			stream->user_height == size->height) {
		if (!stream->user_numerator || !stream->user_denominator) {
			stream->user_numerator = max->numerator;
			stream->user_denominator = max->denominator;
		}
	}

	frmival->stepwise.max.numerator = max->numerator;
	frmival->stepwise.max.denominator = max->denominator;
	frmival->stepwise.min.numerator = min->numerator;
	frmival->stepwise.min.denominator = min->denominator;
	frmival->stepwise.step.numerator = step->numerator;
	frmival->stepwise.step.denominator = step->denominator;

	frmival->frmival_type = RTSCAM_SIZE_STEPWISE;
	frmival->initialized = 1;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_register_frmival_stepwise);

int rtscam_clr_frmival(struct rtscam_video_frmival *frmival)
{
	if (!frmival->initialized)
		return 0;

	if (RTSCAM_SIZE_DISCRETE == frmival->frmival_type) {
		struct rtscam_frame_frmival *cur = frmival->discrete.frmivals;
		struct rtscam_frame_frmival *next;

		while (cur) {
			next = cur->next;
			kfree(cur);
			cur = next;
		}
		frmival->discrete.frmivals = NULL;
	}
	frmival->initialized = 0;

	return 0;
}

int rtscam_clr_frame(struct rtscam_video_frame *frame)
{
	struct rtscam_video_frame *cur = frame;
	struct rtscam_video_frame *next;

	while (cur) {
		next = cur->next;
		rtscam_clr_frmival(&cur->frmival);
		kfree(cur);
		cur = next;
	}

	return 0;
}

int rtscam_clr_format(struct rtscam_video_stream *stream)
{
	struct rtscam_video_format *cur = stream->user_formats;
	struct rtscam_video_format *next = NULL;

	while (cur) {
		next = cur->next;

		if (cur->initialized) {
			if (RTSCAM_SIZE_DISCRETE == cur->frame_type) {
				rtscam_clr_frame(cur->discrete.frames);
				cur->discrete.frames = NULL;
			} else {
				rtscam_clr_frmival(&cur->stepwise.frmival);
			}
			cur->initialized = 0;
		}
		kfree(cur);
		cur = next;
	}
	stream->user_formats = NULL;

	stream->rts_code = 0;
	stream->user_format = 0;
	stream->user_width = 0;
	stream->user_height = 0;
	stream->user_numerator = 1;
	stream->user_denominator = 1;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_clr_format);

static int rtscam_check_frmival(struct rtscam_video_frmival *frmival)
{
	if (!frmival)
		return -EINVAL;

	if (!frmival->initialized)
		return -EINVAL;

	if (RTSCAM_SIZE_DISCRETE == frmival->frmival_type) {
		struct rtscam_frame_frmival *ival;

		if (!frmival->discrete.frmivals)
			return -EINVAL;

		ival = frmival->discrete.frmivals;
		while (ival) {
			if (ival->frmival.numerator == 0 ||
					ival->frmival.denominator == 0)
				return -EINVAL;
			ival = ival->next;
		}
	} else {
		if (frmival->stepwise.max.numerator == 0 ||
				frmival->stepwise.max.denominator == 0 ||
				frmival->stepwise.min.numerator == 0 ||
				frmival->stepwise.min.denominator == 0 ||
				frmival->stepwise.step.numerator == 0 ||
				frmival->stepwise.step.denominator == 0)
			return -EINVAL;
	}

	return 0;
}

int rtscam_check_stream_format(struct rtscam_video_stream *stream)
{
	struct rtscam_video_format *format = NULL;
	int ret;

	if (!stream)
		return -EINVAL;

	if (!stream->user_formats)
		return -EINVAL;

	format = stream->user_formats;

	while (format) {
		if (format->fourcc == 0)
			return -EINVAL;

		if (!format->initialized)
			return -EINVAL;

		if (RTSCAM_SIZE_DISCRETE == format->frame_type) {
			struct rtscam_video_frame *frame = NULL;

			if (!format->discrete.frames)
				return -EINVAL;

			frame = format->discrete.frames;
			while (frame) {
				if (frame->size.width == 0 ||
						frame->size.height == 0)
					return -EINVAL;

				ret = rtscam_check_frmival(&frame->frmival);
				if (ret)
					return ret;
				frame = frame->next;
			}
		} else {
			if (format->stepwise.max.width == 0 ||
					format->stepwise.max.height == 0 ||
					format->stepwise.min.width == 0 ||
					format->stepwise.min.height == 0 ||
					format->stepwise.step.width == 0 ||
					format->stepwise.step.height == 0)
				return -EINVAL;

			ret = rtscam_check_frmival(&format->stepwise.frmival);
			if (ret)
				return ret;
		}

		format = format->next;
	}

	return 0;
}

static int rtscam_check_user_frmival(struct rtscam_video_frmival *frmival,
		__u32 user_numerator, __u32 user_denominator)
{
	if (RTSCAM_SIZE_DISCRETE == frmival->frmival_type) {
		struct rtscam_frame_frmival *ival = frmival->discrete.frmivals;
		while (ival) {
			if (ival->frmival.denominator * user_numerator ==
				ival->frmival.numerator * user_denominator)
				break;
			ival = ival->next;
		}
		if (!ival) {
			rtsprintk(RTS_TRACE_ERROR, "invalid user fps\n");
			return -EINVAL;
		}
	} else {
		int valid = 1;
		if (user_numerator < frmival->stepwise.min.numerator)
			valid = 0;
		if (user_numerator > frmival->stepwise.max.numerator)
			valid = 0;
		if ((user_numerator - frmival->stepwise.min.numerator) %
				frmival->stepwise.step.numerator != 0)
			valid = 0;
		if (user_denominator < frmival->stepwise.min.denominator)
			valid = 0;
		if (user_denominator> frmival->stepwise.max.denominator)
			valid = 0;
		if ((user_denominator - frmival->stepwise.min.denominator) %
				frmival->stepwise.step.denominator != 0)
			valid = 0;
		if (!valid) {
			rtsprintk(RTS_TRACE_ERROR, "invalid user fps\n");
			return -EINVAL;
		}
	}

	return 0;
}

int rtscam_check_user_format(struct rtscam_video_stream *stream,
		__u32 user_format, __u32 user_width, __u32 user_height,
		__u32 user_numerator, __u32 user_denominator)
{
	struct rtscam_video_format *format = NULL;
	struct rtscam_video_frmival *frmival = NULL;
	int ret;

	if (user_format == 0 || user_width == 0 || user_height == 0 ||
			user_numerator == 0 || user_denominator == 0)
		return -EINVAL;

	format = find_format_by_fourcc(stream, user_format);
	if (!format) {
		rtsprintk(RTS_TRACE_ERROR, "invalid user format(%c%c%c%c)\n",
				v4l2pixfmtstr(user_format));
		return -EINVAL;
	}

	frmival = get_video_frmival(stream, user_format,
			user_width, user_height);
	if (!frmival) {
		rtsprintk(RTS_TRACE_ERROR, "invalid user format\n");
		return -EINVAL;
	}

	ret = rtscam_check_user_frmival(frmival,
			user_numerator, user_denominator);
	if (ret)
		return ret;

	return 0;
}

int rtscam_try_user_format(struct rtscam_video_stream *stream,
		__u32 user_format, __u32 user_width, __u32 user_height)
{
	struct rtscam_video_frmival *frmival = NULL;

	if (user_format == 0 || user_width == 0 || user_height == 0)
		return -EINVAL;

	frmival = get_video_frmival(stream, user_format,
			user_width, user_height);
	if (!frmival) {
		rtsprintk(RTS_TRACE_ERROR, "invalid user format\n");
		return -EINVAL;
	}

	return 0;
}

int rtscam_set_user_format(struct rtscam_video_stream *stream,
		__u32 user_format, __u32 user_width, __u32 user_height)
{
	struct rtscam_video_format *format = NULL;
	struct rtscam_video_frmival *frmival = NULL;
	int ret = 0;

	if (user_format == 0 || user_width == 0 || user_height == 0)
		return -EINVAL;

	format = find_format_by_fourcc(stream, user_format);
	if (!format) {
		rtsprintk(RTS_TRACE_ERROR, "invalid user format(%c%c%c%c)\n",
				v4l2pixfmtstr(user_format));
		return -EINVAL;
	}

	frmival = get_video_frmival(stream, user_format,
			user_width, user_height);
	if (!frmival) {
		rtsprintk(RTS_TRACE_ERROR, "invalid user format\n");
		return -EINVAL;
	}

	ret = rtscam_check_user_frmival(frmival,
			stream->user_numerator, stream->user_denominator);
	if (!ret)
		goto exit;

	if (RTSCAM_SIZE_DISCRETE == frmival->frmival_type) {
		struct rtscam_frame_frmival *ival = frmival->discrete.frmivals;
		if (!ival)
			return -EINVAL;
		stream->user_numerator = ival->frmival.numerator;
		stream->user_denominator = ival->frmival.denominator;
	} else {
		stream->user_numerator = frmival->stepwise.max.numerator;
		stream->user_denominator = frmival->stepwise.max.denominator;
	}

exit:
	stream->rts_code = format->rts_code;
	stream->user_format = user_format;
	stream->user_width = user_width;
	stream->user_height = user_height;

	stream->bytesperline = (stream->user_width * format->bpp) >> 3;
	stream->sizeimage = stream->user_height * stream->bytesperline;

	return 0;
}

int rtscam_set_user_frmival(struct rtscam_video_stream *stream,
		__u32 user_numerator, __u32 user_denominator)
{
	int ret = 0;

	ret = rtscam_check_user_format(stream, stream->user_format, 
			stream->user_width, stream->user_height,
			user_numerator, user_denominator);
	if (ret)
		return ret;

	stream->user_numerator = user_numerator;
	stream->user_denominator = user_denominator;

	return 0;
}

int rts_test_bit(const __u8 *data, int bit)
{
	return (data[bit >> 3] >> (bit & 7)) & 1;
}
EXPORT_SYMBOL_GPL(rts_test_bit);

void rts_clear_bit(__u8 *data, int bit)
{
	data[bit >> 3] &= ~(1 << (bit & 7));
}
EXPORT_SYMBOL_GPL(rts_clear_bit);

long rtscam_usercopy(struct file *filp, unsigned int cmd, unsigned long arg,
		rtscam_kioctl func)
{
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = (void *)arg;
	unsigned int size = _IOC_SIZE(cmd);
	int ret = -EINVAL;

	if (!func)
		return -EINVAL;

	if (_IOC_DIR(cmd) != _IOC_NONE) {
		if (size <= sizeof(sbuf)) {
			memset(sbuf, 0, size);
			parg = sbuf;
		} else {
			mbuf = kzalloc(size, GFP_KERNEL);
			if (NULL == mbuf)
				return -ENOMEM;
			parg = mbuf;
		}
		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			ret = copy_from_user(parg, (void __user *)arg, size);
			if (ret)
				goto out;
		}
	}

	ret = func(filp, cmd, parg);

	if (0 == ret && (_IOC_DIR(cmd) & _IOC_READ))
		ret = copy_to_user((void __user *)arg, parg, size);
out:
	if (mbuf)
		kfree(mbuf);
	mbuf = NULL;
	return ret;
}
EXPORT_SYMBOL_GPL(rtscam_usercopy);

MODULE_AUTHOR("Ming Qian <ming_qian@realsil.com.cn>");
MODULE_DESCRIPTION("rts_camera commmon");
MODULE_LICENSE("GPL v2");

