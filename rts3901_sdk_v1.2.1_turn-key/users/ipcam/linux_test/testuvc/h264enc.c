/*
 * @uvc_encoder.c
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
#include "testuvc.h"

/*
#define H264_DUMP_TO_FILE
#define H264_DUMP_NUM 50
*/

struct rts_mmap {
	void *addr;
	int32_t length;
};

struct dma_buffer {
	void *virt_address;
	uint32_t bus_address;
	uint32_t size;
};

struct uvc264con {
	int32_t width;
	int32_t height;
	int32_t fd;
	int32_t fps;
	int32_t gop;
	int32_t bps;
	H264EncConfig cfg;
	H264EncCodingCtrl coding_cfg;
	H264EncRateCtrl rc_cfg;
	H264EncPreProcessingCfg pre_proc_cfg;
	H264EncInst m_encoder;
	struct dma_buffer m_outbuf;
	H264EncIn m_encIn;
	H264EncOut m_encOut;
	int32_t m_stream_flag;
	int32_t frm_idx;
	int32_t m_videostab_flag;
	H264EncPictureType m_input_type;
	uint32_t inputtype;
};

static FILE *g_fp;
static uint32_t g_busOutBuf;
static uint32_t g_pOutBuf;
static uint32_t g_outBufSize;
static struct uvc264con uvc264con;

static int32_t alloc_dma_memory(void **virt_address,
				uint32_t *bus_address, uint32_t *size)
{
	int32_t ret = 0;
	struct rts_isp_dma_buffer buffer;
	uint32_t pgsize = getpagesize();

	if (virt_address == NULL || bus_address == NULL || size == NULL)
		return -1;

	buffer.length = (*size + (pgsize - 1)) & (~(pgsize - 1));
	ret = rts_isp_alloc_dma(&buffer);
	if (ret) {
		DPRINT("alloc buffer failed\n");
		return ret;
	}

	*virt_address = buffer.vm_addr;
	*bus_address = buffer.phy_addr;
	*size = buffer.length;

	return 0;
}

static int32_t free_dma_memory(void *virt_address,
			       uint32_t bus_address, uint32_t size)
{
	struct rts_isp_dma_buffer buffer;

	buffer.vm_addr = virt_address;
	buffer.phy_addr = bus_address;
	buffer.length = size;

	return rts_isp_free_dma(&buffer);
}

static int32_t release_h264enc_eniv()
{
	int32_t ret = 0;
	if (uvc264con.m_stream_flag) {
		ret = H264EncStrmEnd(uvc264con.m_encoder,
				     &uvc264con.m_encIn, &uvc264con.m_encOut);
	}
	uvc264con.m_stream_flag = 0;

	if (uvc264con.m_outbuf.size > 0 && uvc264con.m_outbuf.virt_address) {
		free_dma_memory(uvc264con.m_outbuf.virt_address,
				uvc264con.m_outbuf.bus_address,
				uvc264con.m_outbuf.size);
	}

	uvc264con.m_outbuf.virt_address = NULL;
	uvc264con.m_outbuf.bus_address = 0;
	uvc264con.m_outbuf.size = 0;

	if (uvc264con.m_encoder)
		H264EncRelease(uvc264con.m_encoder);
	uvc264con.m_encoder = NULL;
	return 0;
}

