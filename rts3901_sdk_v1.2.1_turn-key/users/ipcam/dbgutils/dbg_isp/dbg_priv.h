#ifndef _RTS_DBG_PRIV_H_
#define _RTS_DBG_PRIV_H_

#include <unistd.h>
#include <sys/time.h>
#include "list.h"
#include "dbg_opt.h"

extern int g_iMainexit;
extern uint32_t g_show_mask;

enum rts_dbg_rotation {
	DBG_RATETE_0,
	DBG_RATETE_90R,
	DBG_RATETE_90L
};

struct rts_dbg_option {
	char devname[64];
	char savepath[128];
	int saveflag;
	int bmpflag;
	int yuvflag;
	int framenum;
	int previewnum;
	int h264flag;
	int h264stab;
	unsigned int h264bps;
	struct list_head cmd_list;
	void *priv;
	unsigned int gdr;
	int intraQpDelta;
	unsigned int rotation;
	int mjpgflag;
	int roi;
	uint32_t left;
	uint32_t top;
	uint32_t right;
	uint32_t bottom;
};

#define DBG_SHOW_FRAME_TIME		(1<<0)
#define DBG_SHOW_H264_FRAME		(1<<1)
#define DBG_SHOW_H264_BPS		(1<<2)
#define DBG_SHOW_MJPEG			(1<<3)
#define DBG_SHOW_BMP			(1<<4)
#define DBG_SHOW_QP			(1<<5)

#endif
