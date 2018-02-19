/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_isp.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_ISP_H
#define _TESTS_FT2_TEST_ISP_H

int ft2_isp_check(struct protocol_command *pcmd);
int ft2_isp_runonce(void *priv, struct protocol_command *pcmd, char *path);
void ft2_isp_cleanup(void *priv);

#endif
