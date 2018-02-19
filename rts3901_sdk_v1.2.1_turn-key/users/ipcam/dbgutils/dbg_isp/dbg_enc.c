#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <rtsisp.h>

#include "dbg_log.h"
#include "dbg_opt.h"
#include "dbg_h264.h"
#include "dbg_priv.h"

#define VAL_OPT_LOG_SHOW			0x11
#define VAL_OPT_IPT_SIZE			0x12
#define VAL_OPT_GDR				0x14
#define VAL_OPT_LOG_SHOW_H264			0x24
#define VAL_OPT_LOG_SHOW_BPS			0x25
#define VAL_OPT_MAX_NUM				0x26
#define VAL_OPT_LOG_SHOW_QP			0x27

extern uint32_t g_show_mask;

struct option longopts[] = {
	{"size", required_argument, NULL, VAL_OPT_IPT_SIZE},
	{"input", required_argument, NULL, 'i'},
	{"output", required_argument, NULL, 'o'},
	{"fps", required_argument, NULL, 'r'},
	{"bps", required_argument, NULL, 'b'},
	{"show", no_argument, NULL, VAL_OPT_LOG_SHOW},
	{"fmt", required_argument, NULL, 'f'},
	{"quarterPixelMv", required_argument, NULL, 'M'},
	{"trans8x8", required_argument, NULL, '8'},
	{"enableCabac", required_argument, NULL, 'K'},
	{"intraQpDelta", required_argument, NULL, 'A'},
	{"picRc", required_argument, NULL, 'U'},
	{"mbRc", required_argument, NULL, 'u'},
	{"mbQpAdjustment", required_argument, NULL, '2'},
	{"mbQpAutoBoost", required_argument, NULL, '4'},
	{"picSkip", required_argument, NULL, 's'},
	{"rotation", required_argument, NULL, 'R'},
	{"gdr", required_argument, NULL, VAL_OPT_GDR},
	{"show-h264", no_argument, NULL, VAL_OPT_LOG_SHOW_H264},
	{"show-bps", no_argument, NULL, VAL_OPT_LOG_SHOW_BPS},
	{"show-qp", no_argument, NULL, VAL_OPT_LOG_SHOW_QP},
	{"number", required_argument, NULL, VAL_OPT_MAX_NUM},
};

int g_iMainexit = 0;
static void Termination(int sign)
{
	DBG_LOG_PRINT("dbg_isp terminated\n");
	g_iMainexit = 1;
}

