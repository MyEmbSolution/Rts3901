
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <rtscamkit.h>
#include <rtsavapi.h>
#include <rtsvideo.h>

#include "v4l2-common.h"

/**
 * Return timestamps to the user exactly as returned by the kernel
 */
#define V4L_TS_DEFAULT  0
/**
 * Autodetect the kind of timestamps returned by the kernel and convert to
 * absolute (wall clock) timestamps.
 */
#define V4L_TS_ABS      1
/**
 * Assume kernel timestamps are from the monotonic clock and convert to
 * absolute timestamps.
 */
#define V4L_TS_MONO2ABS 2

/**
 * Once the kind of timestamps returned by the kernel have been detected,
 * the value of the timefilter (NULL or not) determines whether a conversion
 * takes place.
 */
#define V4L_TS_CONVERT_READY V4L_TS_DEFAULT

#define GEN_I_FRAME(x) (x & 0x00000001)
#define DISABLE_GEN_I_FRAME(x) (x & 0xFFFFFFFE)

struct video_data {
	AVClass *class;
	int frame_format; /* V4L2_PIX_FMT_* */
	int width, height;
	int ts_mode;
	TimeFilter *timefilter;
	int64_t last_time_m;
	int v4l2_bufs;

	char *pixel_format; /**< Set by a private option. */
	char *framerate;    /**< Set by a private option. */

	char *codec_video; /**< Set by a private option. */
	/*H264 performance tuning params*/
	int h264_flag;
	int sliceSize; /*[0-127]*/
	int seiMessages; /*[0-1]*/
	int videoFullRange; /*[0-1]*/
	int constrainedIntraPrediction; /*[0-1]*/
	int disableDeblockingFilter; /*[0-2]*/
	int enableCabac; /*[0-1]*/
	int cabacInitIdc; /*[0-2]*/
	int transform8x8Mode; /*[0-2]*/
	int quarterPixelMv; /*[0-2]*/
	int roi1DeltaQp; /*[-15,0]*/
	int roi2DeltaQp; /*[-15,0]*/
	int adaptiveRoi; /*[-51,-30]*/
	int pictureRc; /*[0-1]*/
	int pictureSkip; /*[0-1]*/
	int qpHdr; /*-1 or [0,50]*/
	int qpMin;/*[0,51]*/
	int qpMax;/*[qpMin,51]*/
	int hrd;/*[0-1]*/
	int hrdCpbSize; /*[0,25000]*/
	int gopLen; /*[0,150]*/
	int intraQpDelta; /*[-12,12]*/
	int fixedIntraQp; /*[0, 51]*/
	int mbQpAdjustment; /*[-8, 7]*/
	int longTermPicRate; /*[0, n]*/
	int mbQpAutoBoost; /*[0, 1]*/
	int gdr; /*[0, 1]*/
	int drop_frame_en;
	int drop_frame_th;
	int cvbr_en;
	int cvbr_max_bitrate;
	int cvbr_min_bitrate;
	int cvbr_image_quality;
	int cvbr_diff_n;
	int p_wnd_size;
	int p_target_min_percentage;
	int p_target_max_percentage;
	int p_diff_x_th_percentage;
	int p_diff_adjust_percentage;
	int rs_rc_en;

	int rotate_mode; /*[0, 2]*/
	int bitrate;
	char *str_h264_level;

	RtStream stream;
};

static int64_t av_gettime_monotonic(void)
{
	struct timespec tv;

	clock_gettime(CLOCK_MONOTONIC, &tv);
	return (int64_t)tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
}

