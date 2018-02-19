/*
 * Realtek Semiconductor Corp.
 *
 * ap/run_test.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <rtscamkit.h>
#include "rtsftest.h"

extern struct rts_ftest_case test_isp_preview;
extern struct rts_ftest_case test_isp_h264;
extern struct rts_ftest_case test_isp_mjpeg;
extern struct rts_ftest_case test_isp_mixture;

struct test_option {
	char *logfile;
	unsigned int burning_time;
	unsigned int frequency;
	char *testcase;
};

void print_success(RtsFtestEnv tester)
{
	RTS_INFO("######      #     #####   #####\n");
	RTS_INFO("#     #    # #   #     # #     #\n");
	RTS_INFO("#     #   #   #  #       #      \n");
	RTS_INFO("######   #     #  #####   #####\n");
	RTS_INFO("#        #######       #       #\n");
	RTS_INFO("#        #     # #     # #     #\n");
	RTS_INFO("#        #     #  #####   #####\n");
}

void print_fail(RtsFtestEnv tester)
{
	RTS_ERR("#######    #    ###    #      \n");
	RTS_ERR("#         # #    #     #      \n");
	RTS_ERR("#        #   #   #     #      \n");
	RTS_ERR("#####   #     #  #     #      \n");
	RTS_ERR("#       #######  #     #      \n");
	RTS_ERR("#       #     #  #     #      \n");
	RTS_ERR("#       #     # ###    #######\n");
}

int parse_option(int argc, char *argv[], struct test_option *option)
{
	int ret = RTS_FTEST_OK;
	int c;
	struct option longopts[] = {
		{"log", required_argument, NULL, 'l'},
		{"time", required_argument, NULL, 't'},
		{"testcase", required_argument, NULL, 'c'},
		{"frequency", required_argument, NULL, 'f'},
		{0,0,0,0}
	};


	option->logfile = NULL;
	option->burning_time = 0;
	option->frequency = 0;
	option->testcase = NULL;

	while (1) {
		c = getopt_long(argc, argv, ":p:l:t:c:f:", longopts, NULL);
		if (c < 0)
			break;
		switch (c) {
		case 'l':
			option->logfile = optarg;
			break;
		case 't':
			option->burning_time =
				(unsigned int)strtol(optarg, NULL, 0);
			break;
		case 'c':
			option->testcase = optarg;
			break;
		case 'f':
			option->frequency =
				(unsigned int)strtol(optarg, NULL, 0);
			break;
		case ':':
			fprintf(stdout, "required argument : %s\n", argv[optind-1]);
			return -RTS_FTEST_ERROR;
		case '?':
			fprintf(stdout, "required argument : %s\n", argv[optind-1]);
			return -RTS_FTEST_ERROR;
		default:
			break;
		}
	}

	return ret;
}

int main(int argc, char *argv[])
{
	RtsFtestEnv tester;
	struct rts_ftest_case *testcase;
	struct test_option option;
	int  ret = RTS_FTEST_OK;
	int i;
	struct rts_ftest_case *testcases[] = {
		&test_isp_preview,
		&test_isp_h264,
		&test_isp_mjpeg,
		&test_isp_mixture,
		NULL
	};

	rts_set_log_ident("testvideo");
	rts_set_log_mask(RTS_LOG_MASK_CONS);

	ret = parse_option(argc, argv, &option);
	if (ret) {
		RTS_ERR("parse argument fail\n");
		return ret;
	}

	if (option.logfile) {
		rts_set_log_mask(RTS_LOG_MASK_CONS | RTS_LOG_MASK_FILE);
		rts_set_log_file(option.logfile);
	}

	ret = rts_ftest_initialize_env(&tester);
	if (ret)
		return ret;

	i = 0;
	while ((testcase = testcases[i++])) {
		rts_ftest_add_test(tester, testcase);
		if (option.burning_time)
			rts_ftest_set_burning_time(tester, testcase->name,
					option.burning_time);
		if (option.frequency)
			rts_ftest_set_frequence(tester, testcase->name,
					option.frequency);
	}

	RTS_INFO("---->run video testcases\n");

	if (option.testcase)
		ret = rts_ftest_run_test(tester, option.testcase);
	else
		ret = rts_ftest_run_tests(tester);

	RTS_INFO("---->finist video testcases\n");

	if (ret)
		print_fail(tester);
	else
		print_success(tester);

	rts_ftest_cleanup_env(tester);

	return ret;
}

