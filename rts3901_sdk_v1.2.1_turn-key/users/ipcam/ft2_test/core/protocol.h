/*
 * Realtek Semiconductor Corp.
 *
 * inc/ft2protocol.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include "list.h"
#include "ft2log.h"
#include "ft2errno.h"
#include "transfer_net.h"

#define		FT2_TEST_PASS	"pass"
#define		FT2_TEST_FAIL	"fail"

#define		FT2_COMMAND_CODE_REQ		't'
#define		FT2_COMMAND_CODE_RSP		'r'
#define		FT2_COMMAND_CODE_START		'$'
#define		FT2_COMMAND_CODE_END		'#'
#define		FT2_COMMAND_CODE_PARAM 		'&'
#define		FT2_COMMAND_CODE_ACK		'+'
#define		FT2_COMMAND_CODE_NAK		'-'

enum ft2_srv_status {
	srv_ready = 0,
	srv_start,
	srv_stop,
};

struct protocol_data {
	struct list_head list;
	struct tr_data *transfer;
};

#ifndef FT2_COMMAND_NAME_LENGTH
#define FT2_COMMAND_NAME_LENGTH			8
#endif

#ifndef FT2_COMMAND_LENGTH_THRESHOLD
#define FT2_COMMAND_LENGTH_THRESHOLD		64
#endif

struct protocol_param {
	char content[FT2_COMMAND_LENGTH_THRESHOLD];
	struct protocol_param *next;
};

struct protocol_command {
	char type;
	char name[FT2_COMMAND_NAME_LENGTH];
	unsigned int id;
	struct protocol_param *parameter;
	struct protocol_param *result;
};

struct pt_command {
	struct protocol_command command;
	struct list_head list;
};

typedef int (*CHECK_COMMAND)(void *checker, struct protocol_command *pcmd);

struct ft2_protocol {
	enum ft2_srv_status status;
	pthread_t th_parse;

	pthread_mutex_t mutex_recv;
	pthread_mutex_t mutex_parse;
	pthread_mutex_t mutex_resp;

	struct list_head recv_list;
	struct list_head parse_list;

	FT2TrInst transfer;

	void *checker;
	CHECK_COMMAND check_command;

	unsigned char parse_stage;

	char *tmp_content;
	unsigned int tmp_length;
	unsigned int tmp_used;
	unsigned int tmp_left;
};
typedef struct ft2_protocol *FT2ProtInst;


int delete_pt_command_result(struct protocol_command *pcmd);
int append_pt_command_result(struct protocol_command* pcmd, char *param, int len);
struct protocol_command *pop_commond(FT2ProtInst inst);
int send_commond_result(FT2ProtInst inst, struct protocol_command *pcmd);

int ft2_new_protocol_inst(FT2ProtInst *inst, void *checker, CHECK_COMMAND cb);
int ft2_delete_protocol_inst(FT2ProtInst inst);

#endif
