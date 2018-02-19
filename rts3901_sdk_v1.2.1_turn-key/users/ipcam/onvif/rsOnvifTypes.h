#ifndef _RS_ONVIF_TYPES_H
#define _RS_ONVIF_TYPES_H

#include "rsOnvifDefines.h"
#include <netinet/in.h>

/*
 * typedef char bool;
 */
typedef char onvif_bool;
typedef char int8;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long long uint64;

enum {
	SENDER = 0,
	RECEIVER,
};

enum VIDEO_ENC_FMT {
	JPEG = 0,
	MPEG4,
	H264,
};

enum AUDIO_ENC_FMT {
	G711 = 0,
	G726,
	AAC,
};

enum COLOR_SPACE {
	YCbCr = 0,
	CIELuv,
	CIELab,
	HSV,
};

typedef struct s_osd_configuration {
	char token[OSD_TOKEN_LENGTH];
	char osd_string_buf[OSD_STRING_LENGTH];

	float osd_color_x;
	float osd_color_y;
	float osd_color_z;
	uint8 osd_color_type;
	uint8 osd_transparent;

	float osd_bk_color_x;
	float osd_bk_color_y;
	float osd_bk_color_z;
	uint8 osd_bk_color_type;
	uint8 osd_bk_transparent;

	uint8 osd_font_size;

	uint8 osd_type;/*text 0 image 1 others 2+*/

	uint16 osd_start_x;
	uint16 osd_end_x;
	uint16 osd_start_y;
	uint16 osd_end_y;

} osd_configuration_t;

typedef struct s_video_source_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	char source_token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int use_count;
	int bounds_x;
	int bounds_y;
	int bounds_width;
	int bounds_height;
	int rotate_mode; /*0: switch off; 1: switch on (degree is used); 2: auto*/
	int rotate_degree;

} video_source_configuration_t;

typedef struct s_video_encoder_configuration {
	char vec_path[MEDIA_SEC_PATH_LENGTH];
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int  use_count;

	int encoding_format;/*0:JPEG; 1:H264*/
	int encoding_width;
	int encoding_height;
	float quality;

	int frame_rate_limit;
	int encoding_interval;
	int bitrate_limit;

	int h264_gov_length;
	int h264_profile;/* 0: base line; 1:main; 2: extended; 4: high;*/

	int multicast_ip_type; /*0: IPV4; 1: IPV6*/
	char multicast_ip_addr[MAX_IPADDR_STR_LENGTH];
	int multicast_port;
	int multicast_auto_start;
	int multicast_ttl;
	long session_timeout;


} video_encoder_configuration_t;


typedef struct s_audio_source_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	char source_token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int use_count;

} audio_source_configuration_t;

typedef struct s_audio_encoder_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int use_count;

	int encoding_format; /* 0: G711; 1:G726; 2: AAC*/
	int bit_rate;
	int sample_rate;

	int multicast_ip_type; /*0: IPV4; 1: IPV6*/
	char multicast_ip_addr[MAX_IPADDR_STR_LENGTH];
	int port;
	int auto_start;
	int ttl;
	long session_timeout;

} audio_encoder_configuration_t;

typedef struct s_audio_decoder_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int  use_count;

} audio_decoder_configuration_t;

typedef struct s_audio_output_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	char output_token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int  use_count;
	int  output_level;
	char send_primacy[40];
} audio_output_configuration_t;

typedef struct s_ptz_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int  use_count;
	/*
	char default_abs_pt_space[];
	char default_abs_zoom_space[];
	char default_rel_pt_space[];
	char default_rel_zoom_space[];
	char default_cont_pt_velocity_space[];
	char default_cont_zoom_velocity_space[];
	*/
	float default_speed_pt_x;
	float default_speed_pt_y;
	float default_speed_zoom_x;
	uint64 default_ptz_timeout;
	float pt_limits_x_max;
	float pt_limits_x_min;
	float pt_limits_y_max;
	float pt_limits_y_min;
	float zoom_limits_x_max;
	float zoom_limits_x_min;
	int   ptctrl_eflip_mode; /*EFlip Mode OFF 0, EFlipMode ON  1, EFlip Mode Extended  2*/
} ptz_configuration_t;

