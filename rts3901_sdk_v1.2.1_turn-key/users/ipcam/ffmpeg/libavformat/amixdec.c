#include <sys/ipc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "url.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/intfloat.h"
#include "libavutil/opt.h"
#include <libswresample/swresample.h>
#include "avformat.h"
#include "internal.h"
#include "avio_internal.h"
#include "amix.h"

int amix_lock_file(AVFormatContext *s)
{
	struct flock lockinfo;
	int ret = 0;
	AmixData *amix = s->priv_data;

	amix->fd = open(AMIX_FILE_DIR, O_CREAT | O_RDWR, 0666);
	if (amix->fd < 0) {
		av_log(s, AV_LOG_ERROR, "%s: fail to open file %s\n",
				__func__, AMIX_FILE_DIR);
		return amix->fd;
	}

	lockinfo.l_type = F_WRLCK;
	lockinfo.l_whence = SEEK_SET;
	lockinfo.l_start = 0;
	lockinfo.l_len = 0;

	ret = fcntl(amix->fd, F_SETLKW, &lockinfo);

	return ret;
}

void amix_unlock_file(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;

	if (amix->fd > 0)
		close(amix->fd);
	amix->fd = -1;
}

int amix_create_shm(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	key_t key = ftok(AMIX_FILE_DIR, 0);

	if (key == -1)
		return -errno;

	amix->shm_id = shmget(key, amix->shm_size, IPC_CREAT | 0666);
	if (amix->shm_id < 0)
		return -errno;

	amix->shm_buf = shmat(amix->shm_id, NULL, 0);
	if (((void *) -1) == amix->shm_buf)
		return -errno;

	return 0;
}

static void amix_uninit_input_stream(InputStream *stream)
{
	AVStream *avstream;

	if (stream->shm_ctx) {
		avstream = stream->shm_ctx->streams[0];
		if (avstream)
			avcodec_close(avstream->codec);
		avformat_close_input(&stream->shm_ctx);
	}

	if (stream->swr_ctx)
		swr_free(&stream->swr_ctx);

	if (stream->swr_buf) {
		av_freep(&stream->swr_buf[0]);
		av_freep(&stream->swr_buf);
	}
}

static int amix_init_shm_input(InputStream *stream)
{
	AVFormatContext *s = stream->amix->parent;
	AVInputFormat *iformat = NULL;
	AVDictionary *dict = NULL;
	AVStream *avstream = NULL;
	AVCodec *codec = NULL;
	int ret = 0;

	iformat = av_find_input_format("shm");
	if (!iformat) {
		av_log(s, AV_LOG_ERROR, "cannot find input format\n");
		return -1;
	}

	do {
		char value[16] = {0};
		snprintf(value, sizeof(value), "%d", AVFMT_FLAG_NONBLOCK);
		av_dict_set(&dict, "fflags", value, 0);
	} while (0);

	stream->shm_ctx = avformat_alloc_context();
	if (!stream->shm_ctx) {
		av_log(s, AV_LOG_ERROR, "can't alloc context\n");
		ret = -1;
		goto out;
	}

	if (stream->audio_type == AMIX_AUDIO_TYPE_FILE)
		av_dict_set(&stream->shm_ctx->metadata,
				"shm_read_position", "0", 0);

	ret = avformat_open_input(&stream->shm_ctx,
			stream->path, iformat, &dict);
	if (ret) {
		av_log(s, AV_LOG_ERROR,
				"open input format fail: %s, %d\n",
				stream->path, ret);
		goto out;
	}

	avstream = stream->shm_ctx->streams[0];

	codec = avcodec_find_decoder(avstream->codec->codec_id);
	if (!codec) {
		av_log(s, AV_LOG_ERROR, "can't find decoder\n");
		ret = -EINVAL;
		goto out;
	}

	ret = avcodec_open2(avstream->codec, codec, NULL);
	if (ret)
		goto out;
out:
	if (dict)
		av_dict_free(&dict);

	if (ret) {
		if (avstream)
			avcodec_close(avstream->codec);
		if (stream->shm_ctx)
			avformat_close_input(&stream->shm_ctx);
	}

	return ret;
}