static int init_convert_timestamp(AVFormatContext *ctx, int64_t ts)
{
	struct video_data *s = ctx->priv_data;
	int64_t now;

	now = av_gettime();
	if (s->ts_mode == V4L_TS_ABS &&
		ts <= now + 1 * AV_TIME_BASE && ts >= now - 10 * AV_TIME_BASE) {
		av_log(ctx, AV_LOG_INFO, "Detected absolute timestamps\n");
		s->ts_mode = V4L_TS_CONVERT_READY;
		return 0;
	}

	now = av_gettime_monotonic();
	if (s->ts_mode == V4L_TS_MONO2ABS ||
		(ts <= now + 1 * AV_TIME_BASE && ts >= now - 10 * AV_TIME_BASE)) {
		AVRational tb = {AV_TIME_BASE, 1};
		int64_t period = av_rescale_q(1, tb, ctx->streams[0]->avg_frame_rate);
		av_log(ctx, AV_LOG_INFO, "Detected monotonic timestamps, converting\n");
		/* microseconds instead of seconds, MHz instead of Hz */
		s->timefilter = ff_timefilter_new(1, period, 1.0E-6);
		if (!s->timefilter)
			return AVERROR(ENOMEM);
		s->ts_mode = V4L_TS_CONVERT_READY;
		return 0;
	}

	av_log(ctx, AV_LOG_ERROR, "Unknown timestamps\n");
	return AVERROR(EIO);
}

static int convert_timestamp(AVFormatContext *ctx, int64_t *ts)
{
	struct video_data *s = ctx->priv_data;

	if (s->ts_mode) {
		int r = init_convert_timestamp(ctx, *ts);
		if (r < 0)
			return r;
	}

	if (s->timefilter) {
		int64_t nowa = av_gettime();
		int64_t nowm = av_gettime_monotonic();
		ff_timefilter_update(s->timefilter, nowa, nowm - s->last_time_m);
		s->last_time_m = nowm;
		*ts = ff_timefilter_eval(s->timefilter, *ts - nowm);
	}

	return 0;
}

