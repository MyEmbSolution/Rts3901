#include "rsOnvifMsg.h"
#include "rsOnvifTypes.h"
#include "rsOnvifDefines.h"
#include <fcntl.h>   /* For O_* constants */
#include <sys/stat.h>/* For mode constants */
#include <mqueue.h>
#include <stdlib.h>
#define MESSAGE_PRIORITY 2

int rsOnvifMsgImageSetting(control_message_image_setting_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_image_setting_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_IMAGE_SETTING;
	memcpy(msg->data, arg, sizeof(control_message_image_setting_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;


}

int rsOnvifMsgOperateISP(control_message_operate_isp_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_operate_isp_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_OPERATE_ISP;
	memcpy(msg->data, arg, sizeof(control_message_operate_isp_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetManualDNS(control_message_set_manual_dns_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_manual_dns_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_MANUAL_DNS;
	memcpy(msg->data, arg, sizeof(control_message_set_manual_dns_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetSearchDomain(control_message_set_search_domain_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_search_domain_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_SEARCH_DOMAIN;
	memcpy(msg->data, arg, sizeof(control_message_set_search_domain_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetDHCPDNS(void)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_DHCP_DNS;

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetManualNTP(control_message_set_manual_ntp_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_manual_ntp_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_MANUAL_NTP;
	memcpy(msg->data, arg, sizeof(control_message_set_manual_ntp_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetDHCPNTP(void)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_DHCP_NTP;

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);

	return 0;
}

int rsOnvifMsgSetTimeZone(control_message_set_manual_timezone_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_TIME_ZONE;
	memcpy(msg->data, arg, sizeof(control_message_set_manual_timezone_t));
	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetDST(int value)
{
	int ret = 0;
	mqd_t msgq_id;
	int *pvalue;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_DST;
	pvalue = (int *)msg->data;
	*pvalue = value;
	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSyncNTP()
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SYNC_NTP;
	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;

}

int rsOnvifMsgSetDateTime(control_message_set_date_time_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_date_time_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_DATE_TIME;
	memcpy(msg->data, arg, sizeof(control_message_set_date_time_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}


int rsOnvifMsgRebootDevice()
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_REBOOT_DEVICE;
	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}


int rsOnvifMsgSendHello()
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SEND_HELLO;
	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}


int rsOnvifMsgSetHostname(control_message_set_host_name_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_host_name_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_HOST_NAME;
	memcpy(msg->data, arg, sizeof(control_message_set_host_name_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetNetworkInterface(control_message_set_network_interface_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_network_interface_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_NETWORK_INTERFACE;
	memcpy(msg->data, arg, sizeof(control_message_set_network_interface_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;

}

int rsOnvifMsgSetNetworkProtocols(control_message_set_network_protocols_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_network_protocols_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_NETWORK_PROTOCOLS;
	memcpy(msg->data, arg, sizeof(control_message_set_network_protocols_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}

int rsOnvifMsgSetDefaultGateway(control_message_set_default_gateway_t *arg)
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_default_gateway_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_DEFAULT_GATEWAY;
	memcpy(msg->data, arg, sizeof(control_message_set_default_gateway_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}


int rsOnvifMsgSetSystemFactoryDefault(control_message_set_system_factory_default_t *arg)
{
	int ret = 0;

	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_set_system_factory_default_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_SET_SYSTEM_FACTORY_DEFAULT;
	memcpy(msg->data, arg, sizeof(control_message_set_system_factory_default_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);

	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);

	return 0;
}

int rsOnvifMsgCreateSubMgr(control_message_create_submgr_t *arg)
{
	int ret = 0;

	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	ASSERT(sizeof(control_message_create_submgr_t) <= (sizeof(control_message_t) - 4));
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_CREATE_SUBMGR;
	memcpy(msg->data, arg, sizeof(control_message_create_submgr_t));

	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);

	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);

	return 0;
}

int rsOnvifMsgGenerateIntraFrame()
{
	int ret = 0;
	mqd_t msgq_id;
	control_message_t *msg;
	msgq_id = mq_open(ONVIF_CONTROL_MQ_NAME, O_RDWR);
	ASSERT((mqd_t)-1 != msgq_id);
	msg = (control_message_t *)malloc(sizeof(control_message_t));
	ASSERT(msg);

	msg->type = ONVIF_CTRL_MSG_GENERATE_INTRA_FRAME;
	ret = mq_send(msgq_id, (char *)msg, sizeof(control_message_t), MESSAGE_PRIORITY);
	ASSERT(ret == 0);
	mq_close(msgq_id);
	free(msg);
	return 0;
}
