/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_h264.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_H264_H
#define _TESTS_FT2_TEST_H264_H

int ft2_h264_check(struct protocol_command *pcmd);
int ft2_h264_runonce(void *priv, struct protocol_command *pcmd, char *path);
void ft2_h264_cleanup(void *priv);

#endif