static int rtstream_video_read_header(AVFormatContext *s1)
{
	struct video_data *s = s1->priv_data;
	AVStream *st = NULL;
	int res = 0;
	enum AVCodecID codec_id = AV_CODEC_ID_NONE;
	enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
	AVRational framerate_q = { 0 };
	int ret;
	uint32_t desired_format;
	enum AVCodecID v4l2_codec_id = AV_CODEC_ID_NONE;
	uint32_t v4l2_fmt;
	char *ptr_node_num = NULL;

	ret = rts_av_init();
	if (ret)
		return AVERROR(ENOMEM);

	s->stream = NULL;

	st = avformat_new_stream(s1, NULL);
	if (!st) {
		ret = AVERROR(ENOMEM);
		goto exit;
	}

	if (s->framerate &&
		(ret = av_parse_video_rate(&framerate_q, s->framerate)) < 0) {
		av_log(s1, AV_LOG_ERROR, "Could not parse framerate '%s'.\n",
		s->framerate);
		return ret;
	}

	if (s->pixel_format) {
		AVCodec *codec = avcodec_find_decoder_by_name(s->pixel_format);

		if (codec)
			v4l2_codec_id = codec->id;
		pix_fmt = av_get_pix_fmt(s->pixel_format);

		if (pix_fmt == AV_PIX_FMT_NONE && !codec) {
			av_log(s1, AV_LOG_ERROR, "No such input format: %s.\n",
			s->pixel_format);

			return AVERROR(EINVAL);
		}
	}
	desired_format = avpriv_fmt_ff2v4l(pix_fmt, v4l2_codec_id);
	codec_id = avpriv_fmt_v4l2codec(desired_format);

	ptr_node_num = strstr(s1->filename, "video");
	ptr_node_num += strlen("video");

	if (desired_format == V4L2_PIX_FMT_NV12) {
		if (strcasecmp(s->codec_video, "h264") == 0) {
			struct rts_h264_stream_cfg cfg;
			struct rts_video_h264_ctrl h264_ctl;

			cfg.isp_cfg.isp_id = atoi(ptr_node_num) - 51;
			cfg.isp_cfg.format = rts_get_av_fmt_from_v4l2_fmt(desired_format);
			cfg.isp_cfg.width = s->width;
			cfg.isp_cfg.height = s->height;
			cfg.isp_cfg.numerator = framerate_q.den;
			cfg.isp_cfg.denominator = framerate_q.num;
			cfg.isp_cfg.isp_buf_num = s->v4l2_bufs;
			if (!strcmp("h264_level_1", s->str_h264_level))
				cfg.level = H264_LEVEL_1;
			else if (!strcmp("h264_level_1_b", s->str_h264_level))
				cfg.level = H264_LEVEL_1_b;
			else if (!strcmp("h264_level_1_1", s->str_h264_level))
				cfg.level = H264_LEVEL_1_1;
			else if (!strcmp("h264_level_1_2", s->str_h264_level))
				cfg.level = H264_LEVEL_1_2;
			else if (!strcmp("h264_level_1_3", s->str_h264_level))
				cfg.level = H264_LEVEL_1_3;
			else if (!strcmp("h264_level_2", s->str_h264_level))
				cfg.level = H264_LEVEL_2;
			else if (!strcmp("h264_level_2_1", s->str_h264_level))
				cfg.level = H264_LEVEL_2_1;
			else if (!strcmp("h264_level_2_2", s->str_h264_level))
				cfg.level = H264_LEVEL_2_2;
			else if (!strcmp("h264_level_3", s->str_h264_level))
				cfg.level = H264_LEVEL_3;
			else if (!strcmp("h264_level_3_1", s->str_h264_level))
				cfg.level = H264_LEVEL_3_1;
			else if (!strcmp("h264_level_3_2", s->str_h264_level))
				cfg.level = H264_LEVEL_3_2;
			else if (!strcmp("h264_level_4", s->str_h264_level))
				cfg.level = H264_LEVEL_4;
			else if (!strcmp("h264_level_4_1", s->str_h264_level))
				cfg.level = H264_LEVEL_4_1;
			else if (!strcmp("h264_level_4_2", s->str_h264_level))
				cfg.level = H264_LEVEL_4_2;
			else if (!strcmp("h264_level_5", s->str_h264_level))
				cfg.level = H264_LEVEL_5;
			else if (!strcmp("h264_level_5_1", s->str_h264_level))
				cfg.level = H264_LEVEL_5_1;
			else
				cfg.level = H264_LEVEL_4;

			if (s->enableCabac == 0 && s->transform8x8Mode == 0)
				cfg.profile = H264_PROFILE_BASE;
			else if (s->enableCabac == 1 && s->transform8x8Mode == 0)
				cfg.profile = H264_PROFILE_MAIN;
			else if (s->enableCabac == 1 && s->transform8x8Mode > 0)
				cfg.profile = H264_PROFILE_HIGH;

			cfg.qp = s->qpHdr;
			cfg.bps = s->bitrate;
			cfg.gop = s->gopLen;
			cfg.videostab = RTS_FALSE;
			if (0 == s->rotate_mode)
				cfg.rotation = rts_av_rotation_0;
			else if (1 == s->rotate_mode)
				cfg.rotation = rts_av_rotation_90r;
			else if (2 == s->rotate_mode)
				cfg.rotation = rts_av_rotation_90l;

			s->stream = rts_create_h264_stream(&cfg);
			if (!s->stream) {
				fprintf(stderr, "cann't create h264 stream\n");
				ret = -1;
				goto exit;
			}

			codec_id = AV_CODEC_ID_H264;
			v4l2_fmt = V4L2_PIX_FMT_H264;
			memset(&h264_ctl, 0, sizeof(h264_ctl));
			rts_video_get_h264_ctrl(s->stream, &h264_ctl);
			h264_ctl.supported_bitrate_mode = RTS_BITRATE_MODE_CBR |
				RTS_BITRATE_MODE_VBR | RTS_BITRATE_MODE_C_VBR;
			if (s->rs_rc_en && s->cvbr_en)
				h264_ctl.bitrate_mode = RTS_BITRATE_MODE_C_VBR;
			else
				h264_ctl.bitrate_mode = RTS_BITRATE_MODE_CBR;


			h264_ctl.bitrate = s->bitrate;

			h264_ctl.qp = s->qpHdr;
			h264_ctl.max_qp = s->qpMax;
			h264_ctl.min_qp = s->qpMin;

			h264_ctl.gop = s->gopLen;

			h264_ctl.slice_size = s->sliceSize;
			h264_ctl.sei_messages = s->seiMessages;
			h264_ctl.video_full_range = s->videoFullRange;
			h264_ctl.constrained_intra_prediction = s->constrainedIntraPrediction;
			h264_ctl.disable_deblocking_filter = s->disableDeblockingFilter;
			h264_ctl.enable_cabac = s->enableCabac;
			h264_ctl.cabac_init_idc = s->cabacInitIdc;
			h264_ctl.transform8x8mode = s->transform8x8Mode;
			h264_ctl.gdr = s->gdr;

			h264_ctl.hrd = s->hrd;
			h264_ctl.hrd_cpb_size = s->hrdCpbSize;
			h264_ctl.longterm_pic_rate = s->longTermPicRate;

			h264_ctl.drop_frame_en = s->drop_frame_en;
			h264_ctl.drop_frame_th = s->drop_frame_th;
			h264_ctl.cvbr_en = s->cvbr_en;
			h264_ctl.max_bitrate = s->cvbr_max_bitrate;
			h264_ctl.min_bitrate = s->cvbr_min_bitrate;

			h264_ctl.cvbr_diff_n = s->cvbr_diff_n;
			h264_ctl.p_wnd_size = s->p_wnd_size;
			h264_ctl.p_target_min_percentage =
				s->p_target_min_percentage;
			h264_ctl.p_target_max_percentage =
				s->p_target_max_percentage;
			h264_ctl.p_diff_x_th_percentage =
				s->p_diff_x_th_percentage;
			h264_ctl.p_diff_adjust_percentage =
				s->p_diff_adjust_percentage;

			h264_ctl.rs_rc_en = s->rs_rc_en;

			rts_video_set_h264_ctrl(s->stream, &h264_ctl);
		} else if (strcasecmp(s->codec_video, "mjpeg") == 0
			|| strcasecmp(s->codec_video, "mjpg") == 0) {
			struct rts_mjpeg_stream_cfg cfg;

			cfg.isp_cfg.isp_id = atoi(ptr_node_num) - 51;
			cfg.isp_cfg.format = rts_get_av_fmt_from_v4l2_fmt(desired_format);
			cfg.isp_cfg.width = s->width;
			cfg.isp_cfg.height = s->height;
			cfg.isp_cfg.numerator = framerate_q.den;
			cfg.isp_cfg.denominator = framerate_q.num;
			cfg.isp_cfg.isp_buf_num = s->v4l2_bufs;

			if (0 == s->rotate_mode)
				cfg.rotation = rts_av_rotation_0;
			else if (1 == s->rotate_mode)
				cfg.rotation = rts_av_rotation_90r;
			else if (2 == s->rotate_mode)
				cfg.rotation = rts_av_rotation_90l;

			s->stream = rts_create_mjpeg_stream(&cfg);
			if (!s->stream) {
				fprintf(stderr, "cann't create mjpeg stream\n");
				ret = -1;
				goto exit;
			}

			codec_id = AV_CODEC_ID_MJPEG;
			v4l2_fmt = V4L2_PIX_FMT_MJPEG;
		}
	} else if (desired_format == V4L2_PIX_FMT_MJPEG) {
		codec_id = AV_CODEC_ID_MJPEG;
		v4l2_fmt = V4L2_PIX_FMT_MJPEG;
	} else if (desired_format == V4L2_PIX_FMT_H264) {
		codec_id = AV_CODEC_ID_H264;
		v4l2_fmt = V4L2_PIX_FMT_H264;
	} else {
		ret = AVERROR(EINVAL);
		goto exit;
	}

	ret = rts_av_apply(s->stream);
	if (ret) {
		fprintf(stderr, "rts_av_apply fail, ret = %d\n", ret);
		goto exit;
	}

	avpriv_set_pts_info(st, 64, 1, 1000000); /* 64 bits pts in us */

	/* If no pixel_format was specified, the codec_id was not known up
	* until now. Set video_codec_id in the context, as codec_id will
	* not be available outside this function
	*/
	s1->video_codec_id = codec_id;

	if ((res = av_image_check_size(s->width, s->height, 0, s1)) < 0) {
		ret = res;
	}
	s->frame_format = desired_format;

	st->codec->pix_fmt = avpriv_fmt_v4l2ff(v4l2_fmt, codec_id);

	st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
	st->codec->codec_id = codec_id;
	if (codec_id == AV_CODEC_ID_RAWVIDEO)
		st->codec->codec_tag =
		avcodec_pix_fmt_to_codec_tag(st->codec->pix_fmt);
	else if (codec_id == AV_CODEC_ID_H264) {
		st->need_parsing = AVSTREAM_PARSE_NONE;
	}

	if (s->rotate_mode) {
		st->codec->width = s->height;
		st->codec->height = s->width;
	} else {
		st->codec->width = s->width;
		st->codec->height = s->height;
	}
	st->codec->bit_rate = s->bitrate;
	st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	/* emit one intra frame every twelve frames at most */
	st->codec->gop_size = s->gopLen;
	st->codec->intra_quant_bias = s->intraQpDelta;
	st->codec->qmin =  s->qpMin;
	st->codec->qmax =  s->qpMax;

	s1->streams[0]->avg_frame_rate.num = framerate_q.num;
	s1->streams[0]->avg_frame_rate.den = framerate_q.den;
	st->codec->time_base.den = framerate_q.den;
	st->codec->time_base.num = 1;
	s1->streams[0]->r_frame_rate = s1->streams[0]->avg_frame_rate;

	if (codec_id == AV_CODEC_ID_H264) {
		struct rts_video_h264_info info;
		if (rts_video_get_h264_mediainfo(s->stream, &info) == 0) {
			st->codec->extradata = av_malloc(info.sps_pps_len);
			memcpy(st->codec->extradata,
				info.sps_pps,
				info.sps_pps_len);
			st->codec->extradata_size = info.sps_pps_len;
		}
	}

	return 0;

exit:
	if (s->stream) {
		rts_destroy_stream(s->stream);
		s->stream = NULL;
	}

	rts_av_release();

	return ret;
}

