/*
 * @rsht_encoder.c
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
#include <h264encapi.h>

#include "libavutil/opt.h"
#include "avcodec.h"
#include "internal.h"

#ifndef SPS_PPS_SIZE
#define SPS_PPS_SIZE (48)
#endif

#define GEN_I_FRAME(x) (x & 0x00000001)
#define DISABLE_GEN_I_FRAME(x) (x & 0xFFFFFFFE)

struct dma_buffer {
	void *virt_address;
	uint32_t bus_address;
	uint32_t size;
};


typedef struct RsHantroContext {
	/*the first para must be AVClass yafei 2015-7-21*/
	AVClass *class;
	/*H264 performance tuning params*/
	int h264_flag;
	int sliceSize; /*[0-127]*/
	int seiMessages; /*[0-1]*/
	int videoFullRange; /*[0-1]*/
	int constrainedIntraPrediction; /*[0-1]*/
	int disableDeblockingFilter; /*[0-2]*/
	int enableCabac; /*[0-1]*/
	int cabacInitIdc; /*[0-2]*/
	int transform8x8Mode; /*[0-2]*/
	int quarterPixelMv; /*[0-2]*/
	int roi1DeltaQp; /*[-15,0]*/
	int roi2DeltaQp; /*[-15,0]*/
	int adaptiveRoi; /*[-51,-30]*/
	int pictureRc; /*[0-1]*/
	int pictureSkip; /*[0-1]*/
	int qpHdr; /*-1 or [0,50]*/
	int qpMin;/*[0,51]*/
	int qpMax;/*[qpMin,51]*/
	int hrd;/*[0-1]*/
	int hrdCpbSize; /*[0,25000]*/
	int gopLen; /*[0,150]*/
	int intraQpDelta; /*[-12,12]*/
	int fixedIntraQp; /*[0, 51]*/
	int mbQpAdjustment; /*[-8, 7]*/
	int longTermPicRate; /*[0, n]*/
	int mbQpAutoBoost; /*[0, 1]*/
	int gdr; /*[0, 1]*/
	int drop_frame_en;
	int drop_frame_th;
	int cvbr_en;
	int cvbr_max_bitrate;
	int cvbr_min_bitrate;
	int cvbr_image_quality;
	int cvbr_diff_n;
	int p_wnd_size;
	int p_target_min_percentage;
	int p_target_max_percentage;
	int p_diff_x_th_percentage;
	int p_diff_adjust_percentage;
	int rs_rc_en;

	int rotate_mode; /*[0, 2]*/
	char *str_h264_level;

	int fd ;
	enum AVPixelFormat inputtype;
	H264EncConfig cfg;
	H264EncCodingCtrl coding_cfg;
	H264EncRateCtrl rc_cfg;
	H264EncPreProcessingCfg pre_proc_cfg;
	H264EncInst m_encoder;
	struct dma_buffer m_outbuf;
	H264EncIn m_encIn;
	H264EncOut m_encOut;
	int m_stream_flag;
	int frm_idx;
	int m_videostab_flag;
	H264EncPictureType m_input_type;

	char extradata[SPS_PPS_SIZE];
	int extradata_size;
} RsHantroContext;


static int alloc_dma_memory(AVCodecContext *avctx, void **virt_address,
		uint32_t *bus_address, uint32_t *size)
{
	int ret = 0;
	struct rts_isp_dma_buffer buffer;
	uint32_t pgsize = getpagesize();

	if (virt_address == NULL || bus_address == NULL || size == NULL) {
		return -1;
	}

	buffer.length = (*size + (pgsize - 1)) & (~(pgsize - 1));

	ret = rts_isp_alloc_dma(&buffer);
	if (ret) {
		av_log(avctx, AV_LOG_ERROR, "alloc buffer failed\n");
		return ret;
	}

	*virt_address = buffer.vm_addr;
	*bus_address = buffer.phy_addr;
	*size = buffer.length;

	return 0;
}

static int free_dma_memory(AVCodecContext *avctx, void *virt_address,
		uint32_t bus_address, uint32_t size)
{
	struct rts_isp_dma_buffer buffer;

	buffer.length = size;
	buffer.vm_addr = virt_address;
	buffer.phy_addr = bus_address;

	rts_isp_free_dma(&buffer);

	return 0;
}


static int release_h264enc_eniv(AVCodecContext *avctx,
		RsHantroContext *p_rsht_ctx)
{
	int ret = 0;
	if (p_rsht_ctx->m_stream_flag) {
		ret = H264EncStrmEnd(p_rsht_ctx->m_encoder,
				&p_rsht_ctx->m_encIn,
				&p_rsht_ctx->m_encOut);
		if (ret) {
			av_log(avctx, AV_LOG_ERROR,
				"H264EncStrmEnd Failed ret= %d\n", ret);
			goto out;
		}
	}
	p_rsht_ctx->m_stream_flag = 0;

	if (p_rsht_ctx->m_outbuf.size > 0 &&
		p_rsht_ctx->m_outbuf.virt_address) {
		free_dma_memory(avctx,
			p_rsht_ctx->m_outbuf.virt_address,
			p_rsht_ctx->m_outbuf.bus_address,
			p_rsht_ctx->m_outbuf.size);
	}

	p_rsht_ctx->m_outbuf.virt_address = NULL;
	p_rsht_ctx->m_outbuf.bus_address = 0;
	p_rsht_ctx->m_outbuf.size = 0;

	if (p_rsht_ctx->m_encoder) {
		ret = H264EncRelease(p_rsht_ctx->m_encoder);
		if (ret) {
			av_log(avctx, AV_LOG_ERROR,
				"H264EncRelease Failed ret = %d\n", ret);
			goto out;
		}
	}
	p_rsht_ctx->m_encoder = NULL;
out:
	if (p_rsht_ctx->fd > 0) {
		rts_isp_ctrl_close(p_rsht_ctx->fd);
		p_rsht_ctx->fd = -1;
	}
	return ret;
}

