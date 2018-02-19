/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_rtc.h
 *
 * Copyright (C) 2014      Peter Sun<peter_sun@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_RTC_H
#define _TESTS_FT2_TEST_RTC_H

int ft2_rtc_check(struct protocol_command *pcmd);
int ft2_rtc_runonce(void *priv, struct protocol_command *pcmd, char *path);

#endif
