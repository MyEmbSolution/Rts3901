#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dbg_log.h"
#include "dbg_h264.h"
#include "dbg_priv.h"
#include <rtsisp.h>
#include <h264encapi.h>

#ifndef DBG_H264_I_QP
#define DBG_H264_I_QP	0
#endif

struct dma_buffer {
	void *virt_address;
	uint32_t bus_address;
	uint32_t size;
};

static H264EncInst m_encoder = NULL;
static struct dma_buffer m_outbuf = {NULL, 0, 0};
static H264EncIn m_encIn;
static H264EncOut m_encOut;
static int m_stream_flag = 0;
static int g_index = 0;
static int m_videostab_flag = 0;
static int m_ratenum = 0;
static FILE * pH264File = NULL;
H264EncPictureType m_input_type;

struct dbg_h264_encoder {
	H264EncInst encoder;
	uint32_t width;
	uint32_t height;
	struct rts_isp_dma_buffer outbuf;
	int ratenum;
	int gop;
	int videostab;
	H264EncPictureType input_type;
	FILE *pfile;
	long index;
	int start;
	long bitsum;
};

void H264EncTrace(const char *msg)
{
	DBG_LOG_LOG("%s\n", msg);
}

static int print_h264enc_info()
{
	H264EncApiVersion version;
	H264EncBuild build;

	version = H264EncGetApiVersion();
	DBG_LOG_LOG("ApiVersion: %d.%d\n", version.major, version.minor);

	build = H264EncGetBuild();
	DBG_LOG_LOG("Build : sw (%d), hw (0x%x)\n", build.swBuild, build.hwBuild);
}