static void rtstream_buffer_free(void *opa, uint8_t *data)
{
	struct rts_av_buffer *buffer = opa;

	rts_av_put_buffer(buffer);
}

static int update_h264_params(struct video_data *s)
{
	struct rts_video_h264_ctrl h264_ctl;
	int ret;

	rts_video_get_h264_ctrl(s->stream, &h264_ctl);

	h264_ctl.supported_bitrate_mode = RTS_BITRATE_MODE_CBR |
		RTS_BITRATE_MODE_VBR | RTS_BITRATE_MODE_C_VBR;
	if (s->rs_rc_en && s->cvbr_en)
		h264_ctl.bitrate_mode = RTS_BITRATE_MODE_C_VBR;
	else
		h264_ctl.bitrate_mode = RTS_BITRATE_MODE_CBR;

	h264_ctl.bitrate = s->bitrate;

	h264_ctl.qp = s->qpHdr;
	h264_ctl.max_qp = s->qpMax;
	h264_ctl.min_qp = s->qpMin;

	h264_ctl.gop = s->gopLen;

	h264_ctl.slice_size = s->sliceSize;
	h264_ctl.sei_messages = s->seiMessages;
	h264_ctl.video_full_range = s->videoFullRange;
	h264_ctl.constrained_intra_prediction = s->constrainedIntraPrediction;
	h264_ctl.disable_deblocking_filter = s->disableDeblockingFilter;
	h264_ctl.enable_cabac = s->enableCabac;
	h264_ctl.cabac_init_idc = s->cabacInitIdc;
	h264_ctl.transform8x8mode = s->transform8x8Mode;
	h264_ctl.gdr = s->gdr;

	h264_ctl.hrd = s->hrd;
	h264_ctl.hrd_cpb_size = s->hrdCpbSize;
	h264_ctl.longterm_pic_rate = s->longTermPicRate;

	h264_ctl.drop_frame_en = s->drop_frame_en;
	h264_ctl.drop_frame_th = s->drop_frame_th;
	h264_ctl.cvbr_en = s->cvbr_en;
	h264_ctl.max_bitrate = s->cvbr_max_bitrate;
	h264_ctl.min_bitrate = s->cvbr_min_bitrate;
	if (s->rs_rc_en && s->cvbr_en)
		h264_ctl.bitrate = s->cvbr_min_bitrate +
			(s->cvbr_max_bitrate - s->cvbr_min_bitrate) / 2;
	else
		h264_ctl.bitrate = s->bitrate;

	h264_ctl.cvbr_diff_n = s->cvbr_diff_n;
	h264_ctl.p_wnd_size = s->p_wnd_size;
	h264_ctl.p_target_min_percentage = s->p_target_min_percentage;
	h264_ctl.p_target_max_percentage = s->p_target_max_percentage;
	h264_ctl.p_diff_x_th_percentage = s->p_diff_x_th_percentage;
	h264_ctl.p_diff_adjust_percentage = s->p_diff_adjust_percentage;

	h264_ctl.rs_rc_en = s->rs_rc_en;

	if (GEN_I_FRAME(s->h264_flag)) {
		rts_video_request_h264_key_frame(s->stream);
		s->h264_flag = DISABLE_GEN_I_FRAME(s->h264_flag);
	}
	ret = rts_video_set_h264_ctrl(s->stream, &h264_ctl);

	return ret;
}

