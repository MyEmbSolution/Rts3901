#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <rtsisp.h>
#include <rtsv4l2.h>
#include <rtsutils.h>
#include <rtsjpgenc.h>

#include "dbg_log.h"
#include "dbg_strm.h"
#include "dbg_h264.h"

static int list_fps(int fd, int fmt, int w, int h)
{
	int ret = 0;
	struct v4l2_frmivalenum v4l2_frmival;
	int i = 0;

	do {
		ret = rts_v4l2_enum_frmival(fd, fmt, w, h, i, &v4l2_frmival);
		if (ret < 0)
			break;
		if (V4L2_FRMIVAL_TYPE_DISCRETE == v4l2_frmival.type) {
			DBG_LOG_PRINT("%d/%d \n", v4l2_frmival.discrete.numerator,
									v4l2_frmival.discrete.denominator);
			i++;
		} else {
			DBG_LOG_PRINT("[FPS:]min:%d/%d, max:%d/%d, step:%d/%d \n",
					v4l2_frmival.stepwise.min.numerator,
					v4l2_frmival.stepwise.min.denominator,
					v4l2_frmival.stepwise.max.numerator,
					v4l2_frmival.stepwise.max.denominator,
					v4l2_frmival.stepwise.step.numerator,
					v4l2_frmival.stepwise.step.denominator);
			break;
		}
	} while(1);
	return 0;
}

static int list_resolutions(int fd, int fmt)
{
	int ret = 0;
	struct v4l2_frmsizeenum v4l2_frmsize;
	int i = 0;

	do {
		ret = rts_v4l2_enum_frmsizes(fd, fmt, i, &v4l2_frmsize);
		if (ret < 0)
			break;
		if (V4L2_FRMSIZE_TYPE_DISCRETE == v4l2_frmsize.type) {
			DBG_LOG_PRINT( "width = %4d, height = %4d\n", v4l2_frmsize.discrete.width, v4l2_frmsize.discrete.height);
			list_fps(fd, fmt, v4l2_frmsize.discrete.width, v4l2_frmsize.discrete.height);
			i++;
		} else {
			DBG_LOG_PRINT( "[Resolution:]min_w = %d, max_w = %d, step_w = %d, min_h = %d, max_h = %d, step_h = %d\n",
				v4l2_frmsize.stepwise.min_width,
				v4l2_frmsize.stepwise.max_width,
				v4l2_frmsize.stepwise.step_width,
				v4l2_frmsize.stepwise.min_height,
				v4l2_frmsize.stepwise.max_height,
				v4l2_frmsize.stepwise.step_height);
				list_fps(fd, fmt, v4l2_frmsize.stepwise.min_width, v4l2_frmsize.stepwise.min_height);
				break;
		}
	} while(1);
	return 0;
}

int list_stream_info(int fd)
{
	struct v4l2_fmtdesc fmtdesc;
	int ret;
	int i = 0;

	do {
		ret = rts_v4l2_enum_fmt(fd, i, &fmtdesc);
		if (ret < 0)
			break;
		DBG_LOG_PRINT("----format-[%d]-%s----\n", fmtdesc.index, fmtdesc.description);
		list_resolutions(fd, fmtdesc.pixelformat);
		i++;
	} while(1);

	return 0;
}

int get_fmt(int fd, uint32_t *pixelformat, uint32_t *width, uint32_t *height)
{
	struct v4l2_format format;
	uint32_t f, w, h;
	int ret;

	ret = rts_v4l2_get_fmt(fd, &f, &w, &h);
	if (ret) {
		DBG_LOG_ERR("get format failed : %d\n", ret);
		return ret;
	}

	if (pixelformat)
		*pixelformat = f;
	if (width)
		*width = w;
	if (height)
		*height = h;

	DBG_LOG_PRINT("Current fmt is : %c%c%c%c %dx%d\n",
		f & 0xff,
		(f>>8) & 0xff,
		(f>>16)  & 0xff,
		(f>>24)  & 0xff,
		w,
		h);

	return 0;
}

