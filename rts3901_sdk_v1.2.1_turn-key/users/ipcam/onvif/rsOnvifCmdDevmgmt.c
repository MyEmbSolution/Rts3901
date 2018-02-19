#include <string.h>
#include <auth/auth.h>

#include "soapH.h"
#include "rsOnvifSystemCtrl.h"
#include "rsOnvifCommonFunc.h"

#define SMALL_INFO_LENGTH 20
#define LARGE_INFO_LENGTH 1024
#define MID_INFO_LENGTH 40

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

/* enum xsd__boolean {xsd__boolean__false_, xsd__boolean__true_}; */
static enum xsd__boolean gXsdTrue  = xsd__boolean__true_;
static enum xsd__boolean gXsdFalse = xsd__boolean__false_;
#define DEVICE_SERVICE_CAPABILITIES "<tds:Capabilities>"\
	"<tds:Network IPFilter=\"%s\" ZeroConfiguration=\"%s\" IPVersion6=\"%s\" DynDNS=\"%s\" Dot11Configuration=\"%s\" HostnameFromDHCP=\"%s\" NTP=\"%d\"></tds:Network>"\
	"<tds:Security TLS1.0=\"%s\" TLS1.1=\"%s\" TLS1.2=\"%s\" OnboardKeyGeneration=\"%s\" AccessPolicyConfig=\"%s\" DefaultAccessPolicy=\"%s\" Dot1X=\"%s\" RemoteUserHandling=\"%s\" X.509Token=\"%s\" SAMLToken=\"%s\" KerberosToken=\"%s\" UsernameToken=\"%s\" HttpDigest=\"%s\" RELToken=\"%s\" SupportedEAPMethods=\"%s\"></tds:Security>"\
	"<tds:System DiscoveryResolve=\"%s\" DiscoveryBye=\"%s\" RemoteDiscovery=\"%s\" SystemBackup=\"%s\" SystemLogging=\"%s\" FirmwareUpgrade=\"%s\" HttpFirmwareUpgrade=\"%s\" HttpSystemBackup=\"%s\" HttpSystemLogging=\"%s\" HttpSupportInformation=\"%s\"></tds:System>"\
	"<tds:Misc AuxiliaryCommands=\"auxiliary_commands\"></tds:Misc>"\
	"</tds:Capabilities>"

#define MEDIA_SERVICE_CAPABILITIES "<trt:Capabilities><trt:ProfileCapabilities MaximumNumberOfProfiles=\"%d\" /><trt:StreamingCapabilities RTPMulticast=\"%s\" RTP_TCP=\"%s\" RTP_RTSP_TCP=\"%s\" NonAggregateControl=\"%s\" /></trt:Capabilities>"
#define DEVICEIO_SERVICE_CAPABILITIES "<tmd:Capabilities></tmd:Capabilities>"
#define PTZ_SERVICE_CAPABILITIES "<tptz:Capabilities></tptz:Capabilities>"
#define IMAGING_SERVICE_CAPABILITIES "<timg:Capabilities></timg:Capabilities>"
#define ANALYTICSDEVICE_SERVICE_CAPABILITIES "<tad:Capabilities></tad:Capabilities>"
/* #define EVENTS_SERVICE_CAPABILITIES "<tev:Capabilities WSSubscriptionPolicySupport=\"%s\" WSPullPointSupport=\"%s\" WSPausableSubscriptionManagerInterfaceSupport=\"%s\" RateLimitPolicySupport=\"%s\" MaxNotificationProducers=\"%d\" MaxPullPoints=\"%d\"></tev:Capabilities>" */

#define EVENTS_SERVICE_CAPABILITIES "<tev:Capabilities WSSubscriptionPolicySupport=\"%s\" WSPullPointSupport=\"%s\" WSPausableSubscriptionManagerInterfaceSupport=\"%s\" PersistentNotificationStorage=\"%s\" MaxNotificationProducers=\"%d\" MaxPullPoints=\"%d\"></tev:Capabilities>"
/*
int __size;	// sequence of elements <-any>
	char *__any;
	enum xsd__boolean *WSSubscriptionPolicySupport;	// optional attribute of type xsd:boolean
	enum xsd__boolean *WSPullPointSupport;	// optional attribute of type xsd:boolean
	enum xsd__boolean *WSPausableSubscriptionManagerInterfaceSupport;	// optional attribute of type xsd:boolean
	int *MaxNotificationProducers;	//optional attribute of type xsd:int
	int *MaxPullPoints;	// optional attribute of type xsd:int
	enum xsd__boolean *PersistentNotificationStorage;	// optional attribute of type xsd:boolean
	char *__anyAttribute;	// optional attribute of type xsd:anyType
*/


SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServices(struct soap *soap,
	struct _tds__GetServices *tds__GetServices,
	struct _tds__GetServicesResponse *tds__GetServicesResponse)
{

	int i = 0, ret;
	char *xaddr[ONVIF_SERVICE_NUM];
	network_config_t *network;
	onvif_special_info_t *info;
	char *service_capabilities[ONVIF_SERVICE_NUM];

	char *device_service_capabilities;
	char *media_service_capabilities;
	char *deviceio_service_capabilities = DEVICEIO_SERVICE_CAPABILITIES;
	char *ptz_service_capabilities = PTZ_SERVICE_CAPABILITIES;
	char *imaging_service_capabilities = IMAGING_SERVICE_CAPABILITIES;
	char *analyticsdevice_service_capabilities = ANALYTICSDEVICE_SERVICE_CAPABILITIES;
	char *events_service_capabilities;


	if (soap->header)
		soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);
	info = (onvif_special_info_t *)soap_malloc(soap, sizeof(onvif_special_info_t));
	ASSERT(info);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto fault;

	device_service_capabilities = (char *)soap_malloc(soap, 1024);
	memset(device_service_capabilities, 0, 1024);
	sprintf(device_service_capabilities, DEVICE_SERVICE_CAPABILITIES,
		info->device_service_capabilities.network.ip_filter ? "true" : "false",
		info->device_service_capabilities.network.zero_configuration ? "true" : "false",
		info->device_service_capabilities.network.ip_version6 ? "true" : "false",
		info->device_service_capabilities.network.dynamic_dns ? "true" : "false",
		info->device_service_capabilities.network.dot_11_configuration ? "true" : "false",
		info->device_service_capabilities.network.hostname_from_dhcp ? "true" : "false",
		info->device_service_capabilities.network.ntp_server_number,

		info->device_service_capabilities.security.tls1_x002e0 ? "true" : "false",
		info->device_service_capabilities.security.tls1_x002e1 ? "true" : "false",
		info->device_service_capabilities.security.tls1_x002e2 ? "true" : "false",
		info->device_service_capabilities.security.onboard_key_generation ? "true" : "false",
		info->device_service_capabilities.security.access_policy_config ? "true" : "false",
		info->device_service_capabilities.security.default_access_policy ? "true" : "false",
		info->device_service_capabilities.security.dot1x ? "true" : "false",
		info->device_service_capabilities.security.remote_user_handling ? "true" : "false",
		info->device_service_capabilities.security.x_x002e509_token ? "true" : "false",
		info->device_service_capabilities.security.saml_token ? "true" : "false",
		info->device_service_capabilities.security.kerberos_token ? "true" : "false",
		info->device_service_capabilities.security.username_token ? "true" : "false",
		info->device_service_capabilities.security.http_digest ? "true" : "false",
		info->device_service_capabilities.security.rel_token ? "true" : "false",
		info->device_service_capabilities.security.supported_eap_methods,

		info->device_service_capabilities.system.discovery_resolve ? "true" : "false",
		info->device_service_capabilities.system.discovery_bye ? "true" : "false",
		info->device_service_capabilities.system.remote_discovery ? "true" : "false",
		info->device_service_capabilities.system.system_backup ? "true" : "false",
		info->device_service_capabilities.system.system_logging ? "true" : "false",
		info->device_service_capabilities.system.firmware_upgrade ? "true" : "false",
		info->device_service_capabilities.system.http_firmware_upgrade ? "true" : "false",
		info->device_service_capabilities.system.http_system_backup ? "true" : "false",
		info->device_service_capabilities.system.http_system_logging ? "true" : "false",
		info->device_service_capabilities.system.http_support_information ? "true" : "false");

	events_service_capabilities = (char *)soap_malloc(soap, 1024);
	memset(events_service_capabilities, 0, 1024);
	sprintf(events_service_capabilities, EVENTS_SERVICE_CAPABILITIES,
		info->event_service_capabilities.ws_subscription_policy_support ? "true" : "false",
		info->event_service_capabilities.ws_pull_point_support ? "true" : "false",
		info->event_service_capabilities.ws_pausable_submgr_support ? "true" : "false",
		info->event_service_capabilities.persistentNotificationStorage ? "true" : "false",
		info->event_service_capabilities.max_notificationproducers,
		info->event_service_capabilities.max_pull_points);

	media_service_capabilities = (char *)soap_malloc(soap, 1024);
	memset(media_service_capabilities, 0, 1024);
	sprintf(media_service_capabilities, MEDIA_SERVICE_CAPABILITIES,
			MAX_MEDIA_PROFILE,
			"false",
			"false",
			"true",
			"true");


	service_capabilities[SERVICE_INDEX_DEVICE] = device_service_capabilities;
	service_capabilities[SERVICE_INDEX_MEDIA] = media_service_capabilities;
	service_capabilities[SERVICE_INDEX_DEVICEIO] = deviceio_service_capabilities;
	service_capabilities[SERVICE_INDEX_PTZ] = ptz_service_capabilities;
	service_capabilities[SERVICE_INDEX_IMAGING] = imaging_service_capabilities;
	service_capabilities[SERVICE_INDEX_ANALYTICSDEVICE] = analyticsdevice_service_capabilities;
	service_capabilities[SERVICE_INDEX_EVENTS] = events_service_capabilities;

	for (i = 0; i < ONVIF_SERVICE_NUM; i++) {

		xaddr[i] = soap_malloc(soap, XADDR_BUF_SIZE);
		ASSERT(xaddr[i]);
		sprintf(xaddr[i],
			"http://%s/onvif/%s",
			inet_ntoa(network->interfaces[network->interface_idx].ip),
			info->service_name[i]);
		RS_DBG("in func 2 ns = %s \n", info->service_name[i]);
	}

#if 0
	tds__GetServicesResponse->__sizeService = ONVIF_SERVICE_NUM;
	tds__GetServicesResponse->Service = (struct tds__Service *)soap_malloc(soap, sizeof(struct tds__Service)*ONVIF_SERVICE_NUM);
	ASSERT(tds__GetServicesResponse->Service);
	memset(tds__GetServicesResponse->Service, 0, sizeof(struct tds__Service)*ONVIF_SERVICE_NUM);
	for (i = 0; i < ONVIF_SERVICE_NUM; i++) {
		tds__GetServicesResponse->Service[i].XAddr = (char *)soap_malloc(soap, sizeof(char) * XADDR_BUF_SIZE);
		ASSERT(tds__GetServicesResponse->Service[i].XAddr);
		tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char) * ONVIF_SERVICE_NS_LENGTH);
		ASSERT(tds__GetServicesResponse->Service[i].Namespace);
		strcpy(tds__GetServicesResponse->Service[i].Namespace, info->service_ns[i]);
		strcpy(tds__GetServicesResponse->Service[i].XAddr, xaddr[i]);
		if (!strncmp(tds__GetServices->IncludeCapability, "true", 4)) {
			tds__GetServicesResponse->Service[i].Capabilities = (struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities *));
			tds__GetServicesResponse->Service[i].Capabilities->__any = (char *)soap_malloc(soap, sizeof(char) * 1024);
			strcpy(tds__GetServicesResponse->Service[i].Capabilities->__any, service_capabilities[i]);
		}
		tds__GetServicesResponse->Service[i].Version = (struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
		ASSERT(tds__GetServicesResponse->Service[0].Version);
		tds__GetServicesResponse->Service[i].Version->Major = (info->service_version[SERVICE_INDEX_DEVICE] & 0xff00)>>8;
		tds__GetServicesResponse->Service[i].Version->Minor = info->service_version[SERVICE_INDEX_DEVICE] & 0xff;
		tds__GetServicesResponse->Service[i].__any = NULL;
		tds__GetServicesResponse->Service[i].__anyAttribute = NULL;
	}