int main(int argc, char *argv[])
{
	int width = 1280;
	int height = 720;
	int size;
	int filesize;
	char filename[128];
	char inpath[64];
	char outpath[64];
	int saveflag = 0;
	int c = 0;
	FILE *pFile = NULL;
	int ret;
	int i;
	int fps = 30;
	int looptime = 1000/fps;
	struct timeval tv_begin;
	struct timeval tv_end;
	int deltatime = 0;
	int framenum = 0;
	int FRAMENUM = 0;
	int encoded_count = 0;
	uint32_t total_size = 0;
	int bitpersecond = 524288;
	enum dbg_pic_input_type inputtype = DBG_YUV420_SEMIPLANAR;
	struct dbg_h264_parm h264_parm;
	void *h264enc = NULL;
	struct rts_isp_dma_buffer buffer;

	g_iMainexit = 0;
	signal(SIGINT, Termination);
	signal(SIGTERM, Termination);
	signal(SIGQUIT, Termination);
	signal(SIGTSTP, Termination);
	signal(SIGPIPE, SIG_IGN);

	init_dft_h264enc_parm(&h264_parm);

	snprintf(inpath, sizeof(inpath), "/tmp/");
	snprintf(outpath, sizeof(outpath), "/tmp/");
	while((c = getopt_long(argc, argv, ":i:o:r:b:f:M:8:K:A:U:u:2:4:s:R:", longopts, NULL)) != -1) {
		switch (c) {
		case VAL_OPT_IPT_SIZE:
			sscanf(optarg, "%dx%d", &width, &height);
			break;
		case 'i':
			if (get_file_size(optarg) <= 0)
				DBG_LOG_ERR("%s is not valid input path\n", optarg);
			else
				snprintf(inpath, sizeof(inpath), "%s", optarg);
			break;
		case 'o':
			if (check_path(optarg)) {
				DBG_LOG_ERR("%s is not valid output path\n", optarg);
			} else {
				int len = 0;
				snprintf(outpath, sizeof(outpath),
					"%s", optarg);
				saveflag = 1;
				len = strlen(outpath);
				if (len > 0) {
					if ('/' != outpath[len - 1]) {
						outpath[len] = '/';
						outpath[len+1] = '\0';
					}
				}
			}
			break;
		case 'r':
			fps = (int)strtol(optarg, NULL, 0);
			break;
		case 'b':
			bitpersecond = (int)strtol(optarg, NULL, 0);
			break;
		case 'f':
			ret = (int)strtol(optarg, NULL, 0);
			if (DBG_YUV422_INTERLEAVED_YUYV == ret)
				inputtype = DBG_YUV422_INTERLEAVED_YUYV;
			else if (DBG_YUV420_PLANAR == ret)
				inputtype = DBG_YUV420_PLANAR;
			else
				inputtype = DBG_YUV420_SEMIPLANAR;
			break;
		case 'M':
			h264_parm.quarterPixelMv = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case '8':
			h264_parm.transform8x8Mode = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case 'K':
			h264_parm.enableCabac = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case 'A':
			h264_parm.intraQpDelta = (int)strtol(optarg, NULL, 0);
			break;
		case 'U':
			h264_parm.pictureRc = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case 'u':
			h264_parm.mbRc = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case '2':
			h264_parm.mbQpAdjustment = (int)strtol(optarg, NULL, 0);
			break;
		case '4':
			h264_parm.mbQpAutoBoost = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case 's':
			h264_parm.pictureSkip = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case 'R':
			h264_parm.rotation = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_GDR:
			h264_parm.gdr = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_LOG_SHOW:
			g_show_mask |= DBG_SHOW_FRAME_TIME;
			break;
		case VAL_OPT_LOG_SHOW_H264:
			g_show_mask |= DBG_SHOW_H264_FRAME;
			break;
		case VAL_OPT_LOG_SHOW_BPS:
			g_show_mask |= DBG_SHOW_H264_BPS;
			break;
		case VAL_OPT_LOG_SHOW_QP:
			g_show_mask |= DBG_SHOW_QP;
			break;
		case VAL_OPT_MAX_NUM:
			FRAMENUM = (int)strtol(optarg, NULL, 0);
			break;
		case ':':
			DBG_LOG_PRINT("required argument : -%c\n", optopt);
			return -1;
		case '?':
			DBG_LOG_PRINT("invalid param: -%c\n", optopt);
			return -1;
		}
	}

	DBG_LOG_LOG("input path = %s; output path = %s\n", inpath, outpath);
	if (DBG_YUV422_INTERLEAVED_YUYV == inputtype)
		size = width * height * 2;
	else
		size = ((width * 12) >> 3) * height;

	DBG_LOG_LOG("[%d, %d]\n", size, width * height * 2);
	total_size = get_file_size(inpath);
	if (total_size < size) {
		DBG_LOG_LOG("[%d, %d, %d]\n", size, width * height * 2, total_size);
		DBG_LOG_ERR("input data [%s] is invalid\n", inpath);
		return -1;
	}

	pFile = fopen(inpath, "r");
	if (!pFile) {
		DBG_LOG_ERR("input data [%s] is invalid\n", inpath);
		goto error2;
	}
	DBG_LOG_LOG("input data size : %d\n", total_size);


	buffer.length = size;
	ret = rts_isp_alloc_dma(&buffer);
	if (ret) {
		DBG_LOG_ERR("alloc dma memory (pict buf) failed");
		goto error2;
	}

	DBG_LOG_LOG("init_h264enc_eniv\n");
	h264_parm.width = width;
	h264_parm.height = height;
	h264_parm.ratenum = fps;
	h264_parm.videostab = 0;
	h264_parm.bps = bitpersecond;
	h264_parm.inputtype = inputtype;
	//ret = init_h264enc_eniv(width, height, fps, 0, bitpersecond, inputtype);
	h264enc = init_h264enc_eniv(&h264_parm);
	if (!h264enc) {
		DBG_LOG_ERR("init_h264enc_eniv failed\n");
		goto error1;
	}

	looptime = 1000/fps;
	DBG_LOG_LOG("");

	if (size == total_size) {
		fread(buffer.vm_addr, 1, size, pFile);
		while (1) {
			gettimeofday(&tv_begin, F_OK);
			encode_h264(h264enc, buffer.phy_addr, 0, saveflag ? outpath : NULL);
			gettimeofday(&tv_end, F_OK);
			deltatime = rts_calc_timeval(tv_begin, tv_end);
			if (looptime > deltatime) {
				//DBG_LOG_LOG("%d, %d\n", looptime, deltatime);
				usleep((looptime - deltatime) * 1000);
			}
			if (g_iMainexit)
				break;
		}
	} else {
		while (encoded_count + size <= total_size) {
			gettimeofday(&tv_begin, F_OK);
			fread(buffer.vm_addr, 1, size, pFile);
			encode_h264(h264enc, buffer.phy_addr, 0, saveflag ? outpath : NULL);
			encoded_count += size;
			framenum++;
			if (FRAMENUM > 0 && framenum > FRAMENUM)
				break;

			gettimeofday(&tv_end, F_OK);
			deltatime = rts_calc_timeval(tv_begin, tv_end);
			if (looptime > deltatime) {
				//DBG_LOG_LOG("%d, %d\n", looptime, deltatime);
				usleep((looptime - deltatime) * 1000);
			}
			if (g_iMainexit)
				break;
		}
	}

	ret = 0;
error:
	release_h264enc_eniv(h264enc);
error1:

	if (buffer.vm_addr)
		rts_isp_free_dma(&buffer);
error2:
	if (pFile)
		fclose(pFile);
	pFile = NULL;
	return ret;
}
