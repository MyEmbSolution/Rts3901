/*
 * Realtek Semiconductor Corp.
 *
 * tests/test_video.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <rtsavapi.h>
#include <rtsv4l2.h>
#include <rtscamkit.h>
#include "rtsftest.h"
#include "test_video.h"

struct rts_av_stream *create_stream(int idx, unsigned int encid)
{
	struct rts_av_unit *isp_unit = NULL;
	struct rts_av_unit *enc_unit = NULL;
	struct rts_isp_attr isp_attr;
	struct rts_av_stream *stream = NULL;
	int isp_id = idx - 51;
	int ret;

	isp_unit = rts_av_new_unit_ex(RTS_AV_ID_ISP, &isp_id);
	if (!isp_unit)
		return NULL;

	if (encid) {
		enc_unit = rts_av_new_unit(encid);
		if (!enc_unit)
			goto error;
	}

	stream = rts_av_new_stream();
	if (!stream)
		goto error;
	rts_av_stream_add_tail(stream, isp_unit);
	if (enc_unit)
		rts_av_stream_add_tail(stream, enc_unit);

	return stream;
error:
	if (isp_unit) {
		rts_av_delete_unit(isp_unit);
		isp_unit = NULL;
	}
	if (enc_unit) {
		rts_av_delete_unit(enc_unit);
		enc_unit = NULL;
	}
	return NULL;
}

void destroy_stream(struct rts_av_stream *stream)
{
	struct rts_av_unit *cur;

	if (!stream)
		return;

	cur = rts_av_stream_get_item(stream, NULL);
	while (cur) {
		rts_av_stream_del(stream, cur);
		rts_av_delete_unit(cur);
		cur = rts_av_stream_get_item(stream, NULL);
	}
	rts_av_delete_stream(stream);
}

int cleanup_test_instance(struct isp_test_instance *inst)
{
	int i;

	if (!inst)
		return -RTS_FTEST_E_NULL_POINT;

	for (i = 0; i < inst->count; i++) {
		destroy_stream(inst->stream[i]);
		inst->stream[i] = NULL;
	}
	free(inst);

	rts_av_release();

	return RTS_FTEST_OK;
}

int set_isp_attr(struct rts_av_unit *isp_unit)
{
	struct rts_av_ability_t *ability;
	struct rts_isp_ability *isp_ability;
	struct rts_isp_attr isp_attr;
	int ret;
	int i;
	int index = 0;
	uint32_t w, h;
	uint32_t target_format = V4L2_PIX_FMT_NV12;

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

	for (i = 0; i < isp_ability->fmt_number; i++) {
		if (target_format == *(isp_ability->pformats + i))
			break;
	}
	if (i == isp_ability->fmt_number)
		return -RTS_FTEST_E_NOT_SUPPORT;

	if (!isp_ability->frmival_num)
		return -RTS_FTEST_E_NOT_SUPPORT;

	w = isp_ability->resolution.max_width;
	h = isp_ability->resolution.max_height;

	isp_attr.pixelformat = target_format;
	isp_attr.width = w;
	isp_attr.height = h;
	isp_attr.numerator = isp_ability->pfrmivals->numerator;
	isp_attr.denominator = isp_ability->pfrmivals->denominator;
	isp_attr.isp_buf_num = 3;
	ret = rts_av_set_attr(isp_unit, &isp_attr);

exit:
	rts_av_release_ability(ability);
	return ret;
}

int start_test_stream(struct isp_test_instance *inst)
{
	int i;
	int ret = RTS_FTEST_OK;

	if (!inst)
		return -RTS_FTEST_E_NULL_POINT;

	for (i = 0; i < inst->count; i++) {
		struct rts_av_stream *stream = inst->stream[i];
		int result = rts_av_apply(stream);
		if (result) {
			RTS_ERR("stream %d enable fail : %d\n", i, result);
			ret |= -RTS_FTEST_ERROR;
		}
		inst->frame_num[i] = 0;
	}

	if (ret) {
		for (i = 0; i < inst->count; i++)
			rts_av_cancel(inst->stream[i]);
	}

	return ret;
}

int run_test_stream(struct isp_test_instance *inst, unsigned int time)
{
	int i;
	int ret = RTS_FTEST_OK;
	struct timeval begin, end;

	if (!inst)
		return -RTS_FTEST_E_NULL_POINT;

	gettimeofday(&begin, NULL);
	gettimeofday(&end, NULL);
	while (rts_calc_timeval(begin, end) < time) {
		usleep(1000);
		gettimeofday(&end, NULL);
		for (i = 0; i < inst->count; i++) {
			struct rts_av_stream *stream = inst->stream[i];
			struct rts_av_buffer *buffer = NULL;
			if (rts_av_stream_poll(stream))
				continue;
			if (rts_av_stream_recv(stream, &buffer))
				continue;
			if (buffer) {
				rts_av_put_buffer(buffer);
				buffer = NULL;
				inst->frame_num[i]++;
			}
		}
	}
	inst->test_time = time;

	return RTS_FTEST_OK;
}

int stop_test_stream(struct isp_test_instance *inst, unsigned int coef)
{
	int i;
	int ret = RTS_FTEST_OK;

	if (!inst)
		return -RTS_FTEST_E_NULL_POINT;

	if (coef < 1000)
		coef = 1000;
	else if (coef > 3000)
		coef = 3000;

	for (i = 0; i < inst->count; i++) {
		unsigned int pass_frames;

		rts_av_cancel(inst->stream[i]);

		pass_frames = inst->test_fps[i] * inst->test_time / coef;
		RTS_INFO("stream %d got %d frames in %d ms\n",
				i, inst->frame_num[i], inst->test_time);
		if (inst->frame_num[i] == 0 ||
				inst->frame_num[i] < pass_frames) {
			ret |= -RTS_FTEST_ERROR;
			RTS_ERR("stream %d got %d frames less than %d\n",
				i, inst->frame_num[i], pass_frames);
		}
	}

	return ret;
}

