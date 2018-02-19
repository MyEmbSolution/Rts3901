#include <sys/mman.h>
#include <fcntl.h>   /* For O_* constants */
#include <sys/stat.h>/* For mode constants */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/reboot.h>
#include <pthread.h>
#include <mqueue.h>
#include <linux/videodev2.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <netdb.h>

#include <string.h>

#include <octopus/octopus.h>
#include <octopus/rts_cam.h>
#include <peacock.h>
#include <json-c/json.h>
#include <pe_message.h>
#include <sysconf.h>

#include "soapH.h"
#include "wsddapi.h"

#include "rsOnvifDefines.h"
#include "rsOnvifConfig.h"
#include "rsOnvifTypes.h"
#include "rsOnvifCommonFunc.h"
#include "rsOnvifDevCtrl.h"

/*local file defines*/
#define MESSAGE_QUEUE_LENGTH 	(10)
/*#define UDP_LISTEN_TIMEOUT	(120)*/
#define UDP_LISTEN_TIMEOUT	(4)

#define NTPCLIENT_EXE		"ntpclient"
#define NTP_TIMEOUT_CNT		(5)

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

static int ntp_check = 0;
static void *ntp_loop(void *arg)
{
	char buf[256];
	char *server_name = arg;
	/*
	sprintf(buf, NTPCLIENT_EXE " -s -i %d -h %s &\n", NTP_TIMEOUT_CNT, server_name);
	RS_DBG("run ntpclient to sync ntp: %s\n", buf);
	spawn_new_process(buf);
	*/
	sprintf(buf,
		NTPCLIENT_EXE " -s -i %d -h %s > /dev/null\n",
		NTP_TIMEOUT_CNT,
		server_name);
	RS_DBG("run ntpclient to sync ntp: %s\n", buf);
	system(buf);
	sleep(1);
	ntp_check = 1;
	return (void *) 0;
}

int rsOnvifSysSyncNTPServer(char *server_name)
{
	char buf[256];
	pthread_t ntpThread;
	ntp_check = 0;
	int timout_cnt = NTP_TIMEOUT_CNT;
	int ret = 0;

	if (pthread_create(&ntpThread, NULL, ntp_loop, server_name)) {
		return -1;
	}

	while (timout_cnt--) {
		if (ntp_check == 0) {
			sleep(1);
		} else {
			break;
		}
	}

	if (ntp_check == 0) {
		sprintf(buf, "killall -9 ntpclient");
		system(buf);
		ret = -1;
		RS_DBG("UNABLE TO SYNC TO SNTP SERVER in %d seconds\n",
				NTP_TIMEOUT_CNT);
	}

	pthread_join(ntpThread, NULL);
	ntp_check = 0;
	RS_DBG("%s return value %d\n", __func__, ret);
	return ret;
}


int rsOnvifDevCtrlSetManualDNS(control_message_set_manual_dns_t *arg)
{
	int ret = 0;
	int i;
	network_config_t *network =
		(network_config_t *)malloc(sizeof(network_config_t));

	ASSERT(network);
	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);

	ret = rsOnvifNetDisableDHCP(NULL);
	ret = rsOnvifNetSetDNS(arg->dns, arg->dns_num);
	ASSERT(!ret);
	network->dns.dhcp_enable = 0;
	for (i = 0; i < MAX_DNS_NUM; i++) {
		if (i < arg->dns_num)
			network->dns.manul_dns[i] = arg->dns[i];
		else
			memset(&network->dns.manul_dns[i],
					0, sizeof(struct in_addr));

		memset(&network->dns.dhcp_dns[i], 0, sizeof(struct in_addr));
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	ASSERT(!ret);

	free(network);
	return ret;
}

static int rsOnvifDevCtrlSetSearchDomain(
		control_message_set_search_domain_t *arg)
{
	int ret = 0;
	int i;
	network_config_t *network;
	char *domain[MAX_SEARCH_DOMAIN_NUM];

	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);
	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);

	for (i = 0; i < arg->domain_num; i++) {
		domain[i] = arg->search_domain[i];
	}

	ret = rsOnvifNetSetSearchDomain(domain, arg->domain_num);
	ASSERT(!ret);
	for (i = 0; i < MAX_DNS_NUM; i++) {
		if (i < arg->domain_num) {
			strncpy(network->dns.search_domain[i],
					domain[i], SEARCH_DOMAIN_LENGTH);
		} else {
			memset(&network->dns.search_domain[i],
					0, SEARCH_DOMAIN_LENGTH);
		}
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	ASSERT(!ret);

	free(network);
	return ret;
}

