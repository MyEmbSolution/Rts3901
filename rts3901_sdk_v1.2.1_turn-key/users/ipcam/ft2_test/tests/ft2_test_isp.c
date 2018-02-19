/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_isp.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <time.h>
#include <linux/videodev2.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"

#include "tester.h"

struct isp_test_mapping {
	char parameter[16];
	int index;
	uint32_t pixelformat;
	uint32_t width;
	uint32_t height;
};

struct isp_frame {
	void *vm_addr;
	uint32_t length;
	uint32_t reserved;
};

struct isp_test_mapping m_mapping_info[] = {
	{"1080pyuv420",	51,	V4L2_PIX_FMT_NV12,	1920,	1080},
	{"720pyuv420",	51,	V4L2_PIX_FMT_NV12,	1280,	720},
	{"vgayuv420",	52,	V4L2_PIX_FMT_NV12,	640,	480},
	{"qvgayuv420",	53,	V4L2_PIX_FMT_NV12,	320,	240},
};

static int get_mapping_index(char *content)
{
	int i = 0;
	int size = sizeof(m_mapping_info)/sizeof(struct isp_test_mapping);

	if (!content)
		return -1;

	for (i = 0; i < size; i++) {
		if (0 == strcasecmp(content, m_mapping_info[i].parameter))
			return i;
	}

	return -1;
}

int ft2_isp_check(struct protocol_command *pcmd)
{
	struct protocol_param *parameter = NULL;

	if (!pcmd)
		return -FT2_NULL_POINT;

	parameter = pcmd->parameter;
	if (!parameter) {
		FT2_LOG_ERR("test [%s] need parameter\n", pcmd->name);
		return -FT2_PROTOCOL_ERROR;
	}

	if (get_mapping_index(parameter->content) >= 0)
		return FT2_OK;

	return -FT2_INVALID_ARGUMENT;
}

static int set_fmt(int fd, uint32_t fmt, uint32_t width, uint32_t height)
{
	struct v4l2_format format;
	int ret = 0;

	if (fd < 0)
		return -FT2_ERROR;

	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = fmt;
	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	ret = ioctl(fd, VIDIOC_S_FMT, &format);
	if (ret) {
		FT2_LOG_ERR("set fmt (%c%c%c%c %dx%d) fail\n",
				fmt & 0xff,
				(fmt>>8) & 0xff,
				(fmt>>16) & 0xff,
				(fmt>>24) & 0xff,
				width,
				height);
		return -FT2_ERROR;
	}
	if (format.fmt.pix.pixelformat != fmt ||
			format.fmt.pix.width != width ||
			format.fmt.pix.height != height) {
		FT2_LOG_ERR("not support fmt (%c%c%c%c %dx%d) fail\n",
				fmt & 0xff,
				(fmt>>8) & 0xff,
				(fmt>>16) & 0xff,
				(fmt>>24) & 0xff,
				width,
				height);
		return -FT2_ERROR;
	}
	return FT2_OK;
}

static int isp_free_buf(int fd, struct isp_frame *pframe, unsigned int num)
{
	int i = 0;
	struct v4l2_requestbuffers v4l2_reqbufs;

	for (i = 0; i < num; i++) {
		struct isp_frame *frame = pframe + i;
		if (frame->vm_addr != (void*)MAP_FAILED)
			munmap(frame->vm_addr, frame->length);
		frame->vm_addr = NULL;
		frame->length = 0;
	}

	v4l2_reqbufs.count = 0;
	v4l2_reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_reqbufs.memory = V4L2_MEMORY_MMAP;
	ioctl(fd, VIDIOC_REQBUFS, &v4l2_reqbufs);

	return FT2_OK;
}

