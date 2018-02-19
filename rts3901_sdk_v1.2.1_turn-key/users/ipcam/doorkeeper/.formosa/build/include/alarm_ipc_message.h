#ifndef __ALARM_IPC_MSG_H__
#define __ALARM_IPC_MSG_H__
#include "alarm_ipc_server.h"

#define ALARM_DOMIAN "/tmp/alarm_server.sock"
#define ALARM_MSG_ERROR				-1
#define ALARM_MSG_SUCCESS			1
#define ALARM_MSG_POST_EVENT		2
#define ALARM_MSG_RENEW_CONFIG		3
#define ALARM_MSG_RENEW_FTP_CONFIG	4
#define ALARM_MSG_ADD_EVENT_ENTRY	5
#define ALARM_MSG_ADD_METHOD_ENTRY	6


#define ALARM_MSG_REPLY				4


#define DIRECTION_C2S			1
#define DIRECTION_S2C			2


#define MAX_EVENT_DATALEN		512


struct s_alarm_ipc_data {
	int cmd;
	int direction;
	unsigned char recvbuf[MAX_EVENT_DATALEN];
	unsigned char extdata[MAX_EVENT_DATALEN];
};


struct s_event_msg {
	unsigned int	id;
	unsigned int	source;
	time_t			time;
	unsigned int	len;
} ;

int send_cfg_change_notify();
int send_md_notify();
int event_server(struct ipc_server_info *, int (*client_handler)(struct ipc_server_info *, int), int *);
int send_add_event_entry(char *event_name, unsigned int event_id);
int send_notify(int event_id);
int send_add_method_entry(char *method_name, char *libpath, char *plugin_name);

#endif