static int print_enc_parameters(const H264EncConfig *pEncCfg,
		const H264EncCodingCtrl *pCodeParams,
		const H264EncRateCtrl *pRateCtrl,
		const H264EncPreProcessingCfg *pPreProcCfg)
{
	DBG_LOG_PRINT("++++++++++++++++++++++++++++++++\n");
	if (pEncCfg) {
		DBG_LOG_PRINT("H264EncInit#\n");
		DBG_LOG_PRINT("streamType : %d\n", pEncCfg->streamType);
		DBG_LOG_PRINT("viewMode : %d\n", pEncCfg->viewMode);
		DBG_LOG_PRINT("level : %d\n", pEncCfg->level);
		DBG_LOG_PRINT("refFrameAmount : %d\n", pEncCfg->refFrameAmount);
		DBG_LOG_PRINT("width : %d\n", pEncCfg->width);
		DBG_LOG_PRINT("height : %d\n", pEncCfg->height);
		DBG_LOG_PRINT("frameRateNum : %d\n", pEncCfg->frameRateNum);
		DBG_LOG_PRINT("frameRateDenom : %d\n", pEncCfg->frameRateDenom);
		DBG_LOG_PRINT("scaledWidth : %d\n", pEncCfg->scaledWidth);
		DBG_LOG_PRINT("scaledHeight : %d\n", pEncCfg->scaledHeight);
	}

	if (pCodeParams) {
		DBG_LOG_PRINT("H264EncSetCodingCtrl#\n");
		DBG_LOG_PRINT("sliceSize : %d\n", pCodeParams->sliceSize);
		DBG_LOG_PRINT("seiMessages : %d\n", pCodeParams->seiMessages);
		DBG_LOG_PRINT("videoFullRange : %d\n", pCodeParams->videoFullRange);
		DBG_LOG_PRINT("constrainedIntraPrediction : %d\n", pCodeParams->constrainedIntraPrediction);
		DBG_LOG_PRINT("disableDeblockingFilter : %d\n", pCodeParams->disableDeblockingFilter);
		DBG_LOG_PRINT("sampleAspectRatioWidth : %d\n", pCodeParams->sampleAspectRatioWidth);
		DBG_LOG_PRINT("sampleAspectRatioHeight : %d\n", pCodeParams->sampleAspectRatioHeight);
		DBG_LOG_PRINT("enableCabac : %d\n", pCodeParams->enableCabac);
		DBG_LOG_PRINT("cabacInitIdc : %d\n", pCodeParams->cabacInitIdc);
		DBG_LOG_PRINT("transform8x8Mode : %d\n", pCodeParams->transform8x8Mode);
		DBG_LOG_PRINT("quarterPixelMv : %d\n", pCodeParams->quarterPixelMv);
		DBG_LOG_PRINT("cirStart : %d\n", pCodeParams->cirStart);
		DBG_LOG_PRINT("cirInterval : %d\n", pCodeParams->cirInterval);
		DBG_LOG_PRINT("intraSliceMap1 : %d\n", pCodeParams->intraSliceMap1);
		DBG_LOG_PRINT("intraSliceMap2 : %d\n", pCodeParams->intraSliceMap2);
		DBG_LOG_PRINT("intraSliceMap3 : %d\n", pCodeParams->intraSliceMap3);
		DBG_LOG_PRINT("intraArea.enable : %d\n", pCodeParams->intraArea.enable);
		DBG_LOG_PRINT("intraArea.top : %d\n", pCodeParams->intraArea.top);
		DBG_LOG_PRINT("intraArea.bottom : %d\n", pCodeParams->intraArea.bottom);
		DBG_LOG_PRINT("intraArea.left : %d\n", pCodeParams->intraArea.left);
		DBG_LOG_PRINT("intraArea.right : %d\n", pCodeParams->intraArea.right);
		DBG_LOG_PRINT("roi1Area.enable : %d\n", pCodeParams->roi1Area.enable);
		DBG_LOG_PRINT("roi1Area.top : %d\n", pCodeParams->roi1Area.top);
		DBG_LOG_PRINT("roi1Area.bottom : %d\n", pCodeParams->roi1Area.bottom);
		DBG_LOG_PRINT("roi1Area.left : %d\n", pCodeParams->roi1Area.left);
		DBG_LOG_PRINT("roi1Area.right : %d\n", pCodeParams->roi1Area.right);
		DBG_LOG_PRINT("roi2Area.enable : %d\n", pCodeParams->roi2Area.enable);
		DBG_LOG_PRINT("roi2Area.top : %d\n", pCodeParams->roi2Area.top);
		DBG_LOG_PRINT("roi2Area.bottom : %d\n", pCodeParams->roi2Area.bottom);
		DBG_LOG_PRINT("roi2Area.left : %d\n", pCodeParams->roi2Area.left);
		DBG_LOG_PRINT("roi2Area.right : %d\n", pCodeParams->roi2Area.right);
		DBG_LOG_PRINT("roi1DeltaQp : %d\n", pCodeParams->roi1DeltaQp);
		DBG_LOG_PRINT("roi2DeltaQp : %d\n", pCodeParams->roi2DeltaQp);
		DBG_LOG_PRINT("adaptiveRoi : %d\n", pCodeParams->adaptiveRoi);
		DBG_LOG_PRINT("adaptiveRoiColor : %d\n", pCodeParams->adaptiveRoiColor);
		DBG_LOG_PRINT("fieldOrder : %d\n", pCodeParams->fieldOrder);
		DBG_LOG_PRINT("gdrDuration : %d\n", pCodeParams->gdrDuration);
	}

	if (pRateCtrl) {
		DBG_LOG_PRINT("H264EncSetRateCtrl#\n");
		DBG_LOG_PRINT("pictureRc : %d\n", pRateCtrl->pictureRc);
		DBG_LOG_PRINT("mbRc : %d\n", pRateCtrl->mbRc);
		DBG_LOG_PRINT("pictureSkip : %d\n", pRateCtrl->pictureSkip);
		DBG_LOG_PRINT("qpHdr : %d\n", pRateCtrl->qpHdr);
		DBG_LOG_PRINT("qpMin : %d\n", pRateCtrl->qpMin);
		DBG_LOG_PRINT("qpMax : %d\n", pRateCtrl->qpMax);
		DBG_LOG_PRINT("bitPerSecond : %d\n", pRateCtrl->bitPerSecond);
		DBG_LOG_PRINT("gopLen : %d\n", pRateCtrl->gopLen);
		DBG_LOG_PRINT("hrd : %d\n", pRateCtrl->hrd);
		DBG_LOG_PRINT("hrdCpbSize : %d\n", pRateCtrl->hrdCpbSize);
		DBG_LOG_PRINT("intraQpDelta : %d\n", pRateCtrl->intraQpDelta);
		DBG_LOG_PRINT("fixedIntraQp : %d\n", pRateCtrl->fixedIntraQp);
		DBG_LOG_PRINT("mbQpAdjustment : %d\n", pRateCtrl->mbQpAdjustment);
		DBG_LOG_PRINT("longTermPicRate : %d\n", pRateCtrl->longTermPicRate);
		DBG_LOG_PRINT("mbQpAutoBoost : %d\n", pRateCtrl->mbQpAutoBoost);
	}

	if (pPreProcCfg) {
		DBG_LOG_PRINT("H264EncSetPreProcessing#\n");
		DBG_LOG_PRINT("origWidth : %d\n", pPreProcCfg->origWidth);
		DBG_LOG_PRINT("origHeight : %d\n", pPreProcCfg->origHeight);
		DBG_LOG_PRINT("xOffset : %d\n", pPreProcCfg->xOffset);
		DBG_LOG_PRINT("yOffset : %d\n", pPreProcCfg->yOffset);
		DBG_LOG_PRINT("inputType : %d\n", pPreProcCfg->inputType);
		DBG_LOG_PRINT("rotation : %d\n", pPreProcCfg->rotation);
		DBG_LOG_PRINT("videoStabilization : %d\n", pPreProcCfg->videoStabilization);
		DBG_LOG_PRINT("colorConversion.type : %d\n", pPreProcCfg->colorConversion.type);
		DBG_LOG_PRINT("colorConversion.coeffA : %d\n", pPreProcCfg->colorConversion.coeffA);
		DBG_LOG_PRINT("colorConversion.coeffB : %d\n", pPreProcCfg->colorConversion.coeffB);
		DBG_LOG_PRINT("colorConversion.coeffC : %d\n", pPreProcCfg->colorConversion.coeffC);
		DBG_LOG_PRINT("colorConversion.coeffE : %d\n", pPreProcCfg->colorConversion.coeffE);
		DBG_LOG_PRINT("colorConversion.coeffF : %d\n", pPreProcCfg->colorConversion.coeffF);
		DBG_LOG_PRINT("scaledOutput : %d\n", pPreProcCfg->scaledOutput);
		DBG_LOG_PRINT("interlacedFrame : %d\n", pPreProcCfg->interlacedFrame);
	}
	DBG_LOG_PRINT("--------------------------------\n");

	return 0;
}

