/*
 * Realtek Semiconductor Corp.
 *
 * utils/rts_isp_cmd/dbg_isp_cmd.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

int rts_isp_exec_isp_cmd(int fd, uint16_t cmdcode, uint8_t index,
		uint8_t len, uint16_t param, uint16_t addr, uint8_t *data);

void dump_data(uint8_t *data, unsigned int len)
{
	int i;

	if (!data || !len)
		return;

	for (i = 0; i < len; i++) {
		fprintf(stdout, "0x%02x", data[i]);
		if (i == len - 1 || i % 16 == 15)
			fprintf(stdout, "\n");
		else
			fprintf(stdout, " ");
	}
}

int main(int argc, char *argv[])
{
	uint16_t cmdcode;
	uint8_t index;
	uint8_t len;
	uint16_t param;
	uint16_t addr;
	uint8_t *pdata = NULL;
	int fd = -1;
	int i;
	int ret;

	if (argc < 6) {
		fprintf(stdout, "rts_isp_cmd <cmdcode> <index> <len> <param> <addr> <data...>\n");
		return 0;
	}
	cmdcode = (uint16_t)strtol(argv[1], NULL, 0);
	index  = (uint8_t)strtol(argv[2], NULL, 0);
	len = (uint8_t)strtol(argv[3], NULL, 0);
	param = (uint16_t)strtol(argv[4], NULL, 0);
	addr = (uint16_t)strtol(argv[5], NULL, 0);

	fprintf(stdout, "cmdcode = 0x%04x ", cmdcode);
	fprintf(stdout, "index = 0x%02x ", index);
	fprintf(stdout, "len = 0x%02x ", len);
	fprintf(stdout, "param = 0x%04x ", param);
	fprintf(stdout, "addr = 0x%04x\n", addr);

	if (len > 0) {
		pdata = (uint8_t *)calloc(len, sizeof(uint8_t));
		if (!pdata) {
			fprintf(stderr, "there is no memory for data\n");
			return -1;
		}
		if ((cmdcode & 0x80) == 0) {
			if (argc < 6 + len) {
				fprintf(stderr, "please input data\n",
					len);
				ret = -2;
				goto exit;
			}
			for (i = 0; i < len; i++)
				pdata[i] = (uint8_t)strtol(argv[6 + i], NULL, 0);
		}
	}

	fd = open("/dev/video51", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open isp device fail\n");
		goto exit;
	}

	if (((cmdcode & 0x80) == 0) && (len > 0)) {
		fprintf(stdout, "set data : \n");
		dump_data(pdata, len);
	}

	ret = rts_isp_exec_isp_cmd(fd, cmdcode, index, len, param, addr, pdata);
	if (ret < 0) {
		fprintf(stderr, "exec isp cmd fail, ret = %d\n", ret);
		goto exit;
	}

	if (((cmdcode & 0x80) == 0x80) && (len > 0)) {
		fprintf(stdout, "get data : \n");
		dump_data(pdata, len);
	}

exit:
	if (fd > 0)
		close(fd);
	fd = -1;
	if (pdata)
		free(pdata);
	pdata = NULL;
	return 0;
}