static int rsOnvifDevCtrlSetDHCPDNS(void)
{
	int ret = 0;

	int i;
	network_config_t *network =
		(network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);

	ret = rsOnvifNetEnableDHCP(NULL);
	RS_DBG("\nnet_enable_dhcp ret %d\n\n\n", ret);
	if (!ret) {
		network->dns.dhcp_enable = 1;
		sleep(1); /*sleep to wait get dhcp ip address*/
		rsOnvifNetGetDNS(network->dns.dhcp_dns, MAX_DNS_NUM);
	}

	for (i = 0; i < MAX_DNS_NUM; i++) {
		network->dns.manul_dns[i].s_addr = 0;
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	ASSERT(!ret);
	free(network);

	return ret;
}

static int rsOnvifDevCtrlSetManualNTP(control_message_set_manual_ntp_t *arg)
{
	int ret = 0;
	int i;
	network_config_t *network =
		(network_config_t *)malloc(sizeof(network_config_t));

	ASSERT(network);
	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);

	for (i = 0; i < MAX_NTP_NUM; i++) {
		if (i < arg->ntp_num)
			network->ntp.manual_ntp[i] = arg->ntp[i];
		else
			network->ntp.manual_ntp[i].s_addr = 0;
	}

	/*to do if necessary;*/
	network->ntp.dhcp_enable = 0;
	for (i = 0; i < MAX_NTP_NUM; i++) {
		network->ntp.dhcp_ntp[i].s_addr = 0;
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	ASSERT(!ret);
	free(network);

	return ret;
}

static int rsOnvifDevCtrlSetDHCPNTP(void)
{
	int ret = 0;
	int i;
	network_config_t *network =
		(network_config_t *)malloc(sizeof(network_config_t));

	ASSERT(network);
	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);

	network->ntp.dhcp_enable = 1;
	/* should get ntp addr from set_time.json in future*/
	struct hostent *ntpserver;
	ntpserver = gethostbyname(DEFAULT_NTP_ADDRESS);

	if (ntpserver && ntpserver->h_length == 4) {
		memcpy(&(network->ntp.dhcp_ntp[0].s_addr),
				ntpserver->h_addr_list[0], 4);
	} else {
		inet_aton("0.0.0.0", &network->ntp.dhcp_ntp[0]);
	}


	for (i = 0; i < MAX_NTP_NUM; i++) {
		network->ntp.manual_ntp[i].s_addr = 0;
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	ASSERT(!ret);
	free(network);

	return ret;
}

int rsOnvifDevCtrlSetDST(int value)
{
	int ret;
	date_time_settings_t *set =
		(date_time_settings_t *)malloc(sizeof(date_time_settings_t));
	ASSERT(set);

	ret = rsOnvifSysInfoGetDateTimeSettings(set);
	if (ret)
		goto end;

	if (value != set->day_light_savings) {
		if (!rsOnvifSysChangeDstTimeZone(value, set->time_zone)) {
			set->day_light_savings = value;
			set->time_ntp = 0;
			ret = rsOnvifSysInfoSetDateTimeSettings(set);
		}
	}
end:
	free(set);
	return ret;
}

static int rsOnvifDevCtrlSetTimeZone(
		control_message_set_manual_timezone_t *arg)
{
	int ret;
	date_time_settings_t *set =
		(date_time_settings_t *)malloc(sizeof(date_time_settings_t));
	ASSERT(set);

	ret = rsOnvifSysInfoGetDateTimeSettings(set);
	if (ret)
		goto end;
	RS_DBG("%s, new %s, old %s\n", __func__, arg->tz, set->time_zone);

	if (strcmp(arg->tz, set->time_zone)) {

		if (!rsOnvifSysChangeDstTimeZone(set->day_light_savings,
					arg->tz)) {

			strncpy(set->time_zone, arg->tz, MAX_TZ_LENGTH);
			set->time_ntp = 0;
			ret = rsOnvifSysInfoSetDateTimeSettings(set);
		}
	}
end:
	free(set);
	return ret;
}

static int rsOnvifDevCtrlSyncNTP(void)
{
	int ret;
	network_config_t *network;
	date_time_settings_t *set;
	int i;

	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);
	set = (date_time_settings_t *)malloc(sizeof(date_time_settings_t));
	ASSERT(set);
	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto end;

	ret = rsOnvifSysInfoGetDateTimeSettings(set);
	if (ret)
		goto end;

	set->time_ntp = 1;
	for (i = 0; i < MAX_NTP_NUM; i++) {

		struct in_addr ntp;
		if (network->ntp.dhcp_enable)
			ntp = network->ntp.dhcp_ntp[i];
		else
			ntp = network->ntp.manual_ntp[i];

		if (!ntp.s_addr) {
			ret = 1;
			break;
		}

		if (!rsOnvifSysSyncNTPServer(inet_ntoa(ntp))) {
			RS_DBG("Begin SetDateTimeSettings\n");
			ret = rsOnvifSysInfoSetDateTimeSettings(set);
			break;
		}
	}
end:
	free(set);
	free(network);
	return ret;
}


