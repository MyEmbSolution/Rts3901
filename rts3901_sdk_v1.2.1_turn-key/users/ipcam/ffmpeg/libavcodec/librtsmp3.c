#include "avcodec.h"
#include "internal.h"
#include <rtsmp3.h>

#if CONFIG_LIBRTSMP3

typedef struct RTSMP3DecContext {
	void *pMp3Handle;
	void *pSrcBuf;
	void *pDstBuf;
	int srclen;
	int dstlen;
	int consumed;
	int gotptr;
	int initparam;
	int id3;
	int id3_len;
	int state;
} RTSMP3DecContext;

static av_cold int rts_mp3_decode_init(AVCodecContext *avctx)
{
	RTSMP3DecContext *s = avctx->priv_data;

	s->pMp3Handle = MP3Decoder_API_context_Create();
	if (!s->pMp3Handle) {
		av_log(avctx, AV_LOG_ERROR, "fail to create mp3 handle\n");
		return -1;
	}

	s->srclen = MP3Decoder_API_Get_Min_InputLen();
	s->pSrcBuf = (void *)malloc(s->srclen * sizeof(char));
	s->dstlen = MP3Decoder_API_Get_Min_OutputLen();
	s->pDstBuf = (void *)malloc(s->dstlen * sizeof(short));
	s->consumed = s->srclen;
	s->gotptr = 0;
	s->initparam = 0;
	s->id3 = 1;
	s->id3_len = 0;
	s->state = 0;

	avctx->channel_layout = av_get_default_channel_layout(avctx->channels);

	return 0;
}

static int rts_mp3_decode_frame(AVCodecContext *avctx, void *data,
			int *got_frame_ptr, AVPacket *avpkt)
{
	RTSMP3DecContext *s = avctx->priv_data;
	AVFrame *frame = data;
	int frame_size = 0;
	uint8_t *buf = avpkt->data;
	int buf_size = avpkt->size;
	int tmp, ret, tmp1;

	if ((s->consumed - s->gotptr) <= buf_size) {
		tmp = s->consumed - s->gotptr;
		memcpy((char *)s->pSrcBuf + s->gotptr, buf, tmp);
		s->gotptr += tmp;
		buf_size = tmp;

		if (s->id3 != 0) {
			if (s->state == 0) {
				s->id3_len = MP3Decoder_API_ID3Header_Check(
							s->pMp3Handle,
							s->pSrcBuf,
							s->consumed);
				s->id3 = MP3Decoder_API_Get_ID3(s->pMp3Handle);
				if (s->id3 == 0) {
					if (s->id3_len) {
						memcpy((char *)s->pSrcBuf,
							(char *)s->pSrcBuf +
							s->id3_len,
							s->consumed -
							s->id3_len);
						s->gotptr -= s->id3_len;
						*got_frame_ptr = 0;
						return buf_size;
					}
				} else if (s->id3 == 1) {
					s->state = 1;
				} else {
					s->gotptr = 0;
					*got_frame_ptr = 0;
					return buf_size;
				}
			}

			if (s->state == 1) {
				if (s->id3_len >= s->consumed) {
					s->gotptr = 0;
					s->id3_len -= s->consumed;
					*got_frame_ptr = 0;
					return buf_size;
				}

				memcpy((char *)s->pSrcBuf,
					(char *)s->pSrcBuf + s->id3_len,
					s->consumed - s->id3_len);
				s->gotptr -= s->id3_len;
				s->state = 0;
				*got_frame_ptr = 0;
				return buf_size;
			}
		}

		tmp = MP3Decoder_API_Process(s->pMp3Handle,
					s->pSrcBuf,
					s->pDstBuf,
					s->consumed);
		if (tmp == 0) {
			*got_frame_ptr = 0;
			return buf_size;
		}

		if (0 == s->initparam) {
			s->initparam = 1;
			avctx->sample_rate =
				MP3Decoder_API_Get_SampleRate(s->pMp3Handle);
			avctx->channels =
				MP3Decoder_API_Get_Channels(s->pMp3Handle);
			tmp1 = MP3Decoder_API_Get_SampleBits(s->pMp3Handle);
			switch (tmp1) {
			case 16:
				avctx->sample_fmt = AV_SAMPLE_FMT_S16;
				break;
			case 8:
				avctx->sample_fmt = AV_SAMPLE_FMT_U8;
				break;
			default:
				return -EINVAL;
			}
			avctx->channel_layout =
				av_get_default_channel_layout(avctx->channels);
			avctx->time_base.num = avctx->sample_rate;
		}

		frame->nb_samples = tmp / avctx->channels;
		ret = ff_get_buffer(avctx, frame, 0);
		if (ret < 0)
			return ret;
		memcpy(frame->data[0], s->pDstBuf, frame->linesize[0]);
		s->consumed =
			MP3Decoder_API_Get_ByteConsume(s->pMp3Handle);
		s->gotptr = 0;
		frame_size = tmp;
	} else {
		tmp = buf_size;
		memcpy((char *)s->pSrcBuf + s->gotptr, buf, tmp);
		s->gotptr += tmp;
	}

	if (frame_size > 0)
		*got_frame_ptr = 1;
	else
		*got_frame_ptr = 0;

	return buf_size;
}

static int rts_mp3_decode_close(AVCodecContext *avctx)
{
	RTSMP3DecContext *s = avctx->priv_data;

	s->initparam = 0;
	if (s->pMp3Handle)
		MP3Decoder_API_free(s->pMp3Handle);

	if (s->pSrcBuf) {
		free(s->pSrcBuf);
		s->pSrcBuf = NULL;
	}

	if (s->pDstBuf) {
		free(s->pDstBuf);
		s->pDstBuf = NULL;
	}

	return 0;
}

AVCodec ff_librtsmp3_decoder = {
	.name           = "librtsmp3",
	.long_name      = NULL_IF_CONFIG_SMALL("RTS MP3 (MPEG audio layer 3)"),
	.type           = AVMEDIA_TYPE_AUDIO,
	.id             = AV_CODEC_ID_MP3,
	.priv_data_size = sizeof(RTSMP3DecContext),
	.init           = rts_mp3_decode_init,
	.close          = rts_mp3_decode_close,
	.decode         = rts_mp3_decode_frame,
	.capabilities   = CODEC_CAP_DR1,
};
#endif /* CONFIG_LIBRTSMP3 */