int get_pixelformat(int fd, int index, uint32_t *pixelformat)
{
	struct v4l2_fmtdesc fmtdesc;
	int ret;

	ret = rts_v4l2_enum_fmt(fd, index, &fmtdesc);
	if (ret) {
		DBG_LOG_ERR("Get pixelformat failed, index (%d) is invalid\n",
			index);
		return ret;
	}

	if (pixelformat)
		*pixelformat = fmtdesc.pixelformat;
	return 0;
}

int set_fmt(int fd, uint32_t pixelformat, uint32_t width, uint32_t height)
{
	int ret;

	ret = rts_v4l2_set_fmt(fd, pixelformat, width, height);

	if (ret)
		DBG_LOG_ERR("set fmt (%c%c%c%c %dx%d) failed : %d\n",
		(pixelformat) & 0xff,
		(pixelformat>>8) & 0xff,
		(pixelformat>>16) & 0xff,
		(pixelformat>>24) & 0xff,
		width, height, ret);
	else
		DBG_LOG_LOG("set fmt (%c%c%c%c %dx%d) success\n",
		(pixelformat) & 0xff,
		(pixelformat>>8) & 0xff,
		(pixelformat>>16) & 0xff,
		(pixelformat>>24) & 0xff,
		width, height);

	return ret;
}

int get_fps(int fd, uint32_t *numerator, uint32_t *denominator)
{
	int ret;
	uint32_t n, dn;

	ret = rts_v4l2_get_frmival(fd, &n, &dn);
	if (ret) {
		DBG_LOG_ERR("get fps failed : %d\n", ret);
		return ret;
	}

	if (numerator)
		*numerator = n;
	if (denominator)
		*denominator = dn;

	DBG_LOG_PRINT("Current fps is : %d/%d\n", n, dn);

	return 0;
}

int set_fps(int fd, uint32_t numerator, uint32_t denominator)
{
	int ret;

	ret = rts_v4l2_set_frmival(fd, numerator, denominator);
	if (ret)
		DBG_LOG_ERR("set fps (%d/%d) failed : %d\n",
			numerator, denominator, ret);
	else
		DBG_LOG_LOG("set fps (%d/%d) success\n",
			numerator, denominator);
	return ret;
}

struct rts_mmap {
	void *addr;
	int length;
	unsigned long phy_addr;
};

#define RTS_BUFFER_NUMBER	2
struct rts_isp_dma_buffer vbufs[RTS_BUFFER_NUMBER];
int g_mmap_num = 0;

static int release_bufs(int fd)
{
	int i;

	for (i = 0; i < g_mmap_num; i++) {
		struct rts_isp_dma_buffer *vbuf = vbufs + i;
		rts_v4l2_munmap_buf(vbuf->vm_addr, vbuf->length);
	}

	rts_v4l2_request_bufs(fd, 0);

	return 0;
}

static int init_buffers(int fd)
{
	int ret = 0;
	int i;

	DBG_LOG_LOG("reqbufs, count = %d\n", RTS_BUFFER_NUMBER);
	ret = rts_v4l2_request_bufs(fd, RTS_BUFFER_NUMBER);
	if (ret < 0) {
		DBG_LOG_ERR("request bufs failed\n");
		return -1;
	}
	DBG_LOG_PRINT("buffer count = %d\n", ret);
	//sleep(1);
	g_mmap_num = ret;

	for (i = 0; i < g_mmap_num; i++) {
		struct rts_isp_dma_buffer *vbuf = vbufs + i;

		ret = rts_v4l2_query_mmap_buf(fd, i, &vbuf->vm_addr, &vbuf->length);

		if (ret < 0) {
			DBG_LOG_ERR("mmap %d failed\n", i);
			ret = -1;
			break;
		}
		vbuf->phy_addr = rts_isp_get_video_phy_addr(fd,
				(unsigned long)vbuf->vm_addr);
	}

	if (ret < 0)
		release_bufs(fd);

	return ret;
}

static int start_preview(int fd)
{
	int ret = 0;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	DBG_LOG_LOG("init_buffers\n");
	ret = init_buffers(fd);
	if (ret < 0) {
		DBG_LOG_ERR("init buffers failed\n");
		return ret;
	}

	DBG_LOG_LOG("stream on \n");
	if (0 > rts_v4l2_streamon(fd)) {
		DBG_LOG_ERR("stream on failed\n");
		return -1;
	}

	return 0;
}

