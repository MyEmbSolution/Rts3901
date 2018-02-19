/*
 * @rtsjpg_encoder.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <rtsisp.h>
#include <rtsjpgenc.h>

#include "libavutil/opt.h"
#include "avcodec.h"
#include "internal.h"

typedef const void *RsMjpgEncInst;
typedef struct RsMJPGContext {
	AVClass *class;

	enum rtsjpgenc_rotation rotation;

	int fd ;
	RsMjpgEncInst encoder;
	struct rts_isp_dma_buffer m_outbuf;
} RsMJPGContext;

static int release_rtsjpg_enc_eniv(AVCodecContext *avctx)
{
	int ret = 0;
	struct RsMJPGContext *p_rtsjpg_ctx = (struct RsMJPGContext *)avctx->priv_data;

	if (p_rtsjpg_ctx->m_outbuf.length > 0 && p_rtsjpg_ctx->m_outbuf.vm_addr)
		rts_isp_free_dma(&p_rtsjpg_ctx->m_outbuf);

	p_rtsjpg_ctx->m_outbuf.vm_addr = NULL;
	p_rtsjpg_ctx->m_outbuf.phy_addr = 0;
	p_rtsjpg_ctx->m_outbuf.length = 0;

	if (p_rtsjpg_ctx->encoder) {
		ret = rtsjpgenc_release(p_rtsjpg_ctx->encoder);
		if (ret) {
			av_log(avctx, AV_LOG_ERROR,
				"rtsjpgenc_release failed, ret = %d\n", ret);
			goto out;
		}
	}
	p_rtsjpg_ctx->encoder = NULL;

out:
	if (p_rtsjpg_ctx->fd > 0) {
		rts_isp_ctrl_close(p_rtsjpg_ctx->fd);
		p_rtsjpg_ctx->fd = -1;
	}

	return ret;
}

static int encode_mjpg(AVCodecContext *avctx, uint32_t pict_bus_addr,
		uint32_t pict_bus_addr_stab, AVPacket *pkt)
{
	struct RsMJPGContext *p_rtsjpg_ctx = (struct RsMJPGContext *)avctx->priv_data;
	int ret = 0;

	struct rtsjpgenc_encin encin;
	encin.src_busLuma = pict_bus_addr;
	encin.src_busChroma = pict_bus_addr + avctx->width * avctx->height;
	encin.p_outbuf = p_rtsjpg_ctx->m_outbuf.vm_addr;
	encin.out_bus_buf = p_rtsjpg_ctx->m_outbuf.phy_addr;
	encin.out_buf_size = p_rtsjpg_ctx->m_outbuf.length;
	encin.out_bytesused = 0;
	ret = rtsjpgenc_encode(p_rtsjpg_ctx->encoder, &encin);
	if (ret) {
		av_log(avctx, AV_LOG_ERROR, "encoding frame failed, errno: %d", ret);
		return ret;
	}

	pkt->data = encin.p_outbuf;
	pkt->size = encin.out_bytesused;

	return ret;
}

static av_cold int rtsjpg_encode_init(AVCodecContext *avctx)
{
	struct RsMJPGContext *p_rtsjpg_ctx = (struct RsMJPGContext *)avctx->priv_data;
	int ret = 0;
	struct rtsjpgenc_config cfg;

	switch (avctx->pix_fmt) {
	case AV_PIX_FMT_NV12:
		cfg.input_type = RTSJPGENC_YUV420_SEMIPLANAR;
		break;
	default:
		av_log(avctx, AV_LOG_ERROR,
			"input pic type (%d) is no supported\n",
			avctx->pix_fmt);
		return -1;
	}

	switch (p_rtsjpg_ctx->rotation) {
	case 0:
		cfg.rotation = RTSJPGENC_ROTATE_0;
		cfg.width = avctx->width;
		cfg.height = avctx->height;
		break;
	case 1:
		cfg.rotation = RTSJPGENC_ROTATE_90R;
		cfg.width = avctx->height;
		cfg.height = avctx->width;
		break;
	case 2:
		cfg.rotation = RTSJPGENC_ROTATE_90L;
		cfg.width = avctx->height;
		cfg.height = avctx->width;
		break;
	default:
		av_log(avctx, AV_LOG_ERROR,
				"rotation(%d) is not supported by jpgenc\n",
				p_rtsjpg_ctx->rotation);
		return -1;
	}


	ret = rtsjpgenc_init(&p_rtsjpg_ctx->encoder);
	if (ret) {
		av_log(avctx, AV_LOG_ERROR, "Init rtsjpg encoder failed\n");
		return ret;
	}

	ret = rtsjpgenc_set_config(p_rtsjpg_ctx->encoder, &cfg);
	if (ret) {
		av_log(avctx, AV_LOG_ERROR, "set rtsjpg enc config failed\n");
		return ret;
	}

	p_rtsjpg_ctx->fd = rts_isp_ctrl_open();
	if (p_rtsjpg_ctx->fd < -1) {
		av_log(avctx, AV_LOG_ERROR,
				"open memalloc device failed\n");
		return -1;
	}

	p_rtsjpg_ctx->m_outbuf.vm_addr = NULL;
	p_rtsjpg_ctx->m_outbuf.phy_addr = 0;
	p_rtsjpg_ctx->m_outbuf.length = avctx->width * avctx->height;

	ret = rts_isp_alloc_dma(&p_rtsjpg_ctx->m_outbuf);
	if (ret < 0) {
		av_log(avctx, AV_LOG_ERROR, "alloc output buffer failed\n");
		goto error;
	}

	return ret;

error:
	release_rtsjpg_enc_eniv(avctx);

	return ret;
}

static void rtsjpg_fake_free(void *opa, uint8_t *data)
{

}

static av_cold int rtsjpg_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
		const AVFrame *frame, int *got_packet)
{
	struct RsMJPGContext *p_rtsjpg_ctx = (struct RsMJPGContext *)avctx->priv_data;
	int ret = 0;

	if (frame) {
		if (avctx->pix_fmt == AV_PIX_FMT_NV12) {
			uint32_t phy_addr = rts_isp_get_video_phy_addr(p_rtsjpg_ctx->fd,
					(unsigned long)frame->data[0]);
			if (phy_addr) {
				ret = encode_mjpg(avctx, phy_addr, 0, pkt);
				if (ret) {
					/* to avoid abort when encode mjpg failed */
					ret = 0;
					goto error;
				}
			} else {
				av_log(avctx, AV_LOG_ERROR,
					"Get Buf bus addr failed\n");
				ret = -1;
				goto error;
			}
		}

		pkt->pts = frame->pkt_pts;
		pkt->dts = pkt->pts;

		pkt->buf = av_buffer_create(pkt->data, pkt->size,
				rtsjpg_fake_free, NULL, 0);
		if (!pkt->buf) {
			*got_packet = 0;
			pkt->data = NULL;
			return AVERROR(ENOMEM);
		}

		*got_packet = !ret;
	}

	return ret;

