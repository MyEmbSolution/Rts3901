/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_fake.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"

int ft2_fake_check(struct protocol_command *pcmd)
{
	struct protocol_param *parameter = NULL;
	int arg = 0;

	if (!pcmd)
		return FT2_NULL_POINT;

	parameter = pcmd->parameter;

	if (!parameter) {
		FT2_LOG_ERR("test [%s] need parameter\n", pcmd->name);
		return -FT2_PROTOCOL_ERROR;
	}

	arg = (int)atol(parameter->content);
	if (arg > 2 || arg < 1) {
		FT2_LOG_ERR("test [%s]'s parameter (%d) is invalid\n",
				pcmd->name, arg);
		return -FT2_INVALID_ARGUMENT;
	}

	return FT2_OK;
}

int ft2_fake_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	struct protocol_param *parameter = NULL;
	int arg = 0;

	if (!pcmd)
		return -FT2_NULL_POINT;

	parameter = pcmd->parameter;

	if (!parameter) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return FT2_OK;
	}

	arg = (int)atol(parameter->content);
	if (arg == 1)
		append_pt_command_result(pcmd,
				FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	else
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));

	return FT2_OK;
}
