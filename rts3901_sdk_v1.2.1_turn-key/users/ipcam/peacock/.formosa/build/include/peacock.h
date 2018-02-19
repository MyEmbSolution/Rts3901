#ifndef __PEACOCK_H__
#define __PEACOCK_H__

#include <stdint.h>


#define MSG_MASTER_FILE	"peacock.msg"
#define MSG_FILE_PREFIX "/var/tmp"
#define MAX_EVENT_SIZE	1024
#define MAX_PATH_LEN	128
#define SNAPSHOT_DEVICE	"/dev/video55"
#define SNAPSHOT_PATH	"/var/tmp/tmp.jpg"

#ifndef MSG_TYPE_REQUEST
#define MSG_TYPE_REQUEST		(1)
#endif

enum {
	MAX_EVENT_LENTH			= 1024,

	EVENT_STATUS_OK			= 0,
	EVENT_STATUS_FAIL		= 0x80000001,
	EVENT_STATUS_UNKNOWN_EVENT	= 0x80000002,
	EVENT_STATUS_INVAL_PARAM	= 0x80000003,
	EVENT_STATUS_SERVER_ERROR	= 0x80000004,

	/* event type */
	EVENT_TYPE_SNAPSHOT		= 1,
	EVENT_TYPE_CONFIG_CHANGED	= 2,
	EVENT_TYPE_PROBE		= 3,
	EVENT_TYPE_STREAMON		= 4,
	EVENT_TYPE_STREAMOFF		= 5,
	EVENT_TYPE_SET_BITRATE		= 6,
	EVENT_TYPE_SET_FRMSIZE		= 7,
	EVENT_TYPE_SET_FRMIVAL		= 8,
	EVENT_TYPE_SET_H264_PARAM	= 9,
	EVENT_TYPE_GET_SPS_PPS		= 10,
	EVENT_TYPE_SET_AEC_EN		= 11,
	EVENT_TYPE_SET_NS_EN		= 12,
	EVENT_TYPE_SET_NS_LEVEL		= 13
};

struct snapshot_p {
	int width;
	int height;
	uint32_t format;
};

struct event_msg {
	long mtype;
	int pid;
	int status;
	int length;
	uint32_t domain;
	uint32_t event;
	union {
		struct snapshot_p snapshot;
		uint8_t data[0];
	};
};

struct pe_event {
	int type;
	uint8_t data[];
};

#endif
