#ifndef _RTS_DBG_H264_H
#define _RTS_DBG_H264_H

#include <stdint.h>

enum dbg_pic_input_type {
	DBG_YUV420_PLANAR = 0,
	DBG_YUV420_SEMIPLANAR = 1,
	DBG_YUV420_SEMIPLANAR_VU = 2,
	DBG_YUV422_INTERLEAVED_YUYV = 3,
	DBG_YUV422_INTERLEAVED_UYVY = 4,
};

struct dbg_h264_parm {
	int videostab;
	enum dbg_pic_input_type inputtype;
	int width;
	int height;
	int ratenum;

	unsigned int enableCabac;
	unsigned int transform8x8Mode;
	unsigned int quarterPixelMv;
	unsigned int pictureRc;
	unsigned int mbRc;
	unsigned int pictureSkip;
	int qpHdr;
	unsigned int bps;
	unsigned int hrd;
	unsigned int gopLen;
	int intraQpDelta;
	int mbQpAdjustment;
	unsigned int mbQpAutoBoost;
	unsigned int rotation;
	unsigned int gdr;
	int roi;
	uint32_t left;
	uint32_t top;
	uint32_t right;
	uint32_t bottom;
};

int init_dft_h264enc_parm(struct dbg_h264_parm *h264_parm);

void *init_h264enc_eniv(struct dbg_h264_parm *h264_parm);
int release_h264enc_eniv(void *env);
int encode_h264(void *env, uint32_t pict_addr, uint32_t pict_addr_stab,
		const char *savepath);

#endif
