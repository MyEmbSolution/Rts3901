#include "rsOnvifConfig.h"
uint16 DEVICE_SERVICE_VERSION           = 0x0001;
uint16 MEDIA_SERVICE_VERSION            = 0x0001;
uint16 DEVICEIO_SERVICE_VERSION         = 0x0001;
uint16 PTZ_SERVICE_VERSION              = 0x0001;
uint16 IMAGING_SERVICE_VERSION          = 0x0001;
uint16 ANALYTICSDEVICE_SERVICE_VERSION  = 0x0001;
uint16 EVENTS_SERVICE_VERSION           = 0x0001;

/* char *DEVICE_TYPE               = "tdn:NetworkVideoTransmitter tds:Device"; */
char *DEVICE_TYPE               = "tdn:NetworkVideoTransmitter";
char *SCOPES_TYPE               = "NetworkVideoTransmitter";
char *SCOPES_PROFILE            = "Streaming";
char *SCOPES_LOCATION           = "COUNTRY/CHINA CITY/SUZHOU AREA/SIP";
char *SCOPES_HARDWARE           = "RTS_SOC_CAM";
char *SCOPES_NAME               = "IPCAMERA1";
char *SCOPES_OTHERS             = "OTHER1 OTHER2 OTHER3";
char *DEVICE_MANUFACTURER       = "RTS_SUZHOU";
char *DEVICE_MODEL              = "201402010001";
char *DEVICE_FIRMWARE_VERSION   = "1.0";
char *DEVICE_SERIAL_NUMBER      = "012345678";
char *DEVICE_TIME_ZONE          = "GMT8";
char *SYSTEM_LOG_URI            = "log";
char *SYSTEM_BACKUP_URI         = "backup";
char *SUPPORT_INFO_URI          = "support_info";
char *SYSTEM_RESTORE_URI        = "restore";
char *DEFAULT_ACCOUNT_NAME      = "admin";
char *DEFAULT_ACCOUNT_PSWD      = "123456";
int  DEFAULT_ACCOUNT_LEVEL      = 0;
char *DEFAULT_HOSTNAME          = "ipcam00";
char *DEFAULT_SEARCH_DOMAINNAME = "ipcam";
char *DEFAULT_NTP_ADDRESS       = "time.nist.gov";




device_service_capabilities_t device_service_capabilities = {
	.network = {
		.ip_filter = 0,
		.zero_configuration = 0,
		.ip_version6 = 0,
		.dynamic_dns = 0,
		.dot_11_configuration = 0,
		.dot1x_configurations = 0,
		.hostname_from_dhcp = 0,
		.ntp_server_number = MAX_NTP_NUM,
		.dhcp_v6 = 0,
		.any_attribute = {0},
	},
	.security = {
		.tls1_x002e0 = 0,
		.tls1_x002e1 = 0,
		.tls1_x002e2 = 0,
		.onboard_key_generation = 0,
		.access_policy_config = 0,
		.default_access_policy = 0,
		.dot1x = 0,
		.remote_user_handling = 0,
		.x_x002e509_token = 0,
		.saml_token = 0,
		.kerberos_token = 0,
		.username_token = 1,
		.http_digest = 0,
		.rel_token = 0,
		.supported_eap_methods = "0",
		.any_attribute = {0},
	},
	.system = {
		.discovery_resolve = 1,
		.discovery_bye = 1,
		.remote_discovery = 0,
		.system_backup = 0,
		.system_logging = 0,
		.firmware_upgrade = 0,
		.http_firmware_upgrade = 0,
		.http_system_backup = 0,
		.http_system_logging = 0,
		.http_support_information = 0,
		.any_attribute = {0},
	},
	.io = {
		.input_connectors = 0,
		.relay_outputs = 0,
	},
	.misc = {
		.auxiliary_commands = "auxiliary_commands",
		.any_attribute = {0}
	},

};

event_service_capabilities_t event_service_capabilities = {
	.ws_subscription_policy_support = 1,
	.ws_pull_point_support = 1,
	.ws_pausable_submgr_support = 0,
	.persistentNotificationStorage = 0,
	.max_notificationproducers = 16,
	.max_pull_points = 16

};