typedef struct s_video_analytics_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int  use_count;
} video_analytics_configuration_t;


typedef struct s_metadata_configuration {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	uint8 force_persistence;
	int  use_count;
	char ptz_status[MAX_METADATA_DESCRIPTION_LENGTH];
	char ptz_position[MAX_METADATA_DESCRIPTION_LENGTH];
	char event_filter[MAX_METADATA_DESCRIPTION_LENGTH];
	char event_subscription_policy[MAX_METADATA_DESCRIPTION_LENGTH];
	char analytics[MAX_METADATA_DESCRIPTION_LENGTH];
	int  multicast_ip_type; /*0: IPV4; 1: IPV6*/
	char multicast_ip_addr[MAX_IPADDR_STR_LENGTH];
	int  multicast_port;
	int  multicast_auto_start;
	int  multicast_ttl;
	long session_timeout;
} metadata_configuration_t;

typedef struct s_media_config {
	video_source_configuration_t    vsc[MAX_MEDIA_VSC_CONFIG];
	video_encoder_configuration_t   vec[MAX_MEDIA_VEC_CONFIG];
	audio_source_configuration_t    asc[MAX_MEDIA_ASC_CONFIG];
	audio_encoder_configuration_t   aec[MAX_MEDIA_AEC_CONFIG];
	audio_decoder_configuration_t   adc[MAX_MEDIA_ADC_CONFIG];
	audio_output_configuration_t    aoc[MAX_MEDIA_AOC_CONFIG];
	ptz_configuration_t             ptzc[MAX_MEDIA_PTZC_CONFIG];
	video_analytics_configuration_t vac[MAX_MEDIA_VAC_CONFIG];
	metadata_configuration_t        mc[MAX_MEDIA_MC_CONFIG];
} media_config_t;

typedef struct s_media_profile {
	char name[MEDIA_NAME_LENGTH];
	char token[MEDIA_TOKEN_LENGTH];
	uint8 fixed;/*this profile can be deleted or not.*/
	int vsc_index;
	int vec_index;
	int asc_index;
	int aec_index;
	int adc_index;
	int aoc_index;
	int ptzc_index;
	int vac_index;
	int mc_index;
} media_profile_t;

typedef enum e_service_index {
	SERVICE_INDEX_DEVICE,
	SERVICE_INDEX_MEDIA,
	SERVICE_INDEX_DEVICEIO,
	SERVICE_INDEX_PTZ,
	SERVICE_INDEX_IMAGING,
	SERVICE_INDEX_ANALYTICSDEVICE,
	SERVICE_INDEX_EVENTS,
	ONVIF_SERVICE_NUM
} service_index_t;

/*
"ANY" in the string list indicates that the the class itself is in the list of classes.
For instance:
location[0] == "ANY" indicates that "onvif://www.onvif.org/location" is in the list of scopes;
*/
typedef struct s_onvif_scopes {
	char profile[MAX_SCOPES_BUF_SIZE];
	char type[MAX_SCOPES_TYPE_NUM][MAX_SCOPES_BUF_SIZE];
	char name[MAX_SCOPES_BUF_SIZE];
	char location[MAX_SCOPES_LOCATION_NUM][MAX_SCOPES_BUF_SIZE];
	char hardware[MAX_SCOPES_BUF_SIZE];
	/*
	 * all other scopes not included in above classes;
	 * the character directly followes onvif://www.onvif.org/ when replying scopes request;
	 */
	char others[MAX_SCOPES_OTHER_NUM][MAX_SCOPES_BUF_SIZE];
} onvif_scopes_t;