static int update_coding_ctrl_params(RsHantroContext *p_rsht_ctx)
{
	H264EncRet ret;
	p_rsht_ctx->coding_cfg.sliceSize = p_rsht_ctx->sliceSize;
	p_rsht_ctx->coding_cfg.seiMessages = p_rsht_ctx->seiMessages;
	p_rsht_ctx->coding_cfg.videoFullRange = p_rsht_ctx->videoFullRange;
	p_rsht_ctx->coding_cfg.constrainedIntraPrediction =
		p_rsht_ctx->constrainedIntraPrediction;
	p_rsht_ctx->coding_cfg.disableDeblockingFilter =
		p_rsht_ctx->disableDeblockingFilter;
	p_rsht_ctx->coding_cfg.sampleAspectRatioWidth = 0;
	p_rsht_ctx->coding_cfg.sampleAspectRatioHeight = 0;
	p_rsht_ctx->coding_cfg.enableCabac = p_rsht_ctx->enableCabac;
	p_rsht_ctx->coding_cfg.cabacInitIdc = p_rsht_ctx->cabacInitIdc;
	p_rsht_ctx->coding_cfg.transform8x8Mode = p_rsht_ctx->transform8x8Mode;
	p_rsht_ctx->coding_cfg.quarterPixelMv = p_rsht_ctx->quarterPixelMv;
	p_rsht_ctx->coding_cfg.cirStart = 0;
	p_rsht_ctx->coding_cfg.cirInterval = 0;
	p_rsht_ctx->coding_cfg.intraSliceMap1 = 0;
	p_rsht_ctx->coding_cfg.intraSliceMap2 = 0;
	p_rsht_ctx->coding_cfg.intraSliceMap3 = 0;
	p_rsht_ctx->coding_cfg.intraArea.enable = 0;
	/*
	p_rsht_ctx->coding_cfg.intraArea.top = 0;
	p_rsht_ctx->coding_cfg.intraArea.left = 0;
	p_rsht_ctx->coding_cfg.intraArea.bottom = 0;
	p_rsht_ctx->coding_cfg.intraArea.right = 0;
	*/
	p_rsht_ctx->coding_cfg.roi1Area.enable = 0;
	/*
	p_rsht_ctx->coding_cfg.roi1Area.top = 0;
	p_rsht_ctx->coding_cfg.roi1Area.left = 0;
	p_rsht_ctx->coding_cfg.roi1Area.bottom = 0;
	p_rsht_ctx->coding_cfg.roi1Area.right = 0;
	*/
	p_rsht_ctx->coding_cfg.roi2Area.enable = 0;
	/*
	p_rsht_ctx->coding_cfg.roi2Area.top = 0;
	p_rsht_ctx->coding_cfg.roi2Area.left = 0;
	p_rsht_ctx->coding_cfg.roi2Area.bottom = 0;
	p_rsht_ctx->coding_cfg.roi2Area.right = 0;
	*/
	p_rsht_ctx->coding_cfg.roi1DeltaQp = p_rsht_ctx->roi1DeltaQp;
	p_rsht_ctx->coding_cfg.roi2DeltaQp = p_rsht_ctx->roi2DeltaQp;
	p_rsht_ctx->coding_cfg.adaptiveRoi = p_rsht_ctx->adaptiveRoi;
	p_rsht_ctx->coding_cfg.adaptiveRoiColor = 0;
	p_rsht_ctx->coding_cfg.fieldOrder = 0;
	p_rsht_ctx->coding_cfg.gdrDuration = p_rsht_ctx->gdr;

	ret = H264EncSetCodingCtrl(p_rsht_ctx->m_encoder,
			&p_rsht_ctx->coding_cfg);
	return ret;
}

