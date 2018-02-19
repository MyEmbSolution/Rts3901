/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_h264.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"
#include <rtsisp.h>
#include <h264encapi.h>

#include "tester.h"


struct dma_buffer {
	void *vm_addr;
	uint32_t phy_addr;
	uint32_t size;
};

enum dbg_pic_input_type {
	FT2_YUV420_PLANAR = 0,
	FT2_YUV420_SEMIPLANAR = 1,
	FT2_YUV420_SEMIPLANAR_VU = 2,
	FT2_YUV422_INTERLEAVED_YUYV = 3,
	FT2_YUV422_INTERLEAVED_UYVY = 4,
};

struct h264_parm {
	int videostab;
	enum dbg_pic_input_type intputtype;
	int width;
	int height;
	int ratenum;

	unsigned int bps;
};

struct h264_inst {
	H264EncInst m_encoder;
	int m_fd;
	struct dma_buffer m_inbuf;
	struct dma_buffer m_outbuf;
	H264EncIn m_encIn;
	H264EncOut m_encOut;
	int streamping;
	int m_videostab_flag;
	struct dma_buffer m_stabbuf;
	int m_width;
	int m_height;
	int m_ratenum;
	H264EncPictureType m_input_type;

	int gop_len;
	int frame_index;

	FILE *pfile;
};

struct h264_test_mapping {
	char name[16];
	struct h264_parm parm;
};

struct h264_test_mapping h264_mapping_info[] = {
	{"1080pyuv420", {0, FT2_YUV420_SEMIPLANAR, 1920, 1080, 30, 2097152}},
	{"720pyuv420", {0, FT2_YUV420_SEMIPLANAR, 1280, 720, 30, 1048576}}
};

static int get_mapping_index(char *content)
{
	int i = 0;
	int size = sizeof(h264_mapping_info)/sizeof(struct h264_test_mapping);

	if (!content)
		return -1;

	for (i = 0; i < size; i++) {
		if (0 == strcasecmp(content, h264_mapping_info[i].name))
			return i;
	}

	return -1;
}

int ft2_h264_check(struct protocol_command *pcmd)
{
	return FT2_OK;
}

static int alloc_dma_memory(int fd, struct dma_buffer *buffer)
{
	int ret = 0;
	struct rts_isp_dma_buffer dbuf;
	uint32_t pgsize =  getpagesize();

	if (!buffer)
		return -FT2_NULL_POINT;

	if (fd <= 0) {
		FT2_LOG_ERR("memalloc device is not opened\n");
		return -FT2_ERROR;
	}

	dbuf.length = (buffer->size + (pgsize - 1)) & (~(pgsize - 1));
	buffer->size = 0;

	ret = rts_isp_mem_alloc(fd, &dbuf);
	if (ret) {
		FT2_LOG_ERR("alloc dma buffer fail\n");
		return -FT2_ERROR;
	}

	buffer->vm_addr = dbuf.vm_addr;
	buffer->phy_addr = dbuf.phy_addr;
	buffer->size = dbuf.length;

	return FT2_OK;
}

static int free_dma_memory(int fd, struct dma_buffer *buffer)
{
	struct rts_isp_dma_buffer dbuf;

	if (!buffer)
		return FT2_OK;

	if (fd <= 0) {
		FT2_LOG_ERR("memalloc device is not opened\n");
		return -FT2_ERROR;
	}

	dbuf.phy_addr = buffer->phy_addr;
	dbuf.vm_addr = buffer->vm_addr;
	dbuf.length = buffer->size;

	rts_isp_mem_free(fd, &dbuf);

	buffer->vm_addr = NULL;
	buffer->phy_addr = 0;
	buffer->size = 0;

	return FT2_OK;
}

struct h264_inst *open_h264_enc(struct h264_parm *parm)
{
	struct h264_inst *inst = NULL;
	H264EncRet ret;
	H264EncConfig cfg;
	H264EncCodingCtrl codingCfg;
	H264EncRateCtrl rcCfg;
	H264EncPreProcessingCfg preProcCfg;

	if (!parm)
		return NULL;

	inst = (struct h264_inst*)calloc(1, sizeof(struct h264_inst));
	if (!inst)
		return NULL;

	switch(parm->intputtype) {
	case FT2_YUV420_PLANAR:
		inst->m_input_type = H264ENC_YUV420_PLANAR;
		break;
	case FT2_YUV420_SEMIPLANAR:
		inst->m_input_type = H264ENC_YUV420_SEMIPLANAR;
		break;
	case FT2_YUV422_INTERLEAVED_YUYV:
		inst->m_input_type = H264ENC_YUV422_INTERLEAVED_YUYV;
		break;
	default:
		FT2_LOG_ERR("input pic type (%d) is not supported\n",
				parm->intputtype);
		goto error;
	}