static int32_t init_h264enc_eniv()
{
	H264EncRet ret;

	switch (uvc264con.inputtype) {
	case V4L2_PIX_FMT_NV12:
		uvc264con.m_input_type = H264ENC_YUV420_SEMIPLANAR;
		break;
	case V4L2_PIX_FMT_YUYV:
		uvc264con.m_input_type = H264ENC_YUV422_INTERLEAVED_YUYV;
		break;
	default:
		DPRINT("Input pic type (%d) is not supported\n",
		       uvc264con.inputtype);
		return -1;
	}

	uvc264con.cfg.streamType = H264ENC_BYTE_STREAM;
	uvc264con.cfg.viewMode = H264ENC_BASE_VIEW_SINGLE_BUFFER;
	uvc264con.cfg.level = H264ENC_LEVEL_4;

	if (uvc264con.m_videostab_flag) {
		uvc264con.cfg.width = uvc264con.width - 16;
		uvc264con.cfg.height = uvc264con.height - 16;
	} else {
		uvc264con.cfg.width = uvc264con.width;
		uvc264con.cfg.height = uvc264con.height;
	}

	uvc264con.cfg.frameRateDenom = 1;
	uvc264con.cfg.frameRateNum = uvc264con.fps;
	uvc264con.cfg.refFrameAmount = 1;
	uvc264con.cfg.scaledWidth = 0;
	uvc264con.cfg.scaledHeight = 0;

	ret = H264EncInit(&uvc264con.cfg, &uvc264con.m_encoder);
	if (H264ENC_OK != ret) {
		DPRINT("H264EncInit failed\n");
		return ret;
	}

	ret = H264EncGetCodingCtrl(uvc264con.m_encoder, &uvc264con.coding_cfg);
	if (H264ENC_OK != ret) {
		DPRINT("H264EncGetCodingCtrl failed\n");
		goto error;
	}

	uvc264con.coding_cfg.sliceSize = 0;
	uvc264con.coding_cfg.seiMessages = 0;
	uvc264con.coding_cfg.videoFullRange = 0;
	uvc264con.coding_cfg.constrainedIntraPrediction = 0;
	uvc264con.coding_cfg.disableDeblockingFilter = 0;
	uvc264con.coding_cfg.sampleAspectRatioWidth = 0;
	uvc264con.coding_cfg.sampleAspectRatioHeight = 0;
	uvc264con.coding_cfg.enableCabac = 1;
	uvc264con.coding_cfg.cabacInitIdc = 0;
	uvc264con.coding_cfg.transform8x8Mode = 2;
	uvc264con.coding_cfg.quarterPixelMv = 2;
	uvc264con.coding_cfg.cirStart = 0;
	uvc264con.coding_cfg.cirInterval = 0;
	uvc264con.coding_cfg.intraSliceMap1 = 0;
	uvc264con.coding_cfg.intraSliceMap2 = 0;
	uvc264con.coding_cfg.intraSliceMap3 = 0;
	uvc264con.coding_cfg.intraArea.enable = 0;
	uvc264con.coding_cfg.roi1Area.enable = 0;
	uvc264con.coding_cfg.roi2Area.enable = 0;
	uvc264con.coding_cfg.roi1DeltaQp = 0;
	uvc264con.coding_cfg.roi2DeltaQp = 0;
	uvc264con.coding_cfg.adaptiveRoi = 0;
	uvc264con.coding_cfg.adaptiveRoiColor = 0;
	uvc264con.coding_cfg.fieldOrder = 0;
	/* keep all the other parameters default */
	ret = H264EncSetCodingCtrl(uvc264con.m_encoder, &uvc264con.coding_cfg);
	if (H264ENC_OK != ret) {
		DPRINT("H264EncSetCodingCtrl failed\n");
		goto error;
	}

	ret = H264EncGetRateCtrl(uvc264con.m_encoder, &uvc264con.rc_cfg);
	if (H264ENC_OK != ret) {
		DPRINT("H264EncGetRateCtrl failed\n");
		goto error;
	}

	uvc264con.rc_cfg.pictureRc = 1;
	uvc264con.rc_cfg.mbRc = 0;
	uvc264con.rc_cfg.pictureSkip = 0;
	uvc264con.rc_cfg.qpHdr = -1;
	uvc264con.rc_cfg.qpMin = 10;
	uvc264con.rc_cfg.qpMax = 51;
	if (uvc264con.bps)
		uvc264con.rc_cfg.bitPerSecond = uvc264con.bps;
	else
		uvc264con.rc_cfg.bitPerSecond = 1024 * 1024 * 2;

	uvc264con.rc_cfg.hrd = 0;
	uvc264con.rc_cfg.hrdCpbSize = 0;
	uvc264con.rc_cfg.gopLen = uvc264con.gop;
	uvc264con.gop = uvc264con.rc_cfg.gopLen;
	uvc264con.rc_cfg.intraQpDelta = -12;
	uvc264con.rc_cfg.fixedIntraQp = 0;
	uvc264con.rc_cfg.mbQpAdjustment = 0;
	uvc264con.rc_cfg.longTermPicRate = 15;
	uvc264con.rc_cfg.mbQpAutoBoost = 0;

	/*keep all the other parameters default */
	ret = H264EncSetRateCtrl(uvc264con.m_encoder, &uvc264con.rc_cfg);
	if (H264ENC_OK != ret) {
		DPRINT("H264EncSetRateCtrl failed\n");
		goto error;
	}

	ret =
	    H264EncGetPreProcessing(uvc264con.m_encoder,
				    &uvc264con.pre_proc_cfg);
	if (H264ENC_OK != ret) {
		DPRINT("H264EncGetPreProcessing failed\n");
		goto error;
	}

	if (uvc264con.m_videostab_flag) {
		uvc264con.pre_proc_cfg.origWidth = uvc264con.width;
		uvc264con.pre_proc_cfg.origHeight = uvc264con.height;
	}

	uvc264con.pre_proc_cfg.inputType = uvc264con.m_input_type;
	uvc264con.pre_proc_cfg.rotation = H264ENC_ROTATE_0;
	if (uvc264con.m_videostab_flag)
		uvc264con.pre_proc_cfg.videoStabilization = 1;
	else
		uvc264con.pre_proc_cfg.videoStabilization = 0;

	ret =
	    H264EncSetPreProcessing(uvc264con.m_encoder,
				    &uvc264con.pre_proc_cfg);
	if (H264ENC_OK != ret) {
		DPRINT("H264EncSetPreProcessing failed\n");
		goto error;
	}

	uvc264con.m_outbuf.size = uvc264con.cfg.width * uvc264con.cfg.height;
	ret = alloc_dma_memory(&uvc264con.m_outbuf.virt_address,
			       &uvc264con.m_outbuf.bus_address,
			       &uvc264con.m_outbuf.size);
	if (ret < 0) {
		DPRINT("alloc output buffer failed\n");
		goto error;
	}

	g_busOutBuf = uvc264con.m_outbuf.bus_address;
	g_pOutBuf = uvc264con.m_outbuf.virt_address;
	g_outBufSize = uvc264con.m_outbuf.size;

	uvc264con.m_encIn.pOutBuf = uvc264con.m_outbuf.virt_address;
	uvc264con.m_encIn.busOutBuf = uvc264con.m_outbuf.bus_address;
	uvc264con.m_encIn.outBufSize = uvc264con.m_outbuf.size;

	DPRINT("uvc264con.m_outbuf.virt_address = 0x%08x\n",
	       uvc264con.m_outbuf.virt_address);
	DPRINT("uvc264con.m_outbuf.bus_address = 0x%08x\n",
	       uvc264con.m_outbuf.bus_address);
	DPRINT("uvc264con.m_outbuf.size = 0x%08x\n", uvc264con.m_outbuf.size);
	DPRINT("exit %s\n", __func__);
	return 0;
error:
	release_h264enc_eniv(&uvc264con);
	return ret;
}

