#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include "libavutil/intreadwrite.h"
#include "libavutil/intfloat.h"
#include "libavutil/avassert.h"
#include "libavutil/parseutils.h"
#include "url.h"
#include "avformat.h"
#include "internal.h"
#include "amix.h"

static int set_codec_params_4_shm(AVFormatContext *s, AVStream *stream)
{
	AVCodecContext *codec = stream->codec;
	AVStream *avstream = s->streams[0];
	AVCodecContext *dcodec = avstream->codec;

	codec->codec_id = dcodec->codec_id;
	codec->codec_type = dcodec->codec_type;
	codec->bit_rate = dcodec->bit_rate;
	codec->time_base.num = dcodec->time_base.num;
	codec->time_base.den = dcodec->time_base.den;
	codec->channel_layout = dcodec->channel_layout;
	codec->sample_rate = dcodec->sample_rate;
	codec->channels = dcodec->channels;
	codec->sample_fmt = dcodec->sample_fmt;

	return 0;
}

void amix_uninit_shmoutput(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;

	if (amix->shm_ctx) {
		av_write_trailer(amix->shm_ctx);
		if (amix->shm_ctx->pb)
			avio_close(amix->shm_ctx->pb);
		avformat_free_context(amix->shm_ctx);
		amix->shm_ctx = NULL;
	}
}

int amix_init_shmoutput(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	AVOutputFormat  *oformat = NULL;
	AVStream *stream = NULL;
	int ret = 0;

	oformat = av_guess_format("shm", amix->shm_path, "");
	if (!oformat) {
		av_log(s, AV_LOG_ERROR, "can not get muxer shm\n");
		return -1;
	}

	ret = avformat_alloc_output_context2(&amix->shm_ctx,
			oformat, NULL, amix->shm_path);
	if (ret) {
		av_log(s, AV_LOG_ERROR, "Allco AVFormatContext for out fail\n");
		goto out;
	}

	stream = avformat_new_stream(amix->shm_ctx, NULL);
	if (!stream) {
		av_log(s, AV_LOG_ERROR, "alloc stream fail\n");
		goto out;
	}

	set_codec_params_4_shm(s, stream);
	ret = avio_open(&amix->shm_ctx->pb, amix->shm_path,
			AVIO_FLAG_WRITE);
	if (ret) {
		av_log(s, AV_LOG_ERROR, "colud not open %s: %s\n",
			amix->shm_path, av_err2str(ret));
		goto out;
	}

	av_dict_set(&amix->shm_ctx->metadata, "shm_size", "262144", 0);
	avformat_write_header(amix->shm_ctx, NULL);

	return 0;
out:
	amix_uninit_shmoutput(s);

	return -1;
}

static int amix_check_shm_index(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	uint8_t *pos = amix->shm_buf;
	int i;
	uint8_t flag;

	pos += AMIX_POS_FLAG;
	for (i = 0; i < MAX_INPUT_NUMBER; i++) {
		pos += shm_r8(pos, &flag);
		if (0 == flag) {
			pos -= 1;
			pos += shm_w8(pos, 1);
			return i;
		}
	}

	return -1;
}

static int amix_update_write_index(AVFormatContext *s, uint32_t flag,
		uint32_t type)
{
	AmixData *amix = s->priv_data;
	uint8_t *pos = amix->shm_buf;
	PointerInform *pi = NULL;

	pos += 20;
	pos += shm_r64(pos, &amix->write_index);

	amix->ring_pos = amix->ring_begin + amix->write_index;
	if (amix->ring_pos >= amix->ring_end)
		amix->ring_pos = amix->ring_begin;

	pi = (PointerInform *) amix->ring_pos;

	pi->p.id = INFORM_ID;
	pi->p.index = amix->shm_index;
	pi->p.flag = flag;
	pi->p.type = type;

	amix->ring_pos += sizeof(*pi);
	amix->write_index = (uint8_t *)amix->ring_pos -
			(uint8_t *)amix->ring_begin;
	pos -= 8;
	pos += shm_w64(pos, amix->write_index);

	return 0;
}

static int amix_write_header(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	int ret = 0;
	int tmp_index;
	uint8_t *pos = NULL;
	uint32_t tag = 0;
	AVDictionaryEntry *t;
	int audio_type = AMIX_AUDIO_TYPE_DEFAULT;

	ret = amix_lock_file(s);
	if (ret < 0)
		goto fail;

	if (t = av_dict_get(s->metadata, "audio_type", NULL, 0))
		audio_type = atoi(t->value);

	if (audio_type != AMIX_AUDIO_TYPE_VOICE &&
			audio_type != AMIX_AUDIO_TYPE_FILE) {
		ret = -EINVAL;
		goto fail;
	}

	amix->shm_size = AMIX_PACKET_SIZE;
	amix->shm_index = -1;
	ret = amix_create_shm(s);
	if (ret < 0)
		goto fail;

	pos = amix->shm_buf;
	/* header */
	pos += shm_r32(pos, &tag);
	if (tag != MKTAG('A', 'M', 'I', 'C')) {
		ret = -1;
		goto fail;
	}
	pos += shm_r64(pos, &amix->shm_size);
	pos += shm_r32(pos, &amix->max_num);
	pos += shm_r32(pos, &amix->cur_num);
	if (amix->cur_num >= amix->max_num) {
		ret = -1;
		goto fail;
	}
	pos -= 4;
	amix->cur_num++;
	pos += shm_w32(pos, amix->cur_num);

	amix->ring_begin = amix->shm_buf + AMIX_HEADER_SIZE;
	amix->ring_end = amix->shm_buf + amix->shm_size;

	tmp_index = amix_check_shm_index(s);
	if (tmp_index < 0)
		goto fail;

	sprintf(amix->shm_path, "%samix%d.am",
			FILE_PRE_DIRECTORY, tmp_index);
	ret = amix_init_shmoutput(s);
	if (ret < 0)
		goto fail;

	amix->shm_index = tmp_index;

	amix_update_write_index(s, AMIX_ADD_INPUT, audio_type);

	ret = 0;
fail:
	amix_unlock_file(s);

	return ret;
}

static int amix_write_packet(AVFormatContext *s, AVPacket *pkt)
{
	AmixData *amix = s->priv_data;
	int ret;

	ret = av_write_frame(amix->shm_ctx, pkt);
	if (ret < 0) {
printf("amix_write_packet error\n");
		return ret;
	}

	return 0;
}

static int amix_write_trailer(AVFormatContext *s)
{
	AmixData *amix = s->priv_data;
	int ret = 0;

	ret = amix_lock_file(s);
	if (ret < 0)
		goto error;

	if (amix->shm_index >= 0)
		amix_update_write_index(s, AMIX_DEL_INPUT,
				AMIX_AUDIO_TYPE_DEFAULT);

	if (amix->shm_buf) {
		shmdt(amix->shm_buf);
		amix->shm_buf = NULL;
	}

	amix_uninit_shmoutput(s);

	ret = 0;
error:
	amix_unlock_file(s);

	return ret;
}

AVOutputFormat ff_amix_muxer = {
	.name		= "amix",
	.long_name	= NULL_IF_CONFIG_SMALL("audio mix"),
	.priv_data_size	= sizeof(AmixData),
	.audio_codec	= AV_CODEC_ID_MP2,
	.video_codec	= AV_CODEC_ID_NONE,
	.write_header	= amix_write_header,
	.write_packet	= amix_write_packet,
	.write_trailer	= amix_write_trailer,
	.flags		= AVFMT_NOFILE,
};