	inst->m_width = parm->width;
	inst->m_height = parm->height;
	cfg.streamType= H264ENC_BYTE_STREAM;
	cfg.viewMode = H264ENC_BASE_VIEW_DOUBLE_BUFFER;
	cfg.level = H264ENC_LEVEL_4;
	if (parm->videostab) {
		cfg.width = parm->width - 16;
		cfg.height = parm->height - 16;
	} else {
		cfg.width = parm->width;
		cfg.height = parm->height;
	}

	cfg.frameRateDenom = 1;
	cfg.frameRateNum = parm->ratenum;
	cfg.refFrameAmount = 1;
	cfg.scaledWidth = 0;
	cfg.scaledHeight = 0;

	inst->m_ratenum = cfg.frameRateNum;

	ret = H264EncInit(&cfg, &inst->m_encoder);
	if (H264ENC_OK != ret) {
		FT2_LOG_ERR("H264EncInit failed\n");
		goto error;
	}
	FT2_LOG_INFO("init\n");
	usleep(1000*100);

	ret = H264EncGetCodingCtrl(inst->m_encoder, &codingCfg);
	if (H264ENC_OK != ret) {
		FT2_LOG_ERR("H264EncGetCodingCtrl failed\n");
		goto error;
	}
	codingCfg.sliceSize = 0;
	codingCfg.seiMessages = 0;
	codingCfg.videoFullRange = 0;
	codingCfg.constrainedIntraPrediction = 0;
	codingCfg.disableDeblockingFilter = 0;
	codingCfg.sampleAspectRatioWidth = 0;
	codingCfg.sampleAspectRatioHeight = 0;
	codingCfg.enableCabac = 1;
	codingCfg.cabacInitIdc = 0;
	codingCfg.transform8x8Mode = 1;
	codingCfg.quarterPixelMv = 1;
	codingCfg.cirStart = 0;
	codingCfg.cirInterval = 0;
	codingCfg.intraSliceMap1 = 0;
	codingCfg.intraSliceMap2 = 0;
	codingCfg.intraSliceMap3 = 0;
	codingCfg.intraArea.enable = 0;
	codingCfg.roi1Area.enable = 0;
	codingCfg.roi2Area.enable = 0;
	codingCfg.adaptiveRoi = 0;
	codingCfg.adaptiveRoiColor = 0;
	codingCfg.fieldOrder = 0;
	ret = H264EncSetCodingCtrl(inst->m_encoder, &codingCfg);
	if (H264ENC_OK != ret) {
		FT2_LOG_ERR("H264EncSetCodingCtrl failed\n");
		goto error;
	}
	FT2_LOG_INFO("CodingCtrl\n");
	usleep(1000*100);

	ret = H264EncGetRateCtrl(inst->m_encoder, &rcCfg);
	if (H264ENC_OK != ret) {
		FT2_LOG_ERR("H264EncGetRateCtrl failed\n");
		goto error;
	}

	rcCfg.pictureRc = 1;
	rcCfg.mbRc = 1;
	rcCfg.pictureSkip = 0;
	rcCfg.qpHdr = -1;
	rcCfg.qpMin = 10;
	rcCfg.qpMax = 51;
	rcCfg.bitPerSecond = parm->bps;
	rcCfg.hrd = 0;
	rcCfg.hrdCpbSize = 0;
	rcCfg.gopLen = parm->ratenum;
	inst->gop_len = rcCfg.gopLen;
	rcCfg.intraQpDelta = -3;
	rcCfg.fixedIntraQp = 0;
	rcCfg.mbQpAdjustment = 0;
	rcCfg.longTermPicRate = 15;
	rcCfg.mbQpAutoBoost = 0;
	/*keep all the other parameters default*/
	ret = H264EncSetRateCtrl(inst->m_encoder, &rcCfg);
	if (H264ENC_OK != ret) {
		FT2_LOG_ERR("H264EncSetRateCtrl failed\n");
		goto error;
	}
	FT2_LOG_INFO("RateCtrl\n");
	usleep(1000*100);

