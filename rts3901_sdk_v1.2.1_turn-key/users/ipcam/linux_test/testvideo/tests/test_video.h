/*
 * Realtek Semiconductor Corp.
 *
 * tests/test_isp.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _TESTS_TEST_ISP_H
#define _TESTS_TEST_ISP_H
#include "stdint.h"

#define MAX_ISP_STREAM_NUM		5

struct test_resolution {
	uint32_t width;
	uint32_t height;
};

struct isp_test_instance {
	struct rts_av_stream *stream[MAX_ISP_STREAM_NUM];
	unsigned int count;
	unsigned int seq;
	unsigned int frame_num[MAX_ISP_STREAM_NUM];
	uint32_t test_format[MAX_ISP_STREAM_NUM];
	struct test_resolution test_resolution[MAX_ISP_STREAM_NUM];
	unsigned int test_fps[MAX_ISP_STREAM_NUM];
	unsigned int test_time;
	RtsFtestEnv tester;
};

struct rts_av_stream *create_stream(int idx, unsigned int encid);
void destroy_stream(struct rts_av_stream *stream);
int cleanup_test_instance(struct isp_test_instance *inst);
int set_isp_attr(struct rts_av_unit *isp_unit);

int start_test_stream(struct isp_test_instance *inst);
int run_test_stream(struct isp_test_instance *inst, unsigned int time);
int stop_test_stream(struct isp_test_instance *inst, unsigned int coef);

#endif