static int32_t encode_h264_start()
{
	int32_t ret = 0;
	uvc264con.frm_idx = 0;

	ret = H264EncStrmStart(uvc264con.m_encoder,
			       &uvc264con.m_encIn, &uvc264con.m_encOut);
	if (ret) {
		DPRINT("H264EncStrmStart Failed\n");
		return -1;
	}
	uvc264con.m_stream_flag = 1;
	DPRINT("H264EncStrmStart\n");

	return 0;
}

static int32_t encode_h264_end()
{
	int32_t ret = 0;

	if (uvc264con.m_stream_flag) {
		ret = H264EncStrmEnd(uvc264con.m_encoder,
				     &uvc264con.m_encIn, &uvc264con.m_encOut);
		if (ret) {
			DPRINT("H264EncStrmEnd Failed\n");
			return -1;
		}
	}
	uvc264con.m_stream_flag = 0;
	DPRINT("encode_h264_end\n");

	return 0;
}

static int32_t encode_h264(uint32_t pict_bus_addr,
			   uint32_t pict_bus_addr_stab,
			   uint8_t **outdata, int32_t *outlen)
{
	int32_t ret = 0;

	switch (uvc264con.m_input_type) {
	case H264ENC_YUV420_PLANAR:
		uvc264con.m_encIn.busChromaV = pict_bus_addr +
		    uvc264con.width * uvc264con.height +
		    (uvc264con.width / 2) * (uvc264con.height / 2);
	case H264ENC_YUV420_SEMIPLANAR:
		uvc264con.m_encIn.busChromaU = pict_bus_addr +
		    uvc264con.width * uvc264con.height;
	case H264ENC_YUV422_INTERLEAVED_YUYV:
		uvc264con.m_encIn.busLuma = pict_bus_addr;
		break;
	default:
		DPRINT("Pic data type (%d) is not support\n",
		       uvc264con.m_input_type);
		return -1;
	}

	if (uvc264con.m_videostab_flag) {
		if (pict_bus_addr_stab)
			uvc264con.m_encIn.busLumaStab = pict_bus_addr_stab;
		else {
			DPRINT("Invalid stab buff\n");
			return -1;
		}
	}

	if (uvc264con.frm_idx == 0) {
		uvc264con.m_encIn.timeIncrement = 0;
		/*m_encIn.codingType = H264ENC_INTRA_FRAME; */
	} else {
		uvc264con.m_encIn.timeIncrement = 1;
		/*m_encIn.codingType = H264ENC_PREDICTED_FRAME; */
	}

	uvc264con.m_encIn.busOutBuf = g_busOutBuf + 48;
	uvc264con.m_encIn.pOutBuf = g_pOutBuf + 48;
	uvc264con.m_encIn.outBufSize = g_outBufSize - 48;

	if (uvc264con.frm_idx % uvc264con.gop == 0)
		uvc264con.m_encIn.codingType = H264ENC_INTRA_FRAME;
	else
		uvc264con.m_encIn.codingType = H264ENC_PREDICTED_FRAME;

	uvc264con.m_encIn.ipf = H264ENC_REFERENCE_AND_REFRESH;
	uvc264con.m_encIn.ltrf = H264ENC_REFERENCE;

	ret = H264EncStrmEncode(uvc264con.m_encoder,
				&uvc264con.m_encIn,
				&uvc264con.m_encOut, NULL, NULL);

	if (uvc264con.frm_idx % uvc264con.gop == 0) {
		*outdata = g_pOutBuf;
		*outlen = uvc264con.m_encOut.streamSize + 48;
	} else {
		*outdata = g_pOutBuf + 48;
		*outlen = uvc264con.m_encOut.streamSize;
	}

	if (H264ENC_FRAME_READY == ret) {
		uvc264con.frm_idx++;

#ifdef H264_DUMP_TO_FILE
		if (uvc264con.frm_idx < H264_DUMP_NUM) {
			if (g_fp) {
				DPRINT(" writing stream data  %d ....\n",
				       uvc264con.frm_idx - 1);
				fwrite(*outdata, 1, *outlen, g_fp);
			}
		} else {
			if (g_fp) {
				fclose(g_fp);
				g_fp = NULL;
			}
		}
#endif
	}

	return ret;
}