static int amix_need_resample(InputStream *stream)
{
	AVStream *in  = stream->shm_ctx->streams[0];
	AmixData *amix = stream->amix;

	if (in->codec->sample_rate != amix->sample_rate ||
		in->codec->channels != amix->channels ||
		in->codec->channel_layout != amix->channel_layout ||
		in->codec->sample_fmt != amix->sample_fmt)
		return 1;

	return 0;
}

static int amix_init_swr_context(InputStream *stream)
{
	AVFormatContext *s = stream->amix->parent;
	AVCodecContext *ic_ctx = NULL;
	struct SwrContext  *swr_ctx = NULL;
	int ret = 0;
	AmixData *amix = stream->amix;

	if (!stream->shm_ctx)
		return -EINVAL;

	if (stream->shm_ctx->streams[0])
		ic_ctx = stream->shm_ctx->streams[0]->codec;

	if (!ic_ctx)
		return -EINVAL;

	swr_ctx = swr_alloc();
	if (!swr_ctx)
		return -EINVAL;

	av_opt_set_int(swr_ctx, "in_channel_layout", ic_ctx->channel_layout, 0);
	av_opt_set_int(swr_ctx, "in_channel_count", ic_ctx->channels, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", ic_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", ic_ctx->sample_fmt, 0);

	av_opt_set_int(swr_ctx, "out_channel_layout", amix->channel_layout, 0);
	av_opt_set_int(swr_ctx, "out_channel_count", amix->channels, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", amix->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", amix->sample_fmt, 0);

	av_opt_set_int(swr_ctx, "phase_shift", 0, 0);

	ret = swr_init(swr_ctx);
	if (ret < 0) {
		av_log(s, AV_LOG_ERROR, "Failed at initialize SwrContext\n");
		goto fail;
	}

	stream->max_nb_samples = 1024;
	ret = av_samples_alloc_array_and_samples(&stream->swr_buf,
			&stream->swr_linesize, amix->channels,
			stream->max_nb_samples, amix->sample_fmt, 0);
	if (ret < 0) {
		av_log(s, AV_LOG_ERROR, "alloc destination samplse fail\n");
		goto fail;
	}

	stream->swr_ctx = swr_ctx;

	return 0;
fail:
	if (swr_ctx)
		swr_free(&swr_ctx);

	if (stream->swr_buf) {
		av_freep(&stream->swr_buf[0]);
		av_freep(&stream->swr_buf);
	}

	stream->swr_ctx = NULL;
	stream->swr_buf = NULL;
	stream->swr_linesize = 0;
	stream->max_nb_samples = 0;
	return ret;
}

static int amix_init_input_stream(InputStream *stream)
{
	int ret;
	AVFormatContext *s = stream->amix->parent;

	ret = amix_init_shm_input(stream);
	if (ret) {
		av_log(s, AV_LOG_ERROR, "init_ffm_input failed\n");
		goto out;
	}

out:
	if (ret)
		amix_uninit_input_stream(stream);

	return ret;
}

static void amix_get_write_index(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	uint8_t *pos = amix->shm_buf;

	amix_lock_file(s);
	pos += 20;
	pos += shm_r64(pos, &amix->write_index);
	amix_unlock_file(s);
}

static int amix_is_avail_data(AVFormatContext *s, int size)
{
	AmixData *amix = s->priv_data;
	uint64_t avail_size;

	if (amix->read_index == amix->write_index)
		return 0;
	else if (amix->read_index < amix->write_index)
		avail_size = amix->write_index - amix->read_index;
	else
		avail_size = (amix->shm_size - AMIX_HEADER_SIZE)
			- (amix->read_index - amix->write_index);

	if (size <= avail_size)
		return 1;
	else
		return 0;
}