int init_dft_h264enc_parm(struct dbg_h264_parm *h264_parm)
{
	if (NULL == h264_parm)
		return -1;
	h264_parm->videostab = 0;
	h264_parm->inputtype = DBG_YUV420_SEMIPLANAR;
	h264_parm->ratenum = 30;

	h264_parm->enableCabac = 1;
	h264_parm->transform8x8Mode = 2;
	h264_parm->quarterPixelMv = 2;
	h264_parm->pictureRc = 1;
	h264_parm->mbRc = 0;
	h264_parm->pictureSkip = 0;
	h264_parm->qpHdr = -1;
	h264_parm->bps = 2*1024*1024;
	h264_parm->hrd = 0;
	h264_parm->gopLen = 30;
	h264_parm->intraQpDelta = 0;
	h264_parm->mbQpAdjustment = 0;
	h264_parm->mbQpAutoBoost = 0;
	h264_parm->rotation = H264ENC_ROTATE_0;
	h264_parm->gdr = 0;
	h264_parm->roi = 0;

	return 0;
}

void *init_h264enc_eniv(struct dbg_h264_parm *h264_parm)
{
	struct dbg_h264_encoder *encoder = NULL;
	H264EncRet ret;
	H264EncConfig cfg;
	H264EncCodingCtrl codingCfg;
	H264EncRateCtrl rcCfg;
	H264EncPreProcessingCfg preProcCfg;
	unsigned int rotation;

	if (NULL == h264_parm)
		return NULL;

	encoder = (struct dbg_h264_encoder *)calloc(1, sizeof(*encoder));
	if (!encoder)
		return encoder;

	switch (h264_parm->inputtype) {
	case DBG_YUV420_PLANAR:
		encoder->input_type = H264ENC_YUV420_PLANAR;
		break;
	case DBG_YUV420_SEMIPLANAR:
		encoder->input_type = H264ENC_YUV420_SEMIPLANAR;
		break;
	case DBG_YUV422_INTERLEAVED_YUYV:
		encoder->input_type = H264ENC_YUV422_INTERLEAVED_YUYV;
		break;
	default:
		DBG_LOG_ERR("Input pic type (%d) is not supported\n", h264_parm->inputtype);
		goto error;
	}
	encoder->ratenum = h264_parm->ratenum;
	encoder->gop = h264_parm->gopLen;
	encoder->width = h264_parm->width;
	encoder->height = h264_parm->height;
	encoder->bitsum = 0;

	rotation = h264_parm->rotation;
	switch (rotation) {
	case DBG_RATETE_90R:
		h264_parm->rotation = H264ENC_ROTATE_90R;
		break;
	case DBG_RATETE_90L:
		h264_parm->rotation = H264ENC_ROTATE_90L;
		break;
	default:
		h264_parm->rotation = H264ENC_ROTATE_0;
		break;
	}

	//print_h264enc_info();
	cfg.streamType= H264ENC_BYTE_STREAM;
	//cfg.streamType = H264ENC_NAL_UNIT_STREAM;
	cfg.viewMode = H264ENC_BASE_VIEW_SINGLE_BUFFER;
	cfg.level = H264ENC_LEVEL_4;
	if (h264_parm->videostab) {
		cfg.width = h264_parm->width - 16;
		cfg.height = h264_parm->height - 16;
	} else {
		cfg.width = h264_parm->width;
		cfg.height = h264_parm->height;
	}
	if (h264_parm->rotation) {
		int tmp = cfg.width;
		cfg.width = cfg.height;
		cfg.height = tmp;
	}
	cfg.frameRateDenom = 1;
	cfg.frameRateNum = h264_parm->ratenum;
	cfg.refFrameAmount = 1;
	cfg.scaledWidth = 0;
	cfg.scaledHeight = 0;

	ret = H264EncInit(&cfg, &encoder->encoder);
	if (H264ENC_OK != ret) {
		DBG_LOG_ERR("H264EncInit failed\n");
		goto error;
	}
	ret = H264EncGetCodingCtrl(encoder->encoder, &codingCfg);
	if (H264ENC_OK != ret) {
		DBG_LOG_ERR("H264EncGetCodingCtrl failed\n");
		goto error;
	}

	codingCfg.gdrDuration = h264_parm->gdr;
	codingCfg.sliceSize = 0;
	codingCfg.seiMessages = 0;
	codingCfg.videoFullRange = 0;
	codingCfg.constrainedIntraPrediction = 0;
	codingCfg.disableDeblockingFilter = 0;
	codingCfg.sampleAspectRatioWidth = 0;
	codingCfg.sampleAspectRatioHeight = 0;
	codingCfg.enableCabac = h264_parm->enableCabac;
	codingCfg.cabacInitIdc = 0;
	codingCfg.transform8x8Mode = h264_parm->transform8x8Mode;
	codingCfg.quarterPixelMv = h264_parm->quarterPixelMv;
	codingCfg.cirStart = 0;
	codingCfg.cirInterval = 0;
	codingCfg.intraSliceMap1 = 0;
	codingCfg.intraSliceMap2 = 0;
	codingCfg.intraSliceMap3 = 0;
	codingCfg.intraArea.enable = 0;
	if (h264_parm->roi)
		codingCfg.roi1Area.enable = 1;
	else
		codingCfg.roi1Area.enable = 0;
	if (codingCfg.roi1Area.enable) {
		codingCfg.roi1Area.top = h264_parm->top;
		codingCfg.roi1Area.left = h264_parm->left;
		codingCfg.roi1Area.bottom = h264_parm->bottom;
		codingCfg.roi1Area.right = h264_parm->right;
		codingCfg.roi1DeltaQp = -15;
	} else {
		codingCfg.roi1DeltaQp = 0;
	}
	codingCfg.roi2Area.enable = 0;
	if (codingCfg.roi2Area.enable) {
		//codingCfg.roi2Area.top = 0;
		//codingCfg.roi2Area.left = 0;
		//codingCfg.roi2Area.bottom = 0;
		//codingCfg.roi2Area.right = 0;
		codingCfg.roi2DeltaQp = -15;
	} else {
		codingCfg.roi2DeltaQp = 0;
	}
	codingCfg.adaptiveRoi = 0;
	codingCfg.adaptiveRoiColor = 0;
	codingCfg.fieldOrder = 0;
	if (codingCfg.gdrDuration) {
		codingCfg.intraArea.enable = 0;
		codingCfg.roi1Area.enable = 0;
	}
	/* keep all the other parameters default */
	ret = H264EncSetCodingCtrl(encoder->encoder, &codingCfg);
	if (H264ENC_OK != ret) {
		DBG_LOG_ERR("H264EncSetCodingCtrl failed\n");
		goto error;
	}

	ret = H264EncGetRateCtrl(encoder->encoder, &rcCfg);
	if (H264ENC_OK != ret) {
		DBG_LOG_ERR("H264EncGetRateCtrl failed\n");
		goto error;
	}

	rcCfg.pictureRc = h264_parm->pictureRc;
	rcCfg.mbRc = h264_parm->mbRc;
	rcCfg.pictureSkip = h264_parm->pictureSkip;
	rcCfg.qpHdr = h264_parm->qpHdr;
	rcCfg.qpMin = 10;
	rcCfg.qpMax = 42;
	rcCfg.bitPerSecond = h264_parm->bps;
	rcCfg.hrd = h264_parm->hrd;
	rcCfg.hrdCpbSize = 0;
	rcCfg.gopLen = h264_parm->gopLen;
	rcCfg.intraQpDelta = h264_parm->intraQpDelta;
	rcCfg.fixedIntraQp = 0;
	rcCfg.mbQpAdjustment = h264_parm->mbQpAdjustment;
	rcCfg.longTermPicRate = 15;
	rcCfg.mbQpAutoBoost = h264_parm->mbQpAutoBoost;
	/*keep all the other parameters default*/
	ret = H264EncSetRateCtrl(encoder->encoder, &rcCfg);
	if (H264ENC_OK != ret) {
		DBG_LOG_ERR("H264EncSetRateCtrl failed\n");
		goto error;
	}

	ret = H264EncGetPreProcessing(encoder->encoder, &preProcCfg);
	if (H264ENC_OK != ret) {
		DBG_LOG_ERR("H264EncGetPreProcessing failed\n");
		goto error;
	}

	if (h264_parm->videostab || h264_parm->rotation) {
		preProcCfg.origWidth = h264_parm->width;
		preProcCfg.origHeight = h264_parm->height;
	}
	//preProcCfg.xOffset = 0;
	//preProcCfg.yOffset = 0;
	preProcCfg.inputType = encoder->input_type;
	preProcCfg.rotation = h264_parm->rotation;
	if (h264_parm->videostab)
		preProcCfg.videoStabilization = 1;
	else
		preProcCfg.videoStabilization = 0;
	//preProcCfg.colorConversion.coeffA;
	//preProcCfg.colorConversion.coeffB;
	//preProcCfg.colorConversion.coeffC;
	//preProcCfg.colorConversion.coeffD;
	//preProcCfg.colorConversion.coeffE;
	//preProcCfg.colorConversion.coeffF;
	//preProcCfg.scaledOutput = 0;
	//preProcCfg.interlacedFrame = 0;

	ret = H264EncSetPreProcessing(encoder->encoder, &preProcCfg);
	if (H264ENC_OK != ret) {
		DBG_LOG_ERR("H264EncSetPreProcessing failed\n");
		goto error;
	}
	encoder->videostab = h264_parm->videostab;

	encoder->outbuf.length = cfg.width * cfg.height;

	ret = rts_isp_alloc_dma(&encoder->outbuf);
	if (ret < 0) {
		DBG_LOG_ERR("alloc output buffer failed\n");
		goto error;
	}

	print_enc_parameters(&cfg, &codingCfg, &rcCfg, &preProcCfg);

	return encoder;
error:
	if (encoder) {
		if (encoder->outbuf.vm_addr)
			rts_isp_free_dma(&encoder->outbuf);
		if (encoder->encoder)
			H264EncRelease(encoder->encoder);
		encoder->encoder = NULL;
		free(encoder);
		encoder = NULL;
	}
	return NULL;
}

