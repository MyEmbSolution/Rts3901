#ifndef _RS_ONVIF_DEV_CTRL_H
#define _RS_ONVIF_DEV_CTRL_H
#include "rsOnvifTypes.h"
#include "rsOnvifDefines.h"


#define ONVIF_CONTROL_MQ_NAME "/onvif_control_message_queue"

/* 512 - 4 */
#define CONTROL_DATA_LENGTH 508
#define SPAWN_SUBMANAGER_PARM_LENGTH 120

typedef struct s_control_message_operate_isp {
	uint8 MainCMDID;
	uint8 SubCMDID;
	uint8  direction;/* 0: read 1:write */
	uint16 addr;
	uint16 len;
	uint8 *buf;
	uint8  status;
} control_message_operate_isp_t;

typedef struct s_control_message_image_setting {
	uint8  direction;/* 0: read 1:write */
	char *device_name;
	image_setting_t image_setting;
} control_message_image_setting_t;


typedef struct s_control_message_snapshot {
	int width;
	int height;
	int fmt;
} control_message_snapshot_t;



typedef struct s_control_message_set_manul_dns {
	int dns_num;
	struct in_addr    dns[MAX_DNS_NUM];
} control_message_set_manual_dns_t;

typedef struct s_control_message_set_search_domain {
	int domain_num;
	char search_domain[MAX_SEARCH_DOMAIN_NUM][SEARCH_DOMAIN_LENGTH];
} control_message_set_search_domain_t;

typedef struct s_control_message_set_manul_ntp {
	int ntp_num;
	struct in_addr    ntp[MAX_DNS_NUM];
} control_message_set_manual_ntp_t;

typedef struct s_control_message_set_timezone {
	char  tz[MAX_TZ_LENGTH];
} control_message_set_manual_timezone_t;

typedef struct s_control_message_set_host_name {
	char name[MID_INFO_SIZE];
} control_message_set_host_name_t;

typedef struct s_control_message_set_network_interface {
	int idx;
	struct in_addr manual_ip;
	onvif_bool dhcp_enable;

} control_message_set_network_interface_t;



typedef struct s_control_message_set_date_time {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
} control_message_set_date_time_t;


typedef struct s_control_message_set_network_protocols {
	network_protocol_t protocol[NETWORK_PROTOCOL_NUM];
} control_message_set_network_protocols_t;

typedef struct s_control_message_set_default_gateway {
	struct in_addr gateway;
} control_message_set_default_gateway_t;

typedef struct s_control_message_set_system_factory_default {
	onvif_bool soft_default;
} control_message_set_system_factory_default_t;


typedef struct s_control_message_create_submgr_t {
	char submgr_cmd[SPAWN_SUBMANAGER_PARM_LENGTH];
} control_message_create_submgr_t;

typedef enum e_control_message_type {

	ONVIF_CTRL_MSG_SET_MANUAL_DNS = 0,
	ONVIF_CTRL_MSG_SET_SEARCH_DOMAIN,
	ONVIF_CTRL_MSG_SET_DHCP_DNS, /* set dns through dhcp */
	ONVIF_CTRL_MSG_SET_MANUAL_NTP,
	ONVIF_CTRL_MSG_SET_DHCP_NTP, /* set ntp through dhcp */
	ONVIF_CTRL_MSG_SET_TIME_ZONE,
	ONVIF_CTRL_MSG_SET_DST,
	ONVIF_CTRL_MSG_SET_DATE_TIME,
	ONVIF_CTRL_MSG_SET_HOST_NAME,
	ONVIF_CTRL_MSG_SET_NETWORK_INTERFACE,
	ONVIF_CTRL_MSG_SET_NETWORK_PROTOCOLS,
	ONVIF_CTRL_MSG_SET_DEFAULT_GATEWAY,
	ONVIF_CTRL_MSG_SET_SYSTEM_FACTORY_DEFAULT,
	ONVIF_CTRL_MSG_REBOOT_DEVICE,
	ONVIF_CTRL_MSG_SEND_HELLO,
	ONVIF_CTRL_MSG_SYNC_NTP,
	ONVIF_CTRL_MSG_OPERATE_ISP,
	ONVIF_CTRL_MSG_IMAGE_SETTING,
	ONVIF_CTRL_MSG_CREATE_SUBMGR,
	ONVIF_CTRL_MSG_GENERATE_INTRA_FRAME,

	ONVIF_CONTROL_MESSAGE_MAXIMUM_NUMBER

} control_message_type_t;


typedef struct s_control_message {
	control_message_type_t type;
	char data[CONTROL_DATA_LENGTH];

} control_message_t;


void *rsOnvifWSDDHelloThread(void *arg);
void *rsOnvifWSDDMonitorMsgThread(void *arg);
void *rsOnvifSysDevCtrlThread(void *arg);

static void *ntp_loop(void *arg);
int rsOnvifSysSyncNTPServer(char *server_name);

static int rsOnvifDevCtrlSetManualDNS(control_message_set_manual_dns_t *arg);
static int rsOnvifDevCtrlSetSearchDomain(control_message_set_search_domain_t *arg);
static int rsOnvifDevCtrlSetDHCPDNS(void);
static int rsOnvifDevCtrlSetManualNTP(control_message_set_manual_ntp_t *arg);
static int rsOnvifDevCtrlSetDHCPNTP(void);
static int rsOnvifDevCtrlSetDST(int value);
static int rsOnvifDevCtrlSetTimeZone(control_message_set_manual_timezone_t *arg);
static int rsOnvifDevCtrlSyncNTP(void);
static int rsOnvifDevCtrlSetDateTime(control_message_set_date_time_t *arg);
static int rsOnvifDevCtrlReboot(void);
static int rsOnvifDevCtrlSendHello(void);
static int rsOnvifDevCtrlSetHostname(control_message_set_host_name_t *arg);
static int rsOnvifDevCtrlSetNetworkInterface(control_message_set_network_interface_t *arg);
static int rsOnvifDevCtrlSetNetworkProtocols(control_message_set_network_protocols_t *arg);
static int rsOnvifDevCtrlSetDefaultGateway(control_message_set_default_gateway_t *arg);
static int rsOnvifDevCtrlSetSystemFactoryDefault(control_message_set_system_factory_default_t *arg);
static int rsOnvifDevCtrlOperateISP(control_message_operate_isp_t *arg);
static int rsOnvifDevCtrlCreateSubmgr(control_message_create_submgr_t *arg);
static int rsOnvifDevCtrlGenerateIntraFrame(void);
static int process_msg(control_message_t *msg);
void rsOnvifWsddByeHandler(void);
/* int rsOnvifDevCtrlSnapshot(control_message_snapshot_t *arg); */
int parse_peacock_profile_name(char *buf, char *vec_path);
#endif