static int rtstream_video_read_packet(AVFormatContext *s1, AVPacket *pkt)
{
	struct video_data *s = s1->priv_data;
	struct rts_av_buffer *buffer = NULL;
	int  ret = 0;
	unsigned char *p_nal_type = NULL;
	unsigned char is_cur_intra_frame = 0;

	if (s1->video_codec_id == AV_CODEC_ID_H264)
		update_h264_params(s);

	av_init_packet(pkt);

	if (rts_av_stream_poll(s->stream))
		return AVERROR(EAGAIN);

	ret = rts_av_stream_recv(s->stream, &buffer);
	if (ret == 0)
		pkt->pts = buffer->timestamp;
	else
		return ret;

	p_nal_type = buffer->vm_addr + 4;
	is_cur_intra_frame = (1 != (*p_nal_type & 0x1F));
	if (is_cur_intra_frame) {
		pkt->flags |= AV_PKT_FLAG_KEY;
	}

	pkt->data = buffer->vm_addr;
	pkt->size = buffer->bytesused;
	pkt->pts = buffer->timestamp;
	convert_timestamp(s1, &pkt->pts);
	pkt->dts = pkt->pts;

	pkt->buf = av_buffer_create(pkt->data, pkt->size,
					rtstream_buffer_free, buffer, 0);
	if (!pkt->buf) {
		pkt->data = NULL;
		rts_av_put_buffer(buffer);
		return AVERROR(ENOMEM);
	}

	return pkt->size;
}

