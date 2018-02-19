/*
 * Realtek Semiconductor Corp.
 *
 * tests/main.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "ft2log.h"
#include "ft2errno.h"
#include "tester.h"
#include "ft2_tests.h"
#include "ft2_test_eth.h"


static int g_mainexit = 0;
static void Termination(int sign) {
	FT2_LOG_INFO("signal %d\n", sign);
	g_mainexit = 1;
}

int main(int argc, char *argv[])
{
	FT2TesterInst inst;
	int ret = FT2_OK;
	int opt;
	struct ft2_opt_params ft2_params = {
		.ipcam_addr = "",
		.host_addr = "",
		.mnt_filename = "NFSShare",
		.fs_type = "cifs",
	};


	while ((opt = getopt(argc, argv, "d:h:f:s:l")) != EOF) {
		switch (opt) {
		case 'd':
			strcpy(ft2_params.ipcam_addr, optarg);
			break;
		case 'h':
			strcpy(ft2_params.host_addr, optarg);
			break;
		case 'l':
			test_forever = 1;
			break;
		case 'f':
			strcpy(ft2_params.mnt_filename, optarg);
			break;
		case 's':
			strcpy(ft2_params.fs_type, optarg);
			break;
		default:
usage:
			FT2_LOG_INFO("\n"
				"usage: ft2_test [options]\n"
				"options:(must specify -d and -h)\n"
				"-d xx.xx.xx.xx -> set ipcam addr\n"
				"-h xx.xx.xx.xx -> set host addr\n"
				"-f xx		-> set mount filename\n"
				"-s xx		-> set mount filesystem type\n"
				"-l             -> loop test\n");
			return -1;
		}
	}

	if (optind != argc)
		goto usage;

	if (ft2_params.ipcam_addr[0] == '\0'
		|| ft2_params.host_addr[0] == '\0')
		goto usage;

	g_mainexit = 0;
	signal(SIGINT, Termination);
	signal(SIGTERM, Termination);

	ret = net_init(&ft2_params);
	if (ret) {
		FT2_LOG_ERR("net init fail\n");
		return ret;
	}

	ret = ft2_new_tester(&inst);
	if (ret) {
		FT2_LOG_ERR("create tester fail\n");
		goto exit0;
	}

	ft2_set_root_path(inst, "/mnt");

	ret = add_test_items(inst);
	if (ret) {
		FT2_LOG_ERR("register test items fail\n");
		goto exit1;
	}

	ret = ft2_start_run_tests(inst);
	if (ret) {
		FT2_LOG_INFO("start to run tests fail\n");
		goto exit1;
	}

	while (!g_mainexit)
		usleep(1000);

exit1:
	pthread_mutex_lock(&inst->mutex_test);
	inst->stop_all_item = 1;
	pthread_mutex_unlock(&inst->mutex_test);

	ft2_stop_run_tests(inst);
	ft2_delete_tester(inst);
exit0:
	net_exit();
	FT2_LOG_INFO("goodbye, ft2 test\n");
	return 0;
}
