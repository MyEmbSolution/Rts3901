#ifndef __LARK_H__
#define __LARK_H__

#include <stdint.h>
#include <stdio.h>

#define LARK_TAG ((uint32_t)(('R' << 24) | ('T' << 16) | ('R' << 8) | ('S')))

struct payload_header {
	uint32_t tag;
	uint32_t stype;
	uint32_t length;
	uint32_t payload_type; /* G711, ulaw: 0x00, alaw: 0x08, mp3: 0x0e */
	uint32_t timestamp_sec;
	uint32_t timestamp_usec;
};

enum {
	LARK_LOG_ERR = 1 << 0,
	LARK_LOG_DBG = 1 << 1,
	LARK_LOG_INF = 1 << 2,
};

enum {
	PAYLOAD_TYPE_G711_ULAW = 0x00,
	PAYLOAD_TYPE_G711_ALAW = 0x08,
	PAYLOAD_TYPE_MP3 = 0x0E,
};

enum {
	LARK_STREAM_VOICE = 1,
	LARK_STREAM_FILE = 2,
};

#define lark_log(level, fmt, arg...) \
	do { \
		printf(fmt, ## arg); \
	} while (0)

#endif
