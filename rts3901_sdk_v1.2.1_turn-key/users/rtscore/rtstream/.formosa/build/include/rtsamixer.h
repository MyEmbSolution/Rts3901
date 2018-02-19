/*
 * Realtek Semiconductor Corp.
 *
 * libs/include/rtsamixer.h
 *
 * Copyright (C) 2014      Wind Han<wind_han@realsil.com.cn>
 */
#ifndef _LIBS_INCLUDE_RTSAMIXER_H
#define _LIBS_INCLUDE_RTSAMIXER_H

int rts_audio_get_playback_volume(int *per_vol);
int rts_audio_set_playback_volume(int per_vol);
int rts_audio_get_capture_volume(int *per_vol);
int rts_audio_set_capture_volume(int per_vol);
int rts_audio_playback_mute();
int rts_audio_playback_unmute();
int rts_audio_capture_mute();
int rts_audio_capture_unmute();

#endif