#endif
	tds__GetServicesResponse->__sizeService = 4; /* device, event, media, Imaging, ptz; */
	tds__GetServicesResponse->Service =
		(struct tds__Service *)soap_malloc(soap,
			sizeof(struct tds__Service)*tds__GetServicesResponse->__sizeService);
	ASSERT(tds__GetServicesResponse->Service);
	memset(tds__GetServicesResponse->Service, 0,
		sizeof(struct tds__Service)*tds__GetServicesResponse->__sizeService);
	/* device service; */
	tds__GetServicesResponse->Service[0].XAddr = (char *)soap_malloc(soap, sizeof(char) * XADDR_BUF_SIZE);
	ASSERT(tds__GetServicesResponse->Service[0].XAddr);
	tds__GetServicesResponse->Service[0].Namespace = (char *)soap_malloc(soap, sizeof(char) * ONVIF_SERVICE_NS_LENGTH);
	ASSERT(tds__GetServicesResponse->Service[0].Namespace);
	strcpy(tds__GetServicesResponse->Service[0].Namespace, info->service_ns[SERVICE_INDEX_DEVICE]);

	strcpy(tds__GetServicesResponse->Service[0].XAddr, xaddr[SERVICE_INDEX_DEVICE]);

	if (xsd__boolean__true_ == tds__GetServices->IncludeCapability)
	/*if (!strncmp((const char *)(tds__GetServices->IncludeCapability), "true", 4)) */ {

		tds__GetServicesResponse->Service[0].Capabilities =
			(struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities *));

		tds__GetServicesResponse->Service[0].Capabilities->__any =
			(char *)soap_malloc(soap, sizeof(char) * 1024);

		strcpy(tds__GetServicesResponse->Service[0].Capabilities->__any,
			service_capabilities[SERVICE_INDEX_DEVICE]);

	}
	tds__GetServicesResponse->Service[0].Version =
		(struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));

	ASSERT(tds__GetServicesResponse->Service[0].Version);
	tds__GetServicesResponse->Service[0].Version->Major = (info->service_version[SERVICE_INDEX_DEVICE] & 0xff00)>>8;
	tds__GetServicesResponse->Service[0].Version->Minor = info->service_version[SERVICE_INDEX_DEVICE] & 0xff;
	tds__GetServicesResponse->Service[0].__any = NULL;
	tds__GetServicesResponse->Service[0].__anyAttribute = NULL;

	/* events service; */
	tds__GetServicesResponse->Service[1].XAddr = (char *)soap_malloc(soap, sizeof(char) * XADDR_BUF_SIZE);
	ASSERT(tds__GetServicesResponse->Service[1].XAddr);
	tds__GetServicesResponse->Service[1].Namespace = (char *)soap_malloc(soap, sizeof(char) * ONVIF_SERVICE_NS_LENGTH);
	ASSERT(tds__GetServicesResponse->Service[1].Namespace);
	strcpy(tds__GetServicesResponse->Service[1].Namespace, info->service_ns[SERVICE_INDEX_EVENTS]);

	strcpy(tds__GetServicesResponse->Service[1].XAddr, xaddr[SERVICE_INDEX_EVENTS]);

	if (xsd__boolean__true_ == tds__GetServices->IncludeCapability)
		/*if (!strncmp((const char *)(tds__GetServices->IncludeCapability), "true", 4)) */ {
		tds__GetServicesResponse->Service[1].Capabilities =
			(struct _tds__Service_Capabilities *)soap_malloc(soap,
				sizeof(struct _tds__Service_Capabilities *));
		tds__GetServicesResponse->Service[1].Capabilities->__any = (char *)soap_malloc(soap, sizeof(char) * 1024);
		strcpy(tds__GetServicesResponse->Service[1].Capabilities->__any, service_capabilities[SERVICE_INDEX_EVENTS]);
	}
	tds__GetServicesResponse->Service[1].Version =
		(struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
	ASSERT(tds__GetServicesResponse->Service[1].Version);
	tds__GetServicesResponse->Service[1].Version->Major = (info->service_version[SERVICE_INDEX_EVENTS] & 0xff00)>>8;
	tds__GetServicesResponse->Service[1].Version->Minor = info->service_version[SERVICE_INDEX_EVENTS] & 0xff;
	tds__GetServicesResponse->Service[1].__any = NULL;
	tds__GetServicesResponse->Service[1].__anyAttribute = NULL;

	/* media service; */
	tds__GetServicesResponse->Service[2].XAddr = (char *)soap_malloc(soap, sizeof(char) * XADDR_BUF_SIZE);
	ASSERT(tds__GetServicesResponse->Service[2].XAddr);
	tds__GetServicesResponse->Service[2].Namespace = (char *)soap_malloc(soap, sizeof(char) * ONVIF_SERVICE_NS_LENGTH);
	ASSERT(tds__GetServicesResponse->Service[2].Namespace);
	strcpy(tds__GetServicesResponse->Service[2].Namespace, info->service_ns[SERVICE_INDEX_MEDIA]);

	strcpy(tds__GetServicesResponse->Service[2].XAddr, xaddr[SERVICE_INDEX_MEDIA]);

	if (xsd__boolean__true_ == tds__GetServices->IncludeCapability)
	/* if (!strncmp((const char *)(tds__GetServices->IncludeCapability), "true", 4)) */ {
		tds__GetServicesResponse->Service[2].Capabilities =
			(struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities *));
		tds__GetServicesResponse->Service[2].Capabilities->__any = (char *)soap_malloc(soap, sizeof(char) * 1024);
		strcpy(tds__GetServicesResponse->Service[2].Capabilities->__any, service_capabilities[SERVICE_INDEX_MEDIA]);
	}
	tds__GetServicesResponse->Service[2].Version =
		(struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
	ASSERT(tds__GetServicesResponse->Service[2].Version);
	tds__GetServicesResponse->Service[2].Version->Major = (info->service_version[SERVICE_INDEX_MEDIA] & 0xff00)>>8;
	tds__GetServicesResponse->Service[2].Version->Minor = info->service_version[SERVICE_INDEX_MEDIA] & 0xff;
	tds__GetServicesResponse->Service[2].__any = NULL;
	tds__GetServicesResponse->Service[2].__anyAttribute = NULL;



	/* imaging service; */
	tds__GetServicesResponse->Service[3].XAddr = (char *)soap_malloc(soap, sizeof(char) * XADDR_BUF_SIZE);
	ASSERT(tds__GetServicesResponse->Service[3].XAddr);
	tds__GetServicesResponse->Service[3].Namespace = (char *)soap_malloc(soap, sizeof(char) * ONVIF_SERVICE_NS_LENGTH);
	ASSERT(tds__GetServicesResponse->Service[3].Namespace);
	strcpy(tds__GetServicesResponse->Service[3].Namespace, info->service_ns[SERVICE_INDEX_IMAGING]);

	strcpy(tds__GetServicesResponse->Service[3].XAddr, xaddr[SERVICE_INDEX_IMAGING]);

	if (xsd__boolean__true_ == tds__GetServices->IncludeCapability)
	/* if (!strncmp((const char *)(tds__GetServices->IncludeCapability), "true", 4)) */ {
		tds__GetServicesResponse->Service[3].Capabilities =
			(struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities *));
		tds__GetServicesResponse->Service[3].Capabilities->__any = (char *)soap_malloc(soap, sizeof(char) * 1024);
		strcpy(tds__GetServicesResponse->Service[3].Capabilities->__any, service_capabilities[SERVICE_INDEX_IMAGING]);
	}
	tds__GetServicesResponse->Service[3].Version =
		(struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
	ASSERT(tds__GetServicesResponse->Service[3].Version);
	tds__GetServicesResponse->Service[3].Version->Major = (info->service_version[SERVICE_INDEX_IMAGING] & 0xff00)>>8;
	tds__GetServicesResponse->Service[3].Version->Minor = info->service_version[SERVICE_INDEX_IMAGING] & 0xff;
	tds__GetServicesResponse->Service[3].__any = NULL;
	tds__GetServicesResponse->Service[3].__anyAttribute = NULL;


	/* ptz service; */
#if 0
	tds__GetServicesResponse->Service[4].XAddr = (char *)soap_malloc(soap, sizeof(char) * XADDR_BUF_SIZE);
	ASSERT(tds__GetServicesResponse->Service[4].XAddr);
	tds__GetServicesResponse->Service[4].Namespace = (char *)soap_malloc(soap, sizeof(char) * ONVIF_SERVICE_NS_LENGTH);
	ASSERT(tds__GetServicesResponse->Service[4].Namespace);
	strcpy(tds__GetServicesResponse->Service[4].Namespace, info->service_ns[SERVICE_INDEX_PTZ]);

	strcpy(tds__GetServicesResponse->Service[4].XAddr, xaddr[SERVICE_INDEX_PTZ]);

	if (xsd__boolean__true_ == tds__GetServices->IncludeCapability)
	/* if (!strncmp((const char *)(tds__GetServices->IncludeCapability), "true", 4)) */ {
		tds__GetServicesResponse->Service[4].Capabilities =
			(struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities *));
		tds__GetServicesResponse->Service[4].Capabilities->__any = (char *)soap_malloc(soap, sizeof(char) * 1024);
		strcpy(tds__GetServicesResponse->Service[4].Capabilities->__any, service_capabilities[SERVICE_INDEX_PTZ]);
	}
	tds__GetServicesResponse->Service[4].Version =
		(struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
	ASSERT(tds__GetServicesResponse->Service[4].Version);
	tds__GetServicesResponse->Service[4].Version->Major = (info->service_version[SERVICE_INDEX_PTZ] & 0xff00)>>8;
	tds__GetServicesResponse->Service[4].Version->Minor = info->service_version[SERVICE_INDEX_PTZ] & 0xff;
	tds__GetServicesResponse->Service[4].__any = NULL;
	tds__GetServicesResponse->Service[4].__anyAttribute = NULL;
#endif


	return SOAP_OK;

fault:
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:ServiceNotSupported");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServiceCapabilities(struct soap *soap,
	struct _tds__GetServiceCapabilities *tds__GetServiceCapabilities,
	struct _tds__GetServiceCapabilitiesResponse *tds__GetServiceCapabilitiesResponse)
{
	int ret;
	onvif_special_info_t *info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);

	if (soap->header)
		soap->header->wsse__Security = NULL;
	ret = rsOnvifSysInfoGetSpecialInfo(info);

	if (ret)
		goto fault;

	tds__GetServiceCapabilitiesResponse->Capabilities =
		(struct tds__DeviceServiceCapabilities *)soap_malloc(soap,
			sizeof(struct tds__DeviceServiceCapabilities));
	ASSERT(tds__GetServiceCapabilitiesResponse->Capabilities);
	memset(tds__GetServiceCapabilitiesResponse->Capabilities, 0,
			sizeof(struct tds__DeviceServiceCapabilities));

	/* network; */
	tds__GetServiceCapabilitiesResponse->Capabilities->Network =
		(struct tds__NetworkCapabilities *)soap_malloc(soap,
			sizeof(struct tds__NetworkCapabilities));
	ASSERT(tds__GetServiceCapabilitiesResponse->Capabilities->Network);
	memset(tds__GetServiceCapabilitiesResponse->Capabilities->Network, 0,
			sizeof(struct tds__NetworkCapabilities));
	/* tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPFilter = info->device_service_capabilities.network.ip_filter ? "true":"false"; */

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPFilter =
		info->device_service_capabilities.network.ip_filter ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->ZeroConfiguration =
		info->device_service_capabilities.network.zero_configuration ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPVersion6 =
		info->device_service_capabilities.network.ip_version6 ? &gXsdTrue : &gXsdFalse;


	tds__GetServiceCapabilitiesResponse->Capabilities->Network->DynDNS =
		info->device_service_capabilities.network.dynamic_dns ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->Dot11Configuration =
		info->device_service_capabilities.network.dot_11_configuration ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->Dot1XConfigurations =
		(int *)soap_malloc(soap, sizeof(int));
	*tds__GetServiceCapabilitiesResponse->Capabilities->Network->Dot1XConfigurations =
		info->device_service_capabilities.network.dot1x_configurations;

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->HostnameFromDHCP =
		info->device_service_capabilities.network.hostname_from_dhcp ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->NTP = (int *)soap_malloc(soap, sizeof(int));
	*tds__GetServiceCapabilitiesResponse->Capabilities->Network->NTP =
		info->device_service_capabilities.network.ntp_server_number;

	tds__GetServiceCapabilitiesResponse->Capabilities->Network->DHCPv6 =
		info->device_service_capabilities.network.dhcp_v6 ? &gXsdTrue : &gXsdFalse;
	tds__GetServiceCapabilitiesResponse->Capabilities->Network->__anyAttribute =
		(char *)soap_malloc(soap, OPTIONAL_ATTRIBUTE_SIZE);
	memcpy(tds__GetServiceCapabilitiesResponse->Capabilities->Network->__anyAttribute,
		info->device_service_capabilities.network.any_attribute,
		OPTIONAL_ATTRIBUTE_SIZE);

	/* system; */
	tds__GetServiceCapabilitiesResponse->Capabilities->System =
		(struct tds__SystemCapabilities *)soap_malloc(soap,
				sizeof(struct tds__SystemCapabilities));
	ASSERT(tds__GetServiceCapabilitiesResponse->Capabilities->System);
	memset(tds__GetServiceCapabilitiesResponse->Capabilities->System, 0,
			sizeof(struct tds__SystemCapabilities));

	tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryResolve =
		info->device_service_capabilities.system.discovery_resolve ? &gXsdTrue : &gXsdFalse;
	tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryBye =
		info->device_service_capabilities.system.discovery_bye ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->RemoteDiscovery =
		info->device_service_capabilities.system.remote_discovery ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->SystemBackup =
		info->device_service_capabilities.system.system_backup ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->SystemLogging =
		info->device_service_capabilities.system.system_logging ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->FirmwareUpgrade =
		info->device_service_capabilities.system.firmware_upgrade ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->HttpFirmwareUpgrade =
		info->device_service_capabilities.system.http_firmware_upgrade ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->HttpSystemBackup =
		info->device_service_capabilities.system.http_system_backup ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->HttpSystemLogging =
		info->device_service_capabilities.system.http_system_logging ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->HttpSupportInformation =
		info->device_service_capabilities.system.http_support_information ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->System->__anyAttribute =
		(char *)soap_malloc(soap, OPTIONAL_ATTRIBUTE_SIZE);
	memcpy(tds__GetServiceCapabilitiesResponse->Capabilities->System->__anyAttribute,
		info->device_service_capabilities.system.any_attribute,
		OPTIONAL_ATTRIBUTE_SIZE);

	/* security */
	tds__GetServiceCapabilitiesResponse->Capabilities->Security =
		(struct tds__SecurityCapabilities *)soap_malloc(soap,
				sizeof(struct tds__SecurityCapabilities));
	ASSERT(tds__GetServiceCapabilitiesResponse->Capabilities->Security);
	memset(tds__GetServiceCapabilitiesResponse->Capabilities->Security,
		0, sizeof(struct tds__SecurityCapabilities));

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e0 =
		info->device_service_capabilities.security.tls1_x002e0 ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e1 =
		info->device_service_capabilities.security.tls1_x002e1 ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e2 =
		info->device_service_capabilities.security.tls1_x002e2 ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->OnboardKeyGeneration =
		info->device_service_capabilities.security.onboard_key_generation ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->AccessPolicyConfig =
		info->device_service_capabilities.security.access_policy_config ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->DefaultAccessPolicy =
		info->device_service_capabilities.security.default_access_policy ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->Dot1X =
		info->device_service_capabilities.security.dot1x ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->RemoteUserHandling =
		info->device_service_capabilities.security.remote_user_handling ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->X_x002e509Token =
		info->device_service_capabilities.security.x_x002e509_token ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->SAMLToken =
		info->device_service_capabilities.security.saml_token ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->KerberosToken =
		info->device_service_capabilities.security.kerberos_token ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken =
		info->device_service_capabilities.security.username_token ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest =
		info->device_service_capabilities.security.http_digest ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->RELToken =
		info->device_service_capabilities.security.rel_token ? &gXsdTrue : &gXsdFalse;

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->SupportedEAPMethods =
		(char *)soap_malloc(soap, EAP_METHOD_LENGTH);
	memcpy(tds__GetServiceCapabilitiesResponse->Capabilities->Security->SupportedEAPMethods,
		info->device_service_capabilities.security.supported_eap_methods, EAP_METHOD_LENGTH);

	tds__GetServiceCapabilitiesResponse->Capabilities->Security->__anyAttribute =
		(char *)soap_malloc(soap, OPTIONAL_ATTRIBUTE_SIZE);
	memcpy(tds__GetServiceCapabilitiesResponse->Capabilities->Security->__anyAttribute,
		info->device_service_capabilities.security.any_attribute, OPTIONAL_ATTRIBUTE_SIZE);

	tds__GetServiceCapabilitiesResponse->Capabilities->Misc =
		(struct tds__MiscCapabilities *)soap_malloc(soap,
				sizeof(struct tds__MiscCapabilities));
	ASSERT(tds__GetServiceCapabilitiesResponse->Capabilities->Misc);
	tds__GetServiceCapabilitiesResponse->Capabilities->Misc->AuxiliaryCommands =
		(char *)soap_malloc(soap, OPTIONAL_ATTRIBUTE_SIZE);
	memcpy(tds__GetServiceCapabilitiesResponse->Capabilities->Misc->AuxiliaryCommands,
		info->device_service_capabilities.misc.auxiliary_commands, OPTIONAL_ATTRIBUTE_SIZE);
	tds__GetServiceCapabilitiesResponse->Capabilities->Misc->__anyAttribute =
		(char *)soap_malloc(soap, OPTIONAL_ATTRIBUTE_SIZE);
	memcpy(tds__GetServiceCapabilitiesResponse->Capabilities->Misc->__anyAttribute,
		info->device_service_capabilities.misc.any_attribute, OPTIONAL_ATTRIBUTE_SIZE);

	return SOAP_OK;

fault:
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:ServiceNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDeviceInformation(struct soap *soap,
	struct _tds__GetDeviceInformation *tds__GetDeviceInformation,
	struct _tds__GetDeviceInformationResponse *tds__GetDeviceInformationResponse)
{
	int ret;
	device_information_t *info = (device_information_t *)soap_malloc(soap,
			sizeof(device_information_t));