	ret = H264EncGetPreProcessing(inst->m_encoder, &preProcCfg);
	if (H264ENC_OK != ret) {
		FT2_LOG_ERR("H264EncGetPreProcessing failed\n");
		goto error;
	}
	preProcCfg.inputType = inst->m_input_type;
	if (parm->videostab) {
		preProcCfg.origWidth = parm->width;
		preProcCfg.origHeight = parm->height;
	}
	preProcCfg.rotation = 0;
	if (parm->videostab)
		preProcCfg.videoStabilization = 1;
	else
		preProcCfg.videoStabilization = 0;

	ret = H264EncSetPreProcessing(inst->m_encoder, &preProcCfg);
	if (H264ENC_OK != ret) {
		FT2_LOG_ERR("H264EncSetPreProcessing failed\n");
		goto error;
	}

	FT2_LOG_INFO("PreProcessing\n");
	usleep(1000*100);

	inst->m_fd = rts_isp_mem_open(O_RDWR | O_NONBLOCK);
	if (inst->m_fd < 0) {
		FT2_LOG_ERR("open memalloc device failed\n");
		goto error;
	}
	inst->m_outbuf.size = cfg.width *cfg.height;
	ret = alloc_dma_memory(inst->m_fd, &inst->m_outbuf);
	if (ret < 0) {
		FT2_LOG_ERR("alloc output buffer fail\n");
		goto error;
	}

	inst->m_encIn.pOutBuf = inst->m_outbuf.vm_addr;
	inst->m_encIn.busOutBuf = inst->m_outbuf.phy_addr;
	inst->m_encIn.outBufSize = inst->m_outbuf.size;

	inst->m_videostab_flag = parm->videostab;

	inst->pfile = NULL;

	return inst;
error:
	if (inst->m_outbuf.size > 0 && inst->m_outbuf.vm_addr)
		free_dma_memory(inst->m_fd, &inst->m_outbuf);
	if (inst->m_fd > 0)
		rts_isp_mem_close(inst->m_fd);
	inst->m_fd = -1;
	if (inst->m_encoder)
		H264EncRelease(inst->m_encoder);
	inst->m_encoder = NULL;
	free(inst);
	return NULL;
}

int stop_h264_enc(struct h264_inst *inst);
int close_h264_enc(struct h264_inst *inst, char *filename)
{
	if (!inst)
		return FT2_OK;

	stop_h264_enc(inst);

	if (inst->m_outbuf.size > 0 && inst->m_outbuf.vm_addr)
		free_dma_memory(inst->m_fd, &inst->m_outbuf);
	if (inst->m_inbuf.size > 0 && inst->m_inbuf.vm_addr)
		free_dma_memory(inst->m_fd, &inst->m_inbuf);
	if (inst->m_stabbuf.size > 0 && inst->m_stabbuf.vm_addr)
		free_dma_memory(inst->m_fd, &inst->m_stabbuf);
	if (inst->m_fd > 0)
		rts_isp_mem_close(inst->m_fd);
	inst->m_fd = -1;
	if (inst->m_encoder)
		H264EncRelease(inst->m_encoder);
	inst->m_encoder = NULL;

	if (inst->pfile) {
		fclose(inst->pfile);
		inst->pfile = fopen(filename, "r");
		if (inst->pfile == NULL)
			FT2_LOG_ERR("fail to open h264 output file\n");
		fclose(inst->pfile);
	}

	inst->pfile = NULL;

	free(inst);
	return FT2_OK;
}

static int save_h264_data(struct h264_inst *inst)
{
	if (!inst)
		return -FT2_NULL_POINT;

	if (!inst->pfile)
		return FT2_OK;

	fwrite(inst->m_encIn.pOutBuf, 1, inst->m_encOut.streamSize, inst->pfile);
}

int start_h264_enc(struct h264_inst *inst)
{
	int ret = 0;

	if (!inst)
		return -FT2_NULL_POINT;

	ret = H264EncStrmStart(inst->m_encoder, &inst->m_encIn, &inst->m_encOut);
	if (ret) {
		FT2_LOG_ERR("H264EncStrmStart fail\n");
		return -FT2_ERROR;
	}

	inst->frame_index = 0;
	inst->streamping = 1;

	save_h264_data(inst);

	return FT2_OK;
}

int stop_h264_enc(struct h264_inst *inst)
{
	int ret = 0;

	if (!inst)
		return -FT2_NULL_POINT;

	if (inst->streamping) {
		ret = H264EncStrmEnd(inst->m_encoder, &inst->m_encIn, &inst->m_encOut);
		if (ret) {
			FT2_LOG_ERR("H264EncStrmEnd fail\n");
			return -FT2_ERROR;
		}
		save_h264_data(inst);
	}
	inst->streamping = 0;

	return FT2_OK;
}

