
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <rtscamkit.h>
#include <rtsavapi.h>
#include <alsa/asoundlib.h>
#include "libavutil/time.h"
#include "libavformat/internal.h"
#include "avdevice.h"
#include "alsa-audio.h"
#include "libavutil/opt.h"
#include "avdevice.h"

#define AUDIO_MAX_BUF_NUM                   10

struct audio_data {
	AVClass *class;
	unsigned int format;
	unsigned int channels;
	unsigned int rate;
	unsigned long period_frames;
	unsigned long buffer_frames;
	int frame_size;  ///< bytes per sample * channels

	RtStream stream;
	struct rts_audio_stream_cfg cfg;
	struct rts_av_buffer *aplay_buffer[AUDIO_MAX_BUF_NUM];
};

static int audio_write_close(AVFormatContext *s1)
{
	struct audio_data *s = s1->priv_data;
	int i;

	if (s->stream)
		rts_destroy_stream(s->stream);
	s->stream = NULL;

	for (i = 0; i < AUDIO_MAX_BUF_NUM; i++) {
		if (s->aplay_buffer[i]) {
			rts_av_delete_buffer(s->aplay_buffer[i]);
			s->aplay_buffer[i] = NULL;
		}
	}

	rts_av_release();

	return 0;
}

static av_cold int audio_write_header(AVFormatContext *s1)
{
	struct audio_data *s = s1->priv_data;
	AVStream *st;
	int ret;
	enum AVCodecID codec_id;
	unsigned int chunk_bytes;
	int i;
	uint64_t layout = s1->streams[0]->codec->channel_layout;

	ret = rts_av_init();
	if (ret)
		return AVERROR(ENOMEM);

	st = avformat_new_stream(s1, NULL);
	if (!st) {
		av_log(s1, AV_LOG_ERROR, "Cannot add stream\n");

		return AVERROR(ENOMEM);
	}
	codec_id = st->codec->codec_id;
	st = s1->streams[0];

	snprintf(s->cfg.dev_node, sizeof(s->cfg.dev_node), s1->filename);
	s->cfg.rate = st->codec->sample_rate;
	s->cfg.format = 16;
	s->cfg.channels = st->codec->channels;

	s->stream = rts_create_audio_playback_stream(&s->cfg);
	if (!s->stream) {
		fprintf(stderr, "cann't create stream\n");
		ret = AVERROR(ENOMEM);
		goto exit;
	}

	ret = rts_av_apply(s->stream);
	if (ret) {
		fprintf(stderr, "rts_av_apply fail, ret = %d\n", ret);
		goto exit;
	}

	chunk_bytes = s->cfg.rate / 32 * s->cfg.channels * 16 / 8;

	for (i = 0; i < AUDIO_MAX_BUF_NUM; i++) {
		s->aplay_buffer[i] = rts_av_new_buffer(chunk_bytes);
		if (!s->aplay_buffer[i]) {
			fprintf(stderr, "no memory for audio buffer\n");
			goto exit;
		}
	}

	s->frame_size = av_get_bits_per_sample(codec_id) / 8 * s->cfg.channels;

	avpriv_set_pts_info(st, 64, 1, s->cfg.rate);

	return 0;

exit:
	if (s->stream)
		rts_destroy_stream(s->stream);
	s->stream = NULL;

	for (i = 0; i < AUDIO_MAX_BUF_NUM; i++) {
		if (s->aplay_buffer[i]) {
			rts_av_delete_buffer(s->aplay_buffer[i]);
			s->aplay_buffer[i] = NULL;
		}
	}

	rts_av_release();

	return ret;
}

static int audio_write_packet(AVFormatContext *s1, AVPacket *pkt)
{
    struct audio_data *s = s1->priv_data;
    int res;
    int size     = pkt->size;
    uint8_t *buf = pkt->data;
    struct rts_av_buffer *buffer;
    int i;

    for (i = 0; i < AUDIO_MAX_BUF_NUM; i++) {
	    buffer = s->aplay_buffer[i];
	    if (rts_av_get_buffer_refs(buffer) == 0) {
		    buffer->bytesused = 0;
		    buffer->priv = NULL;
		    break;
	    }
	    buffer = NULL;
    }
    
    if (!buffer) {
	    usleep(2000);
	    return AVERROR(EAGAIN);
    }
    
    memcpy(buffer->vm_addr, buf, pkt->size);
    buffer->bytesused = pkt->size;
    buffer->timestamp = 0;
    rts_av_stream_send(s->stream, buffer);
    buffer = NULL;

    return 0;
}

AVOutputFormat ff_rtstream_audio_muxer = {
    .name           = "rtstream audio",
    .long_name      = NULL_IF_CONFIG_SMALL("RTSTREAM audio output"),
    .priv_data_size = sizeof(struct audio_data),
    .audio_codec    = DEFAULT_CODEC_ID,
    .video_codec    = AV_CODEC_ID_NONE,
    .write_header   = audio_write_header,
    .write_packet   = audio_write_packet,
    .write_trailer  = audio_write_close,
    .flags          = AVFMT_NOFILE,
};