static int rtstream_video_read_close(AVFormatContext *s1)
{
	struct video_data *s = s1->priv_data;

	if (s->stream) {
		rts_av_cancel(s->stream);
		rts_destroy_stream(s->stream);
		s->stream = NULL;
	}

	rts_av_release();

	return 0;
}

#define OFFSET(x) offsetof(struct video_data, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM

static const AVOption options[] = {
    { "video_size",   "set frame size",                                           OFFSET(width),        AV_OPT_TYPE_IMAGE_SIZE, {.str = NULL},  0, 0,   DEC },
    { "pixel_format", "set preferred pixel format",                               OFFSET(pixel_format), AV_OPT_TYPE_STRING, {.str = NULL},  0, 0,       DEC },
    { "input_format", "set preferred pixel format (for raw video) or codec name", OFFSET(pixel_format), AV_OPT_TYPE_STRING, {.str = NULL},  0, 0,       DEC },
    { "framerate",    "set frame rate",                                           OFFSET(framerate),    AV_OPT_TYPE_STRING, {.str = NULL},  0, 0,       DEC },
    { "v4l2_bufs", "set v4l2 frame buffers", 			          	  OFFSET(v4l2_bufs),	AV_OPT_TYPE_INT,    {.i64 = 4},	    1, 16,	DEC },
    { "codec_video",    "video codec",  OFFSET(codec_video),    AV_OPT_TYPE_STRING, {.str = NULL},  0, 0,       DEC },

    { "timestamps",   "set type of timestamps for grabbed frames",                OFFSET(ts_mode),      AV_OPT_TYPE_INT,    {.i64 = 0 }, 0, 2, DEC, "timestamps" },
    { "ts",           "set type of timestamps for grabbed frames",                OFFSET(ts_mode),      AV_OPT_TYPE_INT,    {.i64 = 0 }, 0, 2, DEC, "timestamps" },
    { "default",      "use timestamps from the kernel",                           OFFSET(ts_mode),      AV_OPT_TYPE_CONST,  {.i64 = V4L_TS_DEFAULT  }, 0, 2, DEC, "timestamps" },
    { "abs",          "use absolute timestamps (wall clock)",                     OFFSET(ts_mode),      AV_OPT_TYPE_CONST,  {.i64 = V4L_TS_ABS      }, 0, 2, DEC, "timestamps" },
    { "mono2abs",     "force conversion from monotonic to absolute timestamps",   OFFSET(ts_mode),      AV_OPT_TYPE_CONST,  {.i64 = V4L_TS_MONO2ABS }, 0, 2, DEC, "timestamps" },

    { "h264_flag", "h264 operate flag", OFFSET(h264_flag), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, INT_MAX, DEC },
    { "h264_sliceSize", "size of a slice in macroblock rows", OFFSET(sliceSize), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 127, DEC },
    { "h264_seiMessages", "Contain timing information", OFFSET(seiMessages), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_videoFullRange", "YUV or YCbCr", OFFSET(videoFullRange), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_constrainedIntraPrediction", "intra prediction to constrained mode", OFFSET(constrainedIntraPrediction), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_disableDeblockingFilter", "filtering on all macroblock edges", OFFSET(disableDeblockingFilter), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, DEC },
    { "h264_enableCabac", "CABAC entropy coding", OFFSET(enableCabac), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_cabacInitIdc", "Selects one of the three CABAC initialization tables", OFFSET(cabacInitIdc), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, DEC },
    { "h264_transform8x8Mode", "8*8 transform", OFFSET(transform8x8Mode), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, DEC },
    { "h264_quarterPixelMv", "usage of 1/4 pixel motion estimation", OFFSET(quarterPixelMv), AV_OPT_TYPE_INT, { .i64 = 1 }, 0, 2, DEC },
    { "h264_roi1DeltaQp", "Specifies the QP delta value for the first ROI area", OFFSET(roi1DeltaQp), AV_OPT_TYPE_INT, { .i64 = 0 }, -15, 0, DEC },
    { "h264_roi2DeltaQp", "Specifies the QP delta value for the second ROI area", OFFSET(roi2DeltaQp), AV_OPT_TYPE_INT, { .i64 = 0 }, -15, 0, DEC },
    { "h264_adaptiveRoi", "Specifies the QP delta for adaptive ROI", OFFSET(adaptiveRoi), AV_OPT_TYPE_INT, { .i64 = 0 }, -51, 0, DEC },
    { "h264_pictureRc", "Enables rate control to  adjust QP between  frames", OFFSET(pictureRc), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_pictureSkip", "Rate control to skip pictures", OFFSET(pictureSkip), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_qpHdr", "Initial QP", OFFSET(qpHdr), AV_OPT_TYPE_INT, { .i64 = 26 }, -1, 50, DEC },
    { "h264_hrd", "HRD Model", OFFSET(hrd), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_hrdCpbSize", "HRD Cpb Size", OFFSET(hrdCpbSize), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 25000, DEC },
    { "h264_fixedIntraQp", "QP for all I frame", OFFSET(fixedIntraQp), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 51, DEC },
    { "h264_mbQpAdjustment", "threshold for macro block based QP adjustment", OFFSET(mbQpAdjustment), AV_OPT_TYPE_INT, { .i64 = 0 }, -8, 7, DEC },
    { "h264_longTermPicRate", "period between long term picture refreshes", OFFSET(longTermPicRate), AV_OPT_TYPE_INT, { .i64 = 15 }, 0, INT_MAX, DEC },
    { "h264_mbQpAutoBoost", "macroblocks and automatically boost the quality", OFFSET(mbQpAutoBoost), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "h264_gdr", "Gradual Decoder Refresh", OFFSET(gdr), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, DEC },
    { "drop_frame_en", "drop frame enable", OFFSET(drop_frame_en), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 1, DEC },
    { "drop_frame_th", "drop_frame_th", OFFSET(drop_frame_th), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 100, DEC },
    { "h264_cvbr_en", "cvbr_en", OFFSET(cvbr_en), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 1, DEC },
    { "h264_cvbr_max_bitrate", "max_br", OFFSET(cvbr_max_bitrate), AV_OPT_TYPE_INT, { .i64 = 2000000 }, 12000, 64000000, DEC },
    { "h264_cvbr_min_bitrate", "min_br", OFFSET(cvbr_min_bitrate), AV_OPT_TYPE_INT, { .i64 = 256000 }, 10000, 64000000, DEC },
    { "h264_cvbr_image_quality", "image_quality", OFFSET(cvbr_image_quality), AV_OPT_TYPE_INT, { .i64 = 4 }, 0, 8, DEC },

    { "cvbr_diff_n", "cvbr_diff_n", OFFSET(cvbr_diff_n), AV_OPT_TYPE_INT, { .i64 = 15 }, -1, 100, DEC },
    { "p_wnd_size", "p_wnd_size", OFFSET(p_wnd_size), AV_OPT_TYPE_INT, { .i64 = 10 }, -1, 100, DEC },
    { "p_target_min_percentage", "target_min", OFFSET(p_target_min_percentage), AV_OPT_TYPE_INT, { .i64 = 25 }, -1, 200, DEC },
    { "p_target_max_percentage", "target_max", OFFSET(p_target_max_percentage), AV_OPT_TYPE_INT, { .i64 = 150 }, -1, 500, DEC },
    { "p_diff_x_th_percentage", "diff_x_th", OFFSET(p_diff_x_th_percentage), AV_OPT_TYPE_INT, { .i64 = 60 }, -1, 500, DEC },
    { "p_diff_adjust_percentage", "diff_adjust", OFFSET(p_diff_adjust_percentage), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 100, DEC },
    { "h264_rs_rc_en", "use rs rc alg", OFFSET(rs_rc_en), AV_OPT_TYPE_INT, { .i64 = 0 }, -1, 2, DEC },

    { "rotate", "Video Rotate", OFFSET(rotate_mode), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, DEC },
    { "bitrate", "Video bitrate", OFFSET(bitrate), AV_OPT_TYPE_INT, { .i64 = 1048576 }, 12000, 67108864, DEC },
    { "h264_qpMin", "qpMin", OFFSET(qpMin), AV_OPT_TYPE_INT, { .i64 = 10 }, 0, 51, DEC },
    { "h264_qpMax", "qpMax", OFFSET(qpMax), AV_OPT_TYPE_INT, { .i64 = 51 }, 1, 51, DEC },
    { "intra_qp_delta", "Intra qp delta", OFFSET(intraQpDelta), AV_OPT_TYPE_INT, { .i64 = -12 }, -12, 12, DEC },
    { "keyframe", "goplen", OFFSET(gopLen), AV_OPT_TYPE_INT, { .i64 = 26 }, 0, 150, DEC },
    { "h264_level", "H264 level", OFFSET(str_h264_level), AV_OPT_TYPE_STRING, .flags = DEC },

    { NULL },
};

static const AVClass rtstream_video_class = {
    .class_name = "Rtstream video indev",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_rtstream_video_demuxer = {
    .name           = "rtstream video",
    .long_name      = NULL_IF_CONFIG_SMALL("Rtstream device grab"),
    .priv_data_size = sizeof(struct video_data),
    .read_header    = rtstream_video_read_header,
    .read_packet    = rtstream_video_read_packet,
    .read_close     = rtstream_video_read_close,
    .flags          = AVFMT_NOFILE,
    .priv_class     = &rtstream_video_class,
};