int proc_h264_enc(struct h264_inst *inst)
{
	int ret = 0;

	if (!inst)
		return -FT2_NULL_POINT;

	switch(inst->m_input_type) {
	case H264ENC_YUV420_PLANAR:
		inst->m_encIn.busChromaV = inst->m_inbuf.phy_addr +
			inst->m_width * inst->m_height +
			(inst->m_width/2)*(inst->m_height/2);
	case H264ENC_YUV420_SEMIPLANAR:
		inst->m_encIn.busLuma = inst->m_inbuf.phy_addr;
		inst->m_encIn.busChromaU = inst->m_inbuf.phy_addr +
			inst->m_width * inst->m_height;
		inst->m_encIn.busChromaV = inst->m_inbuf.phy_addr +
			inst->m_width * inst->m_height +
			(inst->m_width / 2)  * (inst->m_height / 2);
	case H264ENC_YUV422_INTERLEAVED_YUYV:
		inst->m_encIn.busLuma = inst->m_inbuf.phy_addr;
		break;
	default:
		FT2_LOG_ERR("pic data type (%d) is not support\n",
				inst->m_input_type);
		return -FT2_ERROR;
	}

	if (inst->m_videostab_flag) {
		inst->m_encIn.busLumaStab = inst->m_stabbuf.phy_addr;
	}

	if (inst->frame_index == 0) {
		inst->m_encIn.timeIncrement = 0;
	} else {
		inst->m_encIn.timeIncrement = 1;
	}

	if (inst->frame_index % inst->gop_len == 0)
		inst->m_encIn.codingType = H264ENC_INTRA_FRAME;
	else
		inst->m_encIn.codingType = H264ENC_PREDICTED_FRAME;

	inst->m_encIn.ipf = H264ENC_REFERENCE_AND_REFRESH;
	inst->m_encIn.ltrf = H264ENC_REFERENCE;

	ret = H264EncStrmEncode(inst->m_encoder, &inst->m_encIn, &inst->m_encOut, NULL, NULL);

	if (ret == H264ENC_FRAME_READY) {
		inst->frame_index++;
		save_h264_data(inst);
	}

	return FT2_OK;
}

static int get_file_size(char *filename)
{
	struct stat stat_buf;

	if (0 > stat(filename, &stat_buf))
		return -1;

	return (int) stat_buf.st_size;
}

static int read_h264_test_input(struct h264_inst *inst, char *path, int index, unsigned int  picsize)
{
	char filename[128];
	FILE *pfile = NULL;
	int filesize = 0;

	if (!inst)
		return -FT2_NULL_POINT;

	if (!path || strlen(path) == 0)
		return -FT2_ERROR;

	if (inst->m_inbuf.size < picsize || NULL == inst->m_inbuf.vm_addr) {
		FT2_LOG_ERR("input buf is not alloced or not big enough\n");
		return -FT2_NO_MEMORY;
	}

	FT2_LOG_INFO("%s\n", __FUNCTION__);

	/* warm test and ft2 test read from different input file */
	if (test_forever) {
		while ((h264_count +5) > pic_count || pic_count < 6)
			usleep(10000);

		FT2_LOG_INFO("h264_count=%d, pic_count=%d\n", h264_count, pic_count);

		switch (index) {
		case 0:
			snprintf(filename, sizeof(filename),
				"%s/isp/1080pyuv420.output%d", path, h264_count);
			break;
		case 1:
			snprintf(filename, sizeof(filename),
                                "%s/isp/720pyuv420.output%d", path, h264_count);
                        break;
		default:
			break;
		}

		h264_count++;
	} else {
		switch (index) {
		case 0:
			snprintf(filename, sizeof(filename),
				"%s/h264/1080pyuv420.input", path);
			break;
		case 1:
			snprintf(filename, sizeof(filename),
                                "%s/h264/720pyuv420.input", path);
			break;
		default:
			break;
		}
	}

	filesize = get_file_size(filename);
	if (filesize < picsize) {
		FT2_LOG_ERR("h264 input file size is not correct, input size is %d, but wanted %d\n",
				filesize, picsize);
		return -FT2_ERROR;
	}

	pfile = fopen(filename, "r");
	if (!pfile) {
		FT2_LOG_ERR("open input file %s fail\n", filename);
		return -FT2_ERROR;
	}

	fread(inst->m_inbuf.vm_addr, 1, picsize, pfile);
	fclose(pfile);

	return FT2_OK;
}

