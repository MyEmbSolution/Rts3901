
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

struct audio_data {
	AVClass *class;
	int sample_rate; ///< sample rate set by user
	int channels;    ///< number of channels set by user
	RtStream stream;
	struct rts_audio_stream_cfg cfg;
};

static int audio_read_close(AVFormatContext *s1)
{
	struct audio_data *s = s1->priv_data;

	rts_av_cancel(s->stream);

	if (s->stream)
		rts_destroy_stream(s->stream);
	s->stream = NULL;

	rts_av_release();

	return 0;
}

static av_cold int audio_read_header(AVFormatContext *s1)
{
	struct audio_data *s = s1->priv_data;
	AVStream *st;
	int ret;
	enum AVCodecID codec_id;

	ret = rts_av_init();
	if (ret)
		return AVERROR(ENOMEM);

	st = avformat_new_stream(s1, NULL);
	if (!st) {
		av_log(s1, AV_LOG_ERROR, "Cannot add stream\n");

		return AVERROR(ENOMEM);
	}
	codec_id    = s1->audio_codec_id;

	s->stream = NULL;
	snprintf(s->cfg.dev_node, sizeof(s->cfg.dev_node), s1->filename);
	s->cfg.rate = s->sample_rate;
	s->cfg.format = 16;
	s->cfg.channels = s->channels;

	s->stream = rts_create_audio_capture_stream(&s->cfg);
	if (!s->stream) {
		fprintf(stderr, "cann't create stream\n");
		ret = AVERROR(EINVAL);
		goto exit;
	}

	ret = rts_av_apply(s->stream);
	if (ret) {
		fprintf(stderr, "rts_av_apply fail, ret = %d\n", ret);
		goto exit;
	}

	/* take real parameters */
	st->codec->codec_type  = AVMEDIA_TYPE_AUDIO;
	st->codec->codec_id    = codec_id;
	st->codec->sample_rate = s->cfg.rate;
	st->codec->channels    = s->cfg.channels;

	avpriv_set_pts_info(st, 64, 1, 1000000);  /* 64 bits pts in us */

	return 0;

exit:
	if (s->stream)
		rts_destroy_stream(s->stream);
	s->stream = NULL;

	rts_av_release();

	return ret;
}

static void rtstream_buffer_free(void *opa, uint8_t *data)
{
	struct rts_av_buffer *buffer = opa;

	rts_av_put_buffer(buffer);
}

static int audio_read_packet(AVFormatContext *s1, AVPacket *pkt)
{
	struct audio_data *s = s1->priv_data;
	struct rts_av_buffer *buffer = NULL;
	int  ret = 0;

	av_init_packet(pkt);

	if (rts_av_stream_poll(s->stream))
		return AVERROR(EAGAIN);

	ret = rts_av_stream_recv(s->stream, &buffer);
	if (ret == 0)
		pkt->pts = buffer->timestamp;
	else
		return ret;

	pkt->data = buffer->vm_addr;
	pkt->size = buffer->bytesused;
	pkt->dts = pkt->pts;

	pkt->buf = av_buffer_create(pkt->data, pkt->size,
					rtstream_buffer_free, buffer, 0);
	if (!pkt->buf) {
		pkt->data = NULL;
		rts_av_put_buffer(buffer);
		return AVERROR(ENOMEM);
	}

	return pkt->size;
}

static const AVOption options[] = {
    { "sample_rate", "", offsetof(struct audio_data, sample_rate), AV_OPT_TYPE_INT, {.i64 = 48000}, 1, INT_MAX, AV_OPT_FLAG_DECODING_PARAM },
    { "channels",    "", offsetof(struct audio_data, channels),    AV_OPT_TYPE_INT, {.i64 = 2},     1, INT_MAX, AV_OPT_FLAG_DECODING_PARAM },
    { NULL },
};

static const AVClass rtstream_audio_demuxer_class = {
    .class_name     = "Rtstream audio demuxer",
    .item_name      = av_default_item_name,
    .option         = options,
    .version        = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_rtstream_audio_demuxer = {
    .name           = "rtstream audio",
    .long_name      = NULL_IF_CONFIG_SMALL("RTSTREAM audio input"),
    .priv_data_size = sizeof(struct audio_data),
    .read_header    = audio_read_header,
    .read_packet    = audio_read_packet,
    .read_close     = audio_read_close,
    .flags          = AVFMT_NOFILE,
    .priv_class     = &rtstream_audio_demuxer_class,
};

