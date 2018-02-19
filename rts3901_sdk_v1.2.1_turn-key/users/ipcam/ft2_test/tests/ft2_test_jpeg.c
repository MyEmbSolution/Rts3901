/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_jpeg.c
 *
 * get from
 * users/ipcam/rtstream/test/test_media/video_jpgenc/rts_test_jpgenc.c
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 *
 * modify by
 * Copyright (C) 2015      Lei Wang<lei_wang@realsil.com.cn>
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
#include "tester.h"

#include <linux/videodev2.h>
#include <rtsavapi.h>

#define VIDEO_DEV 1

struct jpeg_parameter {
	int pixelformat;
	int width;
	int height;
	int rotation;
};

struct jpeg_test_mapping {
	char name[16];
	struct jpeg_parameter param;
};

struct jpeg_test_mapping jpeg_mapping_info[] = {
	{"1080pyuv420", {rts_v_fmt_yuv420semiplanar, 1920, 1080, rts_av_rotation_0}},
	{"720pyuv420", {rts_v_fmt_yuv420semiplanar, 1280, 720, rts_av_rotation_0}}
};

static int get_mapping_index(char *content)
{
        int i = 0;
        int size = sizeof(jpeg_mapping_info)/sizeof(struct jpeg_test_mapping);

        if (!content)
                return -1;

        for (i = 0; i < size; i++) {
                if (0 == strcasecmp(content, jpeg_mapping_info[i].name))
                        return i;
        }

        return -1;
}

static int save_jpg(uint8_t *pbuf, uint32_t length, const char *path, int index)
{
	char filename[64];
	FILE *pfile = NULL;

	switch (index) {
	case 0:
		snprintf(filename, sizeof(filename), "%s/jpeg/1080p.output", path);
		break;
	case 1:
		snprintf(filename, sizeof(filename), "%s/jpeg/720p.output", path);
		break;
	default:
		break;
	}

	if (test_forever) {
		snprintf(filename, sizeof(filename), "%s%d", filename, jpeg_count);
		jpeg_count++;
	}

	pfile = fopen(filename, "wb");
	if (pfile) {
		fwrite(pbuf, 1, length, pfile);
		fclose(pfile);
		FT2_LOG_INFO("%s saved!\n", filename);
	}

	return 0;
}

