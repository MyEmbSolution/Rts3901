/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_fake.h
 *
 * Copyright (C) 2014      Peter Sun<peter_sun@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_I2C_H
#define _TESTS_FT2_TEST_I2C_H

int ft2_i2c_check(struct protocol_command *pcmd);
int ft2_i2c_runonce(void *priv, struct protocol_command *pcmd, char *path);

#endif