	ASSERT(info);
	RS_DBG("device info 1\n");

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;

		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}

	RS_DBG("device info 2\n");

	soap->header->wsse__Security = NULL;
	ret = rsOnvifSysInfoGetDevInfo(info);
	if (ret)
		goto fault;

	tds__GetDeviceInformationResponse->Manufacturer =
		(char *)soap_malloc(soap, MID_INFO_SIZE);
	memcpy(tds__GetDeviceInformationResponse->Manufacturer,
			info->manufacturer, MID_INFO_SIZE);

	tds__GetDeviceInformationResponse->Model =
		(char *)soap_malloc(soap, MID_INFO_SIZE);
	memcpy(tds__GetDeviceInformationResponse->Model,
			info->model, MID_INFO_SIZE);

	tds__GetDeviceInformationResponse->FirmwareVersion =
		(char *)soap_malloc(soap, MID_INFO_SIZE);
	memcpy(tds__GetDeviceInformationResponse->FirmwareVersion,
			info->firmware_version, MID_INFO_SIZE);

	tds__GetDeviceInformationResponse->SerialNumber =
		(char *)soap_malloc(soap, MID_INFO_SIZE);
	memcpy(tds__GetDeviceInformationResponse->SerialNumber,
			info->serial_number, MID_INFO_SIZE);

	tds__GetDeviceInformationResponse->HardwareId =
		(char *)soap_malloc(soap, MID_INFO_SIZE);
	memcpy(tds__GetDeviceInformationResponse->HardwareId,
			info->hardware_id, MID_INFO_SIZE);
	RS_DBG("device info 3\n");

	return SOAP_OK;
fault:
	{
		char *error = (char *)soap_malloc(soap, 100);
		sprintf(error, "ter: error reason %d", ret);
		onvif_fault(soap, RECEIVER, error, error);
	}
	RS_DBG("device info 4\n");

	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemDateAndTime(struct soap *soap,
	struct _tds__SetSystemDateAndTime *tds__SetSystemDateAndTime,
	struct _tds__SetSystemDateAndTimeResponse *tds__SetSystemDateAndTimeResponse)
{
	int ret;
	date_time_settings_t date_time;
	int err = -1;
	ret = rsOnvifSysInfoGetDateTimeSettings(&date_time);
	if (ret)
		goto fault;
	/* RS_DBG("start!\n"); */
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	/* Time Zone */
	if (tds__SetSystemDateAndTime->TimeZone) {
		control_message_set_manual_timezone_t *arg =
			(control_message_set_manual_timezone_t *)soap_malloc(soap,
				sizeof(control_message_set_manual_timezone_t));
		RS_DBG("TZ %s\n", tds__SetSystemDateAndTime->TimeZone->TZ);
		if (check_time_zone(tds__SetSystemDateAndTime->TimeZone->TZ) == 100) {
			err = 1;
			goto fault;
		}
		strncpy(arg->tz, tds__SetSystemDateAndTime->TimeZone->TZ,
				MAX_TZ_LENGTH);
		rsOnvifMsgSetTimeZone(arg);
	}

	/* DayLight */
	/* if (!strncmp((const char *)(tds__SetSystemDateAndTime->DaylightSavings), "true", 4)) */
	if (xsd__boolean__true_ == tds__SetSystemDateAndTime->DaylightSavings) {
		rsOnvifMsgSetDST(1);/* DayLight */
	} else if (xsd__boolean__false_ == tds__SetSystemDateAndTime->DaylightSavings) {
		rsOnvifMsgSetDST(0);/* DayLight */
	}

	/*NTP */
	if (tds__SetSystemDateAndTime->DateTimeType == 1) {
		RS_DBG("In func %s, DataTimeType = %d\n", __func__, 1);
		rsOnvifMsgSyncNTP();

		sleep(5); /*  wait for while for ntp sync */
		return SOAP_OK;
	}
	RS_DBG("In func %s, DataTimeType = %d\n", __func__, 0);
	/*UTCDateTime is meaningful only when DateTimeType is 0;*/
	if (tds__SetSystemDateAndTime->UTCDateTime) {
		int year, month, day;
		int hour, minute, second;
		control_message_set_date_time_t arg;
		year = tds__SetSystemDateAndTime->UTCDateTime->Date->Year;
		month = tds__SetSystemDateAndTime->UTCDateTime->Date->Month;
		day = tds__SetSystemDateAndTime->UTCDateTime->Date->Day;
		hour = tds__SetSystemDateAndTime->UTCDateTime->Time->Hour;
		minute = tds__SetSystemDateAndTime->UTCDateTime->Time->Minute;
		second = tds__SetSystemDateAndTime->UTCDateTime->Time->Second;

		if ((hour > 24) || (minute > 60) || (second > 60) ||
				(month > 12) || (day > 31)) {
			err = 2;
			goto fault;
		}
		arg.year = year;
		arg.month = month;
		arg.day = day;
		arg.hour = hour;
		arg.minute = minute;
		arg.second = second;

		rsOnvifMsgSetDateTime(&arg);
	}
	/* RS_DBG("end!\n"); */
	return SOAP_OK;

fault:
	if (err == 0 || err == 2) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:InvalidDateTime");
	} else if (err == 1) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:InvalidTimeZone");
	}
	/* RS_DBG("end fault!\n"); */

	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemDateAndTime(struct soap *soap,
	struct _tds__GetSystemDateAndTime *tds__GetSystemDateAndTime,
	struct _tds__GetSystemDateAndTimeResponse *tds__GetSystemDateAndTimeResponse)
{
	/* UTC - Coordinated Universal Time */
	date_time_t date;
	int ret;
	date_time_settings_t *date_time =
		(date_time_settings_t *)soap_malloc(soap,
				sizeof(date_time_settings_t));

	if (soap->header)
		soap->header->wsse__Security = NULL;

	ret = rsOnvifSysInfoGetDateTimeSettings(date_time);
	if (ret)
		goto fault;

	rsCommGetDateTime(&date);


	tds__GetSystemDateAndTimeResponse->SystemDateAndTime =
		(struct tt__SystemDateTime *)soap_malloc(soap,
				sizeof(struct tt__SystemDateTime));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DateTimeType =
		date_time->time_ntp;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DaylightSavings =
		date_time->day_light_savings ? xsd__boolean__true_ : xsd__boolean__false_ ;/* "true":"false"; */
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone =
		(struct tt__TimeZone *)soap_malloc(soap,
				sizeof(struct tt__TimeZone));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ =
		(char *)soap_malloc(soap, MAX_TZ_LENGTH);
	memcpy(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ,
		date_time->time_zone, MAX_TZ_LENGTH);

	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime =
		(struct tt__DateTime *)soap_malloc(soap,
				sizeof(struct tt__DateTime));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time =
		(struct tt__Time *)soap_malloc(soap, sizeof(struct tt__Time));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Hour = date.hour;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Minute = date.minute;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Second = date.second;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date =
		(struct tt__Date *)soap_malloc(soap,
				sizeof(struct tt__Date));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Year = date.year;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Month = date.month;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Day = date.day;

	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime =
		(struct tt__DateTime *)soap_malloc(soap,
				sizeof(struct tt__DateTime));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time =
		(struct tt__Time *)soap_malloc(soap,
				sizeof(struct tt__Time));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Hour = date.hour;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Minute = date.minute;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Second = date.second;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date =
		(struct tt__Date *)soap_malloc(soap,
				sizeof(struct tt__Date));
	ASSERT(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Year = date.year;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Month = date.month;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Day = date.day;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->Extension = NULL;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->__anyAttribute = NULL;
	return SOAP_OK;

fault:

	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:InvalidDateTime");
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemFactoryDefault(struct soap *soap,
	struct _tds__SetSystemFactoryDefault *tds__SetSystemFactoryDefault,
	struct _tds__SetSystemFactoryDefaultResponse *tds__SetSystemFactoryDefaultResponse)
{
	control_message_set_system_factory_default_t arg;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	arg.soft_default = tds__SetSystemFactoryDefault->FactoryDefault;
	rsOnvifMsgSetSystemFactoryDefault(&arg);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__UpgradeSystemFirmware(struct soap *soap,
	struct _tds__UpgradeSystemFirmware *tds__UpgradeSystemFirmware,
	struct _tds__UpgradeSystemFirmwareResponse *tds__UpgradeSystemFirmwareResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:InvalidFirmware");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SystemReboot(struct soap *soap,
	struct _tds__SystemReboot *tds__SystemReboot,
	struct _tds__SystemRebootResponse *tds__SystemRebootResponse)
{
	char *str  = (char *)soap_malloc(soap, 100);
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Operator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	/* RS_DBG("\n"); */
	rsOnvifMsgRebootDevice();
	/* RS_DBG("\n"); */

	tds__SystemRebootResponse->Message = "Rebooting";

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__RestoreSystem(struct soap *soap,
	struct _tds__RestoreSystem *tds__RestoreSystem,
	struct _tds__RestoreSystemResponse *tds__RestoreSystemResponse)
{
	int ret;
	onvif_special_info_t *info;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);

	if (ret)
		goto fault;

	if (info->device_service_capabilities.system.system_backup) {
		/* restore the system; */
		return SOAP_OK;

	}

fault:
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemBackup(struct soap *soap,
	struct _tds__GetSystemBackup *tds__GetSystemBackup,
	struct _tds__GetSystemBackupResponse *tds__GetSystemBackupResponse)
{
	int ret;
	onvif_special_info_t *info;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);

	if (ret)
		goto fault;

	if (info->device_service_capabilities.system.system_backup) {
		/* fill with the suitable value; to be implemented */
		return SOAP_OK;
	}
fault:

	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:BackupNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemLog(struct soap *soap,
	struct _tds__GetSystemLog *tds__GetSystemLog,
	struct _tds__GetSystemLogResponse *tds__GetSystemLogResponse)
{

	int ret;
	onvif_special_info_t *info;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);

	if (ret)
		goto fault;

	if (info->device_service_capabilities.system.system_logging) {
		char *temp = "http://tempuri.org/";
		char *str = "string.........@@@";
		tds__GetSystemLogResponse->SystemLog =
			(struct tt__SystemLog *)soap_malloc(soap,
					sizeof(struct tt__SystemLog));
		tds__GetSystemLogResponse->SystemLog->Binary =
			(struct tt__AttachmentData *)soap_malloc(soap,
					sizeof(struct tt__AttachmentData));
		tds__GetSystemLogResponse->SystemLog->Binary->xop__Include.__size = 0;
		tds__GetSystemLogResponse->SystemLog->Binary->xop__Include.id = NULL;
		tds__GetSystemLogResponse->SystemLog->Binary->xop__Include.type = NULL;
		tds__GetSystemLogResponse->SystemLog->Binary->xop__Include.options = NULL;
		tds__GetSystemLogResponse->SystemLog->Binary->xop__Include.__ptr = NULL;

		tds__GetSystemLogResponse->SystemLog->Binary->xmime__contentType =
			(char *) soap_malloc(soap,
					sizeof(char) * SMALL_INFO_LENGTH);
		/* System = 0, Access = 1 */
		if (tds__GetSystemLog->LogType == 0) {
			strcpy(tds__GetSystemLogResponse->SystemLog->Binary->xmime__contentType, "System");
		} else if (tds__GetSystemLog->LogType == 1) {
			strcpy(tds__GetSystemLogResponse->SystemLog->Binary->xmime__contentType, "Access");
		}
		tds__GetSystemLogResponse->SystemLog->String = str;

		return SOAP_OK;
	}



fault:
	if (tds__GetSystemLog->LogType == 0) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgs",
				"ter:SystemlogUnavailable");
	} else if (tds__GetSystemLog->LogType == 1) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgs",
				"ter:AccesslogUnavailable");
	}
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemSupportInformation(struct soap *soap,
	struct _tds__GetSystemSupportInformation *tds__GetSystemSupportInformation,
	struct _tds__GetSystemSupportInformationResponse *tds__GetSystemSupportInformationResponse)
{
	int ret;
	onvif_special_info_t *info;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);

	if (ret)
		goto fault;

	if (info->device_service_capabilities.system.system_backup) {
		/* mtom/xop */
		return SOAP_OK;
	}
fault:
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:SupportInformationUnavailable");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetScopes(struct soap *soap,
	struct _tds__GetScopes *tds__GetScopes,
	struct _tds__GetScopesResponse *tds__GetScopesResponse)
{
	int ret;
	onvif_special_info_t *info;

	int i = 0;
	int num_scope = 0;
	int num_type = 0;
	int num_loc = 0;
	int num_other = 0;
	int _size = 0;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto fault;

	while (strlen(info->scopes.type[num_type])) {
		num_type++;
		if (num_type == MAX_SCOPES_TYPE_NUM)
			break;
	}

	while (strlen(info->scopes.location[num_loc])) {
		num_loc++;
		if (num_loc == MAX_SCOPES_LOCATION_NUM)
			break;
	}

	while (strlen(info->scopes.others[num_other])) {
		num_other++;
		if (num_other == MAX_SCOPES_OTHER_NUM)
			break;
	}
	_size = (num_type + num_loc + num_other + 3);
	tds__GetScopesResponse->__sizeScopes = _size;
	tds__GetScopesResponse->Scopes =
		(struct tt__Scope *)soap_malloc(soap,
			(_size * sizeof(struct tt__Scope)));


	tds__GetScopesResponse->Scopes[num_scope].ScopeDef = 0; /*  Fixed = 0, Configurable = 1 */
	tds__GetScopesResponse->Scopes[num_scope].ScopeItem =
		(char *)soap_malloc(soap,
				(sizeof(char) * MAX_SCOPES_BUF_SIZE));
	if (!strcmp(info->scopes.profile, "ANY"))
		/*note: the following Profile's character "P" has to be capital--neil*/
		sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
				"onvif://www.onvif.org/Profile");
	else
		sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
				"onvif://www.onvif.org/Profile/%s",
				info->scopes.profile);
	num_scope++;

	for (i = 0; i < num_type; i++) {
		tds__GetScopesResponse->Scopes[num_scope].ScopeDef = 1; /*  Fixed = 0, Configurable = 1 */
		tds__GetScopesResponse->Scopes[num_scope].ScopeItem =
			(char *)soap_malloc(soap,
					(sizeof(char) * MAX_SCOPES_BUF_SIZE));
		if (!strcmp(info->scopes.type[i], "ANY"))
			sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
					"onvif://www.onvif.org/type");
		else
			sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
					"onvif://www.onvif.org/type/%s",
					info->scopes.type[i]);
		num_scope++;
	}

	tds__GetScopesResponse->Scopes[num_scope].ScopeDef = 0; /*  Fixed = 0, Configurable = 1 */
	tds__GetScopesResponse->Scopes[num_scope].ScopeItem =
		(char *)soap_malloc(soap,
				(sizeof(char) * MAX_SCOPES_BUF_SIZE));
	if (!strcmp(info->scopes.hardware, "ANY"))
		sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
				"onvif://www.onvif.org/hardware");
	else
		sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
				"onvif://www.onvif.org/hardware/%s",
				info->scopes.hardware);
	num_scope++;

	tds__GetScopesResponse->Scopes[num_scope].ScopeDef = 0; /*  Fixed = 0, Configurable = 1 */
	tds__GetScopesResponse->Scopes[num_scope].ScopeItem =
		(char *)soap_malloc(soap, (sizeof(char) * MAX_SCOPES_BUF_SIZE));
	if (!strcmp(info->scopes.name, "ANY"))
		sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
				"onvif://www.onvif.org/name");
	else
		sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
				"onvif://www.onvif.org/name/%s",
				info->scopes.name);
	num_scope++;

	for (i = 0; i < num_loc; i++) {
		tds__GetScopesResponse->Scopes[num_scope].ScopeDef = 1; /*  Fixed = 0, Configurable = 1 */
		tds__GetScopesResponse->Scopes[num_scope].ScopeItem =
			(char *)soap_malloc(soap,
					(sizeof(char) * MAX_SCOPES_BUF_SIZE));
		if (!strcmp(info->scopes.location[i], "ANY"))
			sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
					"onvif://www.onvif.org/location");
		else
			sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
					"onvif://www.onvif.org/location/%s",
					info->scopes.location[i]);
		num_scope++;
	}


	for (i = 0; i < num_other; i++) {
		tds__GetScopesResponse->Scopes[num_scope].ScopeDef = 1; /*  Fixed = 0, Configurable = 1 */
		tds__GetScopesResponse->Scopes[num_scope].ScopeItem =
			(char *)soap_malloc(soap,
					(sizeof(char) * MAX_SCOPES_BUF_SIZE));
		sprintf(tds__GetScopesResponse->Scopes[num_scope].ScopeItem,
				"onvif://www.onvif.org/%s",
				info->scopes.others[i]);
		num_scope++;
	}

	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:Action", "ter:EmptyScope");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetScopes(struct soap *soap,
	struct _tds__SetScopes *tds__SetScopes,
	struct _tds__SetScopesResponse *tds__SetScopesResponse)
{
	int ret;
	onvif_special_info_t *info;