typedef struct s_network_capabilities {
	uint8 ip_filter;
	uint8 zero_configuration;
	uint8 ip_version6;
	uint8 dynamic_dns;
	uint8 dot_11_configuration;
	int  dot1x_configurations;
	uint8 hostname_from_dhcp;
	int  ntp_server_number;
	uint8 dhcp_v6;
	char any_attribute[OPTIONAL_ATTRIBUTE_SIZE];
} network_capabilities_t;

typedef struct s_security_capabilities {
	uint8 tls1_x002e0;
	uint8 tls1_x002e1;
	uint8 tls1_x002e2;
	uint8 onboard_key_generation;
	uint8 access_policy_config;
	uint8 default_access_policy;
	uint8 dot1x;
	uint8 remote_user_handling;
	uint8 x_x002e509_token;
	uint8 saml_token;
	uint8 kerberos_token;
	uint8 username_token;
	uint8 http_digest;
	uint8 rel_token;
	char supported_eap_methods[EAP_METHOD_LENGTH];
	/*
	 * it must be a numeric string for instance,
	 * "0","10" http://www.iana.org/assignments/eap-numbers/eap-numbers.xml
	 */
	char any_attribute[OPTIONAL_ATTRIBUTE_SIZE];
} security_capabilities_t;


typedef struct s_system_capabilities {
	uint8 discovery_resolve;
	uint8 discovery_bye;
	uint8 remote_discovery;
	uint8 system_backup;
	uint8 system_logging;
	uint8 firmware_upgrade;
	uint8 http_firmware_upgrade;
	uint8 http_system_backup;
	uint8 http_system_logging;
	uint8 http_support_information;
	char any_attribute[OPTIONAL_ATTRIBUTE_SIZE];
} system_capabilities_t;

typedef struct s_misc_capabilities {
	char auxiliary_commands[OPTIONAL_ATTRIBUTE_SIZE];    /* optional attribute of type tt:StringAttrList */
	char any_attribute[OPTIONAL_ATTRIBUTE_SIZE];    /* optional attribute of type xsd:anyType */
} misc_capabilities_t;

typedef struct s_io_capabilities {
	int input_connectors;
	int relay_outputs;
} io_capabilities_t;


/* tds:DeviceServiceCapabilities */
typedef struct s_device_service_capabilities {
	network_capabilities_t network;
	security_capabilities_t security;
	system_capabilities_t system;
	io_capabilities_t io;
	misc_capabilities_t misc;
} device_service_capabilities_t;

typedef struct s_event_service_capabilities {
	uint8 ws_subscription_policy_support;
	uint8 ws_pull_point_support;
	uint8 ws_pausable_submgr_support;
	uint8 persistentNotificationStorage;
	int max_notificationproducers;
	int max_pull_points;
} event_service_capabilities_t;


typedef struct s_device_information {
	char manufacturer[MID_INFO_SIZE];
	char model[MID_INFO_SIZE];
	char firmware_version[MID_INFO_SIZE];
	char serial_number[MID_INFO_SIZE];
	char hardware_id[MID_INFO_SIZE];
} device_information_t;

typedef struct s_date_time_settings {
	uint8 time_ntp;  /*fales: manual; true: ntp time;*/
	uint8 day_light_savings; /*false: no dst; true: dst;*/
	char time_zone[MAX_TZ_LENGTH];
} date_time_settings_t;

typedef struct s_system_uris {
	char system_log_uri[MID_INFO_SIZE];
	char system_backup_uri[MID_INFO_SIZE];
	char support_info_uri[MID_INFO_SIZE];
	char system_restore_uri[MID_INFO_SIZE];
} system_uris_t;

typedef struct s_account {
	char    user[USER_LEN];            /*username*/
	char    password[PASSWORD_LEN];    /*password*/
	int8    authority;                /*user authority*/
	uint8    fixed;                    /*indicates if it can be deleted;*/
} acount_t;

typedef struct s_network_interface {
	/*
	 * the indicator of the network interface,
	 * for instance, eth0, eht0, wlan0 etc.
	 */
	char token[NETWORK_TOKEN_LENGTH];
	uint8 mac[MACH_ADDR_LENGTH]; /*please note it is an array of 8-bit integer, not a string;*/
	int  mtu;
	uint8 enabled;
	struct in_addr   ip;
	uint8 dhcp_enable; /*current DHCP status*/
} network_interface_t;

