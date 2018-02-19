/*
 * Realtek Semiconductor Corp.
 *
 * tests/test_isp_mjpeg.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <rtsavapi.h>
#include <rtsv4l2.h>
#include <rtscamkit.h>
#include "rtsftest.h"
#include "test_video.h"

static int set_mjpeg_stream_attr(struct rts_av_stream *stream, int index, int sequence)
{
	struct rts_av_unit *unit;
	struct rts_isp_attr isp_attr;
	struct rts_jpgenc_attr mjpeg_attr;
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
		if (RTS_AV_ID_MJPGENC == id) {
			int rotation[] = {rts_av_rotation_0,
				rts_av_rotation_90r,
				rts_av_rotation_90l};
			int size_r = sizeof(rotation)/sizeof(int);

			sequence %= (size_r * size_r);
			if (!index)
				sequence /= size_r;
			else
				sequence %= size_r;

			mjpeg_attr.rotation = rotation[sequence];

			ret = rts_av_set_attr(unit, &mjpeg_attr);
		} else {
			ret = -RTS_FTEST_ERROR;
		}
	} else {
		ret = -RTS_FTEST_ERROR;
	}

	return ret;
}

int isp_mjpeg_initialize(RtsFtestEnv tester, void **arg)
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

	for (i = 52; i < 55; i++) {
		char name[64];
		int fd;

		snprintf(name, sizeof(name), "/dev/video%d", i);

		fd = rts_v4l2_open(name, O_RDWR);
		if (fd < 0)
			continue;
		rts_v4l2_close(fd);
		fd = -1;
		inst->stream[inst->count] = create_stream(i, RTS_AV_ID_MJPGENC);
		if (!inst->stream[inst->count]) {
			RTS_ERR("create mjpeg stream for %s fail\n", name);
			goto error;
		}
		inst->count++;
		if (inst->count >= MAX_ISP_STREAM_NUM)
			break;
		if (inst->count >= 2)
			break;
	}

	*arg = (void*)inst;

	return RTS_FTEST_OK;
error:
	cleanup_test_instance(inst);

	return -RTS_FTEST_ERROR;
}

int isp_mjpeg_setup(void *arg)
{
	return RTS_FTEST_OK;
}

int isp_mjpeg_warmup(void *arg)
{
	struct isp_test_instance *inst = arg;
	int i;
	int ret = RTS_FTEST_OK;

	if (!inst)
		return -RTS_FTEST_E_NULL_POINT;

	for (i = 0; i < inst->count; i++) {
		struct rts_av_stream *stream = inst->stream[i];

		if (set_mjpeg_stream_attr(stream, i, inst->seq++)) {
			ret |= -RTS_FTEST_ERROR;
			RTS_INFO("set stream attr fail\n");
		} else {
			struct rts_av_unit *isp_unit;
			struct rts_isp_attr isp_attr;
			struct rts_av_unit *mjpeg_unit;
			struct rts_jpgenc_attr mjpeg_attr;

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
			inst->test_fps[i] /= inst->count;
			//inst->test_fps[i] = 1;

			mjpeg_unit = rts_av_stream_get_item(stream, isp_unit);
			rts_av_get_attr(mjpeg_unit, &mjpeg_attr);

			RTS_INFO("mjpeg rotation = %d\n", mjpeg_attr.rotation);
		}
	}

	return RTS_FTEST_OK;
}

int isp_mjpeg_preprocess(void *arg)
{
	return start_test_stream(arg);
}

int isp_mjpeg_runonce(void *arg, unsigned int time)
{
	return run_test_stream(arg, time);
}

int isp_mjpeg_postprocess(void *arg)
{
	return stop_test_stream(arg, 1500);
}

void isp_mjpeg_cleanup(void *arg)
{
	cleanup_test_instance(arg);
}

struct rts_ftest_ops isp_mjpeg_ops = {
	.initialize	= isp_mjpeg_initialize,
	.setup		= isp_mjpeg_setup,
	.warmup		= isp_mjpeg_warmup,
	.preprocess	= isp_mjpeg_preprocess,
	.runonce	= isp_mjpeg_runonce,
	.postprocess	= isp_mjpeg_postprocess,
	.cleanup	= isp_mjpeg_cleanup,
};

struct rts_ftest_case test_isp_mjpeg = {
	.name		= "isp_mjpeg",
	.version	= "1.0.1",
	.frequency	= 10,
	.continuous	= 1,
	.sequence	= 102,
	.ops = &isp_mjpeg_ops,
};