static int rsOnvifDevCtrlSetDateTime(control_message_set_date_time_t *arg)
{
	int ret = 0;

	int year = arg->year;
	int month = arg->month;
	struct tm tm;
	time_t now;

	year = (year > 1900) ? year-1900 : 0;
	tm.tm_year = year;
	month = (month > 0) ? month-1 : 0;
	tm.tm_mon = month;

	tm.tm_mday = arg->day;
	tm.tm_hour = arg->hour;
	tm.tm_min = arg->minute;
	tm.tm_sec = arg->second;

	now = mktime(&tm);
	if (now < 0)
		ret = -1;
	stime(&now);

	if (!ret) {
		date_time_settings_t *set =
			(date_time_settings_t *)malloc(sizeof(date_time_settings_t));
		ASSERT(set);
		ret = rsOnvifSysInfoGetDateTimeSettings(set);
		ASSERT(!ret);
		set->time_ntp = 0;
		RS_DBG("%s, ntp %d\n", __func__, set->time_ntp);
		ret = rsOnvifSysInfoSetDateTimeSettings(set);
		ASSERT(!ret);
		free(set);
	}
	return ret;
}


static int rsOnvifDevCtrlReboot()
{
	int ret = 0;
	/*
	 * notify deivce_discovery_daemon that the system is rebooting,
	 * and stop sending hello message if hello task is working;
	 */
	rsOnvifSysInfoSetSendHelloState(1);
	rsOnvifWsddByeHandler();
	rsOnvifSysInfoWrite2Memory();
	sleep(1);/*reliquished the cpu and wait for a while (request returns);*/
	ret = reboot(LINUX_REBOOT_CMD_RESTART);

	return ret;
}

static int rsOnvifDevCtrlSendHello()
{
	int ret = 0;
	pthread_t hello_thread_handle;
	pthread_create(&hello_thread_handle, NULL,
			rsOnvifWSDDHelloThread, NULL);

	return ret;
}

static int rsOnvifDevCtrlSetHostname(control_message_set_host_name_t *arg)
{
	int ret = 0;

	network_config_t *network;
	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);

	if (sethostname(arg->name, strlen(arg->name) + 1) != -1) {
		strcpy(network->host_name.name, arg->name);
		ret = rsOnvifSysInfoSetNetConfig(network);
		ASSERT(!ret);
	}
	free(network);

	return ret;
}


static int rsOnvifDevCtrlSetNetworkInterface(
		control_message_set_network_interface_t *arg)
{
	int ret = 0;
	network_config_t *network;

	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);
	if (arg->manual_ip.s_addr != 0) {
		ret = rsOnvifNetSetStaticIPAddr(
				network->interfaces[arg->idx].token,
				arg->manual_ip,
				network->netmask,
				network->gateway);
		if (!ret) {
			network->interfaces[arg->idx].dhcp_enable = 0;
			network->interfaces[arg->idx].ip.s_addr =
				arg->manual_ip.s_addr;
			ret = rsOnvifSysInfoSetNetConfig(network);
			ASSERT(!ret);
		}
	} else if (arg->dhcp_enable) {
		RS_DBG("name %s\n", network->interfaces[arg->idx].token);
		ret = rsOnvifNetEnableDHCP(network->interfaces[arg->idx].token);
		RS_DBG("\nnet_enable_dhcp ret %d\n\n\n", ret);
		if (!ret) {
			network->interfaces[arg->idx].dhcp_enable =
				arg->dhcp_enable;
			RS_DBG("\ndhcp_enable %d, idx %d\n\n\n",
					arg->dhcp_enable, arg->idx);
			sleep(1); /*sleep to wait get dhcp ip address*/
			network->interfaces[arg->idx].ip.s_addr =
				rsOnvifNetGetIFAddr(
					network->interfaces[arg->idx].token);
			/*
			 * get new ip from device to shm ,
			 * now we donnt reboot device
			 */
			ret = rsOnvifSysInfoSetNetConfig(network);
			ASSERT(!ret);
		}
	}

	sleep(2); /* sleep to wait the network interface up */

	return ret;
}

int rsOnvifDevCtrlSetNetworkProtocols(
		control_message_set_network_protocols_t *arg)
{
	int ret = 0;
	network_config_t *network;
	int i, j;

	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);
	RS_DBG("%s\n", __func__);
	for (i = 0; i < NETWORK_PROTOCOL_NUM; i++) {

		for (j = 0; j < NETWORK_PROTOCOL_NUM; j++) {

			if (!strcmp(arg->protocol[i].name,
					network->protocol[j].name)) {

				RS_DBG("arg %s: net%s: i %d, j %d\n",
					arg->protocol[i].name,
					network->protocol[j].name, i, j);

				int err = rsOnvifNetConfigProtocol(
						arg->protocol[i].name,
						arg->protocol[i].enabled,
						arg->protocol[i].port);

				if (!err) {
					int n;
					network->protocol[j].enabled =
						arg->protocol[i].enabled;
					for (n = 0; n < MAX_PROTOCOL_PORT_NUM; n++) {

						network->protocol[j].port[n] =
							arg->protocol[i].port[n];
						RS_DBG("port %d ",
							arg->protocol[i].port[n]);
					}
				}
			}
		}
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	ASSERT(!ret);
	free(network);
	return ret;
}

