#ifndef __OCTOPUS_H__
#define __OCTOPUS_H__
#include <stdint.h>

#define MAX_DEV_PATH_LEN		128

enum {
	CMD_DOMAIN_RESERVED	= 0,
	CMD_DOMAIN_PEAOCK	= 1,
	CMD_DOMAIN_RTSP		= 2,
	CMD_DOMAIN_SNAPSHOT	= 3,
	CMD_DOMAIN_ALSA		= 4,
	CMD_DOMAIN_PROFILE1	= 5,
	CMD_DOMAIN_PROFILE2	= 6,
	CMD_DOMAIN_PROFILE3	= 7,
	CMD_DOMAIN_PROFILE4	= 8,

	CMD_DOMAIN_UNKNOWN,
};

struct pe_msg {

	long mtype;
	uint32_t sub_type;
	int pid;
	int status;
	int seq;
	int length;
	uint8_t data[0];
};

struct msg_rsp {
	int domain;
	const uint8_t *msg;
	int msg_len;
	uint8_t *rsp;
	int rsp_len;
	int timeout;
	int recv_len;
	int error;
};

enum {
	MEDIA_TYPE_VIDEO	= 1,
	MEDIA_TYPE_AUDIO	= 2,
};

struct h264_param_req {
	int h264_param;
	char profile[64];
	char param_name[64];
};

struct set_bitrate_req {
	int media;
	int bitrate;
	char profile[128];
};

struct set_framesize_req {
	int width;
	int height;
	char profile[128];
};

struct set_frameival_req {
	int numerator;
	int denominator;
	char profile[128];
};

struct get_sps_pps_req {
	char profile[64];
};

struct get_sps_pps_rsp {
	unsigned char sps[64];
	unsigned char pps[32];
	int sps_len;
	int pps_len;
};

struct set_aec_en_req {
	int enable;
	char profile[128];
};

struct set_ns_en_req {
	int enable;
	char profile[128];
};

struct set_ns_level_req {
	int level;
	char profile[128];
};

#define MAKE_MSG_RSP(_domain, _msg, _len1, _rsp, _len2, __timeout) \
	{\
		.domain = (_domain), .msg = (_msg), .msg_len = (_len1), \
		.rsp = (_rsp), .rsp_len = (_len2), .timeout = (__timeout), \
		.recv_len = 0, .error = 0,\
	}

#define pe_error(fmt, args...) \
	do { \
		fprintf(stderr, "%-20s:%d--> "fmt, __func__, __LINE__, ##args); \
	} while (0)

extern const char *msg_key_files[];

int pe_get_msg_id(const char *path);
int pe_remove_msg_id(int msg_id);
void del_msg_file(const char *path);
int pe_send_msg(struct msg_rsp *msg_rsp);
int pe_send_simple_msg(int domain, int type);
int pe_send_msg_set_bitrate(const char *profile, int video, int bitrate);
int pe_send_msg_set_h264_param(const char *profile, const char *param_name, int h264_param);
int pe_send_msg_generate_intra_frame(const char *profile);
int pe_send_msg_set_frmsize(const char *profile, int width, int height);
int pe_send_msg_set_frmival(const char *profile,
		int numerator, int denominator);
int pe_send_msg_get_sps_pps(const char *profile,
		struct get_sps_pps_rsp *reponse);
int pe_send_msg_set_aec_en(const char *profile, int enable);
int pe_send_msg_set_ns_en(const char *profile, int enable);
int pe_send_msg_set_ns_level(const char *profile, int level);

#endif