int32_t uvc_h264enc_init(int32_t width, int32_t height, int32_t fps)
{
	int32_t ret = 0;

	uvc264con.width = width;
	uvc264con.height = height;
	uvc264con.fps = fps;
	uvc264con.inputtype = V4L2_PIX_FMT_NV12;
	uvc264con.gop = fps;
	uvc264con.bps = 524288;

	DPRINT("h264 encode init, width = %d\n", uvc264con.width);
	DPRINT("h264 encode init, height = %d\n", uvc264con.height);
	DPRINT("h264 encode init, input_type = 0x%08x\n", uvc264con.inputtype);
	DPRINT("h264 encode init, gop = %d\n", uvc264con.gop);

	uvc264con.fd = rts_isp_ctrl_open();
	if (uvc264con.fd < -1) {
		DPRINT("open memalloc device failed\n");
		return -1;
	}

	uvc264con.m_encoder = NULL;
	uvc264con.m_outbuf.virt_address = NULL;
	uvc264con.m_outbuf.bus_address = 0;
	uvc264con.m_outbuf.size = 0;
	uvc264con.m_stream_flag = 0;
	uvc264con.frm_idx = 0;
	uvc264con.m_videostab_flag = 0;

	ret = init_h264enc_eniv();
	if (ret) {
		DPRINT("init_h264enc_eniv failed\n");
		goto error;
	}
#ifdef H264_DUMP_TO_FILE
	g_fp = fopen("/h264.raw", "wb");
#endif
	encode_h264_start();
	return ret;
error:
	if (uvc264con.fd > 0)
		rts_isp_ctrl_close(uvc264con.fd);
	uvc264con.fd = -1;
	DPRINT("exit uvc init failed\n");
	return ret;
}

int32_t uvc_h264enc_frame(uint8_t *fyuv,
			  uint8_t **f264buf, uint32_t *f264len)
{
	int32_t ret = 0;

	if (uvc264con.inputtype == V4L2_PIX_FMT_NV12) {
		unsigned long phy_addr;

		phy_addr = rts_isp_get_video_phy_addr(uvc264con.fd, (unsigned long)fyuv);

		if (phy_addr)
			encode_h264(phy_addr, 0, f264buf, f264len);
		else
			DPRINT("Get Buf bus addr failed\n");
	}

	return 0;
}

int32_t uvc_h264enc_close()
{
	int32_t ret = 0;
#ifdef H264_DUMP_TO_FILE
	if (g_fp) {
		fclose(g_fp);
		g_fp = NULL;
	}
#endif
	encode_h264_end(&uvc264con);
	release_h264enc_eniv(&uvc264con);
	if (uvc264con.fd > 0)
		rts_isp_ctrl_close(uvc264con.fd);
	uvc264con.fd = -1;
	return ret;
}

uint32_t getoutbufstart()
{
	return g_pOutBuf;
}

uint32_t getoutbufsize()
{
	return g_outBufSize;
}