static int h264_check_path(char *path)
{
	char dirpath[128];
	char filename[128];

	if (!path || strlen(path) == 0)
		return -FT2_ERROR;

	snprintf(dirpath, sizeof(dirpath), "%s/h264", path);
	if (0 > access(path, R_OK | W_OK)) {
		FT2_LOG_ERR("%s is not exist or no right to rw\n", path);
		return -FT2_ERROR;
	}

	if (0 > access(dirpath, F_OK)) {
		if (0 > mkdir(dirpath, 0777)) {
			FT2_LOG_ERR("fail to mkdir %s\n", dirpath);
			return -FT2_ERROR;
		}
	}

	snprintf(filename, sizeof(filename), "%s/720pyuv420.input", dirpath);
	if (0 > access(filename, R_OK)) {
		FT2_LOG_ERR("input %s is not exist\n", filename);
		return -FT2_ERROR;
	}

	snprintf(filename, sizeof(filename), "%s/1080pyuv420.input", dirpath);
	if (0 > access(filename, R_OK)) {
		FT2_LOG_ERR("input %s is not exist\n", filename);
		return -FT2_ERROR;
	}

	snprintf(filename, sizeof(filename), "%s/720p.golden", dirpath);
        if (0 > access(filename, R_OK)) {
                FT2_LOG_ERR("golden %s is not exist\n", filename);
                return -FT2_ERROR;
        }

	snprintf(filename, sizeof(filename), "%s/1080p.golden", dirpath);
        if (0 > access(filename, R_OK)) {
                FT2_LOG_ERR("golden %s is not exist\n", filename);
                return -FT2_ERROR;
        }

	return FT2_OK;
}

int ft2_h264_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	struct h264_parm parm;
	struct h264_inst *inst = NULL;
	int i = 0;
	int ret = 0;
	char filename[128];
	unsigned int picsize = 0;
	int index = 0;
	struct protocol_param *parameter = NULL;

	if (!pcmd)
		return -FT2_NULL_POINT;

	if (h264_check_path(path)) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	parameter = pcmd->parameter;
	index = get_mapping_index(parameter->content);
	if (index < 0) {
		FT2_LOG_ERR("can't find correct mapping\n");
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	FT2_LOG_INFO("open_h264_enc\n");
	parm = h264_mapping_info[index].parm;
	inst = open_h264_enc(&parm);
	if (!inst) {
		FT2_LOG_ERR("open h264 encode fail\n");
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	switch (index) {
        case 0:
                snprintf(filename, sizeof(filename),
                        "%s/h264/1080p.output", path);
                break;
        case 1:
                snprintf(filename, sizeof(filename),
                        "%s/h264/720p.output", path);
                break;
        default:
                break;
        }

	if (test_forever)
		snprintf(filename, sizeof(filename), "%s%d", filename, h264_count);

	inst->pfile = fopen(filename, "wb");
	if (inst->pfile == NULL) {
		FT2_LOG_ERR("fail to create h264 output file\n");
		goto error;
	}

	picsize = inst->m_width * inst->m_height * 3 / 2;
	inst->m_inbuf.size = picsize;

	ret = alloc_dma_memory(inst->m_fd, &inst->m_inbuf);
	if (ret) {
		FT2_LOG_ERR("alloc input buffer fail\n");
		goto error;
	}

	FT2_LOG_INFO("start h264 encoder\n");
	ret = start_h264_enc(inst);
	if (ret) {
		FT2_LOG_ERR("start h264 encode fail\n");
		goto error;
	}

	ret = read_h264_test_input(inst, path, index, picsize);
	if (ret) {
		FT2_LOG_ERR("read input file fail\n");
		goto error;
	}

	FT2_LOG_INFO("encode frame\n");
	ret = proc_h264_enc(inst);
	if (ret) {
		FT2_LOG_ERR("encode frame fail\n");
		goto error;
	}

	FT2_LOG_INFO("stop h264 encoder\n");
	ret = stop_h264_enc(inst);
	if (ret) {
		FT2_LOG_ERR("stop h264 encoder fail\n");
		goto error;
	}

	close_h264_enc(inst, filename);
	inst = NULL;

	append_pt_command_result(pcmd,
		FT2_TEST_PASS, strlen(FT2_TEST_PASS));

	return FT2_OK;
error:
	append_pt_command_result(pcmd,
		FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
	close_h264_enc(inst, filename);
	inst = NULL;
	return -FT2_ERROR;
}

void ft2_h264_cleanup(void *priv)
{
	h264_count = 0;
}
