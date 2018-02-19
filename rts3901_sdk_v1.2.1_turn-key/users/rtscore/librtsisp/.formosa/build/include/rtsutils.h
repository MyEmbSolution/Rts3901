/*
 * Realtek Semiconductor Corp.
 *
 * libs/include/rtsutils.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _LIBS_INCLUDE_RTSUTILS_H
#define _LIBS_INCLUDE_RTSUTILS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define RTS_RAW_HEADER_LENGTH		128

struct rts_raw_t {
	uint8_t header[RTS_RAW_HEADER_LENGTH];
	uint32_t length;
	uint8_t *pdata;
	uint32_t width;
	uint32_t height;
	uint32_t fmt;
	unsigned int snr_fmt;
	unsigned int raw_fmt;
	int streamid;
	int fd;
	int initialized;
};

/* deprecated */
int rts_get_raw(unsigned int raw_fmt, struct rts_raw_t **ppraw);
/* deprecated */
void rts_put_raw(struct rts_raw_t *praw);
int rts_init_raw(unsigned int raw_fmt, struct rts_raw_t **ppraw);
void rts_release_raw(struct rts_raw_t *praw);
int rts_get_raw_frame(struct rts_raw_t *praw);
int rts_update_raw_fmt(unsigned int raw_fmt, struct rts_raw_t *praw);

typedef const void *RtsBmpInst;

int rts_bmp_init_env(RtsBmpInst *pinst, uint32_t width, uint32_t height);
int rts_bmp_release_env(RtsBmpInst inst);
int rts_bmp_encode_yuv(RtsBmpInst inst, uint8_t *pyuv, uint32_t pixelfmt,
		const char *filename);

#ifdef __cplusplus
}
#endif
#endif
