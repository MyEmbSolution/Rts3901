/*
 * Realtek Semiconductor Corp.
 *
 * include/rtsaudio.h
 *
 * Copyright (C) 2016      Wind Han<wind_han@realsil.com.cn>
 */
#ifndef _INCLUDE_RTSAUDIO_H
#define _INCLUDE_RTSAUDIO_H

#ifdef __cplusplus
extern "C"
{
#endif

enum enum_rts_audio_type_id {
	RTS_AUDIO_TYPE_ID_MP3 = 1,
	RTS_AUDIO_TYPE_ID_ULAW,
	RTS_AUDIO_TYPE_ID_ALAW,
	RTS_AUDIO_TYPE_ID_PCM,
	RTS_AUDIO_TYPE_ID_G726,
	RTS_AUDIO_TYPE_ID_AMRNB,
	RTS_AUDIO_TYPE_ID_AAC,
};

enum enum_rts_audio_state_id {
	RTS_AUDIO_STATE_ID_START = 1,
	RTS_AUDIO_STATE_ID_STOP,
};

enum enmu_rts_audio_flag_id {
	RTS_AUDIO_FLAG_ID_STREAM = (0x1 << 0),
};

struct rts_amixer_control {
	uint32_t index;
	uint32_t type;
	uint32_t state;
	uint32_t flags;

	uint32_t rate;
	uint32_t format;
	uint32_t channels;
};

int rts_audio_set_mixer_ctrl(struct rts_av_stream *stream,
		struct rts_amixer_control *pctrl);

struct rts_aec_control {
	uint32_t aec_enable;
	uint32_t ns_enable;
	uint32_t ns_level;
};

int rts_audio_set_aec_ctrl(struct rts_av_stream *stream,
		struct rts_aec_control *pctrl);

struct rts_encode_control {
	uint32_t type;

	uint32_t rate;
	uint32_t format;
	uint32_t channels;
	uint32_t bitrate;
};

int rts_audio_set_encode_ctrl(struct rts_av_stream *stream,
		struct rts_encode_control *pctrl);

#ifdef __cplusplus
}
#endif
#endif