media_config_t default_media_config = {
	.vsc = {
		{
			.name = "VideoSourceConfig0",
			.token = "VideoSourceConfig0",
			.source_token = "VideoSource0",
			.force_persistence = 1,
			.use_count = 2,
			.bounds_x = 0,
			.bounds_y = 0,
			.bounds_width = 1280,
			.bounds_height = 720,
			.rotate_mode = 0, /*0: switch off; 1: switch on (degree is used); 2: auto */
			.rotate_degree = 0
		},
	},
	.vec = {
		{
			.vec_path = "/dev/video51",
			.name = "VideoEncoderConfig0",
			.token = "VideoEncoderConfig0",
			.force_persistence = 1,
			.use_count = 1,
			.encoding_format = H264,
			.encoding_width = 1280,
			.encoding_height = 720,
			.quality = 26,
			.frame_rate_limit = 30,
			.encoding_interval = 1,
			.bitrate_limit = 512, /*512kbps*/


			.h264_gov_length = 30,
			.h264_profile = 0, /*  0: base line; 1:main; 2: extended; 4: high; */

			.multicast_ip_type = 0, /* 0: IPV4; 1: IPV6 */
			.multicast_ip_addr = "239.0.0.0",
			.multicast_port = 43794,
			.multicast_auto_start = 1,
			.multicast_ttl = 1500,
			.session_timeout = 200
		},
		{
			.vec_path = "/dev/video52",
			.name = "VideoEncoderConfig3",
			.token = "VideoEncoderConfig3",
			.force_persistence = 1,
			.use_count = 1,
			.encoding_format = JPEG,
			.encoding_width = 1280,
			.encoding_height = 720,
			.quality = 26,
			.frame_rate_limit = 30,
			.encoding_interval = 1,
			.bitrate_limit = 512, /*512kbps*/


			.h264_gov_length = 30,
			.h264_profile = 0, /*  0: base line; 1:main; 2: extended; 4: high; */

			.multicast_ip_type = 0, /* 0: IPV4; 1: IPV6 */
			.multicast_ip_addr = "239.0.0.0",
			.multicast_port = 43794,
			.multicast_auto_start = 1,
			.multicast_ttl = 1500,
			.session_timeout = 200
		},
		{
			.vec_path = "/dev/video54",
			.name = "VideoEncoderConfig2",
			.token = "VideoEncoderConfig2",
			.force_persistence = 1,
			.use_count = 1,
			.encoding_format = H264,
			.encoding_width = 320,
			.encoding_height = 240,
			.quality = 26,

			.frame_rate_limit = 30,
			.encoding_interval = 1,
			.bitrate_limit = 256, /*256Kbps*/


			.h264_gov_length = 30,
			.h264_profile = 0, /*  0: base line; 1:main; 2: extended; 4: high; */

			.multicast_ip_type = 0, /* 0: IPV4; 1: IPV6 */
			.multicast_ip_addr = "239.0.0.0",
			.multicast_port = 43794,
			.multicast_auto_start = 1,
			.multicast_ttl = 1500,
			.session_timeout = 200
		},
		{
			.vec_path = "/dev/video53",
			.name = "VideoEncoderConfig1",
			.token = "VideoEncoderConfig1",
			.force_persistence = 1,
			.use_count = 0,
			.encoding_format = H264,
			.encoding_width = 640,
			.encoding_height = 480,
			.quality = 26,

			.frame_rate_limit = 30,
			.encoding_interval = 1,
			.bitrate_limit = 256, /*256kbps*/

			.h264_gov_length = 30,
			.h264_profile = 0, /*  0: base line; 1:main; 2: extended; 4: high; */

			.multicast_ip_type = 0, /* 0: IPV4; 1: IPV6 */
			.multicast_ip_addr = "239.0.0.0",
			.multicast_port = 43794,
			.multicast_auto_start = 1,
			.multicast_ttl = 1500,
			.session_timeout = 200
		},
	},
	.asc = {
		{
			.name = "AudioSourceConfig0",
			.token = "AudioSourceConfig0",
			.source_token = "AudioSource0",
			.force_persistence = 1,
			.use_count = 1,
		},
		{
			.name = "AudioSourceConfig1",
			.token = "AudioSourceConfig1",
			.source_token = "AudioSource0",
			.force_persistence = 1,
			.use_count = 1,
		}
	},
	.aec = {
		{
			.name = "AudioEncoderConfig0",
			.token = "AudioEncoderConfig0",
			.force_persistence = 1,
			.use_count = 1,

			.encoding_format = G711,
			.bit_rate = 64,
			.sample_rate = 8,

			.multicast_ip_type = 0, /* 0: IPV4; 1: IPV6 */
			.multicast_ip_addr = "239.0.0.0",
			.port = 43794,
			.auto_start = 1,
			.ttl = 1500,
			.session_timeout = 200
		},
		{
			.name = "AudioEncoderConfig1",
			.token = "AudioEncoderConfig1",
			.force_persistence = 1,
			.use_count = 1,

			.encoding_format = G711,
			.bit_rate = 64,
			.sample_rate = 8,

			.multicast_ip_type = 0, /* 0: IPV4; 1: IPV6 */
			.multicast_ip_addr = "239.0.0.0",
			.port = 43794,
			.auto_start = 1,
			.ttl = 1500,
			.session_timeout = 200
		}
	},
	.adc = {0},
	.aoc = {0},
	.ptzc = {0},
	.vac = {0},
	.mc = {0}
};

