/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_uart1.h
 *
 * Copyright (C) 2014      Peter Sun<peter_sun@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_UART02_H
#define _TESTS_FT2_TEST_UART02_H

int ft2_uart02_check(struct protocol_command *pcmd);
int ft2_uart02_runonce(void *priv, struct protocol_command *pcmd, char *path);

#endif
