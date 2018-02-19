#ifndef _RS_ONVIF_MSG_H
#define _RS_ONVIF_MSG_H

#include "rsOnvifDevCtrl.h"

int rsOnvifMsgOperateISP(control_message_operate_isp_t *arg);

int rsOnvifMsgImageSetting(control_message_image_setting_t *arg);

int rsOnvifMsgSetManualDNS(control_message_set_manual_dns_t *arg);

int rsOnvifMsgSetSearchDomain(control_message_set_search_domain_t *arg);

int rsOnvifMsgSetDHCPDNS(void);

int rsOnvifMsgSetManualNTP(control_message_set_manual_ntp_t *arg);

int rsOnvifMsgSetDHCPNTP(void);

int rsOnvifMsgSetTimeZone(control_message_set_manual_timezone_t *arg);

int rsOnvifMsgSetDST(int value);

int rsOnvifMsgSyncNTP();

int rsOnvifMsgSetDateTime(control_message_set_date_time_t *arg);

int rsOnvifMsgRebootDevice();

int rsOnvifMsgSendHello();

int rsOnvifMsgSetHostname(control_message_set_host_name_t *arg);

int rsOnvifMsgSetNetworkInterface(control_message_set_network_interface_t *arg);

int rsOnvifMsgSetNetworkProtocols(control_message_set_network_protocols_t *arg);

int rsOnvifMsgSetDefaultGateway(control_message_set_default_gateway_t *arg);

int rsOnvifMsgSetSystemFactoryDefault(control_message_set_system_factory_default_t *arg);

int rsOnvifMsgCreateSubMgr(control_message_create_submgr_t *arg);

int rsOnvifMsgGenerateIntraFrame();

#endif