media_profile_t default_profile_A = {
	.name = "profile1",
	.token = "profile1",
	.fixed = 1,
	.vsc_index = FIRST_CONFIG,
	.vec_index = FIRST_CONFIG,
	.asc_index = 0,
	.aec_index = 0,
	.adc_index = 0,
	.aoc_index = 0,
	.ptzc_index = 0,
	.vac_index = 0,
	.mc_index = 0
};

media_profile_t default_profile_B = {
	.name = "profile2",
	.token = "profile2",
	.fixed = 1,
	.vsc_index = FIRST_CONFIG,
	.vec_index = SECOND_CONFIG,
	.asc_index = 0,
	.aec_index = 0,
	.adc_index = 0,
	.aoc_index = 0,
	.ptzc_index = 0,
	.vac_index = 0,
	.mc_index = 0
};

media_profile_t default_profile_C = {
	.name = "profile3",
	.token = "profile3",
	.fixed = 0,
	.vsc_index = FIRST_CONFIG,
	.vec_index = THIRD_CONFIG,
	.asc_index = 0,
	.aec_index = 0,
	.adc_index = 0,
	.aoc_index = 0,
	.ptzc_index = 0,
	.vac_index = 0,
	.mc_index = 0
};

osd_configuration_t default_OSD_A = {
	.token = "OSD_A",
	.osd_string_buf = "Realtek001",
	.osd_color_x = 128.0,
	.osd_color_y = 128.0,
	.osd_color_z = 128.0,
	.osd_color_type = YCbCr,
	.osd_transparent = 8,
	.osd_bk_color_x = 100.0,
	.osd_bk_color_y = 100.0,
	.osd_bk_color_z = 100.0,
	.osd_bk_color_type = YCbCr,
	.osd_bk_transparent = 8,
	.osd_font_size = 32,
	.osd_type = 0,
	.osd_start_x = 32,
	.osd_end_x = 1000,
	.osd_start_y = 32,
	.osd_end_y = 120,
};

osd_configuration_t default_OSD_B = {
	.token = "OSD_B",
	.osd_string_buf = "Realsil001",
	.osd_color_x = 128.0,
	.osd_color_y = 128.0,
	.osd_color_z = 128.0,
	.osd_color_type = YCbCr,
	.osd_transparent = 8,
	.osd_bk_color_x = 100.0,
	.osd_bk_color_y = 100.0,
	.osd_bk_color_z = 100.0,
	.osd_bk_color_type = YCbCr,
	.osd_bk_transparent = 8,
	.osd_font_size = 32,
	.osd_type = 0,
	.osd_start_x = 32,
	.osd_end_x = 1000,
	.osd_start_y = 32,
	.osd_end_y = 120,
};
