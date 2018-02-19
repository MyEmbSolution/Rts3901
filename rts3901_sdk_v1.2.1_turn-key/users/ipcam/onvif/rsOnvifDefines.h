#ifndef _RS_ONVIF_DEFINES_H
#define _RS_ONVIF_DEFINES_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#define MULTICAST_ADDR	"239.255.255.250"
#define MULTICAST_PORT	3702

#define SEND_HELLO_MSG_TIMES (4)
#define SEND_BYE_MSG_TIMES   (1)

#define SYSTEM_SHARED_MEMORY_NAME	"/usr"
#define SYSTEM_SEMAPHORE_NAME		"/var"
#define SYSTEM_INFO_NAME		"/usr/conf/rsOnvifSysInfo.json"

#define SCOPES_BUF_SIZE	1024
#define EDR_BUF_SIZE	46

#define MAX_SCOPES_BUF_SIZE	100
#define MAX_SCOPES_LOCATION_NUM	5
#define MAX_SCOPES_TYPE_NUM	5
#define MAX_SCOPES_OTHER_NUM	5

#define UUID_BUF_SIZE		37
#define XADDR_BUF_SIZE		96
#define DEVICE_TYPE_BUF_SIZE	64
#define MESSAGEID_BUF_SZIE	46

#define ONVIF_SERVICE_NS_LENGTH			60
#define ONVIF_SERVICE_NAME_LEGNTH		20
#define ONVIF_SERVICE_CAPABILIIES_LENGTH	900

#define OPTIONAL_ATTRIBUTE_SIZE	100
#define EAP_METHOD_LENGTH	100

#define MID_INFO_SIZE		100

#define USER_LEN                32        /* < Maximum of acount username length. */
#define MIN_USER_LEN            4         /* < Maximum of acount username length. */
#define PASSWORD_LEN            16        /* < Maximum of acount password length. */
#define MAX_ACCOUNT_NUM         16

#define IP_ADDR_BUF_SIZE        37

#define MAX_DNS_NUM             2
#define MAX_NTP_NUM             5
#define MAX_SEARCH_DOMAIN_NUM   6

#define SEARCH_DOMAIN_LENGTH    84

#define MAX_TZ_LENGTH           64

#define NETWORK_TOKEN_LENGTH            20 /*size of buffer containing token name; */
#define MACH_ADDR_LENGTH		6
#define MAX_NETWORK_INTERFACE_NUM       1
#define CUR_USABLE_INTERFACE_NUM	1   /* this <= MAX_NETWORK_INTERFACE_NUM*/
#define SYSTEM_IFNAME              "br0"
#define SYSTEM_IFNAME_IFHAVE_1     "eth1"
#define SYSTEM_IFNAME_IFHAVE_2     "eth2"
#define SYSTEM_IFNAME_IFHAVE_3     "eth3"
#define SYSTEM_IFNAME_IFHAVE_4     "eth4"

#define RESOLV_CONF                "/etc/resolv.conf"
#define INTERFACES                 "/etc/network/interfaces"
#define PROC_NET_ROUTE_PATH        "/proc/net/route"


/*
 * Accodring to the definition of GetNetworkProtocols
 * only 3 protocols are supported https, rtsp, http
*/
#define NETWORK_PROTOCOL_NAME_LENGTH	6
#define NETWORK_PROTOCOL_NUM		3
#define MAX_PROTOCOL_PORT_NUM		6
#define HTTP_NAME			"HTTP"
#define HTTPS_NAME			"HTTPS"
#define RTSP_NAME			"RTSP"
#define HTTP_PORT			80
#define HTTPS_PORT			81
#define H264_PORT_1			43794
#define H264_PORT_2			43795
#define MPEG4_PORT_1			43796
#define MPEG4_PORT_2			43797
#define MJPEG_PORT_1			43798

#define SENSOR_FMT_HEIGHT	(720)
#define SENSOR_FMT_WIDTH	(1280)
#define SENSOR_NAME		"OV9715"
#define VS_MAX_FPS		(30)

/*typde definitions*/

