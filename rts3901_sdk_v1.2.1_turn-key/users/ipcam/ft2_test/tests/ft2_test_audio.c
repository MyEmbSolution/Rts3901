/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_audio.c
 *
 * Copyright (C) 2014      Wind Han<wind_han@realsil.com.cn>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"

#include "tester.h"

#define FT2_TEST_AUDIO_BUFFER_TIME_MAX		500000
#define FT2_TEST_AUDIO_INPUT_FILE_NUM		2
#define FT2_TEST_AUDIO_CAPTURE_TIME		(1000 * 1000)
#define FT2_TEST_AUDIO_DELAY_TIME		(200 * 1000)
#define FT2_TEST_AUDIO_HEADER_LENGTH_MAX	0x44

enum {
	FT2_TEST_AUDIO_STREAM_PATH_I2S = 0,
	FT2_TEST_AUDIO_STREAM_PATH_AMIC = 1,
	FT2_TEST_AUDIO_STREAM_PATH_DMIC = 2,
};

struct audio_param {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;

	int type;

	char *path;
	char *file_name;
	char file_header[FT2_TEST_AUDIO_HEADER_LENGTH_MAX];
	int file_hlen;
};

struct audio_inst {
	snd_pcm_t *handle;

	snd_pcm_uframes_t period_frames;
	snd_pcm_uframes_t buffer_frames;

	struct audio_param *aparam;

	char *audiobuf;
	int buflen;
};

static int audio_ret = FT2_OK;

static char ft2_audio_input_file[FT2_TEST_AUDIO_INPUT_FILE_NUM][25] = {
			"sin1k_48k_24b.input", "sin1k_44k_16b.input"};

int ft2_audio_check(struct protocol_command *pcmd)
{
	return FT2_OK;
}

static int audio_check_path(char *path)
{
	char dirpath[128];
	char filename[128];
	int i;

	if (!path || strlen(path) == 0)
		return -FT2_ERROR;

	if (0 > access(path, R_OK | W_OK)) {
		FT2_LOG_ERR("%s is not exist or no right to rw\n", path);
		return -FT2_ERROR;
	}

	snprintf(dirpath, sizeof(dirpath), "%s/audio", path);
	if (0 > access(dirpath, F_OK)) {
		if (0 > mkdir(dirpath, 0777)) {
			FT2_LOG_ERR("fail to mkdir %s\n", dirpath);
			return -FT2_ERROR;
		}
	}

	for (i = 0; i < FT2_TEST_AUDIO_INPUT_FILE_NUM; i++) {
		snprintf(filename, sizeof(filename),
				"%s/%s", dirpath, ft2_audio_input_file[i]);
		if (0 > access(filename, R_OK)) {
			FT2_LOG_ERR("input %s is not exist\n", filename);
			return -FT2_ERROR;
		}
	}

	return FT2_OK;
}

static int audio_set_params(struct audio_inst *ainst)
{
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_t *handle = ainst->handle;
	unsigned int buffer_time;
	unsigned int period_time;
	int ret;

	/* init hwparams */
	snd_pcm_hw_params_alloca(&hwparams);
	ret = snd_pcm_hw_params_any(handle, hwparams);
	if (ret < 0) {
		FT2_LOG_ERR("no configurations available\n");
		return -FT2_ERROR;
	}

	/* set access type */
	ret = snd_pcm_hw_params_set_access(handle, hwparams,
			SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		FT2_LOG_ERR("access type not available\n");
		return -FT2_ERROR;
	}

	/* set format */
	ret = snd_pcm_hw_params_set_format(handle, hwparams,
			ainst->aparam->format);
	if (ret < 0) {
		FT2_LOG_ERR("sample format not available\n");
		return -FT2_ERROR;
	}

	/* set channels */
	ret = snd_pcm_hw_params_set_channels(handle, hwparams,
			ainst->aparam->channels);
	if (ret < 0) {
		FT2_LOG_ERR("channels count not available\n");
		return -FT2_ERROR;
	}

	/* set sample rate */
	ret = snd_pcm_hw_params_set_rate_near(handle, hwparams,
			&ainst->aparam->rate, 0);
	if (ret < 0) {
		FT2_LOG_ERR("sample rate not available\n");
		return -FT2_ERROR;
	}

	/* set buffer time */
	ret = snd_pcm_hw_params_get_buffer_time_max(hwparams,
			&buffer_time, 0);
	if (ret < 0) {
		FT2_LOG_ERR("fail to get buffer time max\n");
		return -FT2_ERROR;
	}
	if (buffer_time <= 0) {
		FT2_LOG_ERR("buffer time max <= 0\n");
		return -FT2_ERROR;
	}
	if (buffer_time > FT2_TEST_AUDIO_BUFFER_TIME_MAX)
		buffer_time = FT2_TEST_AUDIO_BUFFER_TIME_MAX;

	/* set period time */
	period_time = buffer_time / 4;
	ret = snd_pcm_hw_params_set_period_time_near(handle, hwparams,
			&period_time, 0);
	if (ret < 0) {
		FT2_LOG_ERR("period time not available\n");
		return -FT2_ERROR;
	}

	/* set buffer time */
	ret = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams,
			&buffer_time, 0);
	if (ret < 0) {
		FT2_LOG_ERR("buffer time not available\n");
		return -FT2_ERROR;
	}

	/* set hw_params */
	ret = snd_pcm_hw_params(handle, hwparams);
	if (ret < 0) {
		FT2_LOG_ERR("unable to install hw params\n");
		return -FT2_ERROR;
	}

	snd_pcm_hw_params_get_period_size(hwparams, &ainst->period_frames, 0);
	snd_pcm_hw_params_get_buffer_size(hwparams, &ainst->buffer_frames);
	if (ainst->period_frames == ainst->buffer_frames) {
		FT2_LOG_ERR("can't use period equal to buffer size\n");
		return -FT2_ERROR;
	}

	return FT2_OK;
}