static int isp_init_buf(int fd, struct isp_frame *pframe, unsigned int *pnum)
{
	struct v4l2_requestbuffers v4l2_reqbufs;
	int ret = FT2_OK;
	int i = 0;

	if (fd < 0)
		return -FT2_ERROR;

	if (!pframe || !pnum)
		return -FT2_ERROR;

	for (i = 0; i < *pnum; i++) {
		struct isp_frame *frame = pframe + i;
		frame->vm_addr = NULL;
		frame->length = 0;
	}

	memset(&v4l2_reqbufs, 0, sizeof(v4l2_reqbufs));
	v4l2_reqbufs.count = *pnum;
	v4l2_reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_reqbufs.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(fd, VIDIOC_REQBUFS, &v4l2_reqbufs);
	if (ret < 0) {
		FT2_LOG_ERR("request buffers fail\n");
		return -FT2_ERROR;
	}
	for (i = 0; i < v4l2_reqbufs.count; i++) {
		struct v4l2_buffer v4l2_buf;
		struct isp_frame *frame = pframe + i;
		memset(&v4l2_buf, 0, sizeof(v4l2_buf));
		v4l2_buf.index = i;
		v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &v4l2_buf);
		if (ret) {
			FT2_LOG_ERR("query buf fail\n");
			ret = -FT2_ERROR;
			goto error;
		}
		frame->length = v4l2_buf.length;
		frame->vm_addr = mmap(NULL,
				v4l2_buf.length,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				fd,
				v4l2_buf.m.offset);

		if (frame->vm_addr == (void*)MAP_FAILED) {
			FT2_LOG_ERR("mmap fail\n");
			ret = -FT2_ERROR;
			goto error;
		}

		ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
		if (ret) {
			FT2_LOG_ERR("qbuf fail\n");
			ret = -FT2_ERROR;
			goto error;
		}
	}

	*pnum = v4l2_reqbufs.count;
	return FT2_OK;
error:
	for (i = 0; i < v4l2_reqbufs.count; i++) {
		struct isp_frame *frame = pframe + i;
		if (frame->vm_addr != (void*)MAP_FAILED)
			munmap(frame->vm_addr, frame->length);
		frame->vm_addr = NULL;
		frame->length = 0;
	}
	return ret;
}

static int isp_streamon(int fd)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 > ioctl(fd, VIDIOC_STREAMON, &type)) {
		FT2_LOG_ERR("streamon fail\n");
		return -FT2_ERROR;
	}
	return FT2_OK;
}

static int isp_streamoff(int fd)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 > ioctl(fd, VIDIOC_STREAMOFF, &type)) {
		FT2_LOG_ERR("streamoff fail\n");
		return -FT2_ERROR;
	}
	return FT2_OK;
}

static int isp_poll(int fd)
{
	int ret = 0;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;
	ret = poll(&pfd, 1, 1);
	if (0 >= ret)
		return -1;
	if (pfd.events & POLLIN)
		return 0;
	return -1;
}

static int isp_save_frame(char *path, char *filename, void *data, int length)
{
	char outfile[128] = {0};
	FILE *pfile = NULL;

	if (!path || !filename || !data)
		return -FT2_NULL_POINT;

	if (0 == strlen(path) || 0 == strlen(filename) || 0 == length)
		return -FT2_ERROR;

	if (test_forever) {
		snprintf(outfile, sizeof(outfile),"%s/isp/%s%d", path, filename, pic_count);
		pic_count++;
	}
	else
		snprintf(outfile, sizeof(outfile),"%s/isp/%s", path, filename);

	FT2_LOG_INFO("%s\n", outfile);

	pfile = fopen(outfile, "wb+");
	if (!pfile) {
		FT2_LOG_ERR("save frame to %s fail\n", outfile);
		return -FT2_ERROR;
	}

	fwrite(data, 1, length, pfile);
	fclose(pfile);

	return FT2_OK;
}