static int rsOnvifDevCtrlSetDefaultGateway(
		control_message_set_default_gateway_t *arg)
{
	int ret = 0;
	network_config_t *network;


	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto end;

	RS_DBG("function %s: addr %s\n", __func__, inet_ntoa(arg->gateway));

	if (!rsOnvifNetSetGateway(arg->gateway.s_addr)) {
		network->gateway = arg->gateway;
		ret = rsOnvifSysInfoSetNetConfig(network);
	}

end:
	free(network);
	return ret;
}

static int rsOnvifDevCtrlSetSystemFactoryDefault(
		control_message_set_system_factory_default_t *arg)
{
	int ret = 0;
	/* 0 hard reset 1: soft reset*/
	rsOnvifWsddByeHandler();
	rsOnvifSysInfoSetDefault();

	if (arg->soft_default) {
		/*soft reset*/
		rsOnvifSysInfoWrite2Memory();
	} else {
		/*hard reset */
		;
	}
	sleep(1);
	ret = reboot(LINUX_REBOOT_CMD_RESTART);

	return ret;

}

static int rsOnvifDevCtrlGenerateIntraFrame()
{
	int ret = 0;
	int h264_flag = 0x00000001; /*bit0:1 generate I frame*/
	int i = 0;
	char tmp_buf[256] = { 0 };
	RS_DBG("before parse peacock s\n");
	parse_peacock_profile_name(tmp_buf, NULL);
	RS_DBG("after parse peacock s\n");
	char (*list)[][100];
	RS_DBG("after parse peacock 1\n");
	list = (char (*)[][100])malloc(10 * sizeof(char *));
	RS_DBG("after parse peacock 2\n");
	memset(list, 0, 10 * sizeof(char *));
	RS_DBG("after parse peacock 3\n");
	explodeitem(tmp_buf, '+', *list, 10);
	RS_DBG("profiles = %s\n", tmp_buf);
	for (i = 0; i < 10; i++)
		RS_DBG(" check index = %d, profile = %s\n", i, (*list)[i]);
	for (i = 0; i < 10; i++) {
		if (strlen((*list)[i])) {
			RS_DBG("index = %d, profile = %s\n", i, (*list)[i]);
			ret = pe_send_msg_generate_intra_frame((*list)[i]);
			if (ret)
				goto out;
			RS_DBG("ret = %d\n", ret);
		} else {
			break;
		}
	}

out:
	if (list)
		free(list);
	return ret;
}