	int i, type_cnt = 0, location_cnt = 0, other_cnt = 0;
	int err = 0;
	int size;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	info = (onvif_special_info_t *)soap_malloc(soap, sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto fault;

	/* clear type & location scopes for incoming settings; */
	memset(info->scopes.type, 0, MAX_SCOPES_TYPE_NUM * MAX_SCOPES_BUF_SIZE);
	memset(info->scopes.location, 0, MAX_SCOPES_LOCATION_NUM * MAX_SCOPES_BUF_SIZE);

	size = tds__SetScopes->__sizeScopes;
	for (i = 0; i < size; i++) {
		char *ptr;
		if (strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org", 21))/* if there is the scope not starting with... */ {
			goto fault;
		} else if (!strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org/type", 26)) {
			char *p = tds__SetScopes->Scopes[i];
			if (type_cnt >= MAX_SCOPES_TYPE_NUM)
				continue;
			p += 26; /* move to the end of "onvif://www.onvif.org/type" */
			if (*p == 0) {
				sprintf(info->scopes.type[type_cnt++], "ANY");
			} else if (*p == '/') {
				p++;
				if (*p)
					sprintf(info->scopes.type[type_cnt++], "%s", p); else
					goto fault;
			} else {
				goto fault;
			}
		} else if (!strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org/location", 30)) {
			char *p = tds__SetScopes->Scopes[i];
			p += 30;
			if (location_cnt >= MAX_SCOPES_LOCATION_NUM)
				continue;
			if (*p == 0) {
				sprintf(info->scopes.location[location_cnt++], "ANY");
			} else if (*p == '/') {
				p++;
				if (*p)
					sprintf(info->scopes.location[location_cnt++], "%s", p); else
					goto fault;
			} else {
				goto fault;
			}
		} else if (!strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org/profile", 29) ||
			!strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org/name", 26) ||
			!strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org/hardware", 30)) {
			goto fault;
		} else {
			char *p = tds__SetScopes->Scopes[i];
			p += 21;
			if (other_cnt >= MAX_SCOPES_OTHER_NUM)
				continue;
			if (*p == '/') {
				p++;
				if (*p)
					sprintf(info->scopes.others[other_cnt++], "%s", p); else
					goto fault;
			} else {
				goto fault;
			}

		}

	}
	ret = rsOnvifSysInfoSetSpecialInfo(info);
	if (ret)
		goto fault;

	rsOnvifMsgSendHello();

	return SOAP_OK;
fault:
	onvif_fault(soap,
			SENDER,
			"ter:OperationProhibited",
			"ter:ScopeOverwrite");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__AddScopes(struct soap *soap,
	struct _tds__AddScopes *tds__AddScopes,
	struct _tds__AddScopesResponse *tds__AddScopesResponse)
{
	int ret, err = 1;
	onvif_special_info_t *info;
	int size =  tds__AddScopes->__sizeScopeItem;
	int i;
	int num_type = 0;
	int num_location = 0;
	int num_other = 0;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto fault;

	while (strlen(info->scopes.type[num_type])) {
		num_type++;
		if (num_type == MAX_SCOPES_TYPE_NUM)
			break;
	}

	while (strlen(info->scopes.location[num_location])) {
		num_location++;
		if (num_location == MAX_SCOPES_LOCATION_NUM)
			break;
	}
	while (strlen(info->scopes.others[num_other])) {
		num_other++;
		if (num_other == MAX_SCOPES_OTHER_NUM)
			break;
	}


	for (i = 0; i < size; i++) {
		char *ptr;
		if (!strncmp(tds__AddScopes->ScopeItem[i], "onvif://www.onvif.org/profile", 29) ||
			!strncmp(tds__AddScopes->ScopeItem[i], "onvif://www.onvif.org/name", 26) ||
			!strncmp(tds__AddScopes->ScopeItem[i], "onvif://www.onvif.org/hardware", 30)) {
			err = 1;
			goto fault;
		} else if (!strncmp(tds__AddScopes->ScopeItem[i], "onvif://www.onvif.org/type", 26)) {
			char *p = tds__AddScopes->ScopeItem[i];
			if (num_type >= MAX_SCOPES_TYPE_NUM) {
				err = 2;
				goto fault;
			}
			p += 26; /* move to the end of "onvif://www.onvif.org/type" */
			if (*p == 0) {
				sprintf(info->scopes.type[num_type++], "ANY");
			} else if (*p == '/') {
				p++;
				if (*p)
					sprintf(info->scopes.type[num_type++], "%s", p); else {
					err = 1;
					goto fault;
				}
			} else {
				err = 1;
				goto fault;
			}
		} else if (!strncmp(tds__AddScopes->ScopeItem[i], "onvif://www.onvif.org/location", 30)) {
			char *p = tds__AddScopes->ScopeItem[i];
			if (num_type >= MAX_SCOPES_LOCATION_NUM) {
				err = 2;
				goto fault;
			}
			p += 30; /* move to the end of "onvif://www.onvif.org/location" */
			if (*p == 0) {
				sprintf(info->scopes.location[num_location++], "ANY");
			} else if (*p == '/') {
				p++;
				if (*p)
					sprintf(info->scopes.location[num_location++], "%s", p); else {
					err = 1;
					goto fault;
				}
			} else {
				err = 1;
				goto fault;
			}
		} else if (!strncmp(tds__AddScopes->ScopeItem[i], "onvif://www.onvif.org", 21)) {
			char *p = tds__AddScopes->ScopeItem[i];
			if (num_other >= MAX_SCOPES_OTHER_NUM) {
				err = 2;
				goto fault;
			}
			p += 21; /* move to the end of "onvif://www.onvif.org" */
			if (*p == '/') {
				p++;
				if (*p)
					sprintf(info->scopes.others[num_other++], "%s", p); else {
					err = 1;
					goto fault;
				}
			} else {
				err = 1;
				goto fault;
			}
		} else {
			err = 1;
			goto fault;
		}
	}
	ret = rsOnvifSysInfoSetSpecialInfo(info);

	if (ret)
		goto fault;

	rsOnvifMsgSendHello();

	return SOAP_OK;

fault:
	if (err == 1) {
		onvif_fault(soap,
				RECEIVER,
				"ter:OperationProhibited",
				"ter:InValArgs");
	} else if (err == 2) {
		onvif_fault(soap,
				RECEIVER,
				"ter:OperationProhibited",
				"ter:TooManyScopes");
	}

	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveScopes(struct soap *soap,
	struct _tds__RemoveScopes *tds__RemoveScopes,
	struct _tds__RemoveScopesResponse *tds__RemoveScopesResponse)
{
	int ret, err = 1;
	onvif_special_info_t *info;
	int i;
	int size = tds__RemoveScopes->__sizeScopeItem;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);

	if (ret)
		goto fault;

	for (i = 0; i < size; i++) {
		char *ptr;

		if (strstr(tds__RemoveScopes->ScopeItem[i],
					"onvif://www.onvif.org/profile/") ||
			strstr(tds__RemoveScopes->ScopeItem[i],
				"onvif://www.onvif.org/hardware/") ||
			strstr(tds__RemoveScopes->ScopeItem[i],
				"onvif://www.onvif.org/name/")) {
			err = 1;
			goto fault;

		} else if (strstr(tds__RemoveScopes->ScopeItem[i],
					"onvif://www.onvif.org/type/")) {

			ptr = strstr(tds__RemoveScopes->ScopeItem[i],
					"onvif://www.onvif.org/type/");
			int matched = 0;
			int idx = 0;
			while (strlen(info->scopes.type[i]) &&
					idx < MAX_SCOPES_TYPE_NUM) {

				if (matched == 0) {
					if (strcmp(info->scopes.type[idx],
							ptr + 27) == 0) {
						matched = 1;
						strcpy(info->scopes.type[idx],
							info->scopes.type[idx + 1]);
					}
				} else {
					strcpy(info->scopes.type[idx],
						info->scopes.type[idx + 1]);
				}
				idx++;
			}

			if (matched == 1) {
				memset(info->scopes.type[idx - 1], 0,
						MAX_SCOPES_BUF_SIZE);
			} else {
				err = 3;
				goto fault;
			}

		} else if (strstr(tds__RemoveScopes->ScopeItem[i],
					"onvif://www.onvif.org/location/")) {

			ptr = strstr(tds__RemoveScopes->ScopeItem[i],
					"onvif://www.onvif.org/location/");
			int matched = 0;
			int idx = 0;
			while (strlen(info->scopes.location[i]) &&
					idx < MAX_SCOPES_LOCATION_NUM) {

				if (matched == 0) {
					if (strcmp(info->scopes.location[idx],
							ptr + 31) == 0) {

						matched = 1;
						strcpy(info->scopes.location[idx],
							info->scopes.location[idx + 1]);
					}
				} else {
					strcpy(info->scopes.location[idx],
						info->scopes.location[idx + 1]);
				}
				idx++;
			}
			if (matched == 1) {
				memset(info->scopes.location[idx - 1], 0,
						MAX_SCOPES_BUF_SIZE);
			} else {
				err = 3;
				goto fault;
			}


		} else if (strstr(tds__RemoveScopes->ScopeItem[i],
					"onvif://www.onvif.org")) {

			ptr = strstr(tds__RemoveScopes->ScopeItem[i],
					"onvif://www.onvif.org");

			int matched = 0;
			int idx = 0;
			while (strlen(info->scopes.others[i]) &&
					idx < MAX_SCOPES_OTHER_NUM) {

				if (matched == 0) {

					if (strcmp(info->scopes.others[idx],
							ptr + 22) == 0) {

						matched = 1;
						strcpy(info->scopes.others[idx],
							info->scopes.others[idx + 1]);
					}
				} else {
					strcpy(info->scopes.others[idx],
						info->scopes.others[idx + 1]);
				}
				idx++;
			}
			if (matched == 1) {
				memset(info->scopes.others[idx - 1], 0,
						MAX_SCOPES_BUF_SIZE);
			} else {
				err = 3;
				goto fault;
			}


		} else {
			err = 3;
			goto fault;
		}
	}
	ret = rsOnvifSysInfoSetSpecialInfo(info);

	if (ret)
		goto fault;

	tds__RemoveScopesResponse->__sizeScopeItem = size;
	tds__RemoveScopesResponse->ScopeItem =
		(char **)soap_malloc(soap,
				sizeof(char *) * MAX_SCOPES_BUF_SIZE);
	for (i = 0; i < size; i++) {
		tds__RemoveScopesResponse->ScopeItem[i] =
			(char *)soap_malloc(soap,
					sizeof(char) * MAX_SCOPES_BUF_SIZE);
		strcpy(tds__RemoveScopesResponse->ScopeItem[i],
				tds__RemoveScopes->ScopeItem[i]);
	}

	rsOnvifMsgSendHello();

	return SOAP_OK;
fault:
	if (err == 1) {
		onvif_fault(soap, RECEIVER, "ter:OperationProhibited",
				"ter:InValArgs");
	} else if (err == 2) {
		onvif_fault(soap, RECEIVER, "ter:OperationProhibited",
				"ter:TooManyScopes");
	} else if (err == 3) {
		onvif_fault(soap, RECEIVER, "ter:InvalidArgs",
				"ter:ScopesNotExist");
	}
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDiscoveryMode(struct soap *soap,
	struct _tds__GetDiscoveryMode *tds__GetDiscoveryMode,
	struct _tds__GetDiscoveryModeResponse *tds__GetDiscoveryModeResponse)
{
	int ret;
	network_config_t *network;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	tds__GetDiscoveryModeResponse->DiscoveryMode =  network->discovery_mode;
	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:InvalidArgVal");
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDiscoveryMode(struct soap *soap,
	struct _tds__SetDiscoveryMode *tds__SetDiscoveryMode,
	struct _tds__SetDiscoveryModeResponse *tds__SetDiscoveryModeResponse)
{
	int ret;
	network_config_t *network;
	int  mode = tds__SetDiscoveryMode->DiscoveryMode;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;
	if (mode == 0 || mode == 1) {
		network->discovery_mode = mode;
		ret = rsOnvifSysInfoSetNetConfig(network);
		if (ret)
			goto fault;

	} else {
		goto fault;
	}
	/* Calling of sleep: */
	/* temporary way to avoid synchronization issue in test DISCOVERY-1-1-9, step 3 -6: */
	/* monitor task receives the probe message before the change of discovery_mode is detected */
	/*  */
	sleep(1);
	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:InvalidArgVal");
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteDiscoveryMode(struct soap *soap,
	struct _tds__GetRemoteDiscoveryMode *tds__GetRemoteDiscoveryMode,
	struct _tds__GetRemoteDiscoveryModeResponse *tds__GetRemoteDiscoveryModeResponse)
{
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
			"ter:GetRemoteDiscoveryModeNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteDiscoveryMode(struct soap *soap,
	struct _tds__SetRemoteDiscoveryMode *tds__SetRemoteDiscoveryMode,
	struct _tds__SetRemoteDiscoveryModeResponse *tds__SetRemoteDiscoveryModeResponse)
{
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
			"ter:SetRemoteDiscoveryModeNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDPAddresses(struct soap *soap,
	struct _tds__GetDPAddresses *tds__GetDPAddresses,
	struct _tds__GetDPAddressesResponse *tds__GetDPAddressesResponse)
{
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
			"ter:GetDiscoveryProxyNotSupported");
	return SOAP_FAULT;
}
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDPAddresses(struct soap *soap,
	struct _tds__SetDPAddresses *tds__SetDPAddresses,
	struct _tds__SetDPAddressesResponse *tds__SetDPAddressesResponse)
{
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
			"ter:SetDiscoveryProxyNotSupported");
	return SOAP_FAULT;
}
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetEndpointReference(struct soap *soap,
	struct _tds__GetEndpointReference *tds__GetEndpointReference,
	struct _tds__GetEndpointReferenceResponse *tds__GetEndpointReferenceResponse)
{
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
			"ter:GetEndPointRefNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteUser(struct soap *soap,
	struct _tds__GetRemoteUser *tds__GetRemoteUser,
	struct _tds__GetRemoteUserResponse *tds__GetRemoteUserResponse)
{
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
			"ter:RemoteUserNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteUser(struct soap *soap,
	struct _tds__SetRemoteUser *tds__SetRemoteUser,
	struct _tds__SetRemoteUserResponse *tds__SetRemoteUserResponse)
{
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
			"ter:RemoteUserNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetUsers(struct soap *soap,
	struct _tds__GetUsers *tds__GetUsers,
	struct _tds__GetUsersResponse *tds__GetUsersResponse)
{
	int ret;
	network_config_t *network;
	int idx = 0;
	int i;
	int user_num = 0;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	/*
	 * User Level :- {
	 * onv__UserLevel__Administrator = 0,
	 * onv__UserLevel__Operator = 1,
	 * onv__UserLevel__User = 2,
	 * onv__UserLevel__Anonymous = 3,
	 * onv__UserLevel__Extended = 4}
	 */

	while (user_num < MAX_ACCOUNT_NUM &&
			strlen(network->accounts[user_num].user))
		user_num++;

	tds__GetUsersResponse->__sizeUser = user_num;
	tds__GetUsersResponse->User =
		(struct tt__User *)soap_malloc(soap,
				(user_num * sizeof(struct tt__User)));
	for (i = 0; i < user_num; i++) {
		tds__GetUsersResponse->User[idx].Username =
			network->accounts[idx].user;
		tds__GetUsersResponse->User[idx].Password = NULL;
		tds__GetUsersResponse->User[idx].UserLevel =
			network->accounts[idx].authority;
		tds__GetUsersResponse->User[idx].Extension = NULL;
		idx++;
	}
	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER,
			"ter:InvalidArgVal", "ter:RemoteUserNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateUsers(struct soap *soap,
	struct _tds__CreateUsers *tds__CreateUsers,
	struct _tds__CreateUsersResponse *tds__CreateUsersResponse)
{

	int size = tds__CreateUsers->__sizeUser;
	int ret;
	network_config_t *network;
	int i, j;
	int user_num = 0;
	int err = 1;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	while (user_num < MAX_ACCOUNT_NUM &&
			strlen(network->accounts[user_num].user))
		user_num++;

	if (size > (MAX_ACCOUNT_NUM - user_num)) {
		err = 2;
		goto fault;
	}

	for (i = 0; i < size; i++) {

		if (strlen(tds__CreateUsers->User[i].Username) >= USER_LEN) {
			err = 3;
			goto fault;
		}

		if (strlen(tds__CreateUsers->User[i].Password) >= PASSWORD_LEN) {
			err = 3;
			goto fault;
		}

		if (tds__CreateUsers->User[i].UserLevel >= tt__UserLevel__Anonymous) {
			err = 4;
			goto fault;
		} else {
			for (j = 0; j < user_num; j++) {
				if (strcmp(tds__CreateUsers->User[i].Username,
					network->accounts[j].user) == 0) {
					err = 5;
					goto fault;
				}
			}
			user_num++;

			strcpy(network->accounts[j].user,
				tds__CreateUsers->User[i].Username);
			strcpy(network->accounts[j].password,
				tds__CreateUsers->User[i].Password);
			network->accounts[j].authority =
				tds__CreateUsers->User[i].UserLevel;
			network->accounts[j].fixed = 0;

			LIBAUTH_ADD_REMOVE_USER new_param;
			memset(&new_param, 0, sizeof(LIBAUTH_ADD_REMOVE_USER));
			strcpy(new_param.user_name,
				tds__CreateUsers->User[i].Username);
			strcpy(new_param.pass_word,
				tds__CreateUsers->User[i].Password);
			rts_auth_init();
			ret = rts_auth_add_user(&new_param);
			rts_auth_uninit();
			if (ret)
				RS_DBG("libauth add user fail ret = %d\n", ret);
		}
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	if (ret)
		goto fault;

	return SOAP_OK;

fault:
	if (err == 1) {
		onvif_fault(soap, SENDER, "ter:OperationProhibited",
				"ter:InValArgs");
	} else if (err == 2) {
		onvif_fault(soap, RECEIVER, "ter:OperationProhibited",
				"ter:TooManyUsers");
	} else if (err == 3) {
		onvif_fault(soap, SENDER, "ter:OperationProhibited",
				"ter:UsernameTooLong");
	} else if (err == 4) {
		onvif_fault(soap, RECEIVER, "ter:OperationProhibited",
				"ter:AnonymousNotAllowed");
	} else if (err == 5) {
		onvif_fault(soap, SENDER, "ter:OperationProhibited",
				"ter:UsernameClash");
	}

	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteUsers(struct soap *soap,
	struct _tds__DeleteUsers *tds__DeleteUsers,
	struct _tds__DeleteUsersResponse *tds__DeleteUsersResponse)
{
	int size = tds__DeleteUsers->__sizeUsername;
	int ret;
	network_config_t *network;
	int i, j;
	int user_num = 0;
	int err = 1;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	while (user_num < MAX_ACCOUNT_NUM &&
			strlen(network->accounts[user_num].user))
		user_num++;


	/* check username already exist */
	for (i = 0; i < size; i++) {

		int matched = 0;

		for (j = 0; j <= user_num; j++) {

			if (matched == 0) {

				if (!strncmp(tds__DeleteUsers->Username[i],
					network->accounts[j].user,
					strlen(tds__DeleteUsers->Username[i]))) {

					if (network->accounts[j].fixed) {
						err = 2;
						goto fault;
					}

					matched = 1;

					LIBAUTH_ADD_REMOVE_USER new_param;
					memset(&new_param, 0,
						sizeof(LIBAUTH_ADD_REMOVE_USER));
					strcpy(new_param.user_name,
						network->accounts[j].user);
					strcpy(new_param.pass_word,
						network->accounts[j].password);
					rts_auth_init();
					ret = rts_auth_remove_user(&new_param);
					rts_auth_uninit();
					if (ret)
						RS_DBG("libauth rm user fail \
							ret = %d\n", ret);

					strcpy(network->accounts[j].user,
						network->accounts[j + 1].user);
					strcpy(network->accounts[j].password,
						network->accounts[j + 1].password);
					network->accounts[j].authority =
						network->accounts[j + 1].authority;
					network->accounts[j].fixed =
						network->accounts[j + 1].fixed;
				}
			} else {

				strcpy(network->accounts[j].user,
					network->accounts[j + 1].user);
				strcpy(network->accounts[j].password,
					network->accounts[j + 1].password);
				network->accounts[j].authority =
					network->accounts[j + 1].authority;
				network->accounts[j].fixed =
					network->accounts[j + 1].fixed;
			}
		}

		if (matched == 1) {
			memset(network->accounts[j].user, 0, USER_LEN);
			memset(network->accounts[j].password, 0, PASSWORD_LEN);
			network->accounts[j].authority = 0;
			network->accounts[j].fixed = 0;
			user_num--;

		} else {
			err = 3;
			goto fault;
		}
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	if (ret)
		goto fault;

	return SOAP_OK;

fault:
	if (err == 1) {
		onvif_fault(soap, RECEIVER, "ter:OperationProhibited",
				"ter:InValArgs");
	} else if (err == 2) {
		onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
				"ter:FixedUser");
	} else if (err == 3) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal",
				"ter:UsernameMissing");
	}

	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetUser(struct soap *soap,
	struct _tds__SetUser *tds__SetUser,
	struct _tds__SetUserResponse *tds__SetUserResponse)
{
	int size = tds__SetUser->__sizeUser;
	int ret;
	network_config_t *network;
	int i, j;
	int user_num = 0;
	int err = 1;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	for (i = 0; i < size; i++) {

		if (strlen(tds__SetUser->User[i].Password) > PASSWORD_LEN) {

			err = 2;
			goto fault;
		} else if (strlen(tds__SetUser->User[i].Password) == 0) {

			err = 3;
			goto fault;
		} else {

			int idx = 0;
			int matched = 0;
			while (idx < MAX_ACCOUNT_NUM &&
					strlen(network->accounts[idx].user)) {

				if (strcmp(tds__SetUser->User[i].Username,
					network->accounts[idx].user) == 0) {

					if (network->accounts[idx].fixed) {
						err = 4;
						goto fault;
					} else if (tds__SetUser->User[i].UserLevel >=
							tt__UserLevel__Anonymous) {
						err = 5;
						goto fault;
					}

					LIBAUTH_CHANGE_PASSWD new_param;
					memset(&new_param, 0,
						sizeof(LIBAUTH_CHANGE_PASSWD));
					strcpy(new_param.user_name,
						network->accounts[idx].user);
					strcpy(new_param.ori_pass_wd,
						network->accounts[idx].password);
					strcpy(new_param.new_pass_wd,
						tds__SetUser->User[i].Password);

					rts_auth_init();
					ret = rts_auth_modify_password(&new_param);
					rts_auth_uninit();
					if (ret)
						RS_DBG("libauth change pwd fail \
							ret = %d\n", ret);

					strcpy(network->accounts[idx].password,
						tds__SetUser->User[i].Password);
					network->accounts[idx].authority =
						tds__SetUser->User[i].UserLevel;
					network->accounts[idx].fixed = 0;
					matched = 1;
					break;
				}
				idx++;
			}

			if (matched == 0) {
				err = 6;
				goto fault;

			}
		}
	}

	ret = rsOnvifSysInfoSetNetConfig(network);
	if (ret)
		goto fault;

	return SOAP_OK;

fault:
	if (err == 1) {
		onvif_fault(soap, SENDER, "ter:OperationProhibited",
				"ter:InValArgs");
	} else if (err == 2) {
		onvif_fault(soap, SENDER, "ter:OperationProhibited",
				"ter:PasswordTooLong");
	} else if (err == 3) {
		onvif_fault(soap, SENDER, "ter:OperationProhibited",
				"ter:PasswordTooWeak");
	} else if (err == 4) {
		onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
				"ter:FixedUser");
	} else if (err == 5) {
		onvif_fault(soap, RECEIVER, "ter:OperationProhibited",
				"ter:AnonymousNotAllowed");
	} else if (err == 6) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal",
				"ter:UsernameMissing");
	}

	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetWsdlUrl(struct soap *soap,
	struct _tds__GetWsdlUrl *tds__GetWsdlUrl,
	struct _tds__GetWsdlUrlResponse *tds__GetWsdlUrlResponse)
{
	if (soap->header)
		soap->header->wsse__Security = NULL;
	tds__GetWsdlUrlResponse->WsdlUrl =
		"http://www.onvif.org/Documents/Specifications.aspx";
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCapabilities(struct soap *soap,
	struct _tds__GetCapabilities *tds__GetCapabilities,
	struct _tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse)
{
	int i = 0, ret;
	network_config_t *network;
	onvif_special_info_t *info;
	char deviceio_xaddr[XADDR_BUF_SIZE];
	char analyticsdevice_xaddr[XADDR_BUF_SIZE];
	char device_xaddr[XADDR_BUF_SIZE];
	char events_xaddr[XADDR_BUF_SIZE];
	char imaging_xaddr[XADDR_BUF_SIZE];
	char media_xaddr[XADDR_BUF_SIZE];
	char ptz_xaddr[XADDR_BUF_SIZE];
	char ipaddr[XADDR_BUF_SIZE];
	if (soap->header)
		soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);
	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto fault;


	sprintf(deviceio_xaddr, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_DEVICEIO]);
	sprintf(analyticsdevice_xaddr, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_ANALYTICSDEVICE]);
	sprintf(device_xaddr, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_DEVICE]);
	sprintf(events_xaddr, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_EVENTS]);
	sprintf(imaging_xaddr, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_IMAGING]);
	sprintf(media_xaddr, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_MEDIA]);
	sprintf(ptz_xaddr, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_PTZ]);

	if (tds__GetCapabilities->Category == NULL) {
		tds__GetCapabilities->Category =
			(enum tt__CapabilityCategory *)soap_malloc(soap,
					sizeof(enum tt__CapabilityCategory));
		*tds__GetCapabilities->Category = tt__CapabilityCategory__All;
	}
	tds__GetCapabilitiesResponse->Capabilities =
		(struct tt__Capabilities *)soap_malloc(soap,
				sizeof(struct tt__Capabilities));
	tds__GetCapabilitiesResponse->Capabilities->Analytics = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Device = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Events = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Imaging = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Media = NULL;
	tds__GetCapabilitiesResponse->Capabilities->PTZ = NULL;

	tds__GetCapabilitiesResponse->Capabilities->Extension =
		(struct tt__CapabilitiesExtension *)soap_malloc(soap,
				sizeof(struct tt__CapabilitiesExtension));
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO =
		(struct tt__DeviceIOCapabilities *)soap_malloc(soap,
				sizeof(struct tt__DeviceIOCapabilities));
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->XAddr =
		(char *)soap_malloc(soap, sizeof(char) * XADDR_BUF_SIZE);
	strcpy(tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->XAddr, deviceio_xaddr);
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->VideoSources = 1;
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->VideoOutputs = 1;
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->AudioSources = 1;
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->AudioOutputs = 1;
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->RelayOutputs = 1;
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->__size = 0;
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->__any = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->DeviceIO->__anyAttribute = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->Display = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->Recording = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->Search = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->Replay = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->Receiver = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->AnalyticsDevice = NULL;
	tds__GetCapabilitiesResponse->Capabilities->Extension->Extensions = NULL;

	tds__GetCapabilitiesResponse->Capabilities->__anyAttribute = NULL;
	if ((*tds__GetCapabilities->Category == tt__CapabilityCategory__All)
		|| (*tds__GetCapabilities->Category == tt__CapabilityCategory__Analytics)) {
		tds__GetCapabilitiesResponse->Capabilities->Analytics = NULL;
#if 0
		tds__GetCapabilitiesResponse->Capabilities->Analytics = (struct tt__AnalyticsCapabilities *)soap_malloc(soap, sizeof(struct tt__AnalyticsCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr = (char *) soap_malloc(soap, XADDR_BUF_SIZE);
		strcpy(tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr, analyticsdevice_xaddr);
		tds__GetCapabilitiesResponse->Capabilities->Analytics->RuleSupport = "false";
		tds__GetCapabilitiesResponse->Capabilities->Analytics->AnalyticsModuleSupport = "false";
		tds__GetCapabilitiesResponse->Capabilities->Analytics->__size = 0;
		tds__GetCapabilitiesResponse->Capabilities->Analytics->__any = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Analytics->__anyAttribute = NULL;
#endif

	}
	if ((*tds__GetCapabilities->Category == tt__CapabilityCategory__All)
		|| (*tds__GetCapabilities->Category == tt__CapabilityCategory__Device)) {
		tds__GetCapabilitiesResponse->Capabilities->Device =
			(struct tt__DeviceCapabilities *)soap_malloc(soap,
					sizeof(struct tt__DeviceCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->Device->XAddr =
			(char *) soap_malloc(soap, XADDR_BUF_SIZE);
		strcpy(tds__GetCapabilitiesResponse->Capabilities->Device->XAddr, device_xaddr);

		tds__GetCapabilitiesResponse->Capabilities->Device->Network =
			(struct tt__NetworkCapabilities *)soap_malloc(soap,
					sizeof(struct tt__NetworkCapabilities));

		tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPFilter =
			info->device_service_capabilities.network.ip_filter ? &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->Network->ZeroConfiguration =
			info->device_service_capabilities.network.zero_configuration ? &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPVersion6 =
			info->device_service_capabilities.network.ip_version6 ?  &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->Network->DynDNS =
			info->device_service_capabilities.network.dynamic_dns ?  &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension =
			(struct tt__NetworkCapabilitiesExtension *)soap_malloc(soap,
				sizeof(struct tt__NetworkCapabilitiesExtension));
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension->__size = 0;
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension->__any = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension->Dot11Configuration =
			info->device_service_capabilities.network.dot_11_configuration ?  &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension->Extension = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->__anyAttribute = NULL;

		tds__GetCapabilitiesResponse->Capabilities->Device->System =
			(struct tt__SystemCapabilities *)soap_malloc(soap,
					sizeof(struct tt__SystemCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryResolve =
			info->device_service_capabilities.system.discovery_resolve ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryBye =
		info->device_service_capabilities.system.discovery_bye ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->RemoteDiscovery =
			info->device_service_capabilities.system.remote_discovery ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemBackup =
			info->device_service_capabilities.system.system_backup ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemLogging =
			info->device_service_capabilities.system.system_logging ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->FirmwareUpgrade =
			info->device_service_capabilities.system.firmware_upgrade ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->__sizeSupportedVersions = 1;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions =
			(struct tt__OnvifVersion *)soap_malloc(soap,
					sizeof(struct tt__OnvifVersion));
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions->Major =
			(info->service_version[SERVICE_INDEX_DEVICE] & 0xff00)>>8;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions->Minor =
			info->service_version[SERVICE_INDEX_DEVICE] & 0xff;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension =
			(struct tt__SystemCapabilitiesExtension *)soap_malloc(soap,
				sizeof(struct tt__SystemCapabilitiesExtension));

		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->__size = 0;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->__any = NULL;

		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSystemBackup =
			info->device_service_capabilities.system.http_system_backup ?  &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpFirmwareUpgrade =
			info->device_service_capabilities.system.firmware_upgrade ?  &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSystemLogging =
			info->device_service_capabilities.system.http_system_logging ?  &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSupportInformation =
			info->device_service_capabilities.system.http_support_information ?  &gXsdTrue : &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->Extension = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->__anyAttribute = NULL;

		tds__GetCapabilitiesResponse->Capabilities->Device->IO =
			(struct tt__IOCapabilities *)soap_malloc(soap,
					sizeof(struct tt__IOCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->Device->IO->InputConnectors =
			&info->device_service_capabilities.io.input_connectors;
		tds__GetCapabilitiesResponse->Capabilities->Device->IO->RelayOutputs =
			&info->device_service_capabilities.io.relay_outputs;
		tds__GetCapabilitiesResponse->Capabilities->Device->IO->Extension = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Device->IO->__anyAttribute = NULL;

		tds__GetCapabilitiesResponse->Capabilities->Device->Security =
			(struct tt__SecurityCapabilities *)soap_malloc(soap,
					sizeof(struct tt__SecurityCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->TLS1_x002e1 =
			info->device_service_capabilities.security.tls1_x002e1 ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->TLS1_x002e2 =
			info->device_service_capabilities.security.tls1_x002e2 ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->OnboardKeyGeneration =
			info->device_service_capabilities.security.onboard_key_generation ? xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->AccessPolicyConfig =
			info->device_service_capabilities.security.access_policy_config ? xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->X_x002e509Token =
			info->device_service_capabilities.security.x_x002e509_token ? xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->SAMLToken =
			info->device_service_capabilities.security.saml_token ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->KerberosToken =
			info->device_service_capabilities.security.kerberos_token ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->RELToken =
			info->device_service_capabilities.security.rel_token ?  xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->__size = 0;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->__any = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->Extension = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Device->Security->__anyAttribute = NULL;

		tds__GetCapabilitiesResponse->Capabilities->Device->Extension = NULL;

	}

	if ((*tds__GetCapabilities->Category == tt__CapabilityCategory__All)
		|| (*tds__GetCapabilities->Category == tt__CapabilityCategory__Events)) {
		tds__GetCapabilitiesResponse->Capabilities->Events =
			(struct tt__EventCapabilities *)soap_malloc(soap,
					sizeof(struct tt__EventCapabilities));

		tds__GetCapabilitiesResponse->Capabilities->Events->XAddr =
			(char *) soap_malloc(soap, XADDR_BUF_SIZE);
		strcpy(tds__GetCapabilitiesResponse->Capabilities->Events->XAddr, events_xaddr);

		tds__GetCapabilitiesResponse->Capabilities->Events->WSSubscriptionPolicySupport =
			info->event_service_capabilities.ws_subscription_policy_support ? xsd__boolean__true_ : xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Events->WSPullPointSupport =
			info->event_service_capabilities.ws_pull_point_support ?  xsd__boolean__true_ : xsd__boolean__false_;

		tds__GetCapabilitiesResponse->Capabilities->Events->WSPausableSubscriptionManagerInterfaceSupport =
			info->event_service_capabilities.ws_pausable_submgr_support ?  xsd__boolean__true_ : xsd__boolean__false_;

		tds__GetCapabilitiesResponse->Capabilities->Events->__size = 0;
		tds__GetCapabilitiesResponse->Capabilities->Events->__any = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Events->__anyAttribute = NULL;

	}

	if ((*tds__GetCapabilities->Category == tt__CapabilityCategory__All)
		|| (*tds__GetCapabilities->Category == tt__CapabilityCategory__Imaging)) {
	/*	tds__GetCapabilitiesResponse->Capabilities->Imaging = NULL;*/
		tds__GetCapabilitiesResponse->Capabilities->Imaging =
			(struct tt__ImagingCapabilities *)soap_malloc(soap,
					sizeof(struct tt__ImagingCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr =
			(char *) soap_malloc(soap, XADDR_BUF_SIZE);
		strcpy(tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr, imaging_xaddr);
		tds__GetCapabilitiesResponse->Capabilities->Imaging->__anyAttribute = NULL;
	}

	if ((*tds__GetCapabilities->Category == tt__CapabilityCategory__All)
		|| (*tds__GetCapabilities->Category == tt__CapabilityCategory__Media)) {

		/* tds__GetCapabilitiesResponse->Capabilities->Media = NULL; */

		tds__GetCapabilitiesResponse->Capabilities->Media =
			(struct tt__MediaCapabilities *)soap_malloc(soap,
					sizeof(struct tt__MediaCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->Media->XAddr =
			(char *) soap_malloc(soap, XADDR_BUF_SIZE);
		strcpy(tds__GetCapabilitiesResponse->Capabilities->Media->XAddr, media_xaddr);
		tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities =
			(struct tt__RealTimeStreamingCapabilities *)soap_malloc(soap,
				sizeof(struct tt__RealTimeStreamingCapabilities));

		tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTPMulticast = &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORETCP = &gXsdFalse;

		tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP = &gXsdFalse;
		tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->Extension = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->__anyAttribute = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Media->__size = 0;
		tds__GetCapabilitiesResponse->Capabilities->Media->Extension = NULL;
		tds__GetCapabilitiesResponse->Capabilities->Media->__anyAttribute = NULL;
	}

	if (*tds__GetCapabilities->Category == tt__CapabilityCategory__PTZ) {
		goto fault;
#if 0
		tds__GetCapabilitiesResponse->Capabilities->PTZ = (struct tt__PTZCapabilities *)soap_malloc(soap, sizeof(struct tt__PTZCapabilities));
		tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr = (char *) soap_malloc(soap, XADDR_BUF_SIZE);
		strcpy(tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr, ptz_xaddr);
		tds__GetCapabilitiesResponse->Capabilities->PTZ->__size = 0;
		tds__GetCapabilitiesResponse->Capabilities->PTZ->__any = NULL;
		tds__GetCapabilitiesResponse->Capabilities->PTZ->__anyAttribute = NULL;

#endif
	}

	if (*tds__GetCapabilities->Category == tt__CapabilityCategory__Analytics) {
		goto fault;
	}

	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER,  "ter:ActionNotSupported", "ter:NoSuchService");
	return SOAP_FAULT;


}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetHostname(struct soap *soap,
	struct _tds__GetHostname *tds__GetHostname,
	struct _tds__GetHostnameResponse *tds__GetHostnameResponse)
{
	int ret;
	network_config_t *network;
	if (soap->header)
		soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	tds__GetHostnameResponse->HostnameInformation =
		(struct tt__HostnameInformation *)soap_malloc(soap,
				sizeof(struct tt__HostnameInformation));
	tds__GetHostnameResponse->HostnameInformation->FromDHCP =
		network->host_name.dhcp_enable ? xsd__boolean__true_ : xsd__boolean__false_;/*  "true" : "false"; */
	tds__GetHostnameResponse->HostnameInformation->Name =
		(char *)soap_malloc(soap, MID_INFO_SIZE);
	memcpy(tds__GetHostnameResponse->HostnameInformation->Name,
			network->host_name.name, MID_INFO_SIZE);
	tds__GetHostnameResponse->HostnameInformation->Extension = NULL;
	tds__GetHostnameResponse->HostnameInformation->__anyAttribute = NULL;
	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:InvalidArgVal");
	return SOAP_FAULT;


}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostname(struct soap *soap,
	struct _tds__SetHostname *tds__SetHostname,
	struct _tds__SetHostnameResponse *tds__SetHostnameResponse)
{
	int ret;
	int err = 0;
	control_message_set_host_name_t *arg;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	arg = (control_message_set_host_name_t *)soap_malloc(soap,
			sizeof(control_message_set_host_name_t));
	ASSERT(arg);
	ret = isValidHostname(tds__SetHostname->Name);
	if (!ret)
		goto fault;

	strcpy(arg->name, tds__SetHostname->Name);

	rsOnvifMsgSetHostname(arg);

	return SOAP_OK;

fault:
	onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:InvalidHostname");
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostnameFromDHCP(struct soap *soap,
	struct _tds__SetHostnameFromDHCP *tds__SetHostnameFromDHCP,
	struct _tds__SetHostnameFromDHCPResponse *tds__SetHostnameFromDHCPResponse)
{
	int i = 0, ret;
	network_config_t *network;
	onvif_special_info_t *info;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);
	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto fault;


	if (info->device_service_capabilities.network.hostname_from_dhcp == 0 ||
			network->host_name.dhcp_enable == 0) {
		goto fault;
	}

	/* get the hostname from DHCP; */

	/* TO BE IMPLEMENTED; */

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:DHCPNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDNS(struct soap *soap,
	struct _tds__GetDNS *tds__GetDNS,
	struct _tds__GetDNSResponse *tds__GetDNSResponse)
{
	int ret;
	network_config_t *network;
	int manual_dns_cnt = 0;
	int dhcp_dns_cnt = 0;
	int search_domain_cnt = 0;
	int i;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	while (manual_dns_cnt < MAX_DNS_NUM &&
			network->dns.manul_dns[manual_dns_cnt].s_addr)
		manual_dns_cnt++;

	while (dhcp_dns_cnt < MAX_DNS_NUM &&
			network->dns.dhcp_dns[dhcp_dns_cnt].s_addr)
		dhcp_dns_cnt++;

	while (strlen(network->dns.search_domain[search_domain_cnt])) {
		if (search_domain_cnt++ == MAX_SEARCH_DOMAIN_NUM)
			break;
	}

	tds__GetDNSResponse->DNSInformation =
		(struct tt__DNSInformation *)soap_malloc(soap,
				sizeof(struct tt__DNSInformation));
	tds__GetDNSResponse->DNSInformation->FromDHCP =
		network->dns.dhcp_enable ? xsd__boolean__true_ : xsd__boolean__false_;/* "true" : "false"; */
	tds__GetDNSResponse->DNSInformation->__sizeSearchDomain = search_domain_cnt;
	tds__GetDNSResponse->DNSInformation->SearchDomain =
		(char **)soap_malloc(soap, sizeof(char *) * search_domain_cnt);
	for (i = 0; i < search_domain_cnt; i++) {
		tds__GetDNSResponse->DNSInformation->SearchDomain[i] =
			network->dns.search_domain[i];
	}
	tds__GetDNSResponse->DNSInformation->__sizeDNSFromDHCP = 0;
	tds__GetDNSResponse->DNSInformation->__sizeDNSManual = 0;
	tds__GetDNSResponse->DNSInformation->Extension = NULL;

	/* if (!strncmp(tds__GetDNSResponse->DNSInformation->FromDHCP, "true", 4)) */
	if (tds__GetDNSResponse->DNSInformation->FromDHCP == gXsdTrue) {
		tds__GetDNSResponse->DNSInformation->__sizeDNSFromDHCP =
			dhcp_dns_cnt;
		tds__GetDNSResponse->DNSInformation->DNSFromDHCP =
			(struct tt__IPAddress *)soap_malloc(soap,
					dhcp_dns_cnt * sizeof(struct tt__IPAddress));
		for (i = 0; i < dhcp_dns_cnt; i++) {
			tds__GetDNSResponse->DNSInformation->DNSFromDHCP[i].Type = 0;
			tds__GetDNSResponse->DNSInformation->DNSFromDHCP[i].IPv4Address =
				(char *)soap_malloc(soap, sizeof(char) * IP_ADDR_BUF_SIZE);
			strcpy(tds__GetDNSResponse->DNSInformation->DNSFromDHCP[i].IPv4Address,
				inet_ntoa(network->dns.dhcp_dns[i]));
			tds__GetDNSResponse->DNSInformation->DNSFromDHCP[i].IPv6Address = NULL;
		}
	} else {
		tds__GetDNSResponse->DNSInformation->__sizeDNSManual =
			manual_dns_cnt;
		tds__GetDNSResponse->DNSInformation->DNSManual =
			((struct tt__IPAddress *)soap_malloc(soap,
				manual_dns_cnt * sizeof(struct tt__IPAddress)));
		for (i = 0; i < manual_dns_cnt; i++) {
			tds__GetDNSResponse->DNSInformation->DNSManual[i].Type = 0;
			tds__GetDNSResponse->DNSInformation->DNSManual[i].IPv4Address =
				(char *)soap_malloc(soap, IP_ADDR_BUF_SIZE);
			strcpy(tds__GetDNSResponse->DNSInformation->DNSManual[i].IPv4Address,
				inet_ntoa(network->dns.manul_dns[i]));
			tds__GetDNSResponse->DNSInformation->DNSManual[i].IPv6Address = NULL;
		}

	}

	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:InvalidArgVal");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDNS(struct soap *soap,
	struct _tds__SetDNS *tds__SetDNS,
	struct _tds__SetDNSResponse *tds__SetDNSResponse)
{
	int ret;
	int i;
	int update_info = 0;
	int err = 1;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	if (tds__SetDNS->FromDHCP == xsd__boolean__true_) {
		rsOnvifMsgSetDHCPDNS();
	}

	if (tds__SetDNS->__sizeSearchDomain) {

		control_message_set_search_domain_t *arg;
		arg = (control_message_set_search_domain_t *)soap_malloc(soap,
				sizeof(control_message_set_search_domain_t));
		ASSERT(arg);


		for (i = 0; i < tds__SetDNS->__sizeSearchDomain &&
				i < MAX_SEARCH_DOMAIN_NUM; i++) {

			strncpy(arg->search_domain[i],
					tds__SetDNS->SearchDomain[i],
					SEARCH_DOMAIN_LENGTH);
		}
		arg->domain_num = i;
		ret = rsOnvifMsgSetSearchDomain(arg);
		if (ret)
			goto fault;
	}

	if (tds__SetDNS->__sizeDNSManual) {
		control_message_set_manual_dns_t *arg =
			(control_message_set_manual_dns_t *)soap_malloc(soap,
					sizeof(control_message_set_manual_dns_t));
		ASSERT(arg);
		for (i = 0; i < tds__SetDNS->__sizeDNSManual && i < MAX_DNS_NUM; i++) {
			if (tds__SetDNS->DNSManual[i].Type != 0) {
				err = 2;
				goto fault;
			}

			if (isValidIP4(tds__SetDNS->DNSManual[i].IPv4Address) == 0) {
				err = 2;
				goto fault;
			}

			if (inet_aton(tds__SetDNS->DNSManual[i].IPv4Address, &arg->dns[i]) == 0) {
				err = 2;
				goto fault;
			}
		}

		arg->dns_num = i;
		rsOnvifMsgSetManualDNS(arg);

	}

	return SOAP_OK;

fault:
	if (err == 1) {
		onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
				"ter:InvalidArgVal");
	} else if (err == 2) {
		onvif_fault(soap, RECEIVER, "ter:InvalidArgVal",
				"ter:InvalidIPv4Address");
	}
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNTP(struct soap *soap,
	struct _tds__GetNTP *tds__GetNTP,
	struct _tds__GetNTPResponse *tds__GetNTPResponse)
{
	int ret;
	network_config_t *network;
	int manual_ntp_cnt = 0, dhcp_ntp_cnt = 0;
	int i;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	while (manual_ntp_cnt < MAX_NTP_NUM &&
		network->ntp.manual_ntp[manual_ntp_cnt].s_addr)
		manual_ntp_cnt++;

	while (dhcp_ntp_cnt < MAX_NTP_NUM &&
			network->ntp.dhcp_ntp[dhcp_ntp_cnt].s_addr)
		dhcp_ntp_cnt++;

	tds__GetNTPResponse->NTPInformation =
		(struct tt__NTPInformation *)soap_malloc(soap,
				sizeof(struct tt__NTPInformation));

	struct tt__NTPInformation *p_res_ntp =
		tds__GetNTPResponse->NTPInformation;
	p_res_ntp->FromDHCP =
		network->ntp.dhcp_enable ? xsd__boolean__true_ : xsd__boolean__false_;

	p_res_ntp->__sizeNTPFromDHCP = dhcp_ntp_cnt;

	if (dhcp_ntp_cnt > 0) {
		p_res_ntp->NTPFromDHCP =
			(struct tt__NetworkHost *)soap_malloc(soap,
				dhcp_ntp_cnt * sizeof(struct tt__NetworkHost));

		for (i = 0; i < dhcp_ntp_cnt; i++) {

			p_res_ntp->NTPFromDHCP[i].Type =
				tt__NetworkHostType__IPv4;
			p_res_ntp->NTPFromDHCP[i].IPv4Address =
				(char *)soap_malloc(soap, IP_ADDR_BUF_SIZE);
			strcpy(p_res_ntp->NTPFromDHCP[i].IPv4Address,
				inet_ntoa(network->ntp.dhcp_ntp[i]));
			p_res_ntp->NTPFromDHCP[i].IPv6Address = NULL;
			p_res_ntp->NTPFromDHCP[i].DNSname = NULL;
			p_res_ntp->NTPFromDHCP[i].Extension = NULL;
			p_res_ntp->NTPFromDHCP[i].__anyAttribute = NULL;
		}
	} else {
		p_res_ntp->NTPFromDHCP = NULL;
	}

	p_res_ntp->__sizeNTPManual = manual_ntp_cnt;

	if (manual_ntp_cnt > 0) {

		p_res_ntp->NTPManual =
			(struct tt__NetworkHost *)soap_malloc(soap,
				manual_ntp_cnt * sizeof(struct tt__NetworkHost));
		for (i = 0; i < manual_ntp_cnt; i++) {

			p_res_ntp->NTPManual[i].Type =
				tt__NetworkHostType__IPv4;
			p_res_ntp->NTPManual[i].IPv4Address =
				(char *)soap_malloc(soap, IP_ADDR_BUF_SIZE);
			strcpy(p_res_ntp->NTPManual[i].IPv4Address,
				inet_ntoa(network->ntp.manual_ntp[i]));
			p_res_ntp->NTPManual[i].IPv6Address = NULL;
			p_res_ntp->NTPManual[i].DNSname = NULL;
			p_res_ntp->NTPManual[i].Extension = NULL;
			p_res_ntp->NTPManual[i].__anyAttribute = NULL;
		}
	} else {
		p_res_ntp->NTPManual = NULL;
	}

	p_res_ntp->Extension = NULL;
	p_res_ntp->__anyAttribute = NULL;

	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:InvalidArgVal");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNTP(struct soap *soap,
	struct _tds__SetNTP *tds__SetNTP,
	struct _tds__SetNTPResponse *tds__SetNTPResponse)
{
	char host[LARGE_INFO_LENGTH];
	char service[SMALL_INFO_LENGTH];
	int ret = 0;
	int i;
	int err = 0;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	if (tds__SetNTP->FromDHCP) {
		rsOnvifMsgSetDHCPNTP();
	}

	if (tds__SetNTP->__sizeNTPManual) {

		control_message_set_manual_ntp_t *arg =
			(control_message_set_manual_ntp_t *)soap_malloc(soap,
				sizeof(control_message_set_manual_ntp_t));
		arg->ntp_num = tds__SetNTP->__sizeNTPManual;

		for (i = 0; i < tds__SetNTP->__sizeNTPManual; i++) {

			if (tds__SetNTP->NTPManual[i].IPv6Address != NULL) {
				err = 1;
				goto fault;
			}

			if (tds__SetNTP->NTPManual[i].IPv4Address != NULL) {

				ret = isValidIP4(
					tds__SetNTP[i].NTPManual->IPv4Address);
				if (0 == ret) {
					err = 1;
					goto fault;
				}

				inet_aton(tds__SetNTP[i].NTPManual->IPv4Address,
						&arg->ntp[i]);
			}
		}
		rsOnvifMsgSetManualNTP(arg);
	}
	return SOAP_OK;

fault:
	if (err == 1) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal",
				"ter:InvalidIPv4Address");
	}
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDynamicDNS(struct soap *soap,
	struct _tds__GetDynamicDNS *tds__GetDynamicDNS,
	struct _tds__GetDynamicDNSResponse *tds__GetDynamicDNSResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDynamicDNS(struct soap *soap,
	struct _tds__SetDynamicDNS *tds__SetDynamicDNS,
	struct _tds__SetDynamicDNSResponse *tds__SetDynamicDNSResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkInterfaces(struct soap *soap,
	struct _tds__GetNetworkInterfaces *tds__GetNetworkInterfaces,
	struct _tds__GetNetworkInterfacesResponse *tds__GetNetworkInterfacesResponse)
{
	struct _tds__GetNetworkInterfacesResponse *p_response =
		tds__GetNetworkInterfacesResponse;
	int ret;
	network_config_t *network;
	int i;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);

	if (network->interface_num == 0) {
		goto fault;
	}


	p_response->__sizeNetworkInterfaces = network->interface_num;
	p_response->NetworkInterfaces =
		(struct tt__NetworkInterface *)soap_malloc(soap,
			network->interface_num * sizeof(struct tt__NetworkInterface));
	for (i = 0; i < network->interface_num; i++) {
		unsigned char *mac;
		mac = network->interfaces[network->interface_idx].mac;
		char mac_str[30];

		sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
				mac[0], mac[1], mac[2],
				mac[3], mac[4], mac[5]);

		p_response->NetworkInterfaces[i].token =
			network->interfaces[i].token;
		p_response->NetworkInterfaces[i].Enabled =
			network->interfaces[i].enabled ? xsd__boolean__true_ : xsd__boolean__false_;/* "true" : "false"; */

		p_response->NetworkInterfaces[i].Info =
			(struct tt__NetworkInterfaceInfo *)soap_malloc(soap,
					sizeof(struct tt__NetworkInterfaceInfo));
		p_response->NetworkInterfaces[i].Info->Name = network->interfaces[i].token;
		p_response->NetworkInterfaces[i].Info->HwAddress =
			(char *)soap_malloc(soap, LARGE_INFO_LENGTH);
		strcpy(p_response->NetworkInterfaces[i].Info->HwAddress, mac_str);
		p_response->NetworkInterfaces[i].Info->MTU =
			(int *)soap_malloc(soap, sizeof(int));
		p_response->NetworkInterfaces[i].Info->MTU[0] =
			network->interfaces[i].mtu;

		p_response->NetworkInterfaces[i].Link = NULL;
		p_response->NetworkInterfaces[i].IPv4 =
			(struct tt__IPv4NetworkInterface *)soap_malloc(soap,
					sizeof(struct tt__IPv4NetworkInterface));
		p_response->NetworkInterfaces[i].IPv4->Enabled = xsd__boolean__true_;/* "true"; */
		p_response->NetworkInterfaces[i].IPv4->Config =
			(struct tt__IPv4Configuration *)soap_malloc(soap,
					sizeof(struct tt__IPv4Configuration));
		p_response->NetworkInterfaces[i].IPv4->Config->__sizeManual = 1;
		p_response->NetworkInterfaces[i].IPv4->Config->Manual =
			(struct tt__PrefixedIPv4Address *)soap_malloc(soap,
					sizeof(struct tt__PrefixedIPv4Address));
		p_response->NetworkInterfaces[i].IPv4->Config->Manual->Address =
			(char *)soap_malloc(soap,
					sizeof(char) * MID_INFO_LENGTH);
		strcpy(p_response->NetworkInterfaces[i].IPv4->Config->Manual->Address,
				inet_ntoa(network->interfaces[i].ip));
		p_response->NetworkInterfaces[i].IPv4->Config->Manual->PrefixLength = 24; /* 24 ones means 255.255.255.0 netmask */

		p_response->NetworkInterfaces[i].IPv4->Config->LinkLocal =
			(struct tt__PrefixedIPv4Address *)soap_malloc(soap,
					sizeof(struct tt__PrefixedIPv4Address));
		p_response->NetworkInterfaces[i].IPv4->Config->LinkLocal->Address =
			(char *)soap_malloc(soap, MID_INFO_LENGTH);
		strcpy(p_response->NetworkInterfaces[i].IPv4->Config->LinkLocal->Address,
				inet_ntoa(network->interfaces[i].ip));
		p_response->NetworkInterfaces[i].IPv4->Config->LinkLocal->PrefixLength = 24; /* 24 ones means 255.255.255.0 netmask */

		p_response->NetworkInterfaces[i].IPv4->Config->FromDHCP =
			(struct tt__PrefixedIPv4Address *)soap_malloc(soap,
					sizeof(struct tt__PrefixedIPv4Address));
		p_response->NetworkInterfaces[i].IPv4->Config->FromDHCP->Address =
			(char *)soap_malloc(soap, MID_INFO_LENGTH);
		strcpy(p_response->NetworkInterfaces[i].IPv4->Config->FromDHCP->Address,
				inet_ntoa(network->interfaces[i].ip));
		p_response->NetworkInterfaces[i].IPv4->Config->FromDHCP->PrefixLength = 24; /* 24 ones means 255.255.255.0 netmask */

		p_response->NetworkInterfaces[i].IPv4->Config->DHCP =
			network->interfaces[i].dhcp_enable ? xsd__boolean__true_ : xsd__boolean__false_;/* "true" : "false"; */
		p_response->NetworkInterfaces[i].IPv4->Config->__size = 0;
		p_response->NetworkInterfaces[i].IPv4->Config->__any = NULL;
		p_response->NetworkInterfaces[i].IPv4->Config->__anyAttribute = NULL;
		p_response->NetworkInterfaces[i].IPv6 = NULL;
		p_response->NetworkInterfaces[i].Extension = NULL;
		p_response->NetworkInterfaces[i].__anyAttribute = NULL;
	}
	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:Unknown reason");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkInterfaces(struct soap *soap,
	struct _tds__SetNetworkInterfaces *tds__SetNetworkInterfaces,
	struct _tds__SetNetworkInterfacesResponse *tds__SetNetworkInterfacesResponse)
{
	/* struct in_addr ipaddr, sys_ip; */
	int value;
	int ret;
	int err = 0;
	int i;
	int idx, matched = 0;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	network_config_t *network =
		(network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	if (tds__SetNetworkInterfaces->InterfaceToken == NULL) {
		err = 1;
		goto fault;
	}

	for (i = 0; i < network->interface_num; i++) {
		if (!strcmp(tds__SetNetworkInterfaces->InterfaceToken,
					network->interfaces[i].token)) {
			idx = i;
			matched = 1;
			break;
		}
	}

	if (!matched) {
		err = 1;
		goto fault;
	}


	if (tds__SetNetworkInterfaces->NetworkInterface->MTU != NULL) {
		err = 2;
		goto fault;
	}

	if (tds__SetNetworkInterfaces->NetworkInterface->Link != NULL) {
		err = 3;
		goto fault;
	}

	if (tds__SetNetworkInterfaces->NetworkInterface->IPv6 != NULL) {
		if (*tds__SetNetworkInterfaces->NetworkInterface->IPv6->Enabled == 1) {
			err = 4;
			goto fault;
		}
	}

	if (tds__SetNetworkInterfaces->NetworkInterface->IPv4 == NULL) {
		err = 5;
		goto fault;
	}
	/* RS_DBG("\n"); */
	if (tds__SetNetworkInterfaces->NetworkInterface->IPv4->Enabled != NULL) {
		control_message_set_network_interface_t *arg =
			(control_message_set_network_interface_t *)soap_malloc(soap,
				sizeof(control_message_set_network_interface_t));

		arg->idx = idx;
		arg->manual_ip.s_addr = 0;
		arg->dhcp_enable = 0;

		if (tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual != NULL) {
			if (tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address != NULL) {
				/*check ip address */
				if (isValidIP4(tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address) == 0) {
					err = 6;
					goto fault;
				}
				inet_aton(tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address, &arg->manual_ip);
				/*
				network->interfaces[idx].ip.s_addr = arg->manual_ip.s_addr;
				char tmp_buf[128] = {0};
				sprintf(tmp_buf, "%s", inet_ntoa(arg->manual_ip));
				RS_DBG("arg->manual_ip = %s\n", tmp_buf);
				char tmp_2[128] = {0};
				sprintf(tmp_2, "%s", inet_ntoa(network->interfaces[idx].ip));
				RS_DBG("share memoryip = %s\n", tmp_2);
				*/
			}

		} else {
			if (tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP != NULL) {
				if (xsd__boolean__true_ == *tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP) {
					arg->dhcp_enable = 1;
				}
			}
		}
		rsOnvifMsgSetNetworkInterface(arg);
		/*
		ret = rsOnvifSysInfoSetNetConfig(network);
		ret = rsOnvifSysInfoGetNetConfig(network);
		RS_DBG("network->interface_idx = %d\n", network->interface_idx);
		RS_DBG("now idx = %d\n", idx);
		char tmp_3[128] = {0};
		sprintf(tmp_3, "%s", inet_ntoa(network->interfaces[idx].ip));
		RS_DBG("get from share memoryip = %s\n", tmp_3);
		*/
		/*wait for 5 seconds */
		/* sleep(2); */ /* neil_yan maybe it should put into rsOnvifDevCtrlSetNetworkInterface */
		/* tds__SetNetworkInterfacesResponse->RebootNeeded = xsd__boolean__true_; */
		tds__SetNetworkInterfacesResponse->RebootNeeded = xsd__boolean__false_;
		/* "true";// haisi donot need to reboot !*/

	}
	rsOnvifMsgSendHello();

	return SOAP_OK;

fault:

	if (err == 1) {
		onvif_fault(soap,
				RECEIVER,
				"ter:InvalidArgVal",
				"ter:InvalidNetworkInterface");
	} else if (err == 2) {
		onvif_fault(soap,
				RECEIVER,
				"ter:InvalidArgVal",
				"ter:SettingMTUNotSupported");
	} else if (err == 3) {
		onvif_fault(soap,
				RECEIVER,
				"ter:InvalidArgVal",
				"ter:SettingLinkValuesNotSupported");
	} else if (err == 4) {
		onvif_fault(soap,
				RECEIVER,
				"ter:InvalidArgVal",
				"ter:IPv6NotSupported");
	} else if (err == 5) {
		onvif_fault(soap,
				RECEIVER,
				"ter:InvalidArgVal",
				"ter:IPv4ValuesMissing");
	} else if (err == 6) {
		onvif_fault(soap,
				RECEIVER,
				"ter:InvalidArgVal",
				"ter:InvalidIPv4Address");
	}

	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkProtocols(struct soap *soap,
	struct _tds__GetNetworkProtocols *tds__GetNetworkProtocols,
	struct _tds__GetNetworkProtocolsResponse *tds__GetNetworkProtocolsResponse)
{
	int ret;
	int err = 0;
	int i;
	network_config_t *network;

	int http_idx = -1, https_idx = -1, rtsp_idx = -1;
	int port_cnt;
	int protocol_idx = 0;
	int protocol_num = 0;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;


	for (i = 0; i < NETWORK_PROTOCOL_NUM; i++) {
		if (!strcmp(network->protocol[i].name, HTTP_NAME))
			http_idx = i;
		else if (!strcmp(network->protocol[i].name, HTTPS_NAME))
			https_idx = i;
		else if (!strcmp(network->protocol[i].name, RTSP_NAME))
			rtsp_idx = i;
	}

	protocol_num = 0;
	if (http_idx != -1)
		protocol_num++;
	if (https_idx != -1)
		protocol_num++;
	if (rtsp_idx != -1)
		protocol_num++;

	tds__GetNetworkProtocolsResponse->__sizeNetworkProtocols = protocol_num;
	tds__GetNetworkProtocolsResponse->NetworkProtocols =
		(struct tt__NetworkProtocol *)soap_malloc(soap,
			tds__GetNetworkProtocolsResponse->__sizeNetworkProtocols * sizeof(struct tt__NetworkProtocol));

	/* http */
	protocol_idx = 0;
	if (http_idx != -1) {
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Name = 0;/*  HTTP = 0, HTTPS = 1, RTSP = 2 */
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Enabled =
			network->protocol[http_idx].enabled ? xsd__boolean__true_ : xsd__boolean__false_;/* "true" : "false"; */
		port_cnt = 0;
		while (network->protocol[http_idx].port[port_cnt]) {
			if (port_cnt++ == MAX_PROTOCOL_PORT_NUM)
				break;
		}
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].__sizePort = port_cnt;
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Port =
			(int *)soap_malloc(soap, sizeof(int)*port_cnt);
		while (port_cnt--) {
			tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Port[port_cnt] =
				network->protocol[http_idx].port[port_cnt];
		}
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Extension = NULL;
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].__anyAttribute = NULL;
		protocol_idx++;
	}

	/* https */
	if (https_idx != -1) {
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Name = 1;/*  HTTP = 0, HTTPS = 1, RTSP = 2 */
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Enabled =
			network->protocol[https_idx].enabled ? xsd__boolean__true_ : xsd__boolean__false_;/* "true" : "false"; */
		port_cnt = 0;
		while (network->protocol[https_idx].port[port_cnt]) {
			if (port_cnt++ == MAX_PROTOCOL_PORT_NUM)
				break;
		}
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].__sizePort = port_cnt;
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Port =
			(int *)soap_malloc(soap, sizeof(int)*port_cnt);
		while (port_cnt--) {
			tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Port[port_cnt] =
				network->protocol[https_idx].port[port_cnt];
		}
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Extension = NULL;
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].__anyAttribute = NULL;
		protocol_idx++;
	}

	/* rtsp */
	if (rtsp_idx != -1) {
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Name = 2;/*  HTTP = 0, HTTPS = 1, RTSP = 2 */
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Enabled =
			network->protocol[rtsp_idx].enabled ? xsd__boolean__true_ : xsd__boolean__false_;/* "true" : "false"; */
		port_cnt = 0;
		while (network->protocol[rtsp_idx].port[port_cnt]) {
			if (port_cnt++ == MAX_PROTOCOL_PORT_NUM)
				break;
		}
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].__sizePort = port_cnt;
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Port =
			(int *)soap_malloc(soap, sizeof(int)*port_cnt);
		while (port_cnt--) {
			tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Port[port_cnt] =
				network->protocol[rtsp_idx].port[port_cnt];
		}
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].Extension = NULL;
		tds__GetNetworkProtocolsResponse->NetworkProtocols[protocol_idx].__anyAttribute = NULL;
		protocol_idx++;
	}

	return SOAP_OK;

fault:
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:ServiceNotSupported");
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkProtocols(struct soap *soap,
	struct _tds__SetNetworkProtocols *tds__SetNetworkProtocols,
	struct _tds__SetNetworkProtocolsResponse *tds__SetNetworkProtocolsResponse)
{
	int ret = 0;
	int i;
	control_message_set_network_protocols_t *arg;
	int size;
	int idx = 0;
	int err = 1;
	struct tt__NetworkProtocol *protocols;
	onvif_special_info_t *info;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;


	info = (onvif_special_info_t *)soap_malloc(soap,
			sizeof(onvif_special_info_t));
	ASSERT(info)
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto fault;

	size = tds__SetNetworkProtocols->__sizeNetworkProtocols;

	if (size == 0)
		return SOAP_OK;



	arg = (control_message_set_network_protocols_t *)soap_malloc(soap,
			sizeof(control_message_set_network_protocols_t));
	ASSERT(arg);

	memset(arg, 0, sizeof(control_message_set_network_protocols_t));

	protocols = tds__SetNetworkProtocols->NetworkProtocols;
	idx = 0;
	for (i = 0; i < size; i++) {
		int j;
		ASSERT(i < NETWORK_PROTOCOL_NUM);
		ASSERT(idx < NETWORK_PROTOCOL_NUM);
		if (protocols[i].Name == tt__NetworkProtocolType__HTTP) {
			strcpy(arg->protocol[idx].name, HTTP_NAME);
		} else if (protocols[i].Name == tt__NetworkProtocolType__HTTPS) {
			if (info->device_service_capabilities.security.tls1_x002e0 ||
				info->device_service_capabilities.security.tls1_x002e1 ||
				info->device_service_capabilities.security.tls1_x002e2) {
				strcpy(arg->protocol[idx].name, HTTPS_NAME);
			} else if (xsd__boolean__true_  == protocols[i].Enabled) {
				err = 2;
				goto fault;
			} else {
				continue; /* ignore this protocol; */
			}

		} else if (protocols[i].Name == tt__NetworkProtocolType__RTSP) {
			strcpy(arg->protocol[idx].name, RTSP_NAME);
		} else {
			err = 2;
			goto fault;
		}

		if (xsd__boolean__true_ == protocols[i].Enabled) {
			arg->protocol[idx].enabled = 1;
		} else {
			arg->protocol[idx].enabled = 0;
		}

		for (j = 0; j < protocols[i].__sizePort; j++) {
			ASSERT(j < MAX_PROTOCOL_PORT_NUM);
			arg->protocol[idx].port[j] = protocols[i].Port[j];
		}
		idx++;
	}

	rsOnvifMsgSetNetworkProtocols(arg);


	return SOAP_OK;

fault:
	if (err == 1) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:Unknown Reason");
	} else if (err == 2) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:ServiceNotSupported");
	}
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkDefaultGateway(struct soap *soap,
	struct _tds__GetNetworkDefaultGateway *tds__GetNetworkDefaultGateway,
	struct _tds__GetNetworkDefaultGatewayResponse *tds__GetNetworkDefaultGatewayResponse)
{
	int ret;
	network_config_t *network;
	char _GatewayAddress[LARGE_INFO_LENGTH];

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);

	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto fault;

	strcpy(_GatewayAddress, inet_ntoa(network->gateway));
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway =
		((struct tt__NetworkGateway *)soap_malloc(soap,
			sizeof(struct tt__NetworkGateway *)));
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway->__sizeIPv4Address = 1;
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway->__sizeIPv6Address = 0;
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv4Address =
		(char **)soap_malloc(soap, sizeof(char *));
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv4Address[0] =
		(char *)soap_malloc(soap, IP_ADDR_BUF_SIZE);
	strcpy(*tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv4Address,
			_GatewayAddress);
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv6Address = NULL;
	return SOAP_OK;

fault:
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:InvalidArgVal");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkDefaultGateway(struct soap *soap,
	struct _tds__SetNetworkDefaultGateway *tds__SetNetworkDefaultGateway,
	struct _tds__SetNetworkDefaultGatewayResponse *tds__SetNetworkDefaultGatewayResponse)
{
	control_message_set_default_gateway_t *arg;
	char _IPv4Address[LARGE_INFO_LENGTH];
	int ret = 0;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__Administrator)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	arg = (control_message_set_default_gateway_t *)soap_malloc(soap,
			sizeof(control_message_set_default_gateway_t));
	ASSERT(arg);

	if (tds__SetNetworkDefaultGateway->__sizeIPv6Address) {
		onvif_fault(soap,
				SENDER,
				"ter:NotSupported",
				"ter:InvalidIPv6Address");
		return SOAP_FAULT;
	}

	if (!tds__SetNetworkDefaultGateway->__sizeIPv4Address) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:InvalidGatewayAddress");
		return SOAP_FAULT;
	}
	strcpy(_IPv4Address, *tds__SetNetworkDefaultGateway->IPv4Address);

	if (isValidIP4(_IPv4Address) == 0) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:InvalidGatewayAddress");
		return SOAP_FAULT;
	}
	inet_aton(_IPv4Address, &arg->gateway);

	ret = rsOnvifMsgSetDefaultGateway(arg);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetZeroConfiguration(struct soap *soap,
	struct _tds__GetZeroConfiguration *tds__GetZeroConfiguration,
	struct _tds__GetZeroConfigurationResponse *tds__GetZeroConfigurationResponse)
{

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetZeroConfiguration(struct soap *soap,
	struct _tds__SetZeroConfiguration *tds__SetZeroConfiguration,
	struct _tds__SetZeroConfigurationResponse *tds__SetZeroConfigurationResponse)
{

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetIPAddressFilter(struct soap *soap,
	struct _tds__GetIPAddressFilter *tds__GetIPAddressFilter,
	struct _tds__GetIPAddressFilterResponse *tds__GetIPAddressFilterResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetIPAddressFilter(struct soap *soap,
	struct _tds__SetIPAddressFilter *tds__SetIPAddressFilter,
	struct _tds__SetIPAddressFilterResponse *tds__SetIPAddressFilterResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__AddIPAddressFilter(struct soap *soap,
	struct _tds__AddIPAddressFilter *tds__AddIPAddressFilter,
	struct _tds__AddIPAddressFilterResponse *tds__AddIPAddressFilterResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveIPAddressFilter(struct soap *soap,
	struct _tds__RemoveIPAddressFilter *tds__RemoveIPAddressFilter,
	struct _tds__RemoveIPAddressFilterResponse *tds__RemoveIPAddressFilterResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAccessPolicy(struct soap *soap,
	struct _tds__GetAccessPolicy *tds__GetAccessPolicy,
	struct _tds__GetAccessPolicyResponse *tds__GetAccessPolicyResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetAccessPolicy(struct soap *soap,
	struct _tds__SetAccessPolicy *tds__SetAccessPolicy,
	struct _tds__SetAccessPolicyResponse *tds__SetAccessPolicyResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRelayOutputs(struct soap *soap,
	struct _tds__GetRelayOutputs *tds__GetRelayOutputs,
	struct _tds__GetRelayOutputsResponse *tds__GetRelayOutputsResponse)
{
	tds__GetRelayOutputsResponse->__sizeRelayOutputs = 0;
	tds__GetRelayOutputsResponse->RelayOutputs = NULL;

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputSettings(struct soap *soap,
	struct _tds__SetRelayOutputSettings *tds__SetRelayOutputSettings,
	struct _tds__SetRelayOutputSettingsResponse *tds__SetRelayOutputSettingsResponse)
{

	/*SysInfo *oSysInfo = GetSysInfo();
	relay_conf relays_conf_in;
	int i;
	int Ptoken_exist = NOT_EXIST;
	int ret;
	if (tds__SetRelayOutputSettings->RelayOutputToken != NULL) {
	for (i = 0;i < oSysInfo->nrelays; i++) {
	if (strcmp(tds__SetRelayOutputSettings->RelayOutputToken, oSysInfo->onvif_relay[i].onvif_relay_in.token) == 0) {
	Ptoken_exist = EXIST;
	break;
	}
	}
	}
	if (!Ptoken_exist) {
	onvif_fault(soap, RECEIVER,  "ter:InvalidArgVal", "ter:RelayToken");
	return SOAP_FAULT;
	}

	relays_conf_in.position = i;
	strcpy(relays_conf_in.relay_conf_in_t.token_t, tds__SetRelayOutputSettings->RelayOutputToken);
	relays_conf_in.relay_conf_in_t.relaymode_t = tds__SetRelayOutputSettings->Properties->Mode;
	relays_conf_in.relay_conf_in_t.delaytime_t = tds__SetRelayOutputSettings->Properties->DelayTime;
	relays_conf_in.relay_conf_in_t.idlestate_t = tds__SetRelayOutputSettings->Properties->IdleState;
	ret = ControlSystemData(SFIELD_SET_RELAYS_CONF, (void *)&relays_conf_in, sizeof(relays_conf_in));
	*/
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputState(struct soap *soap,
	struct _tds__SetRelayOutputState *tds__SetRelayOutputState,
	struct _tds__SetRelayOutputStateResponse *tds__SetRelayOutputStateResponse)
{
	/*SysInfo *oSysInfo  = GetSysInfo();
	int i = 0;
	relay_conf  relay;
	int token_exist = NOT_EXIST;
	for (i = 0 ;i < oSysInfo->nrelays; i++) {
	if (strcmp(tds__SetRelayOutputState->RelayOutputToken, oSysInfo->onvif_relay[i].onvif_relay_in.token) == 0) {
	token_exist = EXIST;
	break;
	}
	}
	if (!token_exist) {
	onvif_fault(soap, RECEIVER, "ter:InvalidArgVal ", "ter:RelayToken");
	return SOAP_FAULT;
	}
	relay.logicalstate_t = tds__SetRelayOutputState->LogicalState ;
	ControlSystemData(SFIELD_SET_RELAY_LOGICALSTATE, (void *) &relay, sizeof(relay));*/
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SendAuxiliaryCommand(struct soap *soap,
	struct _tds__SendAuxiliaryCommand *tds__SendAuxiliaryCommand,
	struct _tds__SendAuxiliaryCommandResponse *tds__SendAuxiliaryCommandResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemUris(struct soap *soap,
	struct _tds__GetSystemUris *tds__GetSystemUris,
	struct _tds__GetSystemUrisResponse *tds__GetSystemUrisResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__StartFirmwareUpgrade(struct soap *soap,
	struct _tds__StartFirmwareUpgrade *tds__StartFirmwareUpgrade,
	struct _tds__StartFirmwareUpgradeResponse *tds__StartFirmwareUpgradeResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__StartSystemRestore(struct soap *soap,
	struct _tds__StartSystemRestore *tds__StartSystemRestore,
	struct _tds__StartSystemRestoreResponse *tds__StartSystemRestoreResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:InvalidArgVal",
			"ter:RestoreNotSupported");
	return SOAP_FAULT;
}