static int stop_preview(int fd)
{
	if (0 > rts_v4l2_streamoff(fd))
		DBG_LOG_ERR("stream off failed\n");

	DBG_LOG_LOG("release_bufs\n");
	release_bufs(fd);

	return 0;
}

static int check_frame(int fd)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	tv.tv_sec = 0;
	tv.tv_usec = 1000 * 1;
	ret = select(fd + 1, &fds, NULL, NULL, &tv);
	if (0 >= ret)
		return -1;
	return 0;
}

static int poll_video(int fd)
{
	if (rts_v4l2_check_frame(fd))
		return 0;

	return -1;
}


static int init_yuv_header(unsigned char *header, int width, int height)
{
	int pos = 0;
	uint32_t size = width * height * 2;

	header[pos++] = 'Y';
	header[pos++] = 'U';
	header[pos++] = 'Y';
	header[pos++] = '2';

	header[pos++] = (width & 0XFF);
	header[pos++] = (width & 0XFF00) >> 8;
	header[pos++] = (width & 0XFF0000) >> 16;
	header[pos++] = (width & 0XFF000000) >> 24;

	header[pos++] = (height & 0XFF);
	header[pos++] = (height & 0XFF00) >> 8;
	header[pos++] = (height & 0XFF0000) >> 16;
	header[pos++] = (height & 0XFF000000) >> 24;

	header[pos++] = (size & 0XFF);
	header[pos++] = (size & 0XFF00) >> 8;
	header[pos++] = (size & 0XFF0000) >> 16;
	header[pos++] = (size & 0XFF000000) >> 24;


	return 0;
}

static char *get_date_time(time_t lt)
{
	static char ch[80];
	struct tm * timeinfo;
	timeinfo = localtime ( &lt );
	strftime (ch,80,"%Y/%m/%d %H:%M:%S",timeinfo);

	return ch;
}

char *get_date_time_II(time_t lt)
{
	static char ch[80];
	struct tm * timeinfo;
	timeinfo = localtime ( &lt );
	strftime (ch,80,"%Y%m%d_%H%M%S",timeinfo);

	return ch;
}

char *get_date_time_III(struct timeval tv)
{
	static char ch[160];

	snprintf(ch, sizeof(ch), "%s_%03d",
		get_date_time_II(tv.tv_sec), (int)(tv.tv_usec/1000));

	return ch;
}

static double calc_fps() {
	static int fps_framecount = 0;
	static long fps_frametime;
	long delta_time = 0;
	struct timeval tv;
	long cur_time;

	static double fps_rate = 0.0;

	gettimeofday(&tv, NULL);

	cur_time = (long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);

	if (0 == fps_framecount){
		fps_frametime = cur_time;
	}
	delta_time = cur_time - fps_frametime;
	delta_time = delta_time > 0 ? delta_time : (-delta_time);

	if (delta_time >= 300) {
		fps_rate = (fps_framecount * 1000.0)/delta_time;
		//fprintf(stdout, "real time fps %.2f\n", fps_rate);
		fps_framecount = 0;
		fps_frametime = cur_time;
	}
	fps_framecount++;

	return fps_rate;
}

int switch_td(int fd, int enable)
{
	int ret;

	if (enable)
		ret = rts_isp_enable_temporal_denoise(fd, 1);
	else
		ret = rts_isp_enable_temporal_denoise(fd, 0);

	DBG_LOG_LOG("Current td enable : %d\n",
			rts_isp_get_temporal_denoise_state(fd));
	return ret;
}