/*media*/
#define MAX_MEDIA_PROFILE	4
#define MAX_MEDIA_VSC_CONFIG	4
#define MAX_MEDIA_VEC_CONFIG	4
#define MAX_MEDIA_ASC_CONFIG	4
#define MAX_MEDIA_AEC_CONFIG	4
#define MAX_MEDIA_ADC_CONFIG	4
#define MAX_MEDIA_AOC_CONFIG	4
#define MAX_MEDIA_PTZC_CONFIG	4
#define MAX_MEDIA_VAC_CONFIG	4
#define MAX_MEDIA_MC_CONFIG	4

#define FIRST_CONFIG	1
#define SECOND_CONFIG	2
#define THIRD_CONFIG	3
#define FOURTH_CONFIG	4

#define MEDIA_SEC_PATH_LENGTH		20
#define MEDIA_NAME_LENGTH		20
#define MEDIA_TOKEN_LENGTH		20
#define MAX_IPADDR_STR_LENGTH		40 /*IPV4, 15 + 1; IPV6 39+1;*/
#define MAX_METADATA_DESCRIPTION_LENGTH	32

/*OSD*/
#define MAX_OSD_NUM		4
#define OSD_TOKEN_LENGTH	10
#define OSD_STRING_LENGTH	40


#define MAC_LENGTH			IFHWADDRLEN
#define MAX_LINE_LENGTH_RESOLV_CONF	280
#define MAX_LINE_NUMBER_RESOLV_CONF	300
#define MAX_LINE_LENGTH_INTERFACES	280
#define MAX_LINE_NUMBER_INTERFACES	300


#define DEVICE_NS          "http://www.onvif.org/ver10/device/wsdl"
#define MEDIA_NS           "http://www.onvif.org/ver10/media/wsdl"
#define DEVICEIO_NS        "http://www.onvif.org/ver10/deviceIO/wsdl"
#define PTZ_NS             "http://www.onvif.org/ver20/ptz/wsdl"
#define IMAGING_NS         "http://www.onvif.org/ver20/imaging/wsdl"
#define ANALYTICSDEVICE_NS "http://www.onvif.org/ver20/analyticsdevice/wsdl"
#define EVENTS_NS          "http://www.onvif.org/ver10/events/wsdl"

/*
 * we just design 1 cgi
 * so all the searvice name is device_service
 */
/*
#define DEVICE_SERVICE_NAME		"device_service"
#define MEDIA_SERVICE_NAME		"media"
#define DEVICEIO_SERVICE_NAME		"deviceIO"
#define PTZ_SERVICE_NAME		"ptz"
#define IMAGING_SERVICE_NAME		"imaging"
#define ANALYTICSDEVICE_SERVICE_NAME	"analyticsdevice"
#define EVENTS_SERVICE_NAME             "events"
*/

#define DEVICE_SERVICE_NAME		"device_service"
#define MEDIA_SERVICE_NAME		"device_service"
#define DEVICEIO_SERVICE_NAME		"device_service"
#define PTZ_SERVICE_NAME		"device_service"
#define IMAGING_SERVICE_NAME		"device_service"
#define ANALYTICSDEVICE_SERVICE_NAME	"device_service"
#define EVENTS_SERVICE_NAME		"device_service"

/* 5826 configuration */
#define RTS5826_ENCODING_INTERVAL	1
#define RTS5826_GOP_MIN			1
#define RTS5826_GOP_MAX			255
#define RTS5826_BITRATE_MIN		64000
#define RTS5826_BITRATE_MAX		3000000
#define RTS5826_FPS_MIN			1
#define RTS5826_FPS_MAX			30
#define RTS5826_SUPPORTED_PROFILE_NUM	1
#define RTS5826_QP_MIN			1
#define RTS5826_QP_MAX			51

/* macro function */
/* #define ONVIF_DEBUG_MODE */

#ifdef ONVIF_DEBUG_MODE
#define write_log(fmt, args...)
#else
#define write_log(fmt, args...) \
		syslog(LOG_ERR, "%-20s--> "fmt, __func__, ##args);
#endif

#define ASSERT(a) {if (!(a)) { \
	write_log("Assert Failed, %s, %s, Line %d, errno(if have)%d\n", \
	__FILE__, __func__, __LINE__, errno);\
	assert(0); } }


#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif
