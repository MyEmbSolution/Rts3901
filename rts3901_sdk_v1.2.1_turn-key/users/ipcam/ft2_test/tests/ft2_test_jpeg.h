/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_jpeg.h
 *
 * Copyright (C) 2015      Lei Wang<lei_wang@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_JPEG_H
#define _TESTS_FT2_TEST_JPEG_H

int ft2_jpeg_check(struct protocol_command *pcmd);
int ft2_jpeg_runonce(void *priv, struct protocol_command *pcmd, char *path);
void ft2_jpeg_cleanup(void *priv);

#endif
