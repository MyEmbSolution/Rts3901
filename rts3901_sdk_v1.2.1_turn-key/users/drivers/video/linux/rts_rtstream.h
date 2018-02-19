/*
 * Realtek Semiconductor Corp.
 *
 * include/uapi/linux/rts_rtstream.h
 *
 * Copyright (C) 2016      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _INCLUDE_RTS_RTSTREAM_H
#define _INCLUDE_RTS_RTSTREAM_H

#include <linux/types.h>

struct rtstream_unit_info {
	unsigned int id;
	__u32 format;
	union {
		struct {
			__u32 width;
			__u32 height;
			__u32 numerator;
			__u32 denominator;
		} video;
		struct {
			__u32 samplerate;
			__u32 bitfmt;
			__u32 channel;
			__u32 reserved;
		} audio;
	};
	int isp_id;
	int rotation;
	int videostab;
};

struct rtstream_info {
	int no;

	int unit_count;
	struct rtstream_unit_info *units;
};

struct rtstream_unit_desc {
	int stream_no;
	int index;
	struct rtstream_unit_info unit;
};

struct rtstream_stream_desc {
	int index;
	int stream_no;
};

struct rtstream_ctrl {
	char name[32];
	__s32 value;
};

struct rtstream_ctrls {
	int stream_no;

	int count;
	struct rtstream_ctrl *ctrls;
};

#define RTS_RTSTREAM_IOC_MAGIC		'r'

#define RTSTREAM_IOC_S_STREAMINFO	_IOWR(RTS_RTSTREAM_IOC_MAGIC, 1, struct rtstream_info)
#define RTSTREAM_IOC_DELETE_STREAM	_IOWR(RTS_RTSTREAM_IOC_MAGIC, 2, int)

#define RTSTREAM_IOC_ENUM_STREAM	_IOWR(RTS_RTSTREAM_IOC_MAGIC, 3, struct rtstream_stream_desc)
#define RTSTREAM_IOC_ENUM_UNIT		_IOWR(RTS_RTSTREAM_IOC_MAGIC, 4, struct rtstream_unit_desc)

#define RTSTREAM_IOC_S_CTRLS		_IOWR(RTS_RTSTREAM_IOC_MAGIC, 5, struct rtstream_ctrls)
#define RTSTREAM_IOC_G_CTRLS		_IOWR(RTS_RTSTREAM_IOC_MAGIC, 6, struct rtstream_ctrls)

#endif