int preview_func(int fd, const struct rts_dbg_option *option)
{
	uint32_t pixelformat;
	uint32_t width;
	uint32_t height;
	uint32_t numerator;
	uint32_t denominator;
	int ret;
	long loop = 0;
	char *savepath = NULL;
	int saves = 0;
	int saveflag = 0;
	void *h264enc = NULL;
	RtsJpgEncInst mjpgenc = NULL;
	struct rts_isp_dma_buffer mjpgout;
	RtsBmpInst bmpenc = NULL;
	int h264stab = 0;
	struct v4l2_buffer buf[2];
	struct v4l2_buffer *pbuf1 = NULL;
	struct v4l2_buffer *pbuf2 = NULL;
	struct timeval tv_begin, tv_end;

	if (!option) {
		DBG_LOG_ERR("Null option\n");
		return -1;
	}

	memset(&mjpgout, 0, sizeof(mjpgout));

	if (option->saveflag)
		savepath = (char*)option->savepath;

	ret = get_fmt(fd, &pixelformat, &width, &height);
	if (ret)
		return ret;
	ret = get_fps(fd, &numerator, &denominator);
	if (ret)
		return ret;

	if (option->h264flag && option->priv) {
		struct dbg_h264_parm *h264_parm = option->priv;

		ret = 0;
		h264_parm->width = width;
		h264_parm->height = height;
		h264_parm->ratenum = denominator;
		h264_parm->videostab = option->h264stab;
		h264_parm->gdr = option->gdr;
		h264_parm->intraQpDelta = option->intraQpDelta;
		h264_parm->rotation = option->rotation;
		if (option->h264bps)
			h264_parm->bps = option->h264bps;
		h264_parm->roi = 0;
		if (option->roi) {
			h264_parm->roi = 1;
			h264_parm->left = option->left;
			h264_parm->top = option->top;
			h264_parm->right = option->right;
			h264_parm->bottom = option->bottom;
		}
		if (pixelformat == V4L2_PIX_FMT_NV12)
			h264_parm->inputtype = DBG_YUV420_SEMIPLANAR;
		else if (pixelformat == V4L2_PIX_FMT_YUYV)
			h264_parm->inputtype = DBG_YUV422_INTERLEAVED_YUYV;
		else
			ret = -1;
		if (0 == ret)
			h264enc = init_h264enc_eniv(h264_parm);
		if (!h264enc)
			DBG_LOG_ERR("init h264 env fail\n");
		if (h264enc && option->h264stab)
			h264stab = 1;
	}

	if (option->bmpflag) {
		if (pixelformat == V4L2_PIX_FMT_NV12 ||
				pixelformat == V4L2_PIX_FMT_NV12 ||
				pixelformat == V4L2_PIX_FMT_NV16) {
			ret = rts_bmp_init_env(&bmpenc, width, height);
			if (ret)
				DBG_LOG_ERR("init bmp env fail\n");
		}
	}

	if (option->mjpgflag) {
		struct rtsjpgenc_config jconfig;

		ret = 0;
		switch (pixelformat) {
		case V4L2_PIX_FMT_NV12:
			jconfig.input_type = RTSJPGENC_YUV420_SEMIPLANAR;
			break;
		case V4L2_PIX_FMT_NV16:
			jconfig.input_type = RTSJPGENC_YUV422_SEMIPLANAR;
			break;
		default:
			ret = -1;
			break;
		}
		switch (option->rotation) {
		case DBG_RATETE_90R:
			jconfig.rotation = RTSJPGENC_ROTATE_90R;
			break;
		case DBG_RATETE_90L:
			jconfig.rotation = RTSJPGENC_ROTATE_90L;
			break;
		default:
			jconfig.rotation = RTSJPGENC_ROTATE_0;
			break;
		}
		jconfig.width = width;
		jconfig.height = height;
		if (0 == ret) {
			ret = rtsjpgenc_init(&mjpgenc);
			if (!ret)
				ret = rtsjpgenc_set_config(mjpgenc, &jconfig);
			if (!ret) {
				mjpgout.length = width *height;
				ret = rts_isp_alloc_dma(&mjpgout);
			}
			if (ret) {
				DBG_LOG_ERR("init mjpeg env fail\n");
				if (mjpgenc)
					rtsjpgenc_release(mjpgenc);
				mjpgenc = NULL;
				if (mjpgout.vm_addr)
					rts_isp_free_dma(&mjpgout);
			}
		}
	}

	ret = start_preview(fd);
	if (ret) {
		DBG_LOG_ERR("Stream on failed\n");
		goto exit;
	}

	while (!g_iMainexit) {
		if (0 > poll_video(fd)) {
			continue;
		}

		pbuf2 = &buf[loop % 2];

		ret = rts_v4l2_get_buffer(fd, pbuf2);
		if (0 > ret) {
			DBG_LOG_ERR( "dqbuf failed, ret = %d\n", ret);
			continue;
		}
		if (pbuf2->index >= g_mmap_num) {
			DBG_LOG_ERR("buf index (%d) is invalid, total frame number is %d\n", pbuf2->index, g_mmap_num);
			continue;
		}
		loop++;

		gettimeofday(&tv_begin, F_OK);
		if (g_show_mask & DBG_SHOW_FRAME_TIME)
			DBG_LOG_LOG("Get frame at [%d, %06d]\n",
					tv_begin.tv_sec, tv_begin.tv_usec);

		if (savepath &&
			(option->framenum == 0 || loop <= option->framenum))
			saveflag = 1;
		else
			saveflag = 0;

		if (h264enc && vbufs[pbuf2->index].phy_addr) {
			if (h264stab) {
				if (pbuf1 && pbuf2)
					encode_h264(h264enc,
						vbufs[pbuf1->index].phy_addr,
						vbufs[pbuf2->index].phy_addr,
						savepath);
			} else {
				encode_h264(h264enc,
						vbufs[pbuf2->index].phy_addr,
						0,
						savepath);
			}
			saves = 1;
		}
		if (bmpenc && saveflag) {
			char filename[128];
			snprintf(filename, sizeof(filename),
				"%s%d.bmp", savepath, loop);
			rts_bmp_encode_yuv(bmpenc,
					vbufs[pbuf2->index].vm_addr,
					pixelformat, filename);
			saves = 1;
		}

		if (mjpgenc && vbufs[pbuf2->index].phy_addr) {
			struct rtsjpgenc_encin encin;
			uint32_t phy_addr = vbufs[pbuf2->index].phy_addr;

			encin.src_busLuma = phy_addr;
			encin.src_busChroma = phy_addr + width * height;
			encin.out_bus_buf = mjpgout.phy_addr;
			encin.out_buf_size = mjpgout.length;
			encin.p_outbuf = mjpgout.vm_addr;
			encin.out_bytesused = 0;

			gettimeofday(&tv_begin, F_OK);
			ret = rtsjpgenc_encode(mjpgenc, &encin);
			gettimeofday(&tv_end, F_OK);
			if (!ret) {
				if (saveflag) {
					char filename[128];
					snprintf(filename, sizeof(filename),
						"%s%d.jpg",savepath, loop);
					save_data_to_file(mjpgout.vm_addr,
							encin.out_bytesused,
							filename);
				}
				if (g_show_mask & DBG_SHOW_MJPEG)
					DBG_LOG_LOG("[%d] spend %d ms, size = %d\n",
							loop,
							rts_calc_timeval(tv_begin, tv_end),
							encin.out_bytesused);
			} else {
				DBG_LOG_ERR("encode mjpeg fail\n");
			}

			saves = 1;
		}

		if (saveflag && (0 == saves)) {
			char filename[128];
			if (pixelformat == V4L2_PIX_FMT_MJPEG) {
				snprintf(filename, sizeof(filename),
					"%s%d.jpg",savepath, loop);
			} else {
				snprintf(filename, sizeof(filename),
					"%s%d.yuv",savepath, loop);
			}
			save_data_to_file(vbufs[pbuf2->index].vm_addr,
					pbuf2->bytesused,
					filename);
		}

		if (!h264stab) {
			pbuf1 = pbuf2;
			pbuf2 = NULL;
		}

		if (pbuf1)
			ret = rts_v4l2_put_buffer(fd, pbuf1);
		pbuf1 = NULL;
		if (0 > ret)
			DBG_LOG_ERR("qbuf failed\n");

		if (h264stab) {
			pbuf1 = pbuf2;
			pbuf2 = NULL;
		}

		if (option->previewnum > 0 && loop > option->previewnum) {
			if (pbuf1)
				rts_v4l2_put_buffer(fd, pbuf1);
			pbuf1 = NULL;
			break;
		}
	}

	stop_preview(fd);
exit:
	if (h264enc)
		release_h264enc_eniv(h264enc);
	h264enc = NULL;
	if (mjpgenc)
		rtsjpgenc_release(mjpgenc);
	mjpgenc = NULL;
	if (mjpgout.vm_addr)
		rts_isp_free_dma(&mjpgout);
	if (bmpenc)
		rts_bmp_release_env(bmpenc);
	bmpenc = NULL;

	return ret;
}