static int audio_analyze_header(struct audio_param *aparam, char *filename)
{
	FILE *fp;
	int ret;
	int cksize;
	char buffer[FT2_TEST_AUDIO_HEADER_LENGTH_MAX];

	fp = fopen(filename, "rb");
	if (!fp) {
		FT2_LOG_ERR("no file: %s\n", filename);
		ret = -FT2_ERROR;
		goto error;
	}

	ret = fread(buffer, 1, FT2_TEST_AUDIO_HEADER_LENGTH_MAX, fp);
	if (ret != FT2_TEST_AUDIO_HEADER_LENGTH_MAX) {
		FT2_LOG_ERR("fail to read file header: %d\n", ret);
		ret = -FT2_ERROR;
		goto error;
	}

	aparam->channels = (unsigned int)(buffer[0x16] & 0xff) |
			((unsigned int)(buffer[0x17] & 0xff) << 8);
	aparam->rate = (unsigned int)(buffer[0x18] & 0xff) |
			((unsigned int)(buffer[0x19] & 0xff) << 8) |
			((unsigned int)(buffer[0x1a] & 0xff) << 16) |
			((unsigned int)(buffer[0x1b] & 0xff) << 24);
	aparam->format = (unsigned int)(buffer[0x22] & 0xff) |
			((unsigned int)(buffer[0x23] & 0xff) << 8);
	switch (aparam->format) {
	case 8:
		aparam->format = SND_PCM_FORMAT_U8;
		break;
	case 16:
		aparam->format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		aparam->format = SND_PCM_FORMAT_S24_3LE;
		break;
	default:
		FT2_LOG_ERR("unknow pcm format: %d\n", aparam->format);
		ret = -FT2_ERROR;
		goto error;
	}
	cksize = (unsigned int)(buffer[0x10] & 0xff) |
		((unsigned int)(buffer[0x11] & 0xff) << 8) |
		((unsigned int)(buffer[0x12] & 0xff) << 16) |
		((unsigned int)(buffer[0x13] & 0xff) << 24);
	switch (cksize) {
	case 16:
		aparam->file_hlen = 0x2C;
		break;
	case 18:
		aparam->file_hlen = 0x2E;
		break;
	case 40:
		aparam->file_hlen = 0x44;
		break;
	default:
		FT2_LOG_ERR("unknow chunk size: %d\n", cksize);
		ret = -FT2_ERROR;
		goto error;
	}
	memcpy(aparam->file_header, buffer, aparam->file_hlen);
	ret = FT2_OK;
error:
	if (fp)
		fclose(fp);
	return ret;
}

