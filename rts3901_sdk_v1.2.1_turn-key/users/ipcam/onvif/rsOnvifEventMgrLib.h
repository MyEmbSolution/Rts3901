#ifndef RS_ONVIF_EVENTS_MGRLIB_H
#define RS_ONVIF_EVENTS_MGRLIB_H

#define MAX_EVENTS_SUBMGR_NUM 128

#define SUBMGR_ID_BUFFER_LEN 24 /*real length of the id is 20 the first 20 characters of nonce value passed by client*/

#define SUBMGR_MSGQ_NAME_PREFIX "/onvif_events_msgq_"

#define SUBMGR_MESSAGE_DATA_LENGTH 124

#define MOTION_DETECT_TEMPLATE  "<tt:Message UtcTime=\"%s\" PropertyOperation=\"Initialized\">"\
									"<tt:Source>"\
										"<tt:SimpleItem Name=\"VideoSourceConfigurationToken\" Value=\"1\"/>"\
										"<tt:SimpleItem Name=\"VideoAnalyticsConfigurationToken\" Value=\"2\"/>"\
										"<tt:SimpleItem Value=\"Motion\" Name=\"MotionDetection\"/>"\
								"</tt:Source>"\
								"<tt:Data>"\
									"<tt:SimpleItem Name=\"window1\" Value=\"%d\" />"\
									"<tt:SimpleItem Name=\"window2\" Value=\"%d\" />"\
									"<tt:SimpleItem Name=\"window3\" Value=\"%d\" />"\
								"</tt:Data>"\
								"</tt:Message>"

typedef enum e_events_submgr_message_type {
	EVENTS_SUBMGR_MESSAGE_UNSUBSCRIBE,
	EVENTS_SUBMGR_MESSAGE_RENEW,
	EVENTS_SUBMGR_MESSAGE_MAX
} events_submgr_message_type_t;


typedef enum e_submgr_type {
	SUBMGR_TYPE_PULLPOINT,/*0: pullpoint subscription manager*/
	SUBMGR_TYPE_NOTIFICATION/*1:base notification subscription manager;*/
} submgr_type_t;

typedef struct s_events_submgr_info_item {
    char id[SUBMGR_ID_BUFFER_LEN];
    submgr_type_t type;
} events_submgr_info_item_t;

typedef struct s_events_submgr_message {
	events_submgr_message_type_t type;
	char data[SUBMGR_MESSAGE_DATA_LENGTH];
} events_submgr_message;

typedef struct s_events_submgr_unscribe {
	char submgr_id[SUBMGR_ID_BUFFER_LEN];
} events_submgr_unscribe_t;

typedef struct s_events_submgr_renew {
	char submgr_id[SUBMGR_ID_BUFFER_LEN];
	long time_out;
} events_submgr_renew_t;


typedef struct s_submgr_parms {
	submgr_type_t type;
	char  *id;
	long timeout;
	char *consumer_address;
} events_submgr_parms_t;

typedef struct s_submgr_info {
	int submgr_number;
	unsigned char motion_detect_result;
	events_submgr_info_item_t items[MAX_EVENTS_SUBMGR_NUM];
} submgr_info_t;

/*AP*/
int rs_onvif_events_submgr_info_create();
int rs_onvif_events_submgr_info_destroy();
int rs_onvif_events_submgr_info_put_item(events_submgr_info_item_t *item);
int rs_onvif_events_submgr_info_remove_item(char *submgr_id);
int rs_onvif_events_submgr_info_get_submgr_number(int *number);

/*device:events service*/
int rs_onvif_events_submgr_unsubscribe(events_submgr_unscribe_t *arg);
int rs_onvif_events_submgr_renew(events_submgr_renew_t *arg);
unsigned char motion_detect(const char *dev_path);

#endif
