/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_aes.h
 *
 * Copyright (C) 2014      Wind Han<wind_han@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_AES_H
#define _TESTS_FT2_TEST_AES_H
int ft2_aes_check(struct protocol_command *pcmd);
int ft2_aes_runonce(void *priv, struct protocol_command *pcmd, char *path);
#endif