static int audio_playback(struct audio_inst *ainst)
{
	FILE *fp;
	int ret;
	char filepath[128];

	snprintf(filepath, sizeof(filepath),
			"%s/audio/%s", ainst->aparam->path,
			ainst->aparam->file_name);

	fp = fopen(filepath, "rb");
	if (!fp) {
		FT2_LOG_ERR("no file: %s\n", filepath);
		ret = -FT2_ERROR;
		goto error;
	}
	fseek(fp, ainst->aparam->file_hlen, SEEK_SET);

	while (FT2_OK == audio_ret) {
		ret = fread(ainst->audiobuf, 1, ainst->buflen, fp);
		if (ret == 0)
			break;

		ret = snd_pcm_writei(ainst->handle, ainst->audiobuf,
				ainst->period_frames);
		if (ret < 0) {
			usleep(2000);
			if (ret == -EPIPE) {
				FT2_LOG_ERR("underrun occurred\n");
				snd_pcm_prepare(ainst->handle);
				ret = FT2_OK;
			} else {
				FT2_LOG_ERR("fail to write audio data\n");
				ret = FT2_ERROR;
				goto error;
			}
		}
	}

	ret = FT2_OK;
error:
	if (fp)
		fclose(fp);
	return ret;
}

static int audio_capture(struct audio_inst *ainst)
{
	FILE *fp;
	int ret;
	char filepath[128];
	int rate, format;
	char type[10];
	uint64_t time;
	uint64_t number;
	uint64_t fdcount;
	int header_len = ainst->aparam->file_hlen;

	switch (ainst->aparam->rate) {
	case 48000:
		rate = 48;
		break;
	case 44100:
		rate = 44;
		break;
	default:
		FT2_LOG_ERR("unknow pcm rate: %d\n", ainst->aparam->rate);
		ret = -FT2_ERROR;
		goto error;
	}

	switch (ainst->aparam->format) {
	case SND_PCM_FORMAT_S16_LE:
		format = 16;
		break;
	case SND_PCM_FORMAT_S24_3LE:
		format = 24;
		break;
	default:
		FT2_LOG_ERR("unknow pcm format: %d\n", ainst->aparam->format);
		ret = -FT2_ERROR;
		goto error;
	}

	switch (ainst->aparam->type) {
	case FT2_TEST_AUDIO_STREAM_PATH_I2S:
		snprintf(type, sizeof(type), "i2s");
		break;
	case FT2_TEST_AUDIO_STREAM_PATH_AMIC:
		snprintf(type, sizeof(type), "amic");
		break;
	case FT2_TEST_AUDIO_STREAM_PATH_DMIC:
		snprintf(type, sizeof(type), "dmic");
		break;
	default:
		FT2_LOG_ERR("unknow device: %d\n", ainst->aparam->type);
		ret = -FT2_ERROR;
		goto error;
	}

	if (test_forever) {
			if (ainst->aparam->type == FT2_TEST_AUDIO_STREAM_PATH_I2S
				&& ainst->aparam->rate == 48000) {
				snprintf(filepath, sizeof(filepath),
					"%s/audio/sin1k_%dk_%db_%s.output%d",
					ainst->aparam->path, rate, format, type, aud_i2s_48k_count);
				aud_i2s_48k_count++;
			}
			else if (ainst->aparam->type == FT2_TEST_AUDIO_STREAM_PATH_I2S
				&& ainst->aparam->rate == 44100) {
				snprintf(filepath, sizeof(filepath),
					"%s/audio/sin1k_%dk_%db_%s.output%d",
					ainst->aparam->path, rate, format, type, aud_i2s_44k_count);
				aud_i2s_44k_count++;
			}
			else if (ainst->aparam->type == FT2_TEST_AUDIO_STREAM_PATH_AMIC
				&& ainst->aparam->rate == 48000) {
				snprintf(filepath, sizeof(filepath),
					"%s/audio/sin1k_%dk_%db_%s.output%d",
					ainst->aparam->path, rate, format, type, aud_amic_48k_count);
				aud_amic_48k_count++;
			}
			else if (ainst->aparam->type == FT2_TEST_AUDIO_STREAM_PATH_AMIC
				&& ainst->aparam->rate == 44100) {
				snprintf(filepath, sizeof(filepath),
					"%s/audio/sin1k_%dk_%db_%s.output%d",
					ainst->aparam->path, rate, format, type, aud_amic_44k_count);
				aud_amic_44k_count++;
			}

	} else {
		snprintf(filepath, sizeof(filepath),
				"%s/audio/sin1k_%dk_%db_%s.output",
				ainst->aparam->path, rate, format, type);
	}

	fp = fopen(filepath, "wb");
	if (!fp) {
		FT2_LOG_ERR("cann't open or create file: %s\n", filepath);
		ret = -FT2_ERROR;
		goto error;
	}

	/* begin wav header */
	ret = fwrite(ainst->aparam->file_header, 1, header_len, fp);
	if (ret < 0) {
		FT2_LOG_ERR("fail to write audio header to file\n");
		ret = FT2_ERROR;
		goto error;
	}

	number = 0;
	fdcount = 0;
	while (FT2_OK == audio_ret) {
		ret = snd_pcm_readi(ainst->handle, ainst->audiobuf,
				ainst->period_frames);
		if (ret < 0) {
			usleep(2000);
			if (ret == -EPIPE) {
				FT2_LOG_ERR("underrun occurred\n");
				snd_pcm_prepare(ainst->handle);
				ret = FT2_OK;
			} else {
				FT2_LOG_ERR("fail to read audio data\n");
				ret = FT2_ERROR;
				goto error;
			}
		}

		ret = fwrite(ainst->audiobuf, 1, ainst->buflen, fp);
		if (ret < 0) {
			FT2_LOG_ERR("fail to write audio data to file\n");
			ret = FT2_ERROR;
			goto error;
		}

		fdcount += ret;
		number += ainst->period_frames;
		time = number * 1000 * 1000 / ainst->aparam->rate;
		if (time > FT2_TEST_AUDIO_CAPTURE_TIME)
			break;
	}

	/* end wav header */
	number = fdcount + header_len - 8;
	ainst->aparam->file_header[0x04] = number & 0xFF;
	ainst->aparam->file_header[0x05] = (number >> 8) & 0xFF;
	ainst->aparam->file_header[0x06] = (number >> 16) & 0xFF;
	ainst->aparam->file_header[0x07] = (number >> 24) & 0xFF;
	ainst->aparam->file_header[header_len - 4] = fdcount & 0xFF;
	ainst->aparam->file_header[header_len - 3] = (fdcount >> 8) & 0xFF;
	ainst->aparam->file_header[header_len - 2] = (fdcount >> 16) & 0xFF;
	ainst->aparam->file_header[header_len - 1] = (fdcount >> 24) & 0xFF;
	fseek(fp, 0x0, SEEK_SET);
	ret = fwrite(ainst->aparam->file_header, 1, header_len, fp);
	if (ret < 0) {
		FT2_LOG_ERR("fail to write audio header to file\n");
		ret = FT2_ERROR;
		goto error;
	}

	ret = FT2_OK;
error:
	if (fp)
		fclose(fp);
	return ret;
}