static int amix_resample_audio(InputStream *stream,
		AVFrame *frame_d, AVFrame *frame_s)
{
	AmixData *amix = stream->amix;
	AVFormatContext *s = amix->parent;
	AVFormatContext *shm_ctx = stream->shm_ctx;
	AVStream *in  = shm_ctx->streams[0];
	AVCodecContext *ic_ctx = in->codec;
	struct SwrContext *swr_ctx = stream->swr_ctx;
	int dst_nb_samples = 0;
	int dst_sample_size = 0;
	int ret = 0;

	dst_nb_samples = av_rescale_rnd(
			/* swr_get_delay(swr_ctx, ic_ctx->sample_rate) */
			0
			+ frame_d->nb_samples,
			amix->sample_rate, ic_ctx->sample_rate,
			AV_ROUND_UP);

	if (dst_nb_samples > stream->max_nb_samples) {
		av_free(stream->swr_buf[0]);
		ret = av_samples_alloc(stream->swr_buf, &stream->swr_linesize,
				amix->channels, dst_nb_samples,
				amix->sample_fmt, 1);
		if (ret < 0)
			return ret;
		stream->max_nb_samples = dst_nb_samples;
	}

	ret = swr_convert(swr_ctx, stream->swr_buf, dst_nb_samples,
			(const uint8_t **) (&frame_d->data[0]),
			frame_d->nb_samples);
	if (ret < 0) {
		av_log(s, AV_LOG_ERROR, "swr convert fail\n");
		return ret;
	}

	dst_sample_size = av_samples_get_buffer_size(NULL,
			amix->channels, ret,
			amix->sample_fmt, 1);
	frame_s->nb_samples = ret;
	ret = avcodec_fill_audio_frame(frame_s, amix->channels,
			amix->sample_fmt, stream->swr_buf[0],
			dst_sample_size, 1);
	if (ret < 0) {
		av_log(s, AV_LOG_ERROR, "fill audio frame fail\n");
		return ret;
	}

	return 0;
}

static int amix_decode_audio_frame(InputStream *stream, AVPacket *pkt_enc)
{
	AmixData *amix = stream->amix;
	AVFormatContext *s = amix->parent;
	AVFormatContext *shm_ctx = stream->shm_ctx;
	AVStream *in  = shm_ctx->streams[0];
	AVFrame *frame_d = NULL;
	AVFrame *frame_s = NULL;
	int decode_samples = 0;
	int ret = 0;

	frame_d = av_frame_alloc();
	frame_s = av_frame_alloc();
	if (!frame_d || !frame_s) {
		av_log(s, AV_LOG_ERROR, "alloc frame fail\n");
		return -ENOMEM;
	}

	while (pkt_enc->size > 0) {
		AVFrame *tmp;
		int len = 0;
		int got = 0;
		int res = 0;

		len = avcodec_decode_audio4(in->codec, frame_d, &got, pkt_enc);
		if (len > 0) {
			pkt_enc->size -= len;
			pkt_enc->data += len;
		} else {
			break;
		}

		if (!got)
			continue;

		frame_d->pts = pkt_enc->pts;
		decode_samples += frame_d->nb_samples;

		if (stream->swr_flag == 1) {
			if (amix_need_resample(stream)) {
				ret = amix_init_swr_context(stream);
				if (ret) {
					av_log(s, AV_LOG_ERROR,
						"init_swr_context failed\n");
					break;
				}
				stream->swr_flag = 2;
			} else {
				stream->swr_flag = 0;
			}
		}

		if (stream->swr_ctx) {
			res = amix_resample_audio(stream, frame_d, frame_s);
			if (res < 0)
				continue;
			frame_s->pts = frame_d->pts;
			frame_s->pkt_size = frame_d->pkt_size;
			frame_s->sample_rate = amix->sample_rate;
			frame_s->channel_layout = amix->channel_layout;
			frame_s->channels = amix->channels;
			frame_s->format = amix->sample_fmt;
			tmp = frame_s;
		} else {
			tmp = frame_d;
		}

		got = 0;

		ret = av_audio_fifo_write(stream->fifo,
				(void **)tmp->extended_data,
				tmp->nb_samples);
		if (ret < 0) {
			av_log(s, AV_LOG_ERROR, "fail to write audio fifo (%d)\n", ret);
			break;
		}

		if (frame_d)
			av_frame_unref(frame_d);

		if (frame_s)
			av_frame_unref(frame_s);
	}

	if (frame_d)
		av_frame_free(&frame_d);

	if (frame_s)
		av_frame_free(&frame_s);

	return 0;
}

static void amix_decrease_number(InputStream *stream)
{
	AmixData *amix = stream->amix;
	AVFormatContext *s = amix->parent;
	uint8_t *pos;

	amix_lock_file(s);
	pos = amix->shm_buf;
	pos += AMIX_POS_FLAG + stream->index;
	pos += shm_w8(pos, 0);
	pos = amix->shm_buf;
	pos += AMIX_POS_CUR_NB;
	amix->cur_num--;
	pos += shm_w32(pos, amix->cur_num);
	amix_unlock_file(s);
}