static int process_msg(control_message_t *msg)
{
	int ret = 0;
	switch (msg->type) {
	case ONVIF_CTRL_MSG_SET_MANUAL_DNS:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_MANUAL_DNS\n");
		ret = rsOnvifDevCtrlSetManualDNS(
			(control_message_set_manual_dns_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_SEARCH_DOMAIN:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_SEARCH_DOMAIN\n");
		ret = rsOnvifDevCtrlSetSearchDomain(
			(control_message_set_search_domain_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_DHCP_DNS:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_DHCP_DNS\n");
		ret = rsOnvifDevCtrlSetDHCPDNS();
		break;
	case ONVIF_CTRL_MSG_SET_MANUAL_NTP:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_MANUAL_NTP\n");
		ret = rsOnvifDevCtrlSetManualNTP(
			(control_message_set_manual_ntp_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_DHCP_NTP:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_DHCP_NTP\n");
		ret = rsOnvifDevCtrlSetDHCPNTP();
		break;
	case ONVIF_CTRL_MSG_SET_TIME_ZONE:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_TIME_ZONE\n");
		ret = rsOnvifDevCtrlSetTimeZone(
			(control_message_set_manual_timezone_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_DST:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_DST\n");
		ret = rsOnvifDevCtrlSetDST(*(int *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SYNC_NTP:
		RS_DBG("cmd ONVIF_CTRL_MSG_SYNC_NTP\n");
		ret = rsOnvifDevCtrlSyncNTP();
		break;
	case ONVIF_CTRL_MSG_SET_DATE_TIME:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_DATE_TIME\n");
		ret = rsOnvifDevCtrlSetDateTime(
			(control_message_set_date_time_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_REBOOT_DEVICE:
		RS_DBG("cmd ONVIF_CTRL_MSG_REBOOT_DEVICE\n");
		ret = rsOnvifDevCtrlReboot();
		break;
	case ONVIF_CTRL_MSG_SEND_HELLO:
		RS_DBG("cmd ONVIF_CTRL_MSG_SEND_HELLO\n");
		ret = rsOnvifDevCtrlSendHello();
		break;
	case ONVIF_CTRL_MSG_SET_HOST_NAME:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_HOST_NAME\n");
		ret = rsOnvifDevCtrlSetHostname(
			(control_message_set_host_name_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_NETWORK_INTERFACE:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_NETWORK_INTERFACE\n");
		ret = rsOnvifDevCtrlSetNetworkInterface(
			(control_message_set_network_interface_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_NETWORK_PROTOCOLS:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_NETWORK_PROTOCOLS\n");
		ret = rsOnvifDevCtrlSetNetworkProtocols(
			(control_message_set_network_protocols_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_DEFAULT_GATEWAY:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_DEFAULT_GATEWAY\n");
		ret = rsOnvifDevCtrlSetDefaultGateway(
			(control_message_set_default_gateway_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_SET_SYSTEM_FACTORY_DEFAULT:
		RS_DBG("cmd ONVIF_CTRL_MSG_SET_SYSTEM_FACTORY_DEFAULT\n");
		ret = rsOnvifDevCtrlSetSystemFactoryDefault(
			(control_message_set_system_factory_default_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_OPERATE_ISP:
		RS_DBG("cmd ONVIF_CTRL_MSG_OPERATE_ISP\n");
		ret = rsOnvifDevCtrlOperateISP(
				(control_message_operate_isp_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_CREATE_SUBMGR:
		RS_DBG("cmd ONVIF_CTRL_MSG_CREATE_SUBMGR\n");
		ret = rsOnvifDevCtrlCreateSubmgr(
				(control_message_create_submgr_t *)msg->data);
		break;
	case ONVIF_CTRL_MSG_GENERATE_INTRA_FRAME:
		RS_DBG("cmd ONVIF_CTRL_MSG_GENERATE_INTRA_FRAME\n");
		ret = rsOnvifDevCtrlGenerateIntraFrame();
		break;
deafult:
		RS_DBG("cmd INVALID COMMAND\n");
		break;
	}

	return ret;
}

void rsOnvifWsddByeHandler()
{
	char endpoint[64];
	int send_counter = 0;
	network_config_t *network = NULL;
	onvif_special_info_t *info = NULL;

	sprintf(endpoint, "soap.udp://%s:%d", MULTICAST_ADDR, MULTICAST_PORT);

	info = (onvif_special_info_t *)malloc(sizeof(onvif_special_info_t));
	ASSERT(info);
	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);


	while (send_counter++ < SEND_BYE_MSG_TIMES) {
		int ret;
		ret = rsOnvifSysInfoGetSpecialInfo(info);
		ASSERT(!ret);
		ret = rsOnvifSysInfoGetNetConfig(network);
		ASSERT(!ret);

		if (!network->discovery_mode) {
			char *types;
			char *uuid;
			char *xaddr;
			char *endpoint_reference;
			char *scopes;
			char *message_id;


			struct soap *soap = soap_new(); /* to invoke messages*/
			ASSERT(soap);

			soap_init1(soap, SOAP_IO_UDP|SO_BROADCAST);

			types = (char *)soap_malloc(soap, DEVICE_TYPE_BUF_SIZE);
			ASSERT(types);
			memset(types, 0, DEVICE_TYPE_BUF_SIZE);

			uuid = (char *)soap_malloc(soap, UUID_BUF_SIZE);
			ASSERT(uuid);
			memset(uuid, 0, UUID_BUF_SIZE);

			xaddr = (char *)soap_malloc(soap, XADDR_BUF_SIZE);
			ASSERT(xaddr);
			memset(xaddr, 0, XADDR_BUF_SIZE);

			endpoint_reference = (char *)soap_malloc(soap, EDR_BUF_SIZE);
			ASSERT(endpoint_reference);
			memset(endpoint_reference, 0, EDR_BUF_SIZE);

			message_id = (char *)soap_malloc(soap, MESSAGEID_BUF_SZIE);
			ASSERT(message_id);
			memset(message_id, 0, MESSAGEID_BUF_SZIE);

			scopes = (char *)soap_malloc(soap, SCOPES_BUF_SIZE);
			ASSERT(scopes);
			memset(scopes, 0, SCOPES_BUF_SIZE);

			memcpy(types, info->device_type, DEVICE_TYPE_BUF_SIZE);
			RS_DBG("In func %s types = %s\n", __func__, types);

			sprintf(xaddr,
				"http://%s/onvif/%s",
				inet_ntoa(network->interfaces[network->interface_idx].ip),
				info->service_name[SERVICE_INDEX_DEVICE]);

			ret = rsOnvifSysInfoGetUUID(uuid);
			if (ret)
				goto clean_soap;

			sprintf(message_id, "urn:uuid:%s", uuid);

			assemble_scopes_string(scopes, info);

			if ((strlen(info->scopes.name) + strlen(scopes) + 28) < SCOPES_BUF_SIZE)
				sprintf(scopes+strlen(scopes),
					" onvif://www.onvif.org/name/%s",
					info->scopes.name);

			RS_DBG("send bye message counter %d\n", send_counter);
			ret = soap_wsdd_Bye(soap,
				SOAP_WSDD_ADHOC,/* or SOAP_WSDD_ADHOC for ad-hoc mode*/
				endpoint, /* "http(s):" URL, or "soap.udp:" UDP, or TCP/IP address*/
				message_id, /* a unique message ID*/
				message_id, /*inet_ntoa(network->interfaces[network->interface_idx].ip), where they can find me for WS-Discovery*/
				/*(char *)&types, //this argument must be a pointer to a pointer of string;*/
				types,
				scopes,
				NULL, /* MatchBy*/
				xaddr, /* XAddrs*/
				1);/* MDVersion*/

			if (ret != SOAP_OK) {
				RS_DBG("failed to send bye message: ret %d\n", ret);
			}
clean_soap:
			soap_destroy(soap);
			soap_end(soap);
			soap_done(soap);
			free(soap);
		}
	}

	if (info)
		free(info);

	if (network)
		free(network);

	return ;
}

void *rsOnvifWSDDHelloThread(void *arg)
{
	char endpoint[64];
	int send_counter = 0;
	network_config_t *network = NULL;
	onvif_special_info_t *info = NULL;

	sprintf(endpoint, "soap.udp://%s:%d", MULTICAST_ADDR, MULTICAST_PORT);

	info = (onvif_special_info_t *)malloc(sizeof(onvif_special_info_t));
	ASSERT(info);
	network = (network_config_t *)malloc(sizeof(network_config_t));
	ASSERT(network);

	ASSERT(rsOnvifSysInfoSetSendHelloState(0) == 0);

	while (send_counter++ < SEND_HELLO_MSG_TIMES) {
		int ret;
		int state = 0;
		ret = rsOnvifSysInfoGetSendHelloState(&state);
		ASSERT(!ret);
		RS_DBG("state %d\n", state);
		if (state)
			break;

		ret = rsOnvifSysInfoGetSpecialInfo(info);
		ASSERT(!ret);
		ret = rsOnvifSysInfoGetNetConfig(network);
		ASSERT(!ret);

		if (!network->discovery_mode) {
			char *types;
			char *uuid;
			char *xaddr;
			char *endpoint_reference;
			char *scopes;
			char *message_id;
			int sock;
			int i;
			struct soap *soap = soap_new(); /* to invoke messages*/
			ASSERT(soap);

			soap_init1(soap, SOAP_IO_UDP|SO_BROADCAST);

			types = (char *)soap_malloc(soap, DEVICE_TYPE_BUF_SIZE);
			ASSERT(types);
			memset(types, 0, DEVICE_TYPE_BUF_SIZE);
			uuid = (char *)soap_malloc(soap, UUID_BUF_SIZE);
			ASSERT(uuid);
			memset(uuid, 0, UUID_BUF_SIZE);

			xaddr = (char *)soap_malloc(soap, XADDR_BUF_SIZE);
			ASSERT(xaddr);
			memset(xaddr, 0, XADDR_BUF_SIZE);

			endpoint_reference = (char *)soap_malloc(soap, EDR_BUF_SIZE);
			ASSERT(endpoint_reference);
			memset(endpoint_reference, 0, EDR_BUF_SIZE);

			message_id = (char *)soap_malloc(soap, MESSAGEID_BUF_SZIE);
			ASSERT(message_id);
			memset(message_id, 0, MESSAGEID_BUF_SZIE);

			scopes = (char *)soap_malloc(soap, SCOPES_BUF_SIZE);
			ASSERT(scopes);
			memset(scopes, 0, SCOPES_BUF_SIZE);
			memcpy(types, info->device_type, DEVICE_TYPE_BUF_SIZE);

			RS_DBG("In func:%s types= %s\n", __func__, types);
			sprintf(xaddr,
				"http://%s/onvif/%s",
				inet_ntoa(network->interfaces[network->interface_idx].ip),
				info->service_name[SERVICE_INDEX_DEVICE]);

			ret = rsOnvifSysInfoGetUUID(uuid);
			if (ret)
				goto clean_soap;
			sprintf(message_id, "urn:uuid:%s", uuid);
			assemble_scopes_string(scopes, info);

			RS_DBG("send hello message counter %d\n", send_counter);
			RS_DBG("IP: %s send Hello Message\n", inet_ntoa(network->interfaces[network->interface_idx].ip));
			ret = soap_wsdd_Hello(soap,
				SOAP_WSDD_ADHOC,/*or SOAP_WSDD_ADHOC for ad-hoc mode*/
				endpoint, /* "http(s):" URL, or "soap.udp:" UDP, or TCP/IP address*/
				message_id, /* a unique message ID*/
				NULL,
				message_id, /*inet_ntoa(network->interfaces[network->interface_idx].ip), where they can find me for WS-Discovery*/
				/*(char *)&types, //this argument must be a pointer to a pointer of string;*/
				types,
				scopes,
				NULL, /*MatchBy*/
				xaddr, /* XAddrs*/
				1);/* MDVersion*/

			if (ret != SOAP_OK) {
				RS_DBG("failed to send hello: ret %d\n", ret);
			}

clean_soap:
			soap_destroy(soap);
			soap_end(soap);
			soap_done(soap);
			free(soap);
			sleep(4);
		}
	}
	ASSERT(rsOnvifSysInfoSetSendHelloState(1) == 0);

	if (info) {
		free(info);
		info = NULL;
	}
	if (network) {
		free(network);
		network = NULL;
	}

	return NULL;
}

void *rsOnvifWSDDMonitorMsgThread(void *arg)
{
	int ret = 0;
	struct soap *soap_udp;
	struct ip_mreq mc_req;

	mc_req.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
	mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
	while (1) {
		network_config_t *network = NULL;
		network = (network_config_t *)malloc(sizeof(network_config_t));
		ASSERT(network);
		ret = rsOnvifSysInfoGetNetConfig(network);
		ASSERT(!ret);

		if (!network->discovery_mode) {
			soap_udp = soap_new();
			ASSERT(soap_udp);
			soap_init1(soap_udp, SOAP_IO_UDP);
			soap_udp->errmode = 0;
			soap_udp->bind_flags = SO_REUSEADDR;
			if (!soap_valid_socket(soap_bind(soap_udp, NULL, MULTICAST_PORT, 100))) {
				soap_print_fault(soap_udp, stderr);
				goto clean;
			}

			if ((setsockopt(soap_udp->master,
					IPPROTO_IP,
					IP_ADD_MEMBERSHIP,
					(void *)&mc_req,
					sizeof(mc_req))) < 0) {
				/* the network device my not be ready.*/
				RS_DBG("setsockopt() failed, errno %d\n", errno);
				goto clean;
			}

			if (SOAP_OK != soap_wsdd_listen(soap_udp, UDP_LISTEN_TIMEOUT)) {
				RS_DBG("failed to listen: ret %d\n", ret);
			}
clean:
			soap_destroy(soap_udp);
			soap_end(soap_udp);
			soap_done(soap_udp);
			free(soap_udp);

		} else {
			sleep(UDP_LISTEN_TIMEOUT);
		}
		free(network);
	}

	free(arg);
	return NULL;
}


void *rsOnvifSysDevCtrlThread(void *arg)
{
	control_message_t *msg;
	int stop = 0;
	struct mq_attr attr;
	mqd_t msgq_id;
	int ret = 0;
	mode_t old_mode;

	msg = (control_message_t *)malloc(sizeof(control_message_t));
	if (msg == NULL)
		return NULL;

	/*rm the message queue if it already exists.*/
	/*it is extremely important for all the attribute can not be set correctly;*/
	ret = mq_unlink(ONVIF_CONTROL_MQ_NAME);
	if (!ret) {
		RS_DBG("failed to unlink %s errno %d\n", ONVIF_CONTROL_MQ_NAME, errno);
	}
	/* initialize the queue attributes */
	attr.mq_flags = 0;
	attr.mq_maxmsg = MESSAGE_QUEUE_LENGTH;
	attr.mq_msgsize = sizeof(control_message_t);
	attr.mq_curmsgs = 0;

	/* create the message queue */
	old_mode = umask(0);
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
	if ((mqd_t)-1 == msgq_id) {
		free(msg);
		ASSERT(0);
		return NULL;
	}
	umask(old_mode);

	do {
		ssize_t bytes_read;

		/* receive the message */
		bytes_read = mq_receive(msgq_id, (char *)msg, sizeof(control_message_t), NULL);
		ASSERT(bytes_read != -1);
		process_msg(msg);

	} while (!stop);
	mq_close(msgq_id);
	mq_unlink(ONVIF_CONTROL_MQ_NAME);
	free(msg);
	return NULL;
}


/*this is test function for isp operation*/
static int rsOnvifDevCtrlOperateISP(control_message_operate_isp_t *arg)
{
	int ret = 0;
/*
	char buf[1];
	char wBuf[2] = {0};

	int cam_handle = uvc_open("/dev/video1",2);
	uvc_read_mem(cam_handle,0x8002,1,buf);

	if (arg->direction == 0)
	{
		wBuf[0] = buf[0] | 1;
	}
	else
	{
		wBuf[0] = buf[0] & 0xFE;
	}
	uvc_write_mem(cam_handle,0x8002,1,wBuf);
	wBuf[0] |= 0x20;
	uvc_write_mem(cam_handle,0x8423,1,wBuf);

	uvc_close(cam_handle);
*/
	return ret;
}


static int get_msg_id(const char *path)
{
	key_t key;
	int msg = 0;
	int ret = 0;

	key = ftok(path, 0);
	if (key < 0) {
		ret = key;
		goto fail;
	}

	msg = msgget(key, IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);
	if (msg < 0) {
		ret = msg;
		goto fail;
	}
	return msg;
fail:
	RS_DBG("%s\n", strerror(errno));
	return ret;
}

#if 0
int rsOnvifDevCtrlSnapshot(control_message_snapshot_t *arg)
{
	int ret = 0;
	struct event_msg *event = NULL;
	struct snapshot_p *p = NULL;
	int msg = 0;
	int size = 0;
	int timeout = 1000; /* millisecond */

	msg = get_msg_id(MSG_FILE_PREFIX"/"MSG_MASTER_FILE);
	if (msg < 0) {
		RS_DBG("get message id fail\n");
		ret = -errno;
		goto out;
	}

	event = calloc(1, sizeof(*event) + MAX_PATH_LEN);
	if (!event) {
		RS_DBG("%s:%s\n", __func__, strerror(errno));
		ret = -ENOMEM;
		goto out;
	}

	event->length = 0;
	event->mtype = MSG_TYPE_REQUEST;
	event->pid = getpid();
	event->event = EVENT_TYPE_SNAPSHOT;

	p = &event->snapshot;

	/* set default value */
	p->width = 640;
	p->height = 480;
	p->format = V4L2_PIX_FMT_MJPEG;
	if (arg->width)
		p->width = arg->width;
	if (arg->height)
		p->height = arg->height;

	size = sizeof(*event) - sizeof(event->mtype);
	ret = msgsnd(msg, event, size, IPC_NOWAIT);

	if (ret && EAGAIN == errno) {
		RS_DBG("server is busy, try again\n");
		ret = -errno;
		goto out;
	}

	size = MAX_PATH_LEN + (sizeof(*event) - sizeof(event->mtype));
	memset(event, 0, sizeof(*event) + MAX_PATH_LEN);

	do {
		int recv = 0;
		recv = msgrcv(msg, event, size, getpid(),
				IPC_NOWAIT | MSG_NOERROR);
		if (recv < 0) {
			ret = -errno;
			if (EAGAIN == errno || ENOMSG == errno) {
				usleep(100 * 1000);
				timeout -= 100;
				continue;
			} else {
				RS_DBG("%s:%d %s\n",
					__func__, errno, strerror(errno));
				break;
			}
		} else {
			event->length =
				recv - (sizeof(*event) - sizeof(event->mtype));
			ret = 0;
			break;
		}
	} while (timeout > 0);

out:
	return ret;
}
#endif

int parse_peacock_profile_name(char *buf, char *vec_path)
{
	int i = 0;
	int ret = 0;
	int num = 0;
	char dev_path[128] = {0};
	char profile_name[128] = {0};
	ret = rts_conf_scanf(CFG_DOMAIN_MULTIMEDIA, "%k%z", "profiles", &num);
	if (ret) {
		RS_DBG("In func:%s using libsysconf scanf failed\n", __func__);
		return ret;
	}
	/*have to change later*/
	for (i = 0; i < num; i++) {

		ret = rts_conf_scanf(CFG_DOMAIN_MULTIMEDIA,
				"%k%i%k%l%s", "profiles", i, "video",
				sizeof(dev_path), dev_path);
		ret = rts_conf_scanf(CFG_DOMAIN_MULTIMEDIA,
				"%k%i%k%l%s", "profiles", i, "name",
				sizeof(profile_name), profile_name);
		if (!vec_path) {
			static int idx = 0;
			char *p = buf + idx;
			strcpy(buf, profile_name);
			idx += strlen(profile_name);
			*(buf + idx) = '+';
			idx += 1;
		} else if (!strncmp(vec_path, dev_path, strlen(vec_path))) {
			strcpy(buf, profile_name);
		}
	}

	return 0;
}

#define SUBMGR_TIMEOUT_CNT		(2)
static int submgr_flag = 0;
static void *submgr_loop(void *val)
{
	control_message_create_submgr_t *arg = (control_message_create_submgr_t *)val;
	spawn_new_process(arg->submgr_cmd);
	RS_DBG("creating subscripton: %s\n", arg->submgr_cmd);
	sleep(1);
	submgr_flag = 1;
	return (void *) 0;
}

static int rsOnvifDevCtrlCreateSubmgr(control_message_create_submgr_t *arg)
{
	int ret = 0;
#if 1
	RS_DBG("Msg to create subscription manager:%s\n", arg->submgr_cmd);
	ret = spawn_new_process(arg->submgr_cmd);
#else
	pthread_t submgr_thread;
	submgr_flag = 0;
	int timout_cnt = SUBMGR_TIMEOUT_CNT;

	RS_DBG("Msg to create subscription manager:%s\n", arg->submgr_cmd);
	if (pthread_create(&submgr_thread, NULL, submgr_loop, arg))
		return -1;

	while (timout_cnt--) {
		if (submgr_flag == 0)
			sleep(1);
		else
			break;
	}

	if (submgr_flag == 0)
		RS_DBG("UNABLE TO Create Subscpription in %d seconds\n", SUBMGR_TIMEOUT_CNT);

	pthread_join(submgr_thread, NULL);
	submgr_flag = 0;
	RS_DBG("%s return value %d\n", __func__, ret);
#endif
	return ret;
}
