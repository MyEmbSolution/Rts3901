/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_stop.h
 *
 * Copyright (C) 2015      Lei Wang<lei_wang@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_STOP_H
#define _TESTS_FT2_TEST_STOP_H

int ft2_stop_check(struct protocol_command *pcmd);
int ft2_stop_runonce(void *priv, struct protocol_command *pcmd, char *path);

#endif