static int audio_process_stream(struct audio_param *aparam,
			snd_pcm_stream_t stream)
{
	struct audio_inst *ainst;
	int ret;
	char pcm_name[10];

	ainst = (struct audio_inst *)calloc(1, sizeof(struct audio_inst));
	ainst->aparam = aparam;

	switch (aparam->type) {
	case FT2_TEST_AUDIO_STREAM_PATH_I2S:
		snprintf(pcm_name, sizeof(pcm_name), "hw:1,0");
		break;
	case FT2_TEST_AUDIO_STREAM_PATH_AMIC:
		snprintf(pcm_name, sizeof(pcm_name), "hw:0,1");
		break;
	 case FT2_TEST_AUDIO_STREAM_PATH_DMIC:
                snprintf(pcm_name, sizeof(pcm_name), "hw:0,0");
                break;
	default:
		FT2_LOG_ERR("unknow pcm device: %d\n", aparam->type);
		ret = -FT2_ERROR;
		goto error;
	}

	/* open pcm device */
	ret = snd_pcm_open(&ainst->handle, pcm_name, stream, 0);
	if (ret < 0) {
		FT2_LOG_ERR("fail to open snd device: %s, stream: %d\n",
				pcm_name, stream);
		ret = -FT2_ERROR;
		goto error;
	}

	/* set params */
	audio_set_params(ainst);

	ainst->buflen =
		snd_pcm_format_physical_width(ainst->aparam->format) *
		ainst->aparam->channels * ainst->period_frames / 8;
	ainst->audiobuf = (char *)calloc(1, sizeof(char) * ainst->buflen);

	if (stream == SND_PCM_STREAM_CAPTURE)
		ret = audio_capture(ainst);
	else
		ret = audio_playback(ainst);
	if (ret < 0) {
		FT2_LOG_ERR("fail to playback/capture\n");
		ret = -FT2_ERROR;
		goto error;
	}

	ret = FT2_OK;
error:
	if (ainst) {
		if (ainst->handle)
			snd_pcm_close(ainst->handle);
		if (ainst->audiobuf)
			free(ainst->audiobuf);
		free(ainst);
	}
	if (ret < 0)
		audio_ret = -FT2_ERROR;
	return ret;
}

static int audio_process_capture_stream(struct audio_param *aparam)
{
	return audio_process_stream(aparam, SND_PCM_STREAM_CAPTURE);
}

static int audio_process_playback_stream(struct audio_param *aparam)
{
	return audio_process_stream(aparam, SND_PCM_STREAM_PLAYBACK);
}