static void amix_calculate_scale(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	InputStream *pos = amix->head;
	int shift = 0;
	int scale = 0;

	switch (amix->cur_num) {
	case 0:
	case 1:
		shift = 0;
		scale = 0;
		break;
	case 2:
		shift = 1;
		scale = 0;
		break;
	case 3:
		shift = 0;
		scale = AMIX_DEC_THREE;
		break;
	case 4:
		shift = 2;
		scale = 0;
		break;
	case 5:
		shift = 0;
		scale = AMIX_DEC_FIVE;
	default:
		shift = 3;
		scale = 0;
		break;
	}

	while (pos != NULL) {
		pos->input_scale = scale;
		pos->input_shift = shift;
		pos = pos->next;
	}
}

static void amix_check_stream_eof(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	InputStream *stream = amix->head;
	InputStream *pre_str = NULL;

	while (stream != NULL) {
		stream->avail_samples = av_audio_fifo_size(stream->fifo);
		if (stream->avail_samples == 0 && stream->flag == 1) {
			if (pre_str == NULL)
				amix->head = stream->next;
			else
				pre_str->next = stream->next;
			amix_uninit_input_stream(stream);
			if (stream->path) {
				remove(stream->path);
				free(stream->path);
			}
			if (stream->fifo)
				av_audio_fifo_free(stream->fifo);
			amix_decrease_number(stream);
			free(stream);
			if (pre_str == NULL)
				stream = amix->head;
			else
				stream = pre_str->next;
			amix_calculate_scale(s);
			continue;
		}
		pre_str = stream;
		stream = stream->next;
	}
}

static int amix_get_available_samples(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	InputStream *pos;
	int avail_samples = INT_MAX;

	amix_check_stream_eof(s);

	pos = amix->head;
	while (pos != NULL) {
		avail_samples = FFMIN(avail_samples, pos->avail_samples);
		pos = pos->next;
	}

	if (avail_samples == INT_MAX)
		return 0;
	return avail_samples;
}

static void amix_drop_outdated_frame(InputStream *stream)
{
	AVPacket pkt_enc;
	int ret = 0;

	av_init_packet(&pkt_enc);

	do {
		ret = av_read_frame(stream->shm_ctx, &pkt_enc);
		if (ret < 0 || pkt_enc.size <= 0) {
			av_free_packet(&pkt_enc);
			break;
		}
		av_free_packet(&pkt_enc);
	} while(1);
}

static void amix_get_avarage_base(int64_t *base, int len, int64_t *pts_base)
{
	int i, j;
	int64_t tmp;

	if (len < 3)
		*pts_base = base[0];

	for (i = 0; i < len; i++) {
		for (j = len - 1; j > i; j--) {
			if (base[j] < base[j - 1]) {
				tmp = base[j];
				base[j] = base[j - 1];
				base[j - 1] = tmp;
			}
		}
	}

	*pts_base = base[len / 2];
}

static void amix_init_stream(InputStream *stream)
{
	AmixData *amix = stream->amix;
	AVFormatContext *s = amix->parent;
	InputStream *pos = amix->head;
	int ret;

	pthread_detach(pthread_self());

	ret = amix_init_input_stream(stream);
	if (ret) {
		av_log(s, AV_LOG_ERROR, "init_input_stream failed\n");
		amix_uninit_input_stream(stream);
		free(stream->path);
		free(stream);
		stream = NULL;
		return;
	}

	if (stream->audio_type == AMIX_AUDIO_TYPE_VOICE)
		amix_drop_outdated_frame(stream);

	if (pos == NULL) {
		amix->head = stream;
	} else {
		while (pos->next != NULL)
			pos = pos->next;
		pos->next = stream;
	}

	amix_calculate_scale(s);
}

static void amix_process_add_input(AVFormatContext *s, int index,
		uint32_t audio_type)
{
	AmixData *amix = s->priv_data;
	InputStream *stream;

	amix->cur_num++;
	stream = calloc(1, sizeof(InputStream));
	stream->path = calloc(1, sizeof(char) * 64);
	sprintf(stream->path, "%samix%d.am",
			FILE_PRE_DIRECTORY, index);
	stream->amix = amix;
	stream->flag = 0;
	stream->index = index;
	stream->next = NULL;
	stream->fifo = av_audio_fifo_alloc(amix->sample_fmt,
			amix->channels, 1024);
	if (!stream->fifo) {
		av_log(s, AV_LOG_ERROR, "fail to alloc fifo\n");
		free(stream->path);
		free(stream);
		stream = NULL;
		return;
	}
	stream->input_scale = 0;
	stream->input_shift = 0;
	stream->pts_base = (uint64_t)0;
	stream->swr_ctx = NULL;
	stream->pts_ptr = 0;
	stream->pts_base = AV_NOPTS_VALUE;
	stream->swr_flag = 1;
	stream->audio_type = audio_type;

	amix_init_stream(stream);
}