static int isp_test_func(char *devname, uint32_t fmt,
		uint32_t width, uint32_t height, char *path, char *filename)
{
	int fd = -1;
	int ret = FT2_OK;
	int streaming = 0;
	struct isp_frame frames[5];
	int num = 5;
	time_t ts;
	int frame_num = 0;
	int frame_target = 5;

	if (!devname)
		return -FT2_NULL_POINT;

	fd = open(devname, O_RDWR);
	if (fd < 0) {
		FT2_LOG_ERR("open device %s fail\n", devname);
		return -FT2_ERROR;
	}

	ret = set_fmt(fd, fmt, width, height);
	if (ret)
		goto exit;

	ret = isp_init_buf(fd, frames, &num);
	if (ret)
		goto exit;

	ret = isp_streamon(fd);
	if (ret)
		goto exit;
	streaming = 1;

	ts = time(NULL);
	while (time(NULL) - ts < 3) {
		struct v4l2_buffer v4l2_buf;
		if (0 > isp_poll(fd)) {
			usleep(1000);
			continue;
		}

		memset(&v4l2_buf, 0, sizeof(v4l2_buf));
		v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		v4l2_buf.index = -1;

		ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
		if (0 > ret)
			continue;

		ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
		frame_num++;

		if (frame_num >= frame_target) {
			FT2_LOG_INFO("frame size is %d\n", v4l2_buf.bytesused);
			ret = isp_save_frame(path, filename,
				frames[v4l2_buf.index].vm_addr,
				v4l2_buf.bytesused);
			break;
		}
	}

	if (frame_num >= frame_target)
		ret = FT2_OK;
	else
		ret = -FT2_ERROR;
exit:
	if (streaming)
		isp_streamoff(fd);
	isp_free_buf(fd, frames, num);
	if (fd > 0)
		close(fd);
	fd = -1;
	return ret;
}

static int isp_check_path(char *path)
{
	char dirpath[128];
	char filename[128];

	if (!path || strlen(path) == 0)
		return -FT2_ERROR;

	snprintf(dirpath, sizeof(dirpath), "%s/isp", path);
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

	snprintf(filename, sizeof(filename), "%s/1080pyuv420.golden", dirpath);
        if (0 > access(filename, R_OK)) {
                FT2_LOG_ERR("golden %s is not exist\n", filename);
                return -FT2_ERROR;
        }

	snprintf(filename, sizeof(filename), "%s/720pyuv420.golden", dirpath);
        if (0 > access(filename, R_OK)) {
                FT2_LOG_ERR("golden %s is not exist\n", filename);
                return -FT2_ERROR;
        }

	snprintf(filename, sizeof(filename), "%s/vgayuv420.golden", dirpath);
        if (0 > access(filename, R_OK)) {
                FT2_LOG_ERR("golden %s is not exist\n", filename);
                return -FT2_ERROR;
        }

	snprintf(filename, sizeof(filename), "%s/qvgayuv420.golden", dirpath);
        if (0 > access(filename, R_OK)) {
                FT2_LOG_ERR("golden %s is not exist\n", filename);
                return -FT2_ERROR;
        }

	return FT2_OK;
}
int ft2_isp_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	int index = 0;
	char devname[64] = {0};
	char filename[64] = {0};
	struct protocol_param *parameter = NULL;
	int ret = FT2_OK;

	if (!pcmd)
		return -FT2_NULL_POINT;

	if (isp_check_path(path)) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	parameter = pcmd->parameter;
	index = get_mapping_index(parameter->content);
	if (index < 0) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}
	snprintf(devname, sizeof(devname), "/dev/video%d", m_mapping_info[index].index);
	snprintf(filename, sizeof(filename), "%s.output", m_mapping_info[index].parameter);

	ret = isp_test_func(devname, m_mapping_info[index].pixelformat,
			m_mapping_info[index].width,
			m_mapping_info[index].height,
			path,
			filename);

	if (ret) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return ret;
	}

	append_pt_command_result(pcmd, FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;
}

void ft2_isp_cleanup(void *priv)
{
	pic_count = 0;
}
