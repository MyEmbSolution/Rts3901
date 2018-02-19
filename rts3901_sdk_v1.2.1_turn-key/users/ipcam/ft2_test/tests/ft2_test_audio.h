/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_audio.h
 *
 * Copyright (C) 2014      Wind Han<wind_han@realsil.com.cn>
 */
#ifndef _TESTS_FT2_TEST_AUDIO_H
#define _TESTS_FT2_TEST_AUDIO_H
int ft2_audio_check(struct protocol_command *pcmd);
int ft2_audio_runonce(void *priv, struct protocol_command *pcmd, char *path);
void ft2_audio_cleanup(void *priv);

#endif