static void amix_process_del_input(AVFormatContext *s, int index)
{
	AmixData *amix = s->priv_data;
	InputStream *stream = amix->head;
	char args[64];

	sprintf(args, "%samix%d.am", FILE_PRE_DIRECTORY, index);
	while (stream) {
		if (strcmp(stream->path, args) == 0) {
			stream->flag = 1;
			break;
		}
		stream = stream->next;
	}
}

static int amix_check_read_index(AVFormatContext *s)
{
	int ret;
	PointerInform *pi = NULL;
	AmixData *amix = s->priv_data;

	amix_get_write_index(s);

	ret = amix_is_avail_data(s, sizeof(*pi));
	if (ret) {
		amix->ring_pos = amix->ring_begin + amix->read_index;
		if (amix->ring_pos >= amix->ring_end)
			amix->ring_pos = amix->ring_begin;

		pi = (PointerInform *) amix->ring_pos;

		if (INFORM_ID != pi->p.id) {
			av_log(s, AV_LOG_ERROR,
				"%s: wrong position of ring buffer\n",
				__func__);
			amix->read_index = amix->write_index;
			return AVERROR(EAGAIN);
		}

		amix->ring_pos += sizeof(*pi);
		amix->read_index = (uint8_t *)amix->ring_pos -
				(uint8_t *)amix->ring_begin;

		if (pi->p.flag == AMIX_ADD_INPUT)
			amix_process_add_input(s, pi->p.index, pi->p.type);
		else if (pi->p.flag == AMIX_DEL_INPUT)
			amix_process_del_input(s, pi->p.index);
	}

	return 0;
}

static int amix_close(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;

	shmctl(amix->shm_id, IPC_RMID, NULL);

	if (amix->shm_buf) {
		shmdt(amix->shm_buf);
		amix->shm_buf = NULL;
	}

	return 0;
}

static int amix_read_header(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	AVStream *st;
	AVCodecContext *codec;
	int ret = 0, i;
	uint8_t *pos = NULL;

	ret = amix_lock_file(s);
	if (ret < 0)
		goto error;

	amix->channel_layout = av_get_default_channel_layout(amix->channels);
	amix->sample_fmt = AV_SAMPLE_FMT_S16;
	amix->shm_size = AMIX_PACKET_SIZE;
	ret = amix_create_shm(s);
	if (ret < 0)
		goto error;

	st = avformat_new_stream(s, NULL);
	if (!st)
		goto error;
	avpriv_set_pts_info(st, 64, 1, amix->sample_rate);
	codec = st->codec;
	codec->codec_id = AV_CODEC_ID_PCM_S16LE;
	codec->codec_type = AVMEDIA_TYPE_AUDIO;
	codec->time_base.num = amix->sample_rate;
	codec->time_base.den = 1;
	codec->channel_layout = amix->channel_layout;
	codec->sample_rate = amix->sample_rate;
	codec->channels = amix->channels;
	codec->sample_fmt = amix->sample_fmt;

	pos = amix->shm_buf;
	/* header */
	pos += shm_w32(pos, MKTAG('A', 'M', 'I', 'C'));
	pos += shm_w64(pos, amix->shm_size);
	pos += shm_w32(pos, MAX_INPUT_NUMBER); /* max input number */
	pos += shm_w32(pos, 0); /* cur input number */
	pos += shm_w64(pos, 0); /* current write position */
	for (i = 0; i < MAX_INPUT_NUMBER; i++)
		pos += shm_w8(pos, 0);

	pos += shm_w64(pos, 0); /* end of header */

	while (pos < (amix->shm_buf + AMIX_HEADER_SIZE))
		pos += shm_w8(pos, 0);

	amix->cur_num = 0;
	amix->read_index = 0;
	amix->ring_begin = amix->shm_buf + AMIX_HEADER_SIZE;
	amix->ring_pos = amix->ring_begin;
	amix->ring_end = amix->shm_buf + amix->shm_size;

	amix->head = NULL;
	amix->parent = s;

	ret = 0;

error:
	amix_unlock_file(s);
	return ret;
}