static int update_rate_ctrl_params(AVCodecContext *avctx,
		RsHantroContext *p_rsht_ctx)
{
	H264EncRet ret;
	p_rsht_ctx->rc_cfg.pictureRc = p_rsht_ctx->pictureRc;
	p_rsht_ctx->rc_cfg.mbRc = 0;
	p_rsht_ctx->rc_cfg.pictureSkip = p_rsht_ctx->pictureSkip;
	p_rsht_ctx->rc_cfg.qpHdr = p_rsht_ctx->qpHdr;

	if (avctx->qmin > 51)
		avctx->qmin = 10;

	if (avctx->qmax < avctx->qmin)
		avctx->qmax = 51;

	if (GEN_I_FRAME(p_rsht_ctx->h264_flag))
		p_rsht_ctx->rc_cfg.qpMin = (avctx->qmin + 2 * avctx->qmax) / 3;
	else
		p_rsht_ctx->rc_cfg.qpMin = avctx->qmin;
	p_rsht_ctx->rc_cfg.qpMax = avctx->qmax;
	p_rsht_ctx->rc_cfg.bitPerSecond = avctx->bit_rate;

	p_rsht_ctx->rc_cfg.hrd = p_rsht_ctx->hrd;
	p_rsht_ctx->rc_cfg.hrdCpbSize = p_rsht_ctx->hrdCpbSize;
	p_rsht_ctx->rc_cfg.gopLen = avctx->gop_size;
	/*p_rsht_ctx->rc_cfg.gopLen = ratenum;*/
	avctx->gop_size = p_rsht_ctx->rc_cfg.gopLen;
	p_rsht_ctx->rc_cfg.intraQpDelta = avctx->intra_quant_bias;
	p_rsht_ctx->rc_cfg.fixedIntraQp = p_rsht_ctx->fixedIntraQp;
	p_rsht_ctx->rc_cfg.mbQpAdjustment = p_rsht_ctx->mbQpAdjustment;
	p_rsht_ctx->rc_cfg.longTermPicRate = p_rsht_ctx->longTermPicRate;
	p_rsht_ctx->rc_cfg.mbQpAutoBoost = p_rsht_ctx->mbQpAutoBoost;
	p_rsht_ctx->rc_cfg.drop_frame_en = p_rsht_ctx->drop_frame_en;
	p_rsht_ctx->rc_cfg.drop_frame_th = p_rsht_ctx->drop_frame_th;
	p_rsht_ctx->rc_cfg.cvbr_en = p_rsht_ctx->cvbr_en;

	p_rsht_ctx->rc_cfg.cvbr_diff_n = p_rsht_ctx->cvbr_diff_n;
	p_rsht_ctx->rc_cfg.p_wnd_size = p_rsht_ctx->p_wnd_size;
	p_rsht_ctx->rc_cfg.p_target_min_percentage =
		p_rsht_ctx->p_target_min_percentage;

	p_rsht_ctx->rc_cfg.p_target_max_percentage =
		p_rsht_ctx->p_target_max_percentage;
	p_rsht_ctx->rc_cfg.p_diff_x_th_percentage =
		p_rsht_ctx->p_diff_x_th_percentage;
	p_rsht_ctx->rc_cfg.p_diff_adjust_percentage =
		p_rsht_ctx->p_diff_adjust_percentage;

	p_rsht_ctx->rc_cfg.rs_rc_en = p_rsht_ctx->rs_rc_en;
	p_rsht_ctx->rc_cfg.cvbr_max_bitrate = p_rsht_ctx->cvbr_max_bitrate;
	p_rsht_ctx->rc_cfg.cvbr_min_bitrate = p_rsht_ctx->cvbr_min_bitrate;
	if (p_rsht_ctx->rc_cfg.cvbr_en && p_rsht_ctx->rc_cfg.rs_rc_en)
		p_rsht_ctx->rc_cfg.bitPerSecond =
			(p_rsht_ctx->rc_cfg.cvbr_max_bitrate +
			 p_rsht_ctx->rc_cfg.cvbr_min_bitrate) /2;


	ret = H264EncSetRateCtrl(p_rsht_ctx->m_encoder, &p_rsht_ctx->rc_cfg);
	return ret;
}

