#ifndef AVFORMAT_AMIX_H
#define AVFORMAT_AMIX_H

#include <stdint.h>
#include <pthread.h>
#include "avformat.h"
#include "avio.h"
#include "shm.h"
#include "libavutil/audio_fifo.h"

#define MAX_INPUT_NUMBER		3
#define FILE_PRE_DIRECTORY		"/var/tmp/"
#define AMIX_FILE_DIR			"/var/tmp/amix.am"
#define AMIX_PACKET_SIZE		4096

#define INFORM_ID			0xd666d666

#define AMIX_HEADER_SIZE		1024

#define AMIX_OUT_SAMPLE_RATE		16000
#define AMIX_OUT_FORMAT			AV_SAMPLE_FMT_S16
#define AMIX_OUT_CHANNELS		2
#define AMIX_OUT_CHANNEL_LAYOUT		0x3

#define AMIX_POS_TAG			0
#define AMIX_POS_SIZE			4
#define AMIX_POS_MAX_NB			12
#define AMIX_POS_CUR_NB			16
#define AMIX_POS_CUR_WP			20
#define AMIX_POS_FLAG			28

#define AMIX_STATE_OFF			0
#define AMIX_STATE_ON			1

#define AMIX_DEC_THREE			0x55
#define AMIX_DEC_FIVE			0x33

#define AMIX_FIFO_DOWN_TH		0
#define AMIX_FIFO_UP_TH			512
#define AMIX_FIFO_MAX_TH		32000

#define AMIX_PTS_SAMPLE_NUM		10

enum {
	AMIX_ADD_INPUT = 0,
	AMIX_DEL_INPUT,
};

enum {
	AMIX_AUDIO_TYPE_DEFAULT = 0,
	AMIX_AUDIO_TYPE_VOICE,
	AMIX_AUDIO_TYPE_FILE,
};

typedef struct AmixData {
	AVFormatContext *parent;
	uint32_t max_num;
	uint32_t cur_num;
	uint8_t *shm_buf;
	int shm_id;
	int shm_index;
	int64_t shm_size;
	uint64_t write_index;
	uint64_t read_index;

	int fd;

	uint8_t *ring_begin;
	uint8_t *ring_end;
	uint8_t *ring_pos;

	AVFormatContext *shm_ctx;
	char shm_path[128];

	struct InputStream *head;

	int sample_rate;
	int sample_fmt;
	int channels;
	uint64_t channel_layout;
} AmixData;

typedef struct InputStream {
	struct InputStream *next;

	struct AmixData *amix;
	char *path;
	int flag;
	int index;

	AVFormatContext *shm_ctx;

	struct SwrContext *swr_ctx;
	uint8_t  **swr_buf;
	int swr_linesize;
	int max_nb_samples;
	int swr_flag; /* 0: no resample, 1: wait, 2: need */

	AVAudioFifo *fifo;
	int avail_samples;
	int input_scale;
	int input_shift;

	int64_t pts_base;
	int64_t pts_sample[AMIX_PTS_SAMPLE_NUM];
	int pts_ptr;
	uint32_t audio_type;
} InputStream;

typedef union PointerInform {
	struct {
		uint32_t id;
		uint32_t index;
		uint32_t flag; /* 0 add, 1 del */
		uint32_t type; /* voice or file */
	} p;

	uint8_t aligned[32];
} PointerInform;

int amix_create_shm(AVFormatContext *s);
int amix_lock_file(AVFormatContext *s);
void amix_unlock_file(AVFormatContext *s);
int amix_init_shmoutput(AVFormatContext *s);
void amix_uninit_shmoutput(AVFormatContext *s);

#endif /* AVFORMAT_AMIX_H */
