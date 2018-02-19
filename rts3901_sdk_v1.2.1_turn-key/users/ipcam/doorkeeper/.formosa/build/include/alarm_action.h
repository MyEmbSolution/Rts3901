#ifndef _ALARM_ACTION_H_
#define _ALARM_ACTION_H_

#define MAX_ACTION_ENTRY_SIZE		32
#define MAX_ACTION_ENTRY_NAME_SIZE	32

#define MAX_METHOD_ENTRY_SIZE		32
#define MAX_METHOD_ENTRY_NAME_SIZE	32


#define MAX_ACTIONS_IN_LIST		15
#define VIDEO_CLIP_LENGTH		3

#define ACTION_NAME_SAVESTREAM	"SAVESTREAM"
#define ACTION_NAME_SNAPSHOT	"SAVESNAPSHOT"
#define ACTION_NAME_DO			"DO"
#define ACTION_NAME_NONE		"NONE"


#define METHOD_NONE				"NONE"
#define METHOD_SD				"SD"
#define METHOD_FTP				"FTP"

#define FTP_CMD	"curl -T %s	ftp://%s:%s --user %s:%s"

struct s_method_ctx {
	unsigned int method_id;
	int active;
};

struct s_method_ctx_list {
	int size;
	struct s_method_ctx method_ctx[MAX_METHOD_ENTRY_SIZE];
};

struct s_act_ctx {
	unsigned int action_id;
	int enabled;
	struct s_method_ctx_list method_list;
};

struct s_act_ctx_list {
	int size;
	struct s_act_ctx ctx[MAX_ACTION_ENTRY_SIZE];
};

enum {
	ACTION_TYPE_RESERVED = 0,
	ACTION_TYPE_SAVE_STREAM = 1,
	ACTION_TYPE_TAKE_SNAPSHOT = 2,
	ACTION_TYPE_DO_NOTHING = 3,
} e_action_type;

enum {
	NOTIFY_METHOD_TYPE_RESERVED = 0,
	NOTIFY_METHOD_TYPE_SD = 1,
	NOTIFY_METHOD_TYPE_FTP = 2,
};


struct s_action {
	int enabled;
	int action_type;
	int notify_method_type;
};



struct s_action_list {
	int size;
	struct s_action	action[MAX_ACTIONS_IN_LIST];
};




struct action_entry {
	char name[MAX_ACTION_ENTRY_NAME_SIZE];
	unsigned int type;
};

struct notify_method_entry {
	char name[MAX_METHOD_ENTRY_NAME_SIZE];
	unsigned int type;
};

struct videoclip_info {
	int channel;
	int b_send_email;
	int b_send_ftp;
	int b_send_samba;
	int b_send_sd;
	int event_id;
	char videoclipStr[128];
} ;

struct snapshot_info {
	int channel;
	int b_send_email;
	int b_send_ftp;
	int b_send_samba;
	int b_send_sd;
	int event_id;
};



struct s_event_ftp_data {
	int enabled;
	char ip[16];
	char port[8];
	char account[64];
	char password[64];
};


struct s_event_ftp_list {
	int  size;
	struct s_event_ftp_data  c_entry[10];
};


struct s_event_ftp {
	struct s_event_ftp_list ftp_list;
};

struct action_handler {
	unsigned int action_id;
	int (*handler)(struct s_method_ctx_list *method_ctx_list);
};



struct method_data {
	char filepath[256];
	int save2sd;
};

struct method_handler {
	unsigned int method_id;
	int b_dll_link;
	char dllpath[64];
	char plugin_name[64];
	int (*handler)(struct method_data *data);
};

void init_ftp_data();
void init_action_var();
void init_action_entry();
void init_method_entry();
void init_method_handler();
void init_action_handler();
void add_dll_method_entry(char *method_name, char *dllpath, char *plugin_name);


#endif