typedef struct s_hostname {
	char name[MID_INFO_SIZE];
	uint8 dhcp_enable;
} hostname_t;

typedef struct s_dns_config {
	uint8 dhcp_enable;
	struct in_addr    manul_dns[MAX_DNS_NUM];
	struct in_addr    dhcp_dns[MAX_DNS_NUM];
	char search_domain[MAX_SEARCH_DOMAIN_NUM][SEARCH_DOMAIN_LENGTH];
} dns_config_t;

typedef struct s_ntp_config {
	uint8 dhcp_enable;
	struct in_addr    manual_ntp[MAX_NTP_NUM];
	struct in_addr    dhcp_ntp[MAX_NTP_NUM];
} ntp_config_t;

typedef struct s_network_protocol {
	char name[NETWORK_PROTOCOL_NAME_LENGTH];
	uint8 enabled;
	unsigned short port[MAX_PROTOCOL_PORT_NUM];
} network_protocol_t;

typedef struct s_network_config {
	system_uris_t system_uris;
	int discovery_mode;/*0: discoverable, 1:undiscoverable;*/
	int remote_discovery_mode;/*0: discoverable, 1:undiscoverable;*/
	char discover_proxy_address[IP_ADDR_BUF_SIZE];/*DP address;*/
	acount_t accounts[MAX_ACCOUNT_NUM];
	hostname_t host_name;
	network_interface_t interfaces[MAX_NETWORK_INTERFACE_NUM];
	int interface_num;
	int interface_idx;
	struct in_addr    netmask; /* netmask in static IP mode*/
	struct in_addr    gateway; /* gateway in static IP mode*/
	dns_config_t      dns;
	ntp_config_t      ntp; /* DNS IP in static IP mode*/
	network_protocol_t protocol[NETWORK_PROTOCOL_NUM];
} network_config_t;

typedef struct s_onvif_special_info {
	/*onvif scopes;*/
	onvif_scopes_t scopes;
	/*Provided service namespace;*/
	char service_ns[ONVIF_SERVICE_NUM][ONVIF_SERVICE_NS_LENGTH];
	/*provided service name;*/
	char service_name[ONVIF_SERVICE_NUM][ONVIF_SERVICE_NAME_LEGNTH];
	/*service version, (MAJOR<<8)|MINOR;*/
	short service_version[ONVIF_SERVICE_NUM];
	/*device type*/
	char device_type[DEVICE_TYPE_BUF_SIZE];
	/*device service capabilities.*/
	device_service_capabilities_t device_service_capabilities;
	event_service_capabilities_t  event_service_capabilities;
} onvif_special_info_t;

typedef struct {
	char uuid[UUID_BUF_SIZE];
	onvif_special_info_t onvif_special_info;
	device_information_t device_info;
	date_time_settings_t date_time_settings;
	network_config_t     network_config;
	media_profile_t	 media_profile[MAX_MEDIA_PROFILE];
	media_config_t media_config;
	osd_configuration_t osd_config[MAX_OSD_NUM];
} RS_System_Info;


typedef struct s_date_time {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
} date_time_t;


/*All config info stored in this struct*/
typedef struct {
	int change_counter;
	int state;/*0:sending hello 1:stop sending hello msg;*/
	RS_System_Info system_info;
} RS_LOCAL_SYSTEM_INFO;

typedef struct s_image_setting {
	uint8 sharpness_flag;
	float sharpness;
	float sharpness_max;
	float sharpness_min;

	uint8 contrast_flag;
	float contrast;
	float contrast_max;
	float contrast_min;

	uint8 brightness_flag;
	float brightness;
	float brightness_max;
	float brightness_min;

	uint8 saturation_flag;
	float saturation;
	float saturation_max;
	float saturation_min;
} image_setting_t;


#endif
