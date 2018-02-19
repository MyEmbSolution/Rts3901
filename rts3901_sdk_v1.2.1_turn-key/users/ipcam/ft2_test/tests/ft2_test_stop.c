/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_stop.c
 *
 * Copyright (C) 2015      Lei Wang<lei_wang@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ft2errno.h"
#include "ft2log.h"
#include "protocol.h"

int ft2_stop_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return FT2_NULL_POINT;

	return FT2_OK;
}

int ft2_stop_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	if (!pcmd)
		return -FT2_NULL_POINT;
/*
	stop_all_item = 1;
	FT2_LOG_INFO("pc want to stop all test item\n");
*/
	return FT2_OK;
}
