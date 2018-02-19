/*
 * Realtek Semiconductor Corp.
 *
 * tests/test_isp_preview.c
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
#include <rtstream.h>

static int __set_isp_attr(struct rts_av_unit *isp_unit, int sequence)
{
	struct rts_av_ability_t *ability;
	struct rts_isp_ability *isp_ability;
	struct rts_isp_attr isp_attr;
	struct rts_v4l2_resolution_stepwise *resolution;
	int ret;
	int i, j, k;
	int index;
	struct test_resolution resolutions[] = {
		{1920, 1080},
		{1600, 1200},
		{1280, 720},
		{960, 720},
		{816, 616},
		{800, 600},
		{720, 480},
		{640, 480},
		{320, 240}
};

	ret = rts_av_get_attr(isp_unit, &isp_attr);
	if (ret)
		return ret;

	ret = rts_av_query_ability(isp_unit, &ability);
	if (ret)
		return ret;

	if (ability->type != rts_av_ability_isp) {
		ret = -RTS_FTEST_E_INVALID_ARG;
		goto exit;
	}

	isp_ability = &ability->isp_ability;
	if (isp_ability->fmt_number <= 0) {
		ret = -RTS_FTEST_ERROR;
		goto exit;
	}
	resolution = &isp_ability->resolution;

	index;
	for (k = 0; k < RTS_ARRAY_SIZE(resolutions); k++) {
		uint32_t w,h;

		w = resolutions[k].width;
		h = resolutions[k].height;

		if (w > resolution->max_width)
			continue;
		if (w < resolution->min_width)
			continue;
		if ((w - resolution->min_width) % resolution->step_width)
			continue;
		if (h > resolution->max_height)
			continue;
		if (h < resolution->min_height)
			continue;
		if ((h - resolution->min_height) % resolution->step_height)
			continue;

		index++;
	}

	index = isp_ability->fmt_number * isp_ability->frmival_num * index;

	sequence %= index;

	index = 0;

	for (i = 0; i < isp_ability->fmt_number; i++) {
		for (j = 0; j < isp_ability->frmival_num; j++) {
			for (k = 0; k < RTS_ARRAY_SIZE(resolutions); k++) {
				uint32_t w,h;

				w = resolutions[k].width;
				h = resolutions[k].height;

				if (w > resolution->max_width)
					continue;
				if (w < resolution->min_width)
					continue;
				if ((w - resolution->min_width) % resolution->step_width)
					continue;
				if (h > resolution->max_height)
					continue;
				if (h < resolution->min_height)
					continue;
				if ((h - resolution->min_height) % resolution->step_height)
					continue;

				if (sequence == index) {
					isp_attr.pixelformat = *(isp_ability->pformats + i);
					isp_attr.numerator = (isp_ability->pfrmivals + j)->numerator;
					isp_attr.denominator = (isp_ability->pfrmivals + j)->denominator;
					isp_attr.width = w;
					isp_attr.height = h;
					ret = rts_av_set_attr(isp_unit, &isp_attr);
					goto exit;
				}
				index++;
			}
		}
	}

exit:
	rts_av_release_ability(ability);
	return ret;
}

int isp_prv_initialize(RtsFtestEnv tester, void **arg)
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
	for (i = 51; i <= 54; i++) {
		char name[64];
		int fd;

		snprintf(name, sizeof(name), "/dev/video%d", i);

		fd = rts_v4l2_open(name, O_RDWR);
		if (fd < 0)
			break;
		rts_v4l2_close(fd);
		fd = -1;
		inst->stream[inst->count] = create_stream(i, 0);
		if (!inst->stream[inst->count]) {
			RTS_ERR("create isp stream for %s fail\n", name);
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

int isp_prv_setup(void *arg)
{
	return RTS_FTEST_OK;
}

int isp_prv_warmup(void *arg)
{
	struct isp_test_instance *inst = arg;
	int i;
	int ret = RTS_FTEST_OK;

	if (!inst)
		return -RTS_FTEST_E_NULL_POINT;

	for (i = 0; i < inst->count; i++) {
		struct rts_av_stream *stream = inst->stream[i];
		struct rts_av_unit *isp_unit;
		isp_unit = rts_av_stream_get_item(stream, NULL);
		while (isp_unit) {
			unsigned int id;
			rts_av_get_unit_id(isp_unit, &id);
			if (id == RTS_AV_ID_ISP)
				break;
		}
		if (isp_unit) {
			if (__set_isp_attr(isp_unit, inst->seq)) {
				ret |= -RTS_FTEST_ERROR;
			} else {
				struct rts_isp_attr isp_attr;
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
			}
		} else
			ret |= -RTS_FTEST_ERROR;
	}
	inst->seq++;

	return ret;
}

int isp_prv_preprocess(void *arg)
{
	return start_test_stream(arg);
}

int isp_prv_runonce(void *arg, unsigned int time)
{
	return run_test_stream(arg, time);
}

int isp_prv_postprocess(void *arg)
{
	return stop_test_stream(arg, 1200);
}

void isp_prv_cleanup(void *arg)
{
	cleanup_test_instance(arg);
}

struct rts_ftest_ops isp_preview_ops = {
	.initialize	= isp_prv_initialize,
	.setup		= isp_prv_setup,
	.warmup		= isp_prv_warmup,
	.preprocess	= isp_prv_preprocess,
	.runonce	= isp_prv_runonce,
	.postprocess	= isp_prv_postprocess,
	.cleanup	= isp_prv_cleanup,
};

struct rts_ftest_case test_isp_preview = {
	.name		= "isp_preview",
	.version	= "1.0.1",
	.frequency	= 30,
	.continuous	= 1,
	.sequence	= 100,
	.ops = &isp_preview_ops,
};