static int init_h264enc_eniv(AVCodecContext *avctx,
		RsHantroContext *p_rsht_ctx)
{
	H264EncRet ret;

	switch (p_rsht_ctx->inputtype) {
	/*
	case DBG_YUV420_PLANAR:
	case AV_PIX_FMT_YUV420P:
		p_rsht_ctx->m_input_type = H264ENC_YUV420_PLANAR;
		break;
	case DBG_YUV420_SEMIPLANAR:
	*/
	case V4L2_PIX_FMT_NV12:
		p_rsht_ctx->m_input_type = H264ENC_YUV420_SEMIPLANAR;
		break;
	/*case DBG_YUV422_INTERLEAVED_YUYV:*/
	case V4L2_PIX_FMT_YUYV:
		p_rsht_ctx->m_input_type = H264ENC_YUV422_INTERLEAVED_YUYV;
		break;
	default:
		av_log(avctx, AV_LOG_ERROR,
				"Input pic type (%d) is not supported\n",
				p_rsht_ctx->inputtype);
		return -1;
	}

	p_rsht_ctx->cfg.streamType = H264ENC_BYTE_STREAM;
	/*p_rsht_ctx->cfg.streamType = H264ENC_NAL_UNIT_STREAM;*/
	/*p_rsht_ctx->cfg.viewMode = H264ENC_BASE_VIEW_DOUBLE_BUFFER;*/
	p_rsht_ctx->cfg.viewMode = H264ENC_BASE_VIEW_SINGLE_BUFFER;
	if (!strcmp("h264_level_1", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_1;
	else if (!strcmp("h264_level_1_b", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_1_b;
	else if (!strcmp("h264_level_1_1", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_1_1;
	else if (!strcmp("h264_level_1_2", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_1_2;
	else if (!strcmp("h264_level_1_3", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_1_3;
	else if (!strcmp("h264_level_2", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_2;
	else if (!strcmp("h264_level_2_1", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_2_1;
	else if (!strcmp("h264_level_2_2", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_2_2;
	else if (!strcmp("h264_level_3", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_3;
	else if (!strcmp("h264_level_3_1", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_3_1;
	else if (!strcmp("h264_level_3_2", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_3_2;
	else if (!strcmp("h264_level_4", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_4;
	else if (!strcmp("h264_level_4_1", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_4_1;
	else if (!strcmp("h264_level_4_2", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_4_2;
	else if (!strcmp("h264_level_5", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_5;
	else if (!strcmp("h264_level_5_1", p_rsht_ctx->str_h264_level))
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_5_1;
	else
		p_rsht_ctx->cfg.level = H264ENC_LEVEL_4;


	if (p_rsht_ctx->m_videostab_flag) {
		p_rsht_ctx->cfg.width = avctx->width - 16;
		p_rsht_ctx->cfg.height = avctx->height - 16;
	} else {
		p_rsht_ctx->cfg.width = avctx->width;
		p_rsht_ctx->cfg.height = avctx->height;
	}

	p_rsht_ctx->cfg.frameRateDenom = 1;
	p_rsht_ctx->cfg.frameRateNum = avctx->time_base.den;
	p_rsht_ctx->cfg.refFrameAmount = 1;
	p_rsht_ctx->cfg.scaledWidth = 0;
	p_rsht_ctx->cfg.scaledHeight = 0;

	ret = H264EncInit(&p_rsht_ctx->cfg, &p_rsht_ctx->m_encoder);
	if (H264ENC_OK != ret) {
		av_log(avctx, AV_LOG_ERROR, "H264EncInit failed\n");
		return ret;
	}

	ret = H264EncGetCodingCtrl(p_rsht_ctx->m_encoder,
			&p_rsht_ctx->coding_cfg);
	if (H264ENC_OK != ret) {
		av_log(avctx, AV_LOG_ERROR, "H264EncGetCodingCtrl failed\n");
		return ret;
	}

	ret = update_coding_ctrl_params(p_rsht_ctx);
	/* keep all the other parameters default */
	if (H264ENC_OK != ret) {
		av_log(avctx, AV_LOG_ERROR, "H264EncSetCodingCtrl failed\n");
		return ret;
	}

	ret = H264EncGetRateCtrl(p_rsht_ctx->m_encoder, &p_rsht_ctx->rc_cfg);
	if (H264ENC_OK != ret) {
		av_log(avctx, AV_LOG_ERROR, "H264EncGetRateCtrl failed\n");
		return ret;
	}

	if (avctx->bit_rate)
		p_rsht_ctx->rc_cfg.bitPerSecond = avctx->bit_rate;
	else
		p_rsht_ctx->rc_cfg.bitPerSecond = 2 * 1024 * 1024;

	ret = update_rate_ctrl_params(avctx, p_rsht_ctx);
	if (H264ENC_OK != ret) {
		av_log(avctx, AV_LOG_ERROR, "H264EncSetRateCtrl failed\n");
		return ret;
	}

	ret = H264EncGetPreProcessing(p_rsht_ctx->m_encoder,
			&p_rsht_ctx->pre_proc_cfg);
	if (H264ENC_OK != ret) {
		av_log(avctx, AV_LOG_ERROR,
				"H264EncGetPreProcessing failed\n");
		return ret;
	}


	if (1 == p_rsht_ctx->rotate_mode ||
		2 == p_rsht_ctx->rotate_mode) {
		p_rsht_ctx->pre_proc_cfg.origWidth = avctx->height;
		p_rsht_ctx->pre_proc_cfg.origHeight = avctx->width;
	} else {

		p_rsht_ctx->pre_proc_cfg.origWidth = avctx->width;
		p_rsht_ctx->pre_proc_cfg.origHeight = avctx->height;
	}

	p_rsht_ctx->pre_proc_cfg.inputType = p_rsht_ctx->m_input_type;

	if (0 == p_rsht_ctx->rotate_mode) {
		p_rsht_ctx->pre_proc_cfg.rotation = H264ENC_ROTATE_0;
	} else if (1 == p_rsht_ctx->rotate_mode) {
		p_rsht_ctx->pre_proc_cfg.rotation = H264ENC_ROTATE_90R;
	} else if (2 == p_rsht_ctx->rotate_mode) {
		p_rsht_ctx->pre_proc_cfg.rotation = H264ENC_ROTATE_90L;
	}

	if (p_rsht_ctx->m_videostab_flag)
		p_rsht_ctx->pre_proc_cfg.videoStabilization = 1;
	else
		p_rsht_ctx->pre_proc_cfg.videoStabilization = 0;

	ret = H264EncSetPreProcessing(p_rsht_ctx->m_encoder,
			&p_rsht_ctx->pre_proc_cfg);
	if (H264ENC_OK != ret) {
		av_log(avctx, AV_LOG_ERROR,
				"H264EncSetPreProcessing failed\n");
		return ret;
	}


	p_rsht_ctx->m_outbuf.size =
		p_rsht_ctx->cfg.width * p_rsht_ctx->cfg.height;
	ret = alloc_dma_memory(avctx,
			&p_rsht_ctx->m_outbuf.virt_address,
			&p_rsht_ctx->m_outbuf.bus_address,
			&p_rsht_ctx->m_outbuf.size);
	if (ret < 0) {
		av_log(avctx, AV_LOG_ERROR, "alloc output buffer failed\n");
		return ret;
	}

	p_rsht_ctx->m_encIn.pOutBuf = p_rsht_ctx->m_outbuf.virt_address;
	p_rsht_ctx->m_encIn.busOutBuf = p_rsht_ctx->m_outbuf.bus_address;
	p_rsht_ctx->m_encIn.outBufSize = p_rsht_ctx->m_outbuf.size;
	return ret;
}

static int encode_h264_start(AVCodecContext *avctx,
		RsHantroContext *p_rsht_ctx)
{
	int ret = 0;

	p_rsht_ctx->frm_idx = 0;

	ret = H264EncStrmStart(p_rsht_ctx->m_encoder,
		&p_rsht_ctx->m_encIn,
		&p_rsht_ctx->m_encOut);
	if (ret) {
		av_log(avctx, AV_LOG_ERROR, "H264EncStrmStart Failed\n");
		return -1;
	}
	p_rsht_ctx->m_stream_flag = 1;

	memcpy(p_rsht_ctx->extradata,
		p_rsht_ctx->m_outbuf.virt_address,
		p_rsht_ctx->m_encOut.streamSize);
	p_rsht_ctx->extradata_size = p_rsht_ctx->m_encOut.streamSize;

	if (avctx->flags & CODEC_FLAG_GLOBAL_HEADER) {
		avctx->extradata = av_malloc(p_rsht_ctx->m_encOut.streamSize);
		memcpy(avctx->extradata,
			p_rsht_ctx->m_outbuf.virt_address,
			p_rsht_ctx->m_encOut.streamSize);
		avctx->extradata_size = p_rsht_ctx->m_encOut.streamSize;
	}

	memmove(p_rsht_ctx->m_outbuf.virt_address + SPS_PPS_SIZE - p_rsht_ctx->m_encOut.streamSize,
		p_rsht_ctx->m_outbuf.virt_address,
		p_rsht_ctx->m_encOut.streamSize);

	return 0;
}

static int encode_h264(AVCodecContext *avctx, uint32_t pict_bus_addr,
		uint32_t pict_bus_addr_stab, AVPacket *pkt,
		RsHantroContext *p_rsht_ctx)
{
	int ret = 0;

	switch (p_rsht_ctx->m_input_type) {
	case H264ENC_YUV420_PLANAR:
		p_rsht_ctx->m_encIn.busChromaV = pict_bus_addr +
				avctx->width*avctx->height +
				(avctx->width/2)*(avctx->height/2);
	case H264ENC_YUV420_SEMIPLANAR:
		p_rsht_ctx->m_encIn.busChromaU = pict_bus_addr +
			avctx->width*avctx->height;
	case H264ENC_YUV422_INTERLEAVED_YUYV:
		p_rsht_ctx->m_encIn.busLuma = pict_bus_addr;
		break;
	default:
		av_log(avctx, AV_LOG_ERROR,
				"Pic data type (%d) is not support\n",
				p_rsht_ctx->m_input_type);
		return -1;
	}

	if (p_rsht_ctx->m_videostab_flag) {
		if (pict_bus_addr_stab)
			p_rsht_ctx->m_encIn.busLumaStab = pict_bus_addr_stab;
		else {
			av_log(avctx, AV_LOG_ERROR, "Invalid stab buff\n");
			return -1;
		}
	}

	if (p_rsht_ctx->frm_idx == 0) {
		p_rsht_ctx->m_encIn.timeIncrement = 0;
	} else {
		p_rsht_ctx->m_encIn.timeIncrement = 1;
	}

	p_rsht_ctx->m_encIn.busOutBuf =
		p_rsht_ctx->m_outbuf.bus_address + SPS_PPS_SIZE;
	p_rsht_ctx->m_encIn.pOutBuf =
		p_rsht_ctx->m_outbuf.virt_address + SPS_PPS_SIZE;
	p_rsht_ctx->m_encIn.outBufSize =
		p_rsht_ctx->m_outbuf.size - SPS_PPS_SIZE;

	if (p_rsht_ctx->frm_idx % avctx->gop_size == 0 ||
		GEN_I_FRAME(p_rsht_ctx->h264_flag)) {
		p_rsht_ctx->m_encIn.codingType = H264ENC_INTRA_FRAME;
	} else {
		p_rsht_ctx->m_encIn.codingType = H264ENC_PREDICTED_FRAME;
	}

	p_rsht_ctx->m_encIn.ipf = H264ENC_REFERENCE_AND_REFRESH;
	p_rsht_ctx->m_encIn.ltrf = H264ENC_REFERENCE;

	ret = H264EncStrmEncode(p_rsht_ctx->m_encoder,
				&p_rsht_ctx->m_encIn,
				&p_rsht_ctx->m_encOut,
				NULL, NULL);
	unsigned char *p_nal_type = NULL;
	unsigned char is_cur_intra_frame = 0;
	p_nal_type = p_rsht_ctx->m_outbuf.virt_address + SPS_PPS_SIZE + 4;
	is_cur_intra_frame = 5 == (*p_nal_type & 0x1F);
	if (is_cur_intra_frame) {
		pkt->data = p_rsht_ctx->m_outbuf.virt_address
			+ SPS_PPS_SIZE - p_rsht_ctx->extradata_size;
		pkt->size = p_rsht_ctx->m_encOut.streamSize + p_rsht_ctx->extradata_size;
		pkt->flags |= AV_PKT_FLAG_KEY;
	} else {
		pkt->data = p_rsht_ctx->m_outbuf.virt_address + SPS_PPS_SIZE;
		pkt->size = p_rsht_ctx->m_encOut.streamSize;
	}

	if (H264ENC_FRAME_READY == ret) {
		if (GEN_I_FRAME(p_rsht_ctx->h264_flag) && is_cur_intra_frame) {
			p_rsht_ctx->h264_flag =
				DISABLE_GEN_I_FRAME(p_rsht_ctx->h264_flag);
			/*it is ridiculous, but it do works*/
			p_rsht_ctx->frm_idx = avctx->gop_size + 1;
		} else {
			p_rsht_ctx->frm_idx++;
		}
		/*in case frm_idx overflow*/
		if (p_rsht_ctx->frm_idx > 1000000)
			p_rsht_ctx->frm_idx =
				avctx->gop_size +
				p_rsht_ctx->frm_idx % avctx->gop_size;
	}

	return ret;
}

static av_cold int rsht_encode_init(AVCodecContext *avctx)
{
	int ret = 0;
	RsHantroContext *p_rsht_ctx = avctx->priv_data;
	/*p_rsht_ctx->inputtype = avctx->pix_fmt;//DBG_YUV420_SEMIPLANAR;*/
	p_rsht_ctx->inputtype = V4L2_PIX_FMT_NV12;

	/*
	av_log(avctx, AV_LOG_DEBUG,"rsht encode init, avctx->pix_fmt = 0x%08x\n",avctx->pix_fmt);
	av_log(avctx, AV_LOG_DEBUG,"rsht encode init, input_type = 0x%08x\n",p_rsht_ctx->inputtype);
	av_log(avctx, AV_LOG_DEBUG, "sliceSize = %d\n", p_rsht_ctx->sliceSize);
	av_log(avctx, AV_LOG_DEBUG, "seiMessages = %d\n", p_rsht_ctx->seiMessages);
	av_log(avctx, AV_LOG_DEBUG, "videoFullRange = %d\n", p_rsht_ctx->videoFullRange);
	av_log(avctx, AV_LOG_DEBUG, "constrainedIntraPrediction = %d\n", p_rsht_ctx->constrainedIntraPrediction);
	av_log(avctx, AV_LOG_DEBUG, "disableDeblockingFilter = %d\n", p_rsht_ctx->disableDeblockingFilter);
	av_log(avctx, AV_LOG_DEBUG, "enableCabac = %d\n", p_rsht_ctx->enableCabac);
	av_log(avctx, AV_LOG_DEBUG, "cabacInitIdc = %d\n", p_rsht_ctx->cabacInitIdc);
	av_log(avctx, AV_LOG_DEBUG, "transform8x8Mode = %d\n", p_rsht_ctx->transform8x8Mode);
	av_log(avctx, AV_LOG_DEBUG, "quarterPixelMv = %d\n", p_rsht_ctx->quarterPixelMv);
	av_log(avctx, AV_LOG_DEBUG, "roi1DeltaQp = %d\n", p_rsht_ctx->roi1DeltaQp);
	av_log(avctx, AV_LOG_DEBUG, "roi2DeltaQp = %d\n", p_rsht_ctx->roi2DeltaQp);
	av_log(avctx, AV_LOG_DEBUG, "adaptiveRoi = %d\n", p_rsht_ctx->adaptiveRoi);
	av_log(avctx, AV_LOG_DEBUG, "pictureRc = %d\n", p_rsht_ctx->pictureRc);
	av_log(avctx, AV_LOG_DEBUG, "pictureSkip = %d\n", p_rsht_ctx->pictureSkip);
	av_log(avctx, AV_LOG_DEBUG, "qpHdr = %d\n", p_rsht_ctx->qpHdr);
	av_log(avctx, AV_LOG_DEBUG, "hrd = %d\n", p_rsht_ctx->hrd);
	av_log(avctx, AV_LOG_DEBUG, "hrdCpbSize = %d\n", p_rsht_ctx->hrdCpbSize);
	av_log(avctx, AV_LOG_DEBUG, "fixedIntraQp = %d\n", p_rsht_ctx->fixedIntraQp);
	av_log(avctx, AV_LOG_DEBUG, "mbQpAdjustment = %d\n", p_rsht_ctx->mbQpAdjustment);
	av_log(avctx, AV_LOG_DEBUG, "longTermPicRate = %d\n", p_rsht_ctx->longTermPicRate);
	av_log(avctx, AV_LOG_DEBUG, "mbQpAutoBoost = %d\n", p_rsht_ctx->mbQpAutoBoost);
	av_log(avctx, AV_LOG_DEBUG, "gdr = %d\n", p_rsht_ctx->gdr);
	*/

	p_rsht_ctx->fd = rts_isp_ctrl_open(O_RDWR | O_NONBLOCK);
	if (p_rsht_ctx->fd < -1) {
		av_log(avctx, AV_LOG_ERROR,
				"open memalloc device failed\n");
		return -1;
	}

	p_rsht_ctx->m_encoder = NULL;
	p_rsht_ctx->m_outbuf.virt_address = NULL;
	p_rsht_ctx->m_outbuf.bus_address = 0;
	p_rsht_ctx->m_outbuf.size = 0;
	p_rsht_ctx->m_stream_flag = 0;
	p_rsht_ctx->frm_idx = 0;
	p_rsht_ctx->m_videostab_flag = 0;
	p_rsht_ctx->extradata_size = 0;

	ret = init_h264enc_eniv(avctx, p_rsht_ctx);
	if (ret) {
		av_log(avctx, AV_LOG_ERROR, "init_h264enc_eniv failed\n");
		goto error;
	}

	encode_h264_start(avctx, p_rsht_ctx);
	ret = 0;
	return ret;
error:
	release_h264enc_eniv(avctx, p_rsht_ctx);
	if (p_rsht_ctx->fd > 0)
		rts_isp_ctrl_close(p_rsht_ctx->fd);
	p_rsht_ctx->fd = -1;
	return ret;
}

static int preview_h264_loop(AVCodecContext *avctx, AVPacket *pkt,
		const AVFrame *frame, RsHantroContext *p_rsht_ctx)
{
	int ret = 0;

	if (p_rsht_ctx->inputtype == V4L2_PIX_FMT_NV12) {
		unsigned long phy_addr;
		phy_addr = rts_isp_get_video_phy_addr(p_rsht_ctx->fd,
				(unsigned long)frame->data[0]);


		if (phy_addr) {
			ret = encode_h264(avctx, phy_addr,
					0, pkt, p_rsht_ctx);
			if (H264ENC_FRAME_READY == ret)
				ret = 0; /* 0:means encode frame success */
		} else {
			av_log(avctx, AV_LOG_ERROR,
					"Get Buf bus addr failed\n");
		}
	}

	return ret;
}


static int preview_h264_loop_stab(AVCodecContext *avctx, AVPacket *pkt,
		RsHantroContext *p_rsht_ctx)
{
	int ret = 0;

	if (p_rsht_ctx->inputtype == V4L2_PIX_FMT_NV12) {
		uint32_t pict_bus_addr = 0;
		uint32_t pict_bus_addr_stab = 0;

		/*to be done*/
		if (pict_bus_addr && pict_bus_addr_stab) {
			ret = encode_h264(avctx, pict_bus_addr,
					pict_bus_addr_stab, pkt, p_rsht_ctx);
		} else {
			av_log(avctx, AV_LOG_ERROR,
					"Get Buf bus addr failed\n");
		}
	}

	return ret;
}


static void rsht_fake_free(void *opa, uint8_t *data)
{

}

static av_cold int rsht_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
		const AVFrame *frame, int *got_packet)
{
	RsHantroContext *p_rsht_ctx = avctx->priv_data;
	int ret = 0;
	/*try to change params every frame*/
	update_coding_ctrl_params(p_rsht_ctx);

	update_rate_ctrl_params(avctx, p_rsht_ctx);

	if (frame) {
		/*
		   if (p_rsht_ctx->m_videostab_flag)
		   ret = preview_h264_loop_stab(pkt, p_rsht_ctx);
		   else
		   */
		ret = preview_h264_loop(avctx, pkt, frame, p_rsht_ctx);

		pkt->pts = frame->pkt_pts;
		pkt->dts = pkt->pts;

		pkt->buf = av_buffer_create(pkt->data, pkt->size,
				rsht_fake_free, NULL, 0);
		if (!pkt->buf) {
			*got_packet = 0;
			pkt->data = NULL;
			return AVERROR(ENOMEM);
		}

		*got_packet = !ret;
	}
	return 0;
}


static av_cold int rsht_encode_close(AVCodecContext *avctx)
{
	int ret = 0;
	RsHantroContext *p_rsht_ctx = avctx->priv_data;

	ret = release_h264enc_eniv(avctx, p_rsht_ctx);

	if (p_rsht_ctx->fd > 0) {
		rts_isp_ctrl_close(p_rsht_ctx->fd);
		p_rsht_ctx->fd = -1;
	}

	return ret;
}

#define OFFSET(x) offsetof(RsHantroContext, x)
#define VE (AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM)
static const AVOption options[] = {
	{ "h264_flag", "h264 operate flag", OFFSET(h264_flag), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, INT_MAX, VE },
	{ "h264_sliceSize", "size of a slice in macroblock rows", OFFSET(sliceSize), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 127, VE },
	{ "h264_seiMessages", "Contain timing information", OFFSET(seiMessages), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_videoFullRange", "YUV or YCbCr", OFFSET(videoFullRange), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_constrainedIntraPrediction", "intra prediction to constrained mode", OFFSET(constrainedIntraPrediction), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_disableDeblockingFilter", "filtering on all macroblock edges", OFFSET(disableDeblockingFilter), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, VE },
	{ "h264_enableCabac", "CABAC entropy coding", OFFSET(enableCabac), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_cabacInitIdc", "Selects one of the three CABAC initialization tables", OFFSET(cabacInitIdc), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, VE },
	{ "h264_transform8x8Mode", "8*8 transform", OFFSET(transform8x8Mode), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, VE },
	{ "h264_quarterPixelMv", "usage of 1/4 pixel motion estimation", OFFSET(quarterPixelMv), AV_OPT_TYPE_INT, { .i64 = 1 }, 0, 2, VE },
	{ "h264_roi1DeltaQp", "Specifies the QP delta value for the first ROI area", OFFSET(roi1DeltaQp), AV_OPT_TYPE_INT, { .i64 = 0 }, -15, 0, VE },
	{ "h264_roi2DeltaQp", "Specifies the QP delta value for the second ROI area", OFFSET(roi2DeltaQp), AV_OPT_TYPE_INT, { .i64 = 0 }, -15, 0, VE },
	{ "h264_adaptiveRoi", "Specifies the QP delta for adaptive ROI", OFFSET(adaptiveRoi), AV_OPT_TYPE_INT, { .i64 = 0 }, -51, 0, VE },
	{ "h264_pictureRc", "Enables rate control to  adjust QP between  frames", OFFSET(pictureRc), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_pictureSkip", "Rate control to skip pictures", OFFSET(pictureSkip), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_qpHdr", "Initial QP", OFFSET(qpHdr), AV_OPT_TYPE_INT, { .i64 = 26 }, -1, 50, VE },
	{ "h264_hrd", "HRD Model", OFFSET(hrd), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_hrdCpbSize", "HRD Cpb Size", OFFSET(hrdCpbSize), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 25000, VE },
	{ "h264_fixedIntraQp", "QP for all I frame", OFFSET(fixedIntraQp), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 51, VE },
	{ "h264_mbQpAdjustment", "threshold for macro block based QP adjustment", OFFSET(mbQpAdjustment), AV_OPT_TYPE_INT, { .i64 = 0 }, -8, 7, VE },
	{ "h264_longTermPicRate", "period between long term picture refreshes", OFFSET(longTermPicRate), AV_OPT_TYPE_INT, { .i64 = 15 }, 0, INT_MAX, VE },
	{ "h264_mbQpAutoBoost", "macroblocks and automatically boost the quality", OFFSET(mbQpAutoBoost), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "h264_gdr", "Gradual Decoder Refresh", OFFSET(gdr), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE },
	{ "drop_frame_en", "drop frame enable", OFFSET(drop_frame_en), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 1, VE },
	{ "drop_frame_th", "drop_frame_th", OFFSET(drop_frame_th), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 100, VE },
	{ "h264_cvbr_en", "cvbr_en", OFFSET(cvbr_en), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 1, VE },
	{ "h264_cvbr_max_bitrate", "max_br", OFFSET(cvbr_max_bitrate), AV_OPT_TYPE_INT, { .i64 = 2000000 }, 12000, 64000000, VE },
	{ "h264_cvbr_min_bitrate", "min_br", OFFSET(cvbr_min_bitrate), AV_OPT_TYPE_INT, { .i64 = 256000 }, 10000, 64000000, VE },
	{ "h264_cvbr_image_quality", "image_quality", OFFSET(cvbr_image_quality), AV_OPT_TYPE_INT, { .i64 = 4 }, 0, 8, VE },
	{ "cvbr_diff_n", "cvbr_diff_n", OFFSET(cvbr_diff_n), AV_OPT_TYPE_INT, { .i64 = 15 }, -1, 100, VE },
	{ "p_wnd_size", "p_wnd_size", OFFSET(p_wnd_size), AV_OPT_TYPE_INT, { .i64 = 10 }, -1, 100, VE },
	{ "p_target_min_percentage", "target_min", OFFSET(p_target_min_percentage), AV_OPT_TYPE_INT, { .i64 = 25 }, -1, 200, VE },
	{ "p_target_max_percentage", "target_max", OFFSET(p_target_max_percentage), AV_OPT_TYPE_INT, { .i64 = 150 }, -1, 500, VE },
	{ "p_diff_x_th_percentage", "diff_x_th", OFFSET(p_diff_x_th_percentage), AV_OPT_TYPE_INT, { .i64 = 60 }, -1, 500, VE },
	{ "p_diff_adjust_percentage", "diff_adjust", OFFSET(p_diff_adjust_percentage), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 100, VE },
	{ "h264_rs_rc_en", "use rs rc alg", OFFSET(rs_rc_en), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 2, VE },

	{ "rotate", "Video Rotate", OFFSET(rotate_mode), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, VE },
	{ "h264_level", "H264 level", OFFSET(str_h264_level), AV_OPT_TYPE_STRING, .flags = VE },
	{ NULL },
};

static const AVClass h1encoder_class = {
	.class_name = "libh1encoder",
	.item_name  = av_default_item_name,
	.option     = options,
	.version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_rsht_encoder = {
	.name           = "rsht",
	.long_name      = NULL_IF_CONFIG_SMALL("RS_HANTRO"),
	.type           = AVMEDIA_TYPE_VIDEO,
	.id             = AV_CODEC_ID_H264,
	.priv_data_size = sizeof(RsHantroContext),
	.init           = rsht_encode_init,
	.close          = rsht_encode_close,
	.encode2        = rsht_encode_frame,
	.priv_class     = &h1encoder_class,
	/*.capabilities   = CODEC_CAP_DR1,*/
};

