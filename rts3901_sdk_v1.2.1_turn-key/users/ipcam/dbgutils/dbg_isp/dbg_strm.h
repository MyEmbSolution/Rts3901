#ifndef _RTS_DBG_STRM_H_
#define _RTS_DBG_STRM_H_

#include <stdint.h>
#include "dbg_priv.h"

extern int g_max_preview_num;

int list_stream_info(int fd);

int get_fmt(int fd, uint32_t *pixelformat, uint32_t *width, uint32_t *height);
int get_fps(int fd, uint32_t *numerator, uint32_t *denominator);

int get_pixelformat(int fd, int index, uint32_t *pixelformat);
int set_fmt(int fd, uint32_t pixelformat, uint32_t width, uint32_t height);

int set_fps(int fd, uint32_t numerator, uint32_t denominator);

int switch_td(int fd, int enable);

int preview_func(int fd, const struct rts_dbg_option *option);
#endif