static int test_jpegenc(int fmt, int width, int height, int rotation,
		const char *path, int index)
{
	int ret = FT2_OK;
	struct rts_av_stream *video_stream = NULL;
	struct rts_av_unit *isp_unit = NULL;
	struct rts_isp_attr isp_attr;
	struct rts_av_unit *jenc_unit = NULL;
	struct rts_jpgenc_attr jenc_attr;
	struct rts_av_buffer *buffer = NULL;
	int i = 0, frame_num = 0;
	uint32_t pixelformat = V4L2_PIX_FMT_NV12;
int count = 0;

	if (fmt == rts_v_fmt_yuv422semiplanar)
		pixelformat = V4L2_PIX_FMT_NV16;

	ret = rts_av_init();
	if (ret) {
		FT2_LOG_ERR("rts_av_init fail\n");
		return -FT2_ERROR;
	}

	isp_unit = rts_av_new_unit(RTS_AV_ID_ISP);
	if (!isp_unit) {
		FT2_LOG_ERR("cann't create isp unit\n");
		ret = -FT2_ERROR;
		goto exit;
	}

	isp_attr.isp_id = VIDEO_DEV;
	isp_attr.pixelformat = pixelformat;
	isp_attr.width = width;
	isp_attr.height = height;
	isp_attr.numerator = 1;
	isp_attr.denominator = 30;

	ret = rts_av_set_attr(isp_unit, &isp_attr);
	if (ret) {
		FT2_LOG_ERR("set isp unit attr fail, ret = %d\n", ret);
		ret = -FT2_ERROR;
		goto exit;
	}

	jenc_unit = rts_av_new_unit(RTS_AV_ID_MJPGENC);
	if (!jenc_unit) {
		FT2_LOG_ERR("cann't create mjpeg encoder unit\n");
		ret = -FT2_ERROR;
		goto exit;
	}

	jenc_attr.input_type = fmt;
	jenc_attr.width = width;
	jenc_attr.height = height;
	jenc_attr.rotation = rotation;
	jenc_attr.numerator = 1;
	jenc_attr.denominator = 30;
	ret = rts_av_set_attr(jenc_unit, &jenc_attr);
	if (ret) {
		FT2_LOG_ERR("set mjpeg encoder unit attr fail, ret = %d\n", ret);
		ret = -FT2_ERROR;
		goto exit;
	}

	video_stream = rts_av_new_stream();
	if (!video_stream) {
		FT2_LOG_ERR("cann't create stream\n");
		ret = -FT2_ERROR;
		goto exit;
	}

	rts_av_stream_add_tail(video_stream, isp_unit);
	rts_av_stream_add_tail(video_stream, jenc_unit);

	ret = rts_av_apply(video_stream);
	if (ret) {
		FT2_LOG_ERR("rts_av_apply fail, ret = %d\n", ret);
		ret = -FT2_ERROR;
		goto exit;
	}

	/* wait no more than (1000 + 1000) * 1000 usecond */
	for (i = 0; i < 5000; i++) {//for (i = 0; i < 1000; i++) {
		if (rts_av_stream_poll(video_stream)) {
			usleep(1000);
			continue;
		}

		if (rts_av_stream_recv(video_stream, &buffer)) {
			usleep(1000);
			continue;
		}

		if (buffer) {
			FT2_LOG_INFO("frame size is %d\n", buffer->bytesused);
			count++;
			if (count >= 5) {
				save_jpg(buffer->vm_addr, buffer->bytesused, path, index);
				break;
			}
			rts_av_put_buffer(buffer);
			buffer = NULL;
		}
	}

	if (count >= 5)
		ret = FT2_OK;
	else
		ret = -FT2_ERROR;

	if (i == 1000)
		ret = -FT2_ERROR;

	rts_av_cancel(video_stream);
exit:
	if (video_stream)
		rts_av_delete_stream(video_stream);
	video_stream = NULL;
	if (isp_unit)
		rts_av_delete_unit(isp_unit);
	isp_unit = NULL;
	if (jenc_unit)
		rts_av_delete_unit(jenc_unit);
	jenc_unit = NULL;
	rts_av_release();

	return ret;
}

/* the parameter path is root path "/mnt" */
static int jpeg_check_path(char *path)
{
	char dirpath[128];
	char filename[128];
	int i = 0;

	if (!path || strlen(path) == 0)
		return -FT2_ERROR;

	if (0 > access(path, R_OK | W_OK)) {
		FT2_LOG_ERR("%s is not exist or no right to rw\n", path);
		return -FT2_ERROR;
	}

	snprintf(dirpath, sizeof(dirpath), "%s/jpeg", path);
	if (0 > access(dirpath, F_OK)) {
		if (0 > mkdir(dirpath, 0777)) {
			FT2_LOG_ERR("fail to mkdir %s\n", dirpath);
			return -FT2_ERROR;
		}
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

int ft2_jpeg_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int ft2_jpeg_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	struct jpeg_parameter param;
	struct protocol_param *parameter = NULL;
	int index = 0;
	int ret = 0;

	if (!pcmd)
		return -FT2_NULL_POINT;

	if (jpeg_check_path(path)) {
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

	param = jpeg_mapping_info[index].param;
	ret = test_jpegenc(param.pixelformat,
			param.width,
			param.height,
			param.rotation,
			path,
			index);
	if (ret) {
		append_pt_command_result(pcmd,
			FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return ret;
	}

	FT2_LOG_INFO("jpeg test ok\n");
	append_pt_command_result(pcmd,
		FT2_TEST_PASS, strlen(FT2_TEST_PASS));

	return FT2_OK;
}

void ft2_jpeg_cleanup(void *priv)
{
	jpeg_count = 0;
}