static int dmic_short_test(char *path, int type)
{
	struct audio_param *aparam;
	char filepath[128];
	int ret;

	aparam = (struct audio_param *)calloc(1, sizeof(struct audio_param));
	memset(aparam->file_header, 0, FT2_TEST_AUDIO_HEADER_LENGTH_MAX);
	aparam->file_hlen = 0;

	snprintf(filepath, sizeof(filepath),
				"%s/audio/sin1k_48k_24b.input", path);
	ret = audio_analyze_header(aparam, filepath);
	if (ret < 0) {
		FT2_LOG_ERR("fail to analyze the file header\n");
		goto error;
	}

	aparam->type = type;
	aparam->path = path;

	audio_process_capture_stream(aparam);

error:
	if (aparam)
		free(aparam);
	if (ret < 0)
		return -FT2_ERROR;
	return FT2_OK;
}

void *run_capture(void *data)
{
	struct audio_param *aparam = (struct audio_param *)data;

	usleep(FT2_TEST_AUDIO_DELAY_TIME);

	audio_process_capture_stream(aparam);

	return NULL;
}

static int __audio_start_test(char *path, int type, char *file_name)
{
	struct audio_param *aparam;
	int ret;
	char filepath[128];
	pthread_t pid;

	aparam = (struct audio_param *)calloc(1, sizeof(struct audio_param));
	memset(aparam->file_header, 0, FT2_TEST_AUDIO_HEADER_LENGTH_MAX);
	aparam->file_hlen = 0;

	snprintf(filepath, sizeof(filepath),
				"%s/audio/%s", path, file_name);
	ret = audio_analyze_header(aparam, filepath);
	if (ret < 0) {
		FT2_LOG_ERR("fail to analyze the file header\n");
		goto error;
	}

	aparam->type = type;
	aparam->path = path;
	aparam->file_name = file_name;

	audio_ret = FT2_OK;

	ret = pthread_create(&pid, NULL, run_capture, (void *)aparam);
	if (ret) {
		FT2_LOG_ERR("fail to create capture thread, ret = %d\n", ret);
		goto error;
	}
	audio_process_playback_stream(aparam);
	if (pid)
		pthread_join(pid, NULL);

	ret = audio_ret;
error:
	if (aparam)
		free(aparam);
	if (ret < 0)
		return -FT2_ERROR;
	return FT2_OK;
}

static int audio_start_test(char *path, int type)
{
	int i, ret;

	for (i = 0; i < FT2_TEST_AUDIO_INPUT_FILE_NUM; i++) {
		ret = __audio_start_test(path, type, ft2_audio_input_file[i]);
		if (ret < 0) {
			FT2_LOG_ERR("fail to test %s\n",
					ft2_audio_input_file[i]);
			return -FT2_ERROR;
		}
	}

	return FT2_OK;
}

int ft2_audio_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	char *test_name;
	int ret;

	if (!pcmd)
		return -FT2_NULL_POINT;

	if (audio_check_path(path)) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	test_name = pcmd->parameter->content;
	/* digital audio */
	if (!strcasecmp("i2s_short", test_name)) {
		ret = audio_start_test(path, FT2_TEST_AUDIO_STREAM_PATH_I2S);
		if (ret < 0) {
			FT2_LOG_ERR("fail to test I2S_short\n");
			goto error;
		}
	} else if (!strcasecmp("dmic_short", test_name)) {
		ret = dmic_short_test(path, FT2_TEST_AUDIO_STREAM_PATH_DMIC);
		if (ret < 0) {
                        FT2_LOG_ERR("fail to test DMIC_short\n");
                        goto error;
                }
	} else if (!strcasecmp("pdmout_dmicin", test_name)) {


	/* analog audio */
	} else if (!strcasecmp("hpout_linein", test_name)) {
		ret = audio_start_test(path, FT2_TEST_AUDIO_STREAM_PATH_AMIC);
		if (ret < 0) {
			FT2_LOG_ERR("fail to test HPout_Linein\n");
			goto error;
		}
	} else if (!strcasecmp("lineout_linein", test_name)) {

	} else {
		FT2_LOG_ERR("fail to get correct cmd\n");
                goto error;
	}

	append_pt_command_result(pcmd,
			FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;

error:
	append_pt_command_result(pcmd,
			FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
	return -FT2_ERROR;
}

void ft2_audio_cleanup(void *priv)
{
	aud_amic_44k_count=0;
	aud_i2s_44k_count=0;
	aud_amic_48k_count=0;
	aud_i2s_48k_count=0;
}