static AVFrame *amix_get_default_frame(int nb_samples, AmixData *amix)
{
	AVFrame *frame = av_frame_alloc();
	int ret;

	if (!frame)
		return NULL;

	frame->nb_samples = nb_samples;
	frame->format = amix->sample_fmt;
	av_frame_set_channels(frame, amix->channels);
	frame->channel_layout = amix->channel_layout;
	frame->sample_rate = amix->sample_rate;
	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		av_frame_free(&frame);
		return NULL;
	}

	av_samples_set_silence(frame->extended_data, 0, nb_samples,
			amix->channels, amix->sample_fmt);

	return frame;
}

static void amix_vector_fmac_scalar(char *dst, char *src,
				int sample, int channels,
				int format, InputStream *stream)
{
	int i;
	uint8_t *pos_u8_d;
	uint8_t *pos_u8_s;
	int16_t *pos_s16_d;
	int16_t *pos_s16_s;
	int len = sample * channels;
	uint32_t datatmp;

	if (stream->input_scale == 0) {
		switch (format) {
		case AV_SAMPLE_FMT_U8:
			pos_u8_d = (uint8_t *)dst;
			pos_u8_s = (uint8_t *)src;
			for (i = 0; i < len; i++)
				*(pos_u8_d++) +=
					(*(pos_u8_s++) >>
					 stream->input_shift);
		break;
		case AV_SAMPLE_FMT_S16:
			pos_s16_d = (int16_t *)dst;
			pos_s16_s = (int16_t *)src;
			for (i = 0; i < len; i++)
				*(pos_s16_d++) +=
					(*(pos_s16_s++) >>
					 stream->input_shift);
			break;
		}
	} else {
		switch (format) {
		case AV_SAMPLE_FMT_U8:
			pos_u8_d = (uint8_t *)dst;
			pos_u8_s = (uint8_t *)src;
			for (i = 0; i < len; i++) {
				datatmp = 0x0000;
				datatmp = *(pos_u8_s++);
				datatmp = datatmp * stream->input_scale;
				*(pos_u8_d++) += ((datatmp >> 8) & 0xff);
			}
			break;
		case AV_SAMPLE_FMT_S16:
			pos_s16_d = (int16_t *)dst;
			pos_s16_s = (int16_t *)src;
			for (i = 0; i < len; i++) {
				datatmp = 0x0000;
				datatmp = *(pos_s16_s++);
				datatmp = datatmp * stream->input_scale;
				*(pos_s16_d++) += ((datatmp >> 8) & 0xffff);
			}
			break;
		}
	}
}

