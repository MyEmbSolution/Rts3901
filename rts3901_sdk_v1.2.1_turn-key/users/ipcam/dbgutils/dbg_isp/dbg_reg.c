/*
 * Realtek Semiconductor Corp.
 *
 * dbg_reg.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <rtsisp.h>

int main(int argc, char *argv[])
{
	RtsRegInst reginst;
	unsigned int addr;
	uint32_t value;
	int ret;

	if (argc < 3) {
		printf("dbg_isp <isp/h264/mjpeg> <reg>\n");
		return -1;
	}

	if (0 == strcasecmp("isp", argv[1]))
		ret = rts_reg_isp_open(&reginst);
	else if (0 == strcasecmp("h264", argv[1]))
		ret = rts_reg_h264_open(&reginst);
	else if (0 == strcasecmp("mjpeg", argv[1]))
		ret = rts_reg_mjpg_open(&reginst);
	else {
		printf("valide argument : dbg_isp <isp/h264/mjpeg> <reg>\n");
		return -1;
	}

	if (ret) {
		printf("open reg fail\n");
		return ret;
	}

	addr = (unsigned int)strtol(argv[2], NULL, 0);

	printf("[%08x] = 0x%08x\n", addr, rts_reg_read_reg(reginst, addr));

	rts_reg_close(reginst);

	return 0;
}

