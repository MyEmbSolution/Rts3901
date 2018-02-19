/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_fake.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_FAKE_H
#define _TESTS_FT2_TEST_FAKE_H

int ft2_fake_check(struct protocol_command *pcmd);
int ft2_fake_runonce(void *priv, struct protocol_command *pcmd, char *path);

#endif
