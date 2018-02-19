#ifndef _INC_TESTER_H
#define _INC_TESTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "ft2errno.h"
#include "ft2log.h"
#include "list.h"
#include "protocol.h"

unsigned long pic_count;
unsigned long h264_count;
unsigned long jpeg_count;
unsigned long aud_amic_44k_count;
unsigned long aud_i2s_44k_count;
unsigned long aud_amic_48k_count;
unsigned long aud_i2s_48k_count;
unsigned char test_forever;

struct ft2_opt_params {
        char            ipcam_addr[16];
        char            host_addr[16];
        char            mnt_filename[32];
        char            fs_type[8];
};

struct ft2_test_item {
	char test_name[FT2_COMMAND_NAME_LENGTH];
	unsigned int test_id;

	int (*check)(struct protocol_command *pcmd);

	void* (*initialize)();
	int (*preprocess)(void *priv);
	int (*runonce)(void *priv, struct protocol_command *pcmd, char *path);
	int (*postprocess)(void *priv);
	void (*cleanup)(void *priv);
};

struct test_entry {
	struct ft2_test_item test;
	pthread_t th;
	void *priv;
	struct protocol_command *pcmd;
	struct ft2_tester *owner;
	struct list_head list;
};

struct ft2_tester {
	char root_path[256];
	FT2ProtInst protocol;
	struct list_head test_items;

	pthread_t th_run;
	pthread_mutex_t mutex_test;
	unsigned int stop_all_item;
	unsigned int running_test;
	unsigned char exit_flag;
	unsigned char test_forever;
};
typedef struct ft2_tester *FT2TesterInst;

int ft2_register_test_item(FT2TesterInst inst,
		struct ft2_test_item *test_items, int count);

int ft2_set_root_path(FT2TesterInst inst, char *path);
int ft2_get_root_path(FT2TesterInst inst, char *path, int len);

int ft2_start_run_tests(FT2TesterInst inst);
int ft2_stop_run_tests(FT2TesterInst inst);

int ft2_new_tester(FT2TesterInst *inst);
int ft2_delete_tester(FT2TesterInst inst);

#endif
