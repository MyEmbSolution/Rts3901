/*
 * Realtek Semiconductor Corp.
 *
 * tests/test_isp_h264.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <rtsavapi.h>
#include <rtscamkit.h>
#include <rtsv4l2.h>
#include <rtscamkit.h>
#include "rtsftest.h"
#include "test_video.h"

static int set_h264_stream_attr(struct rts_av_stream *stream, int sequence)
{
	struct rts_av_unit *unit;
	struct rts_isp_attr isp_attr;
	struct rts_h264_attr h264_attr;
	unsigned int id;
	int ret;

	unit = rts_av_stream_get_item(stream, NULL);
	rts_av_get_unit_id(unit, &id);
	if (id != RTS_AV_ID_ISP)
		return -RTS_FTEST_ERROR;

	ret = set_isp_attr(unit);
	if (ret)
		return ret;

	ret = rts_av_get_attr(unit, &isp_attr);
	if (ret)
		return ret;

	unit = rts_av_stream_get_item(stream, unit);
	if (unit) {
		rts_av_get_unit_id(unit, &id);
		if (RTS_AV_ID_H264 == id) {
			int i, j;
			int bps[] = {256*1024,
				512*1024,
				1024*1024,
				2*1024*1024,
				4*1024*1024};
			int size_b = sizeof(bps)/sizeof(int);
			int rotation[] = {rts_av_rotation_0,
				rts_av_rotation_90r,
				rts_av_rotation_90l};
			int size_r = sizeof(rotation)/sizeof(int);

			sequence = sequence * sequence;

			i = sequence % size_b;
			j = sequence % size_r;

			h264_attr.profile = H264_PROFILE_HIGH;
			h264_attr.level = H264_LEVEL_4;
			h264_attr.qp = -1;
			h264_attr.bps = bps[i];
			h264_attr.gop = 300;
			h264_attr.rotation = rotation[j];
			if (sequence % 2)
				h264_attr.videostab = RTS_TRUE;
			else
				h264_attr.videostab = RTS_FALSE;
			ret = rts_av_set_attr(unit, &h264_attr);
		} else {
			ret = -RTS_FTEST_ERROR;
		}
	} else {
		ret = -RTS_FTEST_ERROR;
	}

	return ret;
}

int isp_h264_initialize(RtsFtestEnv tester, void **arg)
{
	struct isp_test_instance *inst = NULL;
	int i;
	int ret;

	if (!tester || !arg)
		return -RTS_FTEST_E_NULL_POINT;

	inst = (struct isp_test_instance *)calloc(1, sizeof(*inst));
	if (!inst)
		return -RTS_FTEST_E_NO_MEMORY;
	inst->tester = tester;

	ret = rts_av_init();
	if (ret) {
		free(inst);
		return -RTS_FTEST_ERROR;
	}

	for (i = 51; i < 55; i++) {
		char name[64];
		int fd;

		if (52 == i)
			continue;

		snprintf(name, sizeof(name), "/dev/video%d", i);

		fd = rts_v4l2_open(name, O_RDWR);
		if (fd < 0)
			continue;
		rts_v4l2_close(fd);
		fd = -1;
		inst->stream[inst->count] = create_stream(i, RTS_AV_ID_H264);
		if (!inst->stream[inst->count]) {
			RTS_ERR("create h264 stream for %s fail\n", name);
			goto error;
		}
		inst->count++;
		if (inst->count >= MAX_ISP_STREAM_NUM)
			break;
	}

	*arg = (void*)inst;

	return RTS_FTEST_OK;
error:
	cleanup_test_instance(inst);

	return -RTS_FTEST_ERROR;
}

int isp_h264_setup(void *arg)
{
	return RTS_FTEST_OK;
}

int isp_h264_warmup(void *arg)
{
	struct isp_test_instance *inst = arg;
	int i;
	int ret = RTS_FTEST_OK;

	if (!inst)
		return -RTS_FTEST_E_NULL_POINT;

	for (i = 0; i < inst->count; i++) {
		struct rts_av_stream *stream = inst->stream[i];

		if (set_h264_stream_attr(stream, inst->seq++)) {
			ret |= -RTS_FTEST_ERROR;
			RTS_ERR("set stream attr fail\n");
		} else {
			struct rts_av_unit *isp_unit;
			struct rts_isp_attr isp_attr;
			struct rts_av_unit *h264_unit;
			struct rts_h264_attr h264_attr;

			isp_unit = rts_av_stream_get_item(stream, NULL);
			rts_av_get_attr(isp_unit, &isp_attr);
			RTS_INFO("stream<%d> test %c%c%c%c %dx%d %d/%d\n",
				i,
				isp_attr.pixelformat & 0xff,
				(isp_attr.pixelformat >> 8) & 0xff,
				(isp_attr.pixelformat >> 16) & 0xff,
				(isp_attr.pixelformat >> 24) & 0xff,
				isp_attr.width, isp_attr.height,
				isp_attr.numerator,
				isp_attr.denominator);
			inst->test_fps[i] = isp_attr.denominator / isp_attr.numerator;

			h264_unit = rts_av_stream_get_item(stream, isp_unit);
			rts_av_get_attr(h264_unit, &h264_attr);

			RTS_INFO("h264 bps = %d, rotation = %d, stab = %d\n",
					h264_attr.bps,
					h264_attr.rotation,
					h264_attr.videostab);
		}
	}

	return RTS_FTEST_OK;
}

int isp_h264_preprocess(void *arg)
{
	return start_test_stream(arg);
}

int isp_h264_runonce(void *arg, unsigned int time)
{
	return run_test_stream(arg, time);
}

int isp_h264_postprocess(void *arg)
{
	return stop_test_stream(arg, 1200);
}

void isp_h264_cleanup(void *arg)
{
	cleanup_test_instance(arg);
}

struct rts_ftest_ops isp_h264_ops = {
	.initialize	= isp_h264_initialize,
	.setup		= isp_h264_setup,
	.warmup		= isp_h264_warmup,
	.preprocess	= isp_h264_preprocess,
	.runonce	= isp_h264_runonce,
	.postprocess	= isp_h264_postprocess,
	.cleanup	= isp_h264_cleanup,
};

struct rts_ftest_case test_isp_h264 = {
	.name		= "isp_h264",
	.version	= "1.0.2",
	.frequency	= 30,
	.continuous	= 1,
	.sequence	= 101,
	.ops = &isp_h264_ops,
};