int release_h264enc_eniv(void *env)
{
	struct dbg_h264_encoder *encoder = env;

	if (!encoder)
		return 0;

	if (encoder->start) {
		int ret = 0;
		H264EncIn encin;
		H264EncOut encout;

		encin.pOutBuf = encoder->outbuf.vm_addr;
		encin.busOutBuf = encoder->outbuf.phy_addr;
		encin.outBufSize = encoder->outbuf.length;
		H264EncStrmEnd(encoder->encoder, &encin, &encout);
		if (ret) {
			DBG_LOG_ERR("H264EncStrmEnd Failed\n");
		} else {
			if (encoder->pfile)
				fwrite(encin.pOutBuf,
						1,
						encout.streamSize,
						encoder->pfile);
		}
		encoder->start = 0;
	}

	if(encoder->pfile) {
		fclose(encoder->pfile);
		encoder->pfile = NULL;
	}

	if (encoder->outbuf.vm_addr)
		rts_isp_free_dma(&encoder->outbuf);
	if (encoder->encoder)
		H264EncRelease(encoder->encoder);
	encoder->encoder = NULL;
	free(encoder);
	encoder = NULL;

	return 0;
}

int encode_h264(void *env, uint32_t pict_addr, uint32_t pict_addr_stab,
		const char *savepath)
{
	struct dbg_h264_encoder *encoder = env;
	int ret = 0;
	struct timeval tv_begin;
	struct timeval tv_end;
	static uint64_t tsl = 0, tsc = 0;
	uint32_t width;
	uint32_t height;
	H264EncIn encin;
	H264EncOut encout;
	H264EncRateCtrl rcCfg;

	if (!encoder || !encoder->encoder)
		return -1;

	width = encoder->width;
	height = encoder->height;
	encin.pOutBuf = encoder->outbuf.vm_addr;
	encin.busOutBuf = encoder->outbuf.phy_addr;
	encin.outBufSize = encoder->outbuf.length;

	if (!encoder->pfile && savepath) {
		char filename[128];
		int len = strlen(savepath);
		if (savepath[len-1] == '/')
			snprintf(filename, sizeof(filename), "%sframes.h264", savepath);
		else
			snprintf(filename, sizeof(filename), "%s/frames.h264", savepath);
		encoder->pfile = fopen(filename, "wb");
	}

	if (!encoder->start) {
		ret = H264EncStrmStart(encoder->encoder, &encin, &encout);
		if (ret) {
			DBG_LOG_ERR("H264EncStrmStart Failed\n");
			return -1;
		}
		encoder->start = 1;
		if (encoder->pfile)
			fwrite(encin.pOutBuf, 1,
					encout.streamSize, encoder->pfile);
	}

	switch (encoder->input_type) {
	case H264ENC_YUV420_PLANAR:
		encin.busChromaV = pict_addr + width*height + (width/2)*(height/2);
	case H264ENC_YUV420_SEMIPLANAR:
		encin.busChromaU = pict_addr + width*height;
	case H264ENC_YUV422_INTERLEAVED_YUYV:
		encin.busLuma = pict_addr;
		break;
	default:
		DBG_LOG_ERR("Pic data type (%d) is not support\n", m_input_type);
		return -1;
	}

	if (encoder->videostab) {
		if (pict_addr_stab)
			encin.busLumaStab = pict_addr_stab;
		else {
			DBG_LOG_ERR("Invalid stab buff\n");
			return -1;
		}
	}

	if (encoder->index == 0)
		encin.timeIncrement = 0;
	else
		encin.timeIncrement = 1;

#if DBG_H264_I_QP
	H264EncGetRateCtrl(encoder->encoder, &rcCfg);
#endif
	if (encoder->index % encoder->gop == 0) {
		encin.codingType = H264ENC_INTRA_FRAME;
#if DBG_H264_I_QP
		if (rcCfg.bitPerSecond <= 512 * 1024)
			rcCfg.qpMax = 30;
		else if (rcCfg.bitPerSecond <= 1024 * 1024)
			rcCfg.qpMax = 25;
		else
			rcCfg.qpMax = 20;
		rcCfg.qpMin = 10;
		rcCfg.qpHdr = -1;
#endif
	} else {
		encin.codingType = H264ENC_PREDICTED_FRAME;
#if DBG_H264_I_QP
		rcCfg.qpMax = 51;
		rcCfg.qpMin = 10;
		rcCfg.qpHdr = -1;
#endif
	}
#if DBG_H264_I_QP
	DBG_LOG_LOG("Test : change I frame's QP\n");
	H264EncSetRateCtrl(encoder->encoder, &rcCfg);
#endif

	encin.ipf = H264ENC_REFERENCE_AND_REFRESH;
	encin.ltrf = H264ENC_REFERENCE;

	if (encoder->index == 0) {
		DBG_LOG_PRINT("H264EncStrmEncode#\n");
		DBG_LOG_PRINT("busLuma : %d\n", m_encIn.busLuma);
		DBG_LOG_PRINT("busChromaU : %d\n", m_encIn.busChromaU);
		DBG_LOG_PRINT("busChromaV : %d\n", m_encIn.busChromaV);
		DBG_LOG_PRINT("timeIncrement : %d\n", m_encIn.timeIncrement);
		DBG_LOG_PRINT("pOutBuf : 0x%x\n", m_encIn.pOutBuf);
		DBG_LOG_PRINT("busOutBuf : %d\n", m_encIn.busOutBuf);
		DBG_LOG_PRINT("outBufSize : %d\n", m_encIn.outBufSize);
		DBG_LOG_PRINT("codingType : %d\n", m_encIn.codingType);
		DBG_LOG_PRINT("busLumaStab : %d\n", m_encIn.busLumaStab);
		DBG_LOG_PRINT("ipf : %d\n", m_encIn.ipf);
		DBG_LOG_PRINT("ltrf : %d\n", m_encIn.ltrf);
		DBG_LOG_PRINT("--------------------------------\n");
	}
	gettimeofday(&tv_begin, F_OK);
	ret = H264EncStrmEncode(encoder->encoder, &encin, &encout, NULL, NULL);
	gettimeofday(&tv_end, F_OK);
	tsc = tv_end.tv_sec * 1000000 + tv_end.tv_usec;
	tsl = tsc;

	if (H264ENC_FRAME_READY == ret) {
		encoder->index++;
		if (encoder->pfile)
			fwrite(encin.pOutBuf, 1,
					encout.streamSize, encoder->pfile);

		if (g_show_mask & DBG_SHOW_H264_FRAME) {
			H264EncGetRateCtrl(encoder->encoder, &rcCfg);
			DBG_LOG_LOG("%s, [%d], qp = %d, size = %d, spend %d ms, begin time [%d,%06d], end time [%d,%06d]\n",
			encout.codingType == H264ENC_INTRA_FRAME ? "I frame" : "P frame",
			encoder->index,
			rcCfg.qpHdr, encout.streamSize,
			rts_calc_timeval(tv_begin, tv_end),
			tv_begin.tv_sec, tv_begin.tv_usec,
			tv_end.tv_sec, tv_end.tv_usec);
		}
		if (g_show_mask & DBG_SHOW_H264_BPS) {
			encoder->bitsum += encout.streamSize;
			if (0 == encout.streamSize)
				DBG_LOG_OPT("skip frame %d\n", g_index);
			if (encoder->index % encoder->ratenum == 0) {
				DBG_LOG_OPT("%d\n", encoder->bitsum * 8 / 1024);
				encoder->bitsum = 0;
			}
		}
		if (g_show_mask & DBG_SHOW_QP) {
			H264EncGetRateCtrl(encoder->encoder, &rcCfg);
			fprintf(stdout, "frame\t%4d\t%s\t%4d\t%8d\n",
					encoder->index,
					encout.codingType == H264ENC_INTRA_FRAME ? "I" : "P",
					rcCfg.qpHdr,
					encout.streamSize);
		}
	}

	return ret;
}