static int amix_read_packet(AVFormatContext *s, AVPacket *pkt)
{
	AmixData *amix = s->priv_data;
	int ret, len, available_samples = 0;
	AVFrame *out_frame = NULL;
	AVFrame *in_frame = NULL;
	InputStream *stream;

	amix_check_read_index(amix->parent);

	if (amix->head != NULL) {
		stream = amix->head;

		while (stream != NULL) {
			len = av_audio_fifo_size(stream->fifo);
			if (len >= AMIX_FIFO_MAX_TH) {
				stream = stream->next;
				continue;
			}

			AVPacket pkt;

			av_init_packet(&pkt);

			/* TODO: limit pkt */
			ret = av_read_frame(stream->shm_ctx, &pkt);
			if (ret < 0 || pkt.size <= 0) {
				av_free_packet(&pkt);
				stream = stream->next;
				continue;
			}

			if (stream->audio_type == AMIX_AUDIO_TYPE_VOICE &&
				stream->pts_ptr < AMIX_PTS_SAMPLE_NUM) {
				struct timeval tv;

				if (pkt.pts != AV_NOPTS_VALUE) {
					gettimeofday(&tv, NULL);
					stream->pts_sample[stream->pts_ptr++] =
					pkt.pts -
					(((uint64_t)tv.tv_sec) * 1000000 +
					tv.tv_usec);
				} else {
					stream->pts_sample[stream->pts_ptr++] =
						AV_NOPTS_VALUE;
				}

				if (stream->pts_ptr == AMIX_PTS_SAMPLE_NUM) {
					amix_get_avarage_base(stream->pts_sample,
							AMIX_PTS_SAMPLE_NUM,
							&stream->pts_base);
					if (stream->pts_base != AV_NOPTS_VALUE)
						stream->pts_base -= 500000;
				}
			}

			if (stream->audio_type == AMIX_AUDIO_TYPE_VOICE &&
				pkt.pts != AV_NOPTS_VALUE &&
				stream->pts_ptr >= AMIX_PTS_SAMPLE_NUM &&
				stream->pts_base != AV_NOPTS_VALUE) {
				struct timeval tv;
				int64_t curpts_delta;

				gettimeofday(&tv, NULL);
				curpts_delta = pkt.pts -
					(((uint64_t)tv.tv_sec) * 1000000 +
					 tv.tv_usec);
				if (curpts_delta < stream->pts_base) {
					av_free_packet(&pkt);
					amix_drop_outdated_frame(stream);
					stream = stream->next;
					continue;
				}
			}

			amix_decode_audio_frame(stream, &pkt);

			av_free_packet(&pkt);
			stream = stream->next;
		}
	}

	/* TODO min available value */
	available_samples = amix_get_available_samples(s);
	available_samples = FFMIN(available_samples, AMIX_FIFO_UP_TH);

	if (available_samples == 0) {
		if (amix->head == NULL)
			ret = AVERROR(ENODATA);
		else
			ret = AVERROR(EAGAIN);
		goto err;
	}
	out_frame = amix_get_default_frame(available_samples, amix);
	if (!out_frame) {
		ret = AVERROR(ENOMEM);
		goto err;
	}

	in_frame = amix_get_default_frame(available_samples, amix);
	if (!in_frame) {
		ret = AVERROR(ENOMEM);
		goto err;
	}

	if (amix->head != NULL) {
		stream = amix->head;

		while (stream != NULL) {
			av_audio_fifo_read(stream->fifo,
					(void **)in_frame->extended_data,
					available_samples);
			amix_vector_fmac_scalar(
					(char *)out_frame->extended_data[0],
					(char *)in_frame->extended_data[0],
					available_samples, amix->channels,
					amix->sample_fmt,
					stream);
			stream = stream->next;
		}
	}

	if (out_frame->format == AV_SAMPLE_FMT_S16)
		len = out_frame->nb_samples * out_frame->channels * 2;
	else
		len = out_frame->nb_samples * out_frame->channels;
	ret = av_new_packet(pkt, len);
	if (ret < 0) {
		ret = AVERROR(ENOMEM);
		goto err;
	}

	memcpy(pkt->data, out_frame->data[0], len);
	pkt->pts = AV_NOPTS_VALUE;
	pkt->dts = AV_NOPTS_VALUE;
	pkt->stream_index = 0;
	ret = 0;
err:
	if (out_frame)
		av_frame_free(&out_frame);
	if (in_frame)
		av_frame_free(&in_frame);

	return ret;
}

static int amix_seek(AVFormatContext *s, int stream_index,
		int64_t wanted_pts, int flags)
{
	return -1;
}

static int amix_probe(AVProbeData *p)
{
	return 0;
}

static const AVOption options[] = {
	{"sample_rate", "", offsetof(AmixData, sample_rate), AV_OPT_TYPE_INT, {.i64 = AMIX_OUT_SAMPLE_RATE}, 1, INT_MAX, AV_OPT_FLAG_DECODING_PARAM},
	{"channels", "", offsetof(AmixData, channels), AV_OPT_TYPE_INT, {.i64 = AMIX_OUT_CHANNELS}, 1, INT_MAX, AV_OPT_FLAG_DECODING_PARAM},
	{NULL},
};

static const AVClass amix_demuxer_class = {
	.class_name = "amix demuxer",
	.item_name = av_default_item_name,
	.option = options,
	.version = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_amix_demuxer = {
	.name = "amix",
	.long_name = NULL_IF_CONFIG_SMALL("audio mix"),
	.priv_data_size = sizeof(AmixData),
	.read_probe = amix_probe,
	.read_header = amix_read_header,
	.read_packet = amix_read_packet,
	.read_close = amix_close,
	.read_seek = amix_seek,
	.flags = AVFMT_NOFILE/* | AVFMT_NOSTREAMS*/,
	.priv_class = &amix_demuxer_class,
};