error:
	*got_packet = 0;
	return ret;
}


static av_cold int rtsjpg_encode_close(AVCodecContext *avctx)
{
	int ret = 0;
	ret = release_rtsjpg_enc_eniv(avctx);

	return ret;
}

/*
 * in fact, rtsjpgenc support yuv420 semiplanar and yuv422 semiplanar
 * but there is no yuv422 semiplanar in ffmpeg AVPixelFormat
 */
static const enum AVPixelFormat pix_fmts_rtsjpg[] = {
	AV_PIX_FMT_NV12,
	AV_PIX_FMT_NONE
};


#define OFFSET(x) offsetof(RsMJPGContext, x)
#define VE (AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM)
static const AVOption options[] = {
	{ "rotate", "rotation video", OFFSET(rotation), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 3, VE },
	{ NULL },
};

static const AVClass rtsjpeg_class = {
    .class_name = "librtsjpeg",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_rtsjpg_encoder = {
	.name           = "mjpeg",
	.long_name      = NULL_IF_CONFIG_SMALL("RTS motion jpeg"),
	.type           = AVMEDIA_TYPE_VIDEO,
	.id             = AV_CODEC_ID_MJPEG,
	.priv_data_size = sizeof(RsMJPGContext),
	.init           = rtsjpg_encode_init,
	.close          = rtsjpg_encode_close,
	.encode2        = rtsjpg_encode_frame,
	.priv_class     = &rtsjpeg_class,
	.pix_fmts	= pix_fmts_rtsjpg,
};
