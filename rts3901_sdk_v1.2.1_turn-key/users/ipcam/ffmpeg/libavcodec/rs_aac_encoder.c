/*
 * author: yafei_meng@realsil.com.cn
 * @rtsaac_encoder.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avcodec.h"
#include "internal.h"
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <AACEncoder_API.h>
#include <AACEncoder_Parameters.h>
#include "libavutil/audio_fifo.h"

#define RS_AAC_ENCODE_RATE	16000
#define RS_AAC_ENCODE_BITRATE	16000

typedef struct RsAudioEncContext {
	void *pAACAPI;
	AVAudioFifo *datafifo;
	uint16_t *StaticBuf;
} RsAudioEncContext;

static av_cold int rtsaac_encode_close(AVCodecContext *avctx)
{
	RsAudioEncContext *s = avctx->priv_data;

	if (s->pAACAPI) {
		AACEncoder_API_free(s->pAACAPI);
		s->pAACAPI = NULL;
	}

	if (s->datafifo) {
		av_audio_fifo_free(s->datafifo);
		s->datafifo = NULL;
	}

	if (s->StaticBuf) {
		free(s->StaticBuf);
		s->StaticBuf = NULL;
	}

	return 0;
}

static av_cold int rtsaac_encode_init(AVCodecContext *avctx)
{
	int ret = 0;
	AacEncoder_Para0 Para0;

	RsAudioEncContext *s = avctx->priv_data;

	if (avctx->channels != 2) {
		av_log(avctx, AV_LOG_ERROR,
			"Only Stereo tracks are allowed.\n");
		return AVERROR_INVALIDDATA;
	}

	if (avctx->sample_rate != RS_AAC_ENCODE_RATE) {
		av_log(avctx, AV_LOG_ERROR,
			"Only %dHz sample rate supported\n",
			RS_AAC_ENCODE_RATE);
		return AVERROR(ENOSYS);
	}

	if (avctx->bit_rate != RS_AAC_ENCODE_BITRATE) {
		av_log(avctx, AV_LOG_ERROR,
			"Only %dHz bitrate supported\n",
			RS_AAC_ENCODE_BITRATE);
		return AVERROR(ENOSYS);
	}

	s->pAACAPI = AACEncoder_API_context_Create();
	if (!s->pAACAPI)
		return AVERROR(ENOMEM);

	Para0.m_SampleRate = avctx->sample_rate;
	Para0.m_BitRate = avctx->bit_rate;

	AACEncoder_API_Set(s->pAACAPI, &Para0, 4, 0);

	avctx->frame_size = AACEncoder_API_GetBlockLen(s->pAACAPI) >> 2;

	s->datafifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, 2, 256);
	if (!s->datafifo) {
		AACEncoder_API_free(s->pAACAPI);
		s->pAACAPI = NULL;
		return AVERROR(ENOMEM);
	}

	s->StaticBuf = (uint16_t *)calloc(1, avctx->frame_size << 2);
	if (!s->StaticBuf) {
		AACEncoder_API_free(s->pAACAPI);
		s->pAACAPI = NULL;
		av_audio_fifo_free(s->datafifo);
		s->datafifo = NULL;
		return AVERROR(ENOMEM);
	}

	return 0;
}

static av_cold int rtsaac_encode_frame(AVCodecContext *avctx, AVPacket *avpkt,
		const AVFrame *frame, int *got_packet_ptr)
{
	RsAudioEncContext *s = avctx->priv_data;
	short *samples = (short *)s->StaticBuf;
	int size, ret, enc_size;

	if (frame) {
		ret = av_audio_fifo_write(s->datafifo,
			(void **)frame->extended_data,
			frame->nb_samples);
		if (ret < 0) {
			av_log(avctx, AV_LOG_ERROR, "fail to write data fifo\n");
			return -1;
		}
	}

	ret = ff_alloc_packet2(avctx, avpkt, avctx->frame_size << 2);
	if (ret < 0)
		return ret;

	size = av_audio_fifo_size(s->datafifo);
	if (size >= avctx->frame_size) {
		av_audio_fifo_read(s->datafifo,
				(void **)&s->StaticBuf,
				avctx->frame_size);
		enc_size = AACEncoder_API_Process(s->pAACAPI,
				samples, avpkt->data, avctx->frame_size);
		if (enc_size > 0) {
			avpkt->size = enc_size;
			*got_packet_ptr = 1;
		} else {
			*got_packet_ptr = 0;
		}
	} else {
		*got_packet_ptr = 0;
	}

	return 0;
}

AVCodec ff_rtsaac_encoder = {
	.name           = "rtsaac",
	.long_name      = "RS_ADUIO_ENCODER",
	.type           = AVMEDIA_TYPE_AUDIO,
	.id             = AV_CODEC_ID_AAC,
	.priv_data_size = sizeof(RsAudioEncContext),
	.init           = rtsaac_encode_init,
	.close          = rtsaac_encode_close,
	.encode2        = rtsaac_encode_frame,
	.capabilities   = CODEC_CAP_VARIABLE_FRAME_SIZE,
	.sample_fmts    = (const enum AVSampleFormat[]){ AV_SAMPLE_FMT_S16,
							AV_SAMPLE_FMT_NONE },
};
