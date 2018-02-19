#include "rsOnvifDefines.h"
#include "rsOnvifTypes.h"
#include "rsOnvifConfig.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <arpa/inet.h>/*solve inet_ntoa segment fault issue*/

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

#ifdef __cplusplus
extern "C" {
#endif

static json_object *json_GetScopes(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "profile", json_object_new_string(local_info->system_info.onvif_special_info.scopes.profile));

	json_object *array_type = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_SCOPES_TYPE_NUM) {
		if (local_info->system_info.onvif_special_info.scopes.type[i][0] != 0)
			json_object_array_add(array_type, json_object_new_string(local_info->system_info.onvif_special_info.scopes.type[i]));
		else
			break;
		i++;
	}
	json_object_object_add(json_ret, "type", array_type);

	json_object_object_add(json_ret, "name", json_object_new_string(local_info->system_info.onvif_special_info.scopes.name));

	i = 0;
	json_object *array_location = json_object_new_array();
	while (i < MAX_SCOPES_LOCATION_NUM) {
		if (local_info->system_info.onvif_special_info.scopes.location[i][0] != 0)
			json_object_array_add(array_location, json_object_new_string(local_info->system_info.onvif_special_info.scopes.location[i]));
		else
			break;
		i++;
	}
	json_object_object_add(json_ret, "location", array_location);

	json_object_object_add(json_ret, "hardware", json_object_new_string(local_info->system_info.onvif_special_info.scopes.hardware));

	i = 0;
	json_object *array_others = json_object_new_array();
	while (i < MAX_SCOPES_OTHER_NUM) {
		if (local_info->system_info.onvif_special_info.scopes.others[i][0] != 0)
			json_object_array_add(array_others, json_object_new_string(local_info->system_info.onvif_special_info.scopes.others[i]));
		else
			break;
		i++;
	}
	json_object_object_add(json_ret, "others", array_others);


	return json_ret;
}

static json_object *json_GetServiceNameSpace(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object *array_service_ns = json_object_new_array();
	uint8 i = 0;
	while (i < ONVIF_SERVICE_NUM) {
		if (local_info->system_info.onvif_special_info.service_ns[i][0] != 0)
			json_object_array_add(array_service_ns, json_object_new_string(local_info->system_info.onvif_special_info.service_ns[i]));
		else
			break;
		i++;
	}
	json_object_object_add(json_ret, "service_namespace", array_service_ns);
	return json_ret;
}

static json_object *json_GetServiceName(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();
	json_object *array_service_name = json_object_new_array();
	uint8 i = 0;
	while (i < ONVIF_SERVICE_NUM) {
		if (local_info->system_info.onvif_special_info.service_name[i][0] != 0)
			json_object_array_add(array_service_name, json_object_new_string(local_info->system_info.onvif_special_info.service_name[i]));
		else
			break;
		i++;
	}
	json_object_object_add(json_ret, "service_name", array_service_name);
	return json_ret;
}

static json_object *json_GetServiceVersion(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();
	json_object *array_service_version = json_object_new_array();
	uint8 i = 0;
	while (i < ONVIF_SERVICE_NUM) {
		if (local_info->system_info.onvif_special_info.service_name[i][0] != 0)
			json_object_array_add(array_service_version, json_object_new_int(local_info->system_info.onvif_special_info.service_version[i]));
		else
			break;
		i++;
	}
	json_object_object_add(json_ret, "service_version", array_service_version);
	return json_ret;
}

static json_object *json_GetDeviceType(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();
	json_object_object_add(json_ret, "device_type", json_object_new_string(local_info->system_info.onvif_special_info.device_type));
	return json_ret;
}

static json_object *json_GetNetworkCapabilities(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "IPFilter", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.ip_filter));
	json_object_object_add(json_ret, "ZeroConfiguration", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.zero_configuration));
	json_object_object_add(json_ret, "IPV6", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.ip_version6));
	json_object_object_add(json_ret, "DynamicDNS", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.dynamic_dns));
	json_object_object_add(json_ret, "Dot11Configuration", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.dot_11_configuration));
	json_object_object_add(json_ret, "Dot1xConfigurations", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.dot1x_configurations));
	json_object_object_add(json_ret, "HostnameFromDHCP", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.hostname_from_dhcp));
	json_object_object_add(json_ret, "NTPServerNum", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.ntp_server_number));
	json_object_object_add(json_ret, "DHCP_V6", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.network.dhcp_v6));
	json_object_object_add(json_ret, "Any_attribute", json_object_new_string(local_info->system_info.onvif_special_info.device_service_capabilities.network.any_attribute));

	return json_ret;
}

static json_object *json_GetSecurityCapabilities(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "TLS1_x002e0", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.tls1_x002e0));
	json_object_object_add(json_ret, "TLS1_x002e1", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.tls1_x002e1));
	json_object_object_add(json_ret, "TLS1_x002e2", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.tls1_x002e2));
	json_object_object_add(json_ret, "OnboardKeyGen", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.onboard_key_generation));
	json_object_object_add(json_ret, "AccessPolicyConfig", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.access_policy_config));
	json_object_object_add(json_ret, "DefaultAccessPolicy", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.default_access_policy));
	json_object_object_add(json_ret, "Dot1x", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.dot1x));
	json_object_object_add(json_ret, "RemoteUserHandling", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.remote_user_handling));
	json_object_object_add(json_ret, "Xx002e509Token", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.x_x002e509_token));
	json_object_object_add(json_ret, "SamlToken", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.saml_token));
	json_object_object_add(json_ret, "KerberosToken", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.kerberos_token));
	json_object_object_add(json_ret, "UsernameToken", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.username_token));
	json_object_object_add(json_ret, "HttpDigest", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.http_digest));
	json_object_object_add(json_ret, "RelToken", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.security.rel_token));
	json_object_object_add(json_ret, "SupportedEAPMethods", json_object_new_string(local_info->system_info.onvif_special_info.device_service_capabilities.security.supported_eap_methods));
	json_object_object_add(json_ret, "AnyAttribute", json_object_new_string(local_info->system_info.onvif_special_info.device_service_capabilities.security.any_attribute));

	return json_ret;
}

static json_object *json_GetSystemCapabilities(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "DiscoveryResolve", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.discovery_resolve));
	json_object_object_add(json_ret, "DiscoveryBye", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.discovery_bye));
	json_object_object_add(json_ret, "RemoteDiscovery", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.remote_discovery));
	json_object_object_add(json_ret, "SystemBackup", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.system_backup));
	json_object_object_add(json_ret, "SystemLog", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.system_logging));
	json_object_object_add(json_ret, "FWUpgrade", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.firmware_upgrade));
	json_object_object_add(json_ret, "HttpFWUpgrate", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.http_firmware_upgrade));
	json_object_object_add(json_ret, "HttpSysBackup", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.http_system_backup));
	json_object_object_add(json_ret, "HttpSysLog", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.http_system_logging));
	json_object_object_add(json_ret, "HttpSupportInfo", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.system.http_support_information));

	json_object_object_add(json_ret, "AnyAttribute", json_object_new_string(local_info->system_info.onvif_special_info.device_service_capabilities.system.any_attribute));

	return json_ret;
}

static json_object *json_GetIOCapabilities(RS_LOCAL_SYSTEM_INFO *local_info)
{

	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "InputConnectors", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.io.input_connectors));
	json_object_object_add(json_ret, "RelayOutputs", json_object_new_int(local_info->system_info.onvif_special_info.device_service_capabilities.io.relay_outputs));

	return json_ret;

}

static json_object *json_GetMiscCapabilities(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "AuxiliaryCmds", json_object_new_string(local_info->system_info.onvif_special_info.device_service_capabilities.misc.auxiliary_commands));
	json_object_object_add(json_ret, "AnyAttribute", json_object_new_string(local_info->system_info.onvif_special_info.device_service_capabilities.misc.any_attribute));

	return json_ret;

}

static json_object *json_GetDeviceServiceCapabilities(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "network_capabilities", json_GetNetworkCapabilities(local_info));
	json_object_object_add(json_ret, "security_capabilities", json_GetSecurityCapabilities(local_info));
	json_object_object_add(json_ret, "system_capabilities", json_GetSystemCapabilities(local_info));
	json_object_object_add(json_ret, "io_capabilities", json_GetIOCapabilities(local_info));
	json_object_object_add(json_ret, "misc_capabilities", json_GetMiscCapabilities(local_info));

	return json_ret;
}

static json_object *json_GetEventServiceCapabilities(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "WSSubscriptionPolicySupport", json_object_new_int(local_info->system_info.onvif_special_info.event_service_capabilities.ws_subscription_policy_support));
	json_object_object_add(json_ret, "WSPullPointSupprot", json_object_new_int(local_info->system_info.onvif_special_info.event_service_capabilities.ws_pull_point_support));
	json_object_object_add(json_ret, "WSPausableSubmgrSupport", json_object_new_int(local_info->system_info.onvif_special_info.event_service_capabilities.ws_pausable_submgr_support));
	json_object_object_add(json_ret, "PersistentNotifactionStorage", json_object_new_int(local_info->system_info.onvif_special_info.event_service_capabilities.persistentNotificationStorage));
	json_object_object_add(json_ret, "MaxProducers", json_object_new_int(local_info->system_info.onvif_special_info.event_service_capabilities.max_notificationproducers));
	json_object_object_add(json_ret, "MaxPullPoints", json_object_new_int(local_info->system_info.onvif_special_info.event_service_capabilities.max_pull_points));

	return json_ret;
}

static json_object *json_GetOnvifSpecialInfo(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "onvif_scopes", json_GetScopes(local_info));
	json_object_object_add(json_ret, "onvif_service_namespace", json_GetServiceNameSpace(local_info));
	json_object_object_add(json_ret, "onvif_service_name", json_GetServiceName(local_info));
	json_object_object_add(json_ret, "onvif_service_version", json_GetServiceVersion(local_info));
	json_object_object_add(json_ret, "onvif_device_type", json_GetDeviceType(local_info));
	json_object_object_add(json_ret, "onvif_device_service_capabilities", json_GetDeviceServiceCapabilities(local_info));
	json_object_object_add(json_ret, "onvif_event_service_capabilities", json_GetEventServiceCapabilities(local_info));

	return json_ret;
}

static json_object *json_GetOnvifDeviceInfo(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "Manufacturer", json_object_new_string(local_info->system_info.device_info.manufacturer));
	json_object_object_add(json_ret, "Model", json_object_new_string(local_info->system_info.device_info.model));
	json_object_object_add(json_ret, "FWVersion", json_object_new_string(local_info->system_info.device_info.firmware_version));
	json_object_object_add(json_ret, "SN", json_object_new_string(local_info->system_info.device_info.serial_number));
	json_object_object_add(json_ret, "HWID", json_object_new_string(local_info->system_info.device_info.hardware_id));

	return json_ret;
}

static json_object *json_GetOnvifDatetimeInfo(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();

	json_object_object_add(json_ret, "NTPTime", json_object_new_int(local_info->system_info.date_time_settings.time_ntp));
	json_object_object_add(json_ret, "DayLightSavings", json_object_new_int(local_info->system_info.date_time_settings.day_light_savings));
	json_object_object_add(json_ret, "TimeZone", json_object_new_string(local_info->system_info.date_time_settings.time_zone));

	return json_ret;
}

static json_object *json_GetSystemUris(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();
	json_object_object_add(json_ret, "SystemLogUri", json_object_new_string(local_info->system_info.network_config.system_uris.system_log_uri));
	json_object_object_add(json_ret, "SystemBackupUri", json_object_new_string(local_info->system_info.network_config.system_uris.system_backup_uri));
	json_object_object_add(json_ret, "SupportInfoUri", json_object_new_string(local_info->system_info.network_config.system_uris.support_info_uri));
	json_object_object_add(json_ret, "SystemRestoreUri", json_object_new_string(local_info->system_info.network_config.system_uris.system_restore_uri));

	return json_ret;
}

static json_object *json_GetAccounts(RS_LOCAL_SYSTEM_INFO *local_info)
{
/*we just save text here , maybe encrypt later*/

	json_object *array_accounts = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_ACCOUNT_NUM) {
		if (local_info->system_info.network_config.accounts[i].user[0] != 0) {
			json_object *json_node = json_object_new_object();
			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.network_config.accounts[i].user));
			json_object_object_add(json_node, "Pwd", json_object_new_string(local_info->system_info.network_config.accounts[i].password));
			json_object_object_add(json_node, "Authority", json_object_new_int(local_info->system_info.network_config.accounts[i].authority));
			json_object_object_add(json_node, "Fixed", json_object_new_int(local_info->system_info.network_config.accounts[i].fixed));

			json_object_array_add(array_accounts, json_node);
		} else
			break;
		i++;
	}

	return array_accounts;

}

static json_object *json_GetHostname(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();
	json_object_object_add(json_ret, "Name", json_object_new_string(local_info->system_info.network_config.host_name.name));
	json_object_object_add(json_ret, "FromDHCP", json_object_new_int(local_info->system_info.network_config.host_name.dhcp_enable));

	return json_ret;
}

static json_object *json_GetNetworkInterface(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_network_interfaces = json_object_new_array();
	uint8 i = 0;

	while (i < MAX_NETWORK_INTERFACE_NUM) {
		if (local_info->system_info.network_config.interfaces[i].token[0] != 0) {

			unsigned char *mac;
			mac = local_info->system_info.network_config.interfaces[i].mac;
			char mac_str[30] = {0};
			sprintf(mac_str,  "%02X:%02X:%02X:%02X:%02X:%02X",  mac[0],  mac[1],  mac[2],  mac[3],  mac[4],  mac[5]);
			json_object *json_node = json_object_new_object();
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.network_config.interfaces[i].token));
			json_object_object_add(json_node, "Mac", json_object_new_string(mac_str));
			json_object_object_add(json_node, "Mtu", json_object_new_int(local_info->system_info.network_config.interfaces[i].mtu));
			json_object_object_add(json_node, "Enable", json_object_new_int(local_info->system_info.network_config.interfaces[i].enabled));
			json_object_object_add(json_node, "IP", json_object_new_string(inet_ntoa(local_info->system_info.network_config.interfaces[i].ip)));
			json_object_object_add(json_node, "DHCP_Enable", json_object_new_int(local_info->system_info.network_config.interfaces[i].dhcp_enable));
			json_object_array_add(array_network_interfaces, json_node);
		} else
			break;
		i++;
	}

	return array_network_interfaces;
}

static json_object *json_GetDNSConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 i = 0;
	json_object *json_ret = json_object_new_object();
	json_object_object_add(json_ret, "DHCPEnable", json_object_new_int(local_info->system_info.network_config.dns.dhcp_enable));
	json_object *array_manual_dns = json_object_new_array();
	while (i < MAX_DNS_NUM) {
		if (local_info->system_info.network_config.dns.manul_dns[i].s_addr != 0) {
			json_object_array_add(array_manual_dns, json_object_new_string(inet_ntoa(local_info->system_info.network_config.dns.manul_dns[i])));
		} else
			break;
		i++;
	}
	json_object_object_add(json_ret, "ManulDNS", array_manual_dns);
	json_object *array_dhcp_dns = json_object_new_array();
	i = 0;
	while (i < MAX_DNS_NUM) {
		if (local_info->system_info.network_config.dns.dhcp_dns[i].s_addr != 0) {
			json_object_array_add(array_dhcp_dns, json_object_new_string(inet_ntoa(local_info->system_info.network_config.dns.dhcp_dns[i])));
		} else
			break;
		i++;
	}
	json_object_object_add(json_ret, "DHCPDNS", array_dhcp_dns);
	json_object_object_add(json_ret, "SearchDomain", json_object_new_string(local_info->system_info.network_config.dns.search_domain));

	return json_ret;
}

static json_object *json_GetNTPConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 i = 0;
	json_object *json_ret = json_object_new_object();
	json_object_object_add(json_ret, "DHCPEnable", json_object_new_int(local_info->system_info.network_config.ntp.dhcp_enable));

	json_object *array_manual_ntp = json_object_new_array();
	while (i < MAX_NTP_NUM) {
		if (local_info->system_info.network_config.ntp.manual_ntp[i].s_addr != 0) {
			json_object_array_add(array_manual_ntp, json_object_new_string(inet_ntoa(local_info->system_info.network_config.ntp.manual_ntp[i])));
		} else
			break;
		i++;
	}
	json_object_object_add(json_ret, "ManulNTP", array_manual_ntp);
	i = 0;
	json_object *array_dhcp_ntp = json_object_new_array();
	while (i < MAX_NTP_NUM) {
		if (local_info->system_info.network_config.ntp.dhcp_ntp[i].s_addr != 0) {
			json_object_array_add(array_dhcp_ntp, json_object_new_string(inet_ntoa(local_info->system_info.network_config.ntp.dhcp_ntp[i])));
		} else
			break;
		i++;
	}
	json_object_object_add(json_ret, "DHCPNTP", array_dhcp_ntp);


	return json_ret;
}

static json_object *json_GetNetworkProtocolConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_network_protocol = json_object_new_array();
	uint8 i = 0;
	uint8 j = 0;
	while (i < NETWORK_PROTOCOL_NUM) {
		if (local_info->system_info.network_config.protocol[i].name[0] != 0) {
			json_object *json_node = json_object_new_object();
			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.network_config.protocol[i].name));
			json_object_object_add(json_node, "Enable", json_object_new_int(local_info->system_info.network_config.protocol[i].enabled));
			for (j = 0; j < MAX_PROTOCOL_PORT_NUM; j++) {
				if (local_info->system_info.network_config.protocol[i].port[j] != 0) {
					char port_str[10] = {0};
					sprintf(port_str, "Port_%d", j);
					json_object_object_add(json_node, port_str, json_object_new_int(local_info->system_info.network_config.protocol[i].port[j]));
				} else
					break;
			}
			json_object_array_add(array_network_protocol, json_node);
		} else
			break;
		i++;
	}

	return array_network_protocol;
}


static json_object *json_GetOnvifNetworkInfo(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_ret = json_object_new_object();
	json_object_object_add(json_ret, "SystemUris", json_GetSystemUris(local_info));
	json_object_object_add(json_ret, "DiscoveryMode", json_object_new_int(local_info->system_info.network_config.discovery_mode));
	json_object_object_add(json_ret, "RemoteDiscoveryMode", json_object_new_int(local_info->system_info.network_config.remote_discovery_mode));
	json_object_object_add(json_ret, "DiscoveryProxyAddress", json_object_new_string(local_info->system_info.network_config.discover_proxy_address));
	json_object_object_add(json_ret, "Accounts", json_GetAccounts(local_info));
	json_object_object_add(json_ret, "Hostname", json_GetHostname(local_info));
	json_object_object_add(json_ret, "NetworkInterface", json_GetNetworkInterface(local_info));
	json_object_object_add(json_ret, "NetInterfaceNum", json_object_new_int(local_info->system_info.network_config.interface_num));
	json_object_object_add(json_ret, "NetInterfaceSelectedIdx", json_object_new_int(local_info->system_info.network_config.interface_idx));
	json_object_object_add(json_ret, "Netmask", json_object_new_string(inet_ntoa(local_info->system_info.network_config.netmask)));
	json_object_object_add(json_ret, "Gateway", json_object_new_string(inet_ntoa(local_info->system_info.network_config.gateway)));
	json_object_object_add(json_ret, "DNS", json_GetDNSConfig(local_info));
	json_object_object_add(json_ret, "NTP", json_GetNTPConfig(local_info));
	json_object_object_add(json_ret, "NetworkProtocol", json_GetNetworkProtocolConfig(local_info));


	return json_ret;
}

static json_object *json_GetVideoSourceConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_vsc = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_VSC_CONFIG) {
		if (local_info->system_info.media_config.vsc[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.vsc[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.vsc[i].token));
			json_object_object_add(json_node, "VideoSourceToken", json_object_new_string(local_info->system_info.media_config.vsc[i].source_token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.vsc[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.vsc[i].use_count));
			json_object_object_add(json_node, "BoundsX", json_object_new_int(local_info->system_info.media_config.vsc[i].bounds_x));
			json_object_object_add(json_node, "BoundsY", json_object_new_int(local_info->system_info.media_config.vsc[i].bounds_y));
			json_object_object_add(json_node, "BoundsWidth", json_object_new_int(local_info->system_info.media_config.vsc[i].bounds_width));
			json_object_object_add(json_node, "BoundsHeight", json_object_new_int(local_info->system_info.media_config.vsc[i].bounds_height));
			json_object_object_add(json_node, "RotateMode", json_object_new_int(local_info->system_info.media_config.vsc[i].rotate_mode));
			json_object_object_add(json_node, "RotateDegree", json_object_new_int(local_info->system_info.media_config.vsc[i].rotate_degree));

			json_object_array_add(array_vsc, json_node);
		} else
			break;
		i++;
	}

	return array_vsc;
}

static json_object *json_GetVideoEncoderConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_vec = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_VEC_CONFIG) {
		if (local_info->system_info.media_config.vec[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Vec_Path", json_object_new_string(local_info->system_info.media_config.vec[i].vec_path));
			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.vec[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.vec[i].token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.vec[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.vec[i].use_count));
			json_object_object_add(json_node, "EncodingFormat", json_object_new_int(local_info->system_info.media_config.vec[i].encoding_format));
			json_object_object_add(json_node, "EncodingWidth", json_object_new_int(local_info->system_info.media_config.vec[i].encoding_width));
			json_object_object_add(json_node, "EncodingHeight", json_object_new_int(local_info->system_info.media_config.vec[i].encoding_height));
			json_object_object_add(json_node, "Quality", json_object_new_double(local_info->system_info.media_config.vec[i].quality));

			json_object_object_add(json_node, "FrameRateLimit", json_object_new_int(local_info->system_info.media_config.vec[i].frame_rate_limit));
			json_object_object_add(json_node, "EncodingInterval", json_object_new_int(local_info->system_info.media_config.vec[i].encoding_interval));
			json_object_object_add(json_node, "BitrateLimit", json_object_new_int(local_info->system_info.media_config.vec[i].bitrate_limit));

			json_object_object_add(json_node, "H264GovLength", json_object_new_int(local_info->system_info.media_config.vec[i].h264_gov_length));
			json_object_object_add(json_node, "H264Profile", json_object_new_int(local_info->system_info.media_config.vec[i].h264_profile));

			json_object_object_add(json_node, "MulticastIPType", json_object_new_int(local_info->system_info.media_config.vec[i].multicast_ip_type));
			json_object_object_add(json_node, "MulticastIPAddr", json_object_new_string(local_info->system_info.media_config.vec[i].multicast_ip_addr));
			json_object_object_add(json_node, "MulticastIPPort", json_object_new_int(local_info->system_info.media_config.vec[i].multicast_port));
			json_object_object_add(json_node, "MulticastAutoStart", json_object_new_int(local_info->system_info.media_config.vec[i].multicast_auto_start));
			json_object_object_add(json_node, "MulticastTTL", json_object_new_int(local_info->system_info.media_config.vec[i].multicast_ttl));
			json_object_object_add(json_node, "SessionTimeout", json_object_new_int64(local_info->system_info.media_config.vec[i].session_timeout));

			json_object_array_add(array_vec, json_node);
		} else
			break;
		i++;
	}

	return array_vec;
}

static json_object *json_GetAudioSourceConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_asc = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_ASC_CONFIG) {
		if (local_info->system_info.media_config.asc[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.asc[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.asc[i].token));
			json_object_object_add(json_node, "AudioSourceToken", json_object_new_string(local_info->system_info.media_config.asc[i].source_token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.asc[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.asc[i].use_count));


			json_object_array_add(array_asc, json_node);
		} else
			break;
		i++;
	}

	return array_asc;
}

static json_object *json_GetAudioEncoderConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_aec = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_AEC_CONFIG) {
		if (local_info->system_info.media_config.aec[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.aec[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.aec[i].token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.aec[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.aec[i].use_count));
			json_object_object_add(json_node, "EncodingFormat", json_object_new_int(local_info->system_info.media_config.aec[i].encoding_format));
			json_object_object_add(json_node, "Bitrate", json_object_new_int(local_info->system_info.media_config.aec[i].bit_rate));
			json_object_object_add(json_node, "SampleRate", json_object_new_int(local_info->system_info.media_config.aec[i].sample_rate));
			json_object_object_add(json_node, "MulticastIPType", json_object_new_int(local_info->system_info.media_config.aec[i].multicast_ip_type));
			json_object_object_add(json_node, "MulticastIPAddr", json_object_new_string(local_info->system_info.media_config.aec[i].multicast_ip_addr));
			json_object_object_add(json_node, "MulticastIPPort", json_object_new_int(local_info->system_info.media_config.aec[i].port));
			json_object_object_add(json_node, "MulticastAutoStart", json_object_new_int(local_info->system_info.media_config.aec[i].auto_start));
			json_object_object_add(json_node, "MulticastTTL", json_object_new_int(local_info->system_info.media_config.aec[i].ttl));
			json_object_object_add(json_node, "SessionTimeout", json_object_new_int64(local_info->system_info.media_config.aec[i].session_timeout));

			json_object_array_add(array_aec, json_node);
		} else
			break;
		i++;
	}

	return array_aec;

}

static json_object *json_GetAudioDecoderConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_adc = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_ADC_CONFIG) {
		if (local_info->system_info.media_config.adc[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.adc[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.adc[i].token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.adc[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.adc[i].use_count));

			json_object_array_add(array_adc, json_node);
		} else
			break;
		i++;
	}

	return array_adc;
}

static json_object *json_GetAudioOutputConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_aoc = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_AOC_CONFIG) {
		if (local_info->system_info.media_config.aoc[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.aoc[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.aoc[i].token));
			json_object_object_add(json_node, "OutputToken", json_object_new_string(local_info->system_info.media_config.aoc[i].output_token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.aoc[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.aoc[i].use_count));
			json_object_object_add(json_node, "OutputLevel", json_object_new_int(local_info->system_info.media_config.aoc[i].output_level));
			json_object_object_add(json_node, "SendPrimacy", json_object_new_string(local_info->system_info.media_config.aoc[i].send_primacy));

			json_object_array_add(array_aoc, json_node);
		} else
			break;
		i++;
	}

	return array_aoc;
}


static json_object *json_GetPTZConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_ptzc = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_PTZC_CONFIG) {
		if (local_info->system_info.media_config.ptzc[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.ptzc[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.ptzc[i].token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.ptzc[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.ptzc[i].use_count));
			json_object_object_add(json_node, "DefSpeedPtX", json_object_new_double(local_info->system_info.media_config.ptzc[i].default_speed_pt_x));
			json_object_object_add(json_node, "DefSpeedPtY", json_object_new_double(local_info->system_info.media_config.ptzc[i].default_speed_pt_y));
			json_object_object_add(json_node, "DefSpeedZoomX", json_object_new_double(local_info->system_info.media_config.ptzc[i].default_speed_zoom_x));
			json_object_object_add(json_node, "PtLimitsXmax", json_object_new_int64(local_info->system_info.media_config.ptzc[i].pt_limits_x_max));
			json_object_object_add(json_node, "PtLimitsXmin", json_object_new_double(local_info->system_info.media_config.ptzc[i].pt_limits_x_min));
			json_object_object_add(json_node, "PtLimitsYmax", json_object_new_double(local_info->system_info.media_config.ptzc[i].pt_limits_y_max));
			json_object_object_add(json_node, "PtLimitsYmin", json_object_new_double(local_info->system_info.media_config.ptzc[i].pt_limits_y_min));
			json_object_object_add(json_node, "ZoomLimitsXmax", json_object_new_double(local_info->system_info.media_config.ptzc[i].zoom_limits_x_max));
			json_object_object_add(json_node, "ZoomLimitsXmin", json_object_new_double(local_info->system_info.media_config.ptzc[i].zoom_limits_x_min));
			json_object_object_add(json_node, "PTCtrlEFlipMode", json_object_new_int(local_info->system_info.media_config.ptzc[i].ptctrl_eflip_mode));

			json_object_array_add(array_ptzc, json_node);
		} else
			break;
		i++;
	}

	return array_ptzc;
}

static json_object *json_GetVideoAnalyticsConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_vac = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_VAC_CONFIG) {
		if (local_info->system_info.media_config.vac[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.vac[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.vac[i].token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.vac[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.vac[i].use_count));

			json_object_array_add(array_vac, json_node);
		} else
			break;
		i++;
	}

	return array_vac;
}

static json_object *json_GetMetadataConfig(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_mc = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_MC_CONFIG) {
		if (local_info->system_info.media_config.mc[i].token[0] != '\0') {
			json_object *json_node = json_object_new_object();

			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_config.mc[i].name));
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_config.mc[i].token));
			json_object_object_add(json_node, "ForcePersistence", json_object_new_int(local_info->system_info.media_config.mc[i].force_persistence));
			json_object_object_add(json_node, "UseCount", json_object_new_int(local_info->system_info.media_config.mc[i].use_count));

			json_object_object_add(json_node, "PTZStatus", json_object_new_string(local_info->system_info.media_config.mc[i].ptz_status));
			json_object_object_add(json_node, "PTZPosition", json_object_new_string(local_info->system_info.media_config.mc[i].ptz_position));
			json_object_object_add(json_node, "EventFilter", json_object_new_string(local_info->system_info.media_config.mc[i].event_filter));
			json_object_object_add(json_node, "EventSubPolicy", json_object_new_string(local_info->system_info.media_config.mc[i].event_subscription_policy));
			json_object_object_add(json_node, "Analytics", json_object_new_string(local_info->system_info.media_config.mc[i].analytics));

			json_object_object_add(json_node, "H264GovLength", json_object_new_int(local_info->system_info.media_config.vec[i].h264_gov_length));
			json_object_object_add(json_node, "H264Profile", json_object_new_int(local_info->system_info.media_config.vec[i].h264_profile));

			json_object_object_add(json_node, "MulticastIPType", json_object_new_int(local_info->system_info.media_config.mc[i].multicast_ip_type));
			json_object_object_add(json_node, "MulticastIPAddr", json_object_new_string(local_info->system_info.media_config.mc[i].multicast_ip_addr));
			json_object_object_add(json_node, "MulticastIPPort", json_object_new_int(local_info->system_info.media_config.mc[i].multicast_port));
			json_object_object_add(json_node, "MulticastAutoStart", json_object_new_int(local_info->system_info.media_config.mc[i].multicast_auto_start));
			json_object_object_add(json_node, "MulticastTTL", json_object_new_int(local_info->system_info.media_config.mc[i].multicast_ttl));
			json_object_object_add(json_node, "SessionTimeout", json_object_new_int64(local_info->system_info.media_config.mc[i].session_timeout));

			json_object_array_add(array_mc, json_node);
		} else
			break;
		i++;
	}

	return array_mc;
}

static json_object *json_GetOnvifMediaConfigInfo(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_media_config_info = json_object_new_object();

	json_object_object_add(json_media_config_info, "VideoSourceConfiguraton", json_GetVideoSourceConfig(local_info));
	json_object_object_add(json_media_config_info, "VideoEncoderConfiguraton", json_GetVideoEncoderConfig(local_info));
	json_object_object_add(json_media_config_info, "AudioSourceConfiguraton", json_GetAudioSourceConfig(local_info));
	json_object_object_add(json_media_config_info, "AudioEncoderConfiguraton", json_GetAudioEncoderConfig(local_info));
	json_object_object_add(json_media_config_info, "AudioDecoderConfiguraton", json_GetAudioDecoderConfig(local_info));
	json_object_object_add(json_media_config_info, "AudioOutputConfiguraton", json_GetAudioOutputConfig(local_info));
	json_object_object_add(json_media_config_info, "PTZConfiguraton", json_GetPTZConfig(local_info));
	json_object_object_add(json_media_config_info, "VideoAnalyticsConfiguraton", json_GetVideoAnalyticsConfig(local_info));
	json_object_object_add(json_media_config_info, "MetadataConfiguraton", json_GetMetadataConfig(local_info));

	return json_media_config_info;

}

static json_object *json_GetOnvifMediaInfo(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_media = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_MEDIA_PROFILE) {
		if (local_info->system_info.media_profile[i].token[0] != 0) {
			json_object *json_node = json_object_new_object();
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.media_profile[i].token));
			json_object_object_add(json_node, "Name", json_object_new_string(local_info->system_info.media_profile[i].name));
			json_object_object_add(json_node, "Fixed", json_object_new_int(local_info->system_info.media_profile[i].fixed));

			json_object_object_add(json_node, "vsc_index", json_object_new_int(local_info->system_info.media_profile[i].vsc_index));
			json_object_object_add(json_node, "vec_index", json_object_new_int(local_info->system_info.media_profile[i].vec_index));
			json_object_object_add(json_node, "asc_index", json_object_new_int(local_info->system_info.media_profile[i].asc_index));
			json_object_object_add(json_node, "aec_index", json_object_new_int(local_info->system_info.media_profile[i].aec_index));
			json_object_object_add(json_node, "adc_index", json_object_new_int(local_info->system_info.media_profile[i].adc_index));
			json_object_object_add(json_node, "aoc_index", json_object_new_int(local_info->system_info.media_profile[i].aoc_index));
			json_object_object_add(json_node, "ptzc_index", json_object_new_int(local_info->system_info.media_profile[i].ptzc_index));
			json_object_object_add(json_node, "vac_index", json_object_new_int(local_info->system_info.media_profile[i].vac_index));
			json_object_object_add(json_node, "mc_index", json_object_new_int(local_info->system_info.media_profile[i].mc_index));

			json_object_array_add(array_media, json_node);
		} else
			break;
		i++;
	}

	return array_media;
}

static json_object *json_GetOnvifOSDInfo(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_osd = json_object_new_array();
	uint8 i = 0;
	while (i < MAX_OSD_NUM) {
		if (local_info->system_info.osd_config[i].token[0] != 0) {
			json_object *json_node = json_object_new_object();
			json_object_object_add(json_node, "Token", json_object_new_string(local_info->system_info.osd_config[i].token));
			json_object_object_add(json_node, "String", json_object_new_string(local_info->system_info.osd_config[i].osd_string_buf));
			json_object_object_add(json_node, "OSD_CLR_X", json_object_new_double(local_info->system_info.osd_config[i].osd_color_x));
			json_object_object_add(json_node, "OSD_CLR_Y", json_object_new_double(local_info->system_info.osd_config[i].osd_color_y));
			json_object_object_add(json_node, "OSD_CLR_Z", json_object_new_double(local_info->system_info.osd_config[i].osd_color_z));
			json_object_object_add(json_node, "OSD_CLR_Type", json_object_new_int(local_info->system_info.osd_config[i].osd_color_type));
			json_object_object_add(json_node, "OSDTransparent", json_object_new_int(local_info->system_info.osd_config[i].osd_transparent));

			json_object_object_add(json_node, "OSD_BKCLR_X", json_object_new_double(local_info->system_info.osd_config[i].osd_bk_color_x));
			json_object_object_add(json_node, "OSD_BKCLR_Y", json_object_new_double(local_info->system_info.osd_config[i].osd_bk_color_y));
			json_object_object_add(json_node, "OSD_BKCLR_Z", json_object_new_double(local_info->system_info.osd_config[i].osd_bk_color_z));
			json_object_object_add(json_node, "OSD_BKCLR_Type", json_object_new_int(local_info->system_info.osd_config[i].osd_bk_color_type));
			json_object_object_add(json_node, "OSDBKTransparent", json_object_new_int(local_info->system_info.osd_config[i].osd_bk_transparent));

			json_object_object_add(json_node, "OSDFontSize", json_object_new_int(local_info->system_info.osd_config[i].osd_font_size));
			json_object_object_add(json_node, "OSDType", json_object_new_int(local_info->system_info.osd_config[i].osd_type));

			json_object_object_add(json_node, "OSDStartX", json_object_new_int(local_info->system_info.osd_config[i].osd_start_x));
			json_object_object_add(json_node, "OSDStartY", json_object_new_int(local_info->system_info.osd_config[i].osd_start_y));
			json_object_object_add(json_node, "OSDEndX", json_object_new_int(local_info->system_info.osd_config[i].osd_end_x));
			json_object_object_add(json_node, "OSDEndY", json_object_new_int(local_info->system_info.osd_config[i].osd_end_y));

			json_object_array_add(array_osd, json_node);
		} else
			break;
		i++;
	}

	return array_osd;
}


static json_object *json_GetConfigObject(RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *json_system_info = json_object_new_object();
	json_object_object_add(json_system_info, "UUID", json_object_new_string(local_info->system_info.uuid));
	json_object_object_add(json_system_info, "onvif_special_info", json_GetOnvifSpecialInfo(local_info));
	json_object_object_add(json_system_info, "onvif_device_info", json_GetOnvifDeviceInfo(local_info));
	json_object_object_add(json_system_info, "onvif_datetime_info", json_GetOnvifDatetimeInfo(local_info));
	json_object_object_add(json_system_info, "onvif_network_info", json_GetOnvifNetworkInfo(local_info));
	json_object_object_add(json_system_info, "onvif_media_info", json_GetOnvifMediaInfo(local_info));
	json_object_object_add(json_system_info,  "onvif_media_config_info",  json_GetOnvifMediaConfigInfo(local_info));
	json_object_object_add(json_system_info, "onvif_osd_info", json_GetOnvifOSDInfo(local_info));

	return json_system_info;
}


uint8 rsOnvifSysInfoWrite2JsonFile(RS_LOCAL_SYSTEM_INFO *local_info, const char *json_fn)
{
	ASSERT(json_fn);
	ASSERT(local_info);
	json_object *json_root;

	uint8 ret = 0;
/*begin to write 2 json file*/
	json_object *root_obj = json_object_new_object();
	json_object_object_add(root_obj, "change_counter", json_object_new_int(local_info->change_counter));
	json_object_object_add(root_obj, "state", json_object_new_int(local_info->state));
	json_object *config_object = json_GetConfigObject(local_info);
	json_object_object_add(root_obj, "system_info", config_object);

	json_object_to_file_ext(json_fn, root_obj,
			JSON_C_TO_STRING_PRETTY);

	json_object_put(root_obj);
	json_object_put(config_object);

	return 0;

}

static uint8 rsJsonReadOnvifScopesInfo(json_object *Scopes_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(Scopes_object, "profile");
	strcpy(local_info->system_info.onvif_special_info.scopes.profile, json_object_get_string(pval));
	RS_DBG("\t\t\tSystemInfo->SpecialInfo->Profile = %s\n", local_info->system_info.onvif_special_info.scopes.profile);

	json_object *array_type = json_object_object_get(Scopes_object, "type");
	uint8 i = 0;
	uint8 num = min(MAX_SCOPES_TYPE_NUM, json_object_array_length(array_type));
	for (i = 0; i < num; i++) {
		json_object *pval = json_object_array_get_idx(array_type, i);
		strcpy(local_info->system_info.onvif_special_info.scopes.type[i], json_object_get_string(pval));
		RS_DBG("\t\t\tSystemInfo->SpecialInfo->type[%d] = %s\n", i, local_info->system_info.onvif_special_info.scopes.type[i]);
	}

	pval = json_object_object_get(Scopes_object, "name");
	strcpy(local_info->system_info.onvif_special_info.scopes.name, json_object_get_string(pval));
	RS_DBG("\t\t\tSystemInfo->SpecialInfo->Name = %s\n", local_info->system_info.onvif_special_info.scopes.name);

	json_object *array_location = json_object_object_get(Scopes_object, "location");
	num = min(MAX_SCOPES_LOCATION_NUM, json_object_array_length(array_location));
	for (i = 0; i < num; i++) {
		json_object *pval = json_object_array_get_idx(array_location, i);
		strcpy(local_info->system_info.onvif_special_info.scopes.location[i], json_object_get_string(pval));
		RS_DBG("\t\t\tSystemInfo->SpecialInfo->location[%d] = %s\n", i, local_info->system_info.onvif_special_info.scopes.location[i]);
	}

	pval = json_object_object_get(Scopes_object, "hardware");
	strcpy(local_info->system_info.onvif_special_info.scopes.hardware, json_object_get_string(pval));
	RS_DBG("\t\t\tSystemInfo->SpecialInfo->hardware = %s\n", local_info->system_info.onvif_special_info.scopes.hardware);

	json_object *array_others = json_object_object_get(Scopes_object, "others");
	num = min(MAX_SCOPES_OTHER_NUM, json_object_array_length(array_others));
	for (i = 0; i < num; i++) {
		json_object *pval = json_object_array_get_idx(array_others, i);
		strcpy(local_info->system_info.onvif_special_info.scopes.others[i], json_object_get_string(pval));
		RS_DBG("\t\t\tSystemInfo->SpecialInfo->others[%d] = %s\n", i, local_info->system_info.onvif_special_info.scopes.others[i]);
	}

	return 0;
}

static uint8 rsJsonReadOnvifServiceNSInfo(json_object *Service_ns_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_service_ns = json_object_object_get(Service_ns_object, "service_namespace");
	uint8 i = 0;
	uint8 num = min(ONVIF_SERVICE_NUM, json_object_array_length(array_service_ns));
	for (i = 0; i < num; i++) {
		json_object *pval = json_object_array_get_idx(array_service_ns, i);
		strcpy(local_info->system_info.onvif_special_info.service_ns[i], json_object_get_string(pval));
		RS_DBG("\t\t\tSystemInfo->SpecialInfo->service_ns[%d] = %s\n", i, local_info->system_info.onvif_special_info.service_ns[i]);
	}

	return 0;
}

static uint8 rsJsonReadOnvifServiceNameInfo(json_object *Service_name_object, RS_LOCAL_SYSTEM_INFO *local_info)
{

	json_object *array_service_name = json_object_object_get(Service_name_object, "service_name");
	uint8 i = 0;
	uint8 num = min(ONVIF_SERVICE_NUM, json_object_array_length(array_service_name));
	for (i = 0; i < num; i++) {
		json_object *pval = json_object_array_get_idx(array_service_name, i);
		strcpy(local_info->system_info.onvif_special_info.service_name[i], json_object_get_string(pval));
		RS_DBG("\t\t\tSystemInfo->SpecialInfo->service_name[%d] = %s\n", i, local_info->system_info.onvif_special_info.service_name[i]);
	}

	return 0;
}

static uint8 rsJsonReadOnvifServiceVersionInfo(json_object *Service_version_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *array_service_version = json_object_object_get(Service_version_object, "service_version");
	uint8 i = 0;
	uint8 num = min(ONVIF_SERVICE_NUM, json_object_array_length(array_service_version));
	for (i = 0; i < num; i++) {
		json_object *pval = json_object_array_get_idx(array_service_version, i);
		local_info->system_info.onvif_special_info.service_version[i] = (uint16)json_object_get_int(pval);
		RS_DBG("\t\t\tSystemInfo->SpecialInfo->service_version[%d] = %d\n", i, local_info->system_info.onvif_special_info.service_version[i]);
	}

	return 0;
}

static uint8 rsJsonReadOnvifDeviceTypeInfo(json_object *device_type_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(device_type_object, "device_type");
	strcpy(local_info->system_info.onvif_special_info.device_type, json_object_get_string(pval));
	RS_DBG("\t\t\tSystemInfo->SpecialInfo->device_type = %s\n", local_info->system_info.onvif_special_info.device_type);

	return 0;
}

static uint8 rsJsonReadOnvifNetworkCapabilities(json_object *NetworkCap, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(NetworkCap, "IPFilter");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.ip_filter = (uint8)json_object_get_int(pval);
	RS_DBG("\t\t\t\tsystem_info.onvif_special_info.device_service_capabilities.network.ip_filter = %d\n", local_info->system_info.onvif_special_info.device_service_capabilities.network.ip_filter);

	pval = json_object_object_get(NetworkCap, "ZeroConfiguration");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.zero_configuration = (uint8)json_object_get_int(pval);
	RS_DBG("\t\t\t\tsystem_info.onvif_special_info.device_service_capabilities.network.zero_configuration = %d\n", local_info->system_info.onvif_special_info.device_service_capabilities.network.zero_configuration);

	pval = json_object_object_get(NetworkCap, "IPV6");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.ip_version6 = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(NetworkCap, "DynamicDNS");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.dynamic_dns = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(NetworkCap, "Dot11Configuration");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.dot_11_configuration = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(NetworkCap, "Dot1xConfigurations");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.dot1x_configurations = json_object_get_int(pval);

	pval = json_object_object_get(NetworkCap, "HostnameFromDHCP");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.hostname_from_dhcp = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(NetworkCap, "NTPServerNum");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.ntp_server_number = json_object_get_int(pval);

	pval = json_object_object_get(NetworkCap, "DHCP_V6");
	local_info->system_info.onvif_special_info.device_service_capabilities.network.dhcp_v6 = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(NetworkCap, "Any_attribute");
	strcpy(local_info->system_info.onvif_special_info.device_service_capabilities.network.any_attribute, json_object_get_string(pval));
	return 0;
}

static uint8 rsJsonReadOnvifSecurityCapabilities(json_object *SecurityCap, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = NULL;
	pval = json_object_object_get(SecurityCap, "TLS1_x002e0");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.tls1_x002e0 = (uint8)json_object_get_int(pval);
	RS_DBG("\t\t\t\tsystem_info.onvif_special_info.device_service_capabilities.security.tls1_x002e0 = %d\n", local_info->system_info.onvif_special_info.device_service_capabilities.security.tls1_x002e0);

	pval = json_object_object_get(SecurityCap, "TLS1_x002e1");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.tls1_x002e1 = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "TLS1_x002e2");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.tls1_x002e2 = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "OnboardKeyGen");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.onboard_key_generation = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "AccessPolicyConfig");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.access_policy_config = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "DefaultAccessPolicy");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.default_access_policy = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "Dot1x");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.dot1x = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "RemoteUserHandling");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.remote_user_handling = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "Xx002e509Token");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.x_x002e509_token = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "SamlToken");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.saml_token = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "KerberosToken");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.kerberos_token = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "UsernameToken");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.username_token = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "HttpDigest");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.http_digest = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "RelToken");
	local_info->system_info.onvif_special_info.device_service_capabilities.security.rel_token = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SecurityCap, "SupportedEAPMethods");
	strcpy(local_info->system_info.onvif_special_info.device_service_capabilities.security.supported_eap_methods, json_object_get_string(pval));
	pval = json_object_object_get(SecurityCap, "AnyAttribute");
	strcpy(local_info->system_info.onvif_special_info.device_service_capabilities.security.any_attribute, json_object_get_string(pval));

	return 0;
}

static uint8 rsJsonReadOnvifSystemCapabilities(json_object *SystemCap, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = NULL;
	pval = json_object_object_get(SystemCap, "DiscoveryResolve");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.discovery_resolve = (uint8)json_object_get_int(pval);
	RS_DBG("\t\t\t\tsystem_info.onvif_special_info.device_service_capabilities.system.discovery = %d\n", local_info->system_info.onvif_special_info.device_service_capabilities.system.discovery_resolve);
	pval = json_object_object_get(SystemCap, "DiscoveryBye");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.discovery_bye = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "RemoteDiscovery");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.remote_discovery = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "SystemBackup");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.system_backup = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "SystemLog");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.system_logging = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "FWUpgrade");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.firmware_upgrade = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "HttpFWUpgrate");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.http_firmware_upgrade = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "HttpSysBackup");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.http_system_backup = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "HttpSysLog");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.http_system_logging = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "HttpSupportInfo");
	local_info->system_info.onvif_special_info.device_service_capabilities.system.http_support_information = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(SystemCap, "AnyAttribute");
	strcpy(local_info->system_info.onvif_special_info.device_service_capabilities.system.any_attribute, json_object_get_string(pval));

	return 0;

}

static uint8 rsJsonReadOnvifIOCapabilities(json_object *IOCap, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(IOCap, "InputConnectors");
	local_info->system_info.onvif_special_info.device_service_capabilities.io.input_connectors = json_object_get_int(pval);
	RS_DBG("\t\t\t\tsystem_info.onvif_special_info.device_service_capabilities.io.input_connectors = %d\n", local_info->system_info.onvif_special_info.device_service_capabilities.io.input_connectors);

	pval = json_object_object_get(IOCap, "RelayOutputs");
	local_info->system_info.onvif_special_info.device_service_capabilities.io.relay_outputs = json_object_get_int(pval);
	return 0;
}

static uint8 rsJsonReadOnvifMiscCapabilities(json_object *MiscCap, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(MiscCap, "AuxiliaryCmds");
	strcpy(local_info->system_info.onvif_special_info.device_service_capabilities.misc.auxiliary_commands, json_object_get_string(pval));
	RS_DBG("\t\t\t\tsystem_info.onvif_special_info.device_service_capabilities.misc.auxiliary_commands = %s\n", local_info->system_info.onvif_special_info.device_service_capabilities.misc.auxiliary_commands);

	pval = json_object_object_get(MiscCap, "AnyAttribute");
	strcpy(local_info->system_info.onvif_special_info.device_service_capabilities.misc.any_attribute, json_object_get_string(pval));

	return 0;
}

static uint8 rsJsonReadOnvifDevServiceCapInfo(json_object *device_service_cap_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *NetworkCap = json_object_object_get(device_service_cap_object, "network_capabilities");
	rsJsonReadOnvifNetworkCapabilities(NetworkCap, local_info);

	json_object *SecurityCap = json_object_object_get(device_service_cap_object, "security_capabilities");
	rsJsonReadOnvifSecurityCapabilities(SecurityCap, local_info);

	json_object *SystemCap = json_object_object_get(device_service_cap_object, "system_capabilities");
	rsJsonReadOnvifSystemCapabilities(SystemCap, local_info);

	json_object *IOCap = json_object_object_get(device_service_cap_object, "io_capabilities");
	rsJsonReadOnvifIOCapabilities(IOCap, local_info);

	json_object *MiscCap = json_object_object_get(device_service_cap_object, "misc_capabilities");
	rsJsonReadOnvifMiscCapabilities(MiscCap, local_info);
}

static uint8 rsJsonReadOnvifEventServiceCapInfo(json_object *event_service_cap_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(event_service_cap_object, "WSSubscriptionPolicySupport");
	local_info->system_info.onvif_special_info.event_service_capabilities.ws_subscription_policy_support = (uint8)json_object_get_int(pval);
	RS_DBG("\t\t\tsystem_info.onvif_special_info.event_service_capabilities.ws_subscription_policy_support = %d\n", local_info->system_info.onvif_special_info.event_service_capabilities.ws_subscription_policy_support);

	pval = json_object_object_get(event_service_cap_object, "WSPullPointSupprot");
	local_info->system_info.onvif_special_info.event_service_capabilities.ws_pull_point_support = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(event_service_cap_object, "WSPausableSubmgrSupport");
	local_info->system_info.onvif_special_info.event_service_capabilities.ws_pausable_submgr_support = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(event_service_cap_object, "PersistentNotifactionStorage");
	local_info->system_info.onvif_special_info.event_service_capabilities.persistentNotificationStorage = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(event_service_cap_object, "MaxProducers");
	local_info->system_info.onvif_special_info.event_service_capabilities.max_notificationproducers = json_object_get_int(pval);
	pval = json_object_object_get(event_service_cap_object, "MaxPullPoints");
	local_info->system_info.onvif_special_info.event_service_capabilities.max_pull_points = json_object_get_int(pval);
	RS_DBG("\t\t\tsystem_info.onvif_special_info.event_service_capabilities.max_pull_points = %d\n", local_info->system_info.onvif_special_info.event_service_capabilities.max_pull_points);

	return 0;
}

static uint8 rsJsonReadOnvifSpecialInfo(json_object *SpecialInfo_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *Scopes_object = json_object_object_get(SpecialInfo_object, "onvif_scopes");
	rsJsonReadOnvifScopesInfo(Scopes_object, local_info);

	json_object *Service_ns_object = json_object_object_get(SpecialInfo_object, "onvif_service_namespace");
	rsJsonReadOnvifServiceNSInfo(Service_ns_object, local_info);

	json_object *Service_name_object = json_object_object_get(SpecialInfo_object, "onvif_service_name");
	rsJsonReadOnvifServiceNameInfo(Service_name_object, local_info);

	json_object *Service_version_object = json_object_object_get(SpecialInfo_object, "onvif_service_version");
	rsJsonReadOnvifServiceVersionInfo(Service_version_object, local_info);

	json_object *device_type_object = json_object_object_get(SpecialInfo_object, "onvif_device_type");
	rsJsonReadOnvifDeviceTypeInfo(device_type_object, local_info);

	json_object *device_service_cap_object = json_object_object_get(SpecialInfo_object, "onvif_device_service_capabilities");
	rsJsonReadOnvifDevServiceCapInfo(device_service_cap_object, local_info);
	json_object *event_service_cap_object = json_object_object_get(SpecialInfo_object, "onvif_event_service_capabilities");
	rsJsonReadOnvifEventServiceCapInfo(event_service_cap_object, local_info);
	/*
	json_object_object_add(json_ret, "onvif_scopes", json_GetScopes(local_info));
	json_object_object_add(json_ret, "onvif_service_namespace", json_GetServiceNameSpace(local_info));
	json_object_object_add(json_ret, "onvif_service_name", json_GetServiceName(local_info));
	json_object_object_add(json_ret, "onvif_service_version", json_GetServiceVersion(local_info));
	json_object_object_add(json_ret, "onvif_device_type", json_GetDeviceType(local_info));
	json_object_object_add(json_ret, "onvif_device_service_capabilities", json_GetDeviceServiceCapabilities(local_info));
	json_object_object_add(json_ret, "onvif_event_service_capabilities", json_GetEventServiceCapabilities(local_info));
	*/

}

static uint8 rsJsonReadOnvifDeviceInfo(json_object *DeviceInfo_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(DeviceInfo_object, "Manufacturer");
	strcpy(local_info->system_info.device_info.manufacturer, json_object_get_string(pval));
	pval = json_object_object_get(DeviceInfo_object, "Model");
	strcpy(local_info->system_info.device_info.model, json_object_get_string(pval));
	pval = json_object_object_get(DeviceInfo_object, "FWVersion");
	strcpy(local_info->system_info.device_info.firmware_version, json_object_get_string(pval));
	pval = json_object_object_get(DeviceInfo_object, "SN");
	strcpy(local_info->system_info.device_info.serial_number, json_object_get_string(pval));
	pval = json_object_object_get(DeviceInfo_object, "HWID");
	strcpy(local_info->system_info.device_info.hardware_id, json_object_get_string(pval));
	RS_DBG("\t\t\tsystem_info.device_info.hardware_id = %s\n", local_info->system_info.device_info.hardware_id);

	return 0;
}

static uint8 rsJsonReadOnvifDateTimeInfo(json_object *DateTimeInfo_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = json_object_object_get(DateTimeInfo_object, "NTPTime");
	local_info->system_info.date_time_settings.time_ntp = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(DateTimeInfo_object, "DayLightSavings");
	local_info->system_info.date_time_settings.day_light_savings = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(DateTimeInfo_object, "TimeZone");
	strcpy(local_info->system_info.date_time_settings.time_zone, json_object_get_string(pval));

	RS_DBG("\t\t\tsystem_info.date_time_settings.time_zone = %s\n", local_info->system_info.date_time_settings.time_zone);
	return 0;
}

static uint8 rsJsonReadOnvifSystemUris(json_object *pSysUris, RS_LOCAL_SYSTEM_INFO *local_info)
{
	/*json_object_object_add(json_ret, "SystemUris", json_GetSystemUris(local_info));*/
	json_object *pval = json_object_object_get(pSysUris, "SystemLogUri");
	strcpy(local_info->system_info.network_config.system_uris.system_log_uri, json_object_get_string(pval));
	pval = json_object_object_get(pSysUris, "SystemBackupUri");
	strcpy(local_info->system_info.network_config.system_uris.system_backup_uri, json_object_get_string(pval));
	pval = json_object_object_get(pSysUris, "SupportInfoUri");
	strcpy(local_info->system_info.network_config.system_uris.support_info_uri, json_object_get_string(pval));
	pval = json_object_object_get(pSysUris, "SystemRestoreUri");
	strcpy(local_info->system_info.network_config.system_uris.system_restore_uri, json_object_get_string(pval));
	return 0;
}

static uint8 rsJsonReadOnvifAccounts(json_object *pAccounts, RS_LOCAL_SYSTEM_INFO *local_info)
{
	/*json_object_object_add(json_ret, "Accounts", json_GetAccounts(local_info));*/
	json_object *array_accounts = pAccounts;
	uint8 i = 0;
	uint8 num = min(MAX_ACCOUNT_NUM, json_object_array_length(array_accounts));
	for (i = 0; i < num; i++) {
		json_object *pval = json_object_array_get_idx(array_accounts, i);
		json_object *pName = json_object_object_get(pval, "Name");
		strcpy(local_info->system_info.network_config.accounts[i].user, json_object_get_string(pName));

		json_object *pPwd = json_object_object_get(pval, "Pwd");
		strcpy(local_info->system_info.network_config.accounts[i].password, json_object_get_string(pPwd));

		json_object *pAuthority = json_object_object_get(pval, "Authority");
		local_info->system_info.network_config.accounts[i].authority = (int8)json_object_get_int(pAuthority);

		json_object *pFixed = json_object_object_get(pval, "Fixed");
		local_info->system_info.network_config.accounts[i].fixed = (uint8)json_object_get_int(pFixed);
	}
	return 0;
}

static uint8 rsJsonReadOnvifHostname(json_object *pHostname, RS_LOCAL_SYSTEM_INFO *local_info)
{
	/*json_object_object_add(json_ret, "Hostname", json_GetHostname(local_info));*/
	json_object *pval = json_object_object_get(pHostname, "Name");
	strcpy(local_info->system_info.network_config.host_name.name, json_object_get_string(pval));
	pval = json_object_object_get(pHostname, "FromDHCP");
	local_info->system_info.network_config.host_name.dhcp_enable = (uint8)json_object_get_int(pval);

	return 0;
}

static uint8 rsJsonReadOnvifNetworkInterface(json_object *pNetworkInterface, RS_LOCAL_SYSTEM_INFO *local_info)
{
	/*json_object_object_add(json_ret, "NetworkInterface", json_GetNetworkInterface(local_info));*/
	uint8 i = 0;
	uint8 num = min(MAX_NETWORK_INTERFACE_NUM, json_object_array_length(pNetworkInterface));

	for (i = 0; i < num; i++) {
		json_object *json_node = json_object_array_get_idx(pNetworkInterface, i);
		json_object *pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.network_config.interfaces[i].token, json_object_get_string(pval));

		char mac_str[20] = {0};
		pval = json_object_object_get(json_node, "Mac");
		strcpy(mac_str, json_object_get_string(pval));
		char temp[5] = {0};
		uint8 j = 0;
		for (j = 0; j < 6; j++) {
			temp[0] = '0';
			temp[1] = 'x';
			temp[2] = mac_str[3*j];
			temp[3] = mac_str[3*j+1];

			local_info->system_info.network_config.interfaces[i].mac[j] = strtol(temp, NULL, 16);
		}

		pval = json_object_object_get(json_node, "Mtu");
		local_info->system_info.network_config.interfaces[i].mtu = json_object_get_int(pval);

		pval = json_object_object_get(json_node, "Enable");
		local_info->system_info.network_config.interfaces[i].enabled = (uint8)json_object_get_int(pval);

		pval = json_object_object_get(json_node, "IP");
		inet_aton(json_object_get_string(pval), &local_info->system_info.network_config.interfaces[i].ip);

		pval = json_object_object_get(json_node, "DHCP_Enable");
		local_info->system_info.network_config.interfaces[i].dhcp_enable = (uint8)json_object_get_int(pval);
	}

	return 0;
}

static uint8 rsJsonReadOnvifDNS(json_object *pDNS, RS_LOCAL_SYSTEM_INFO *local_info)
{
	/*json_object_object_add(json_ret, "DNS", json_GetDNSConfig(local_info));*/
	json_object *pval = json_object_object_get(pDNS, "DHCPEnable");
	local_info->system_info.network_config.dns.dhcp_enable = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(pDNS, "ManulDNS");
	uint8 i = 0;
	uint8 num = min(MAX_DNS_NUM, json_object_array_length(pval));
	for (i = 0; i < num; i++) {
		json_object *pNode = json_object_array_get_idx(pval, i);
		inet_aton(json_object_get_string(pNode), &local_info->system_info.network_config.dns.manul_dns[i]);
	}
	pval = json_object_object_get(pDNS, "DHCPDNS");
	num = min(MAX_DNS_NUM, json_object_array_length(pval));
	for (i = 0; i < num; i++) {
		json_object *pNode = json_object_array_get_idx(pval, i);
		inet_aton(json_object_get_string(pNode), &local_info->system_info.network_config.dns.dhcp_dns[i]);
	}
	pval = json_object_object_get(pDNS, "SearchDomain");
	strcpy(local_info->system_info.network_config.dns.search_domain, json_object_get_string(pval));

	return 0;
}

static uint8 rsJsonReadOnvifNTP(json_object *pNTP, RS_LOCAL_SYSTEM_INFO *local_info)
{
	/*json_object_object_add(json_ret, "NTP", json_GetNTPConfig(local_info));*/
	uint8 i = 0;
	uint8 num = 0;
	json_object *pval = json_object_object_get(pNTP, "DHCPEnable");
	local_info->system_info.network_config.ntp.dhcp_enable = (uint8)json_object_get_int(pval);

	pval = json_object_object_get(pNTP, "ManulNTP");
	num = min(MAX_NTP_NUM, json_object_array_length(pval));
	for (i = 0; i < num; i++) {
		json_object *pNode = json_object_array_get_idx(pval, i);
		inet_aton(json_object_get_string(pNode), &local_info->system_info.network_config.ntp.manual_ntp[i]);
	}

	pval = json_object_object_get(pNTP, "DHCPNTP");
	num = min(MAX_NTP_NUM, json_object_array_length(pval));
	for (i = 0; i < num; i++) {
		json_object *pNode = json_object_array_get_idx(pval, i);
		inet_aton(json_object_get_string(pNode), &local_info->system_info.network_config.ntp.dhcp_ntp[i]);
	}

	return 0;
}

static uint8 rsJsonReadOnvifNetworkProtocol(json_object *pNetworkProtocol, RS_LOCAL_SYSTEM_INFO *local_info)
{
	/*json_object_object_add(json_ret, "NetworkProtocol", json_GetNetworkProtocolConfig(local_info));*/
	uint8 i = 0;
	uint8 j = 0;
	uint8 num = 0;

	num = min(NETWORK_PROTOCOL_NUM, json_object_array_length(pNetworkProtocol));
	for (i = 0; i < num; i++) {
		json_object *json_node = json_object_array_get_idx(pNetworkProtocol, i);
		json_object *pName = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.network_config.protocol[i].name, json_object_get_string(pName));
		json_object *pEnable = json_object_object_get(json_node, "Enable");
		local_info->system_info.network_config.protocol[i].enabled = (uint8)json_object_get_int(pEnable);

		for (j = 0; j < MAX_PROTOCOL_PORT_NUM; j++) {
			char port_str[10] = {0};
			sprintf(port_str, "Port_%d", j);
			json_object *json_obj = json_object_object_get(json_node, port_str);
			if (json_obj != NULL) {
				local_info->system_info.network_config.protocol[i].port[j] = json_object_get_int(json_obj);
			} else
				break;
		}

	}

	return 0;
}

static uint8 rsJsonReadOnvifNetworkInfo(json_object *NetworkInfo_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pval = NULL;

	json_object *pSysUris = json_object_object_get(NetworkInfo_object, "SystemUris");
	rsJsonReadOnvifSystemUris(pSysUris, local_info);

	pval = json_object_object_get(NetworkInfo_object, "DiscoveryMode");
	local_info->system_info.network_config.discovery_mode = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(NetworkInfo_object, "RemoteDiscoveryMode");
	local_info->system_info.network_config.remote_discovery_mode = (uint8)json_object_get_int(pval);
	pval = json_object_object_get(NetworkInfo_object, "DiscoveryProxyAddress");
	strcpy(local_info->system_info.network_config.discover_proxy_address, json_object_get_string(pval));
	RS_DBG("\t\tsystem_info.network_config.discover_proxy_address = %s\n", local_info->system_info.network_config.discover_proxy_address);
	json_object *pAccounts = json_object_object_get(NetworkInfo_object, "Accounts");
	rsJsonReadOnvifAccounts(pAccounts, local_info);

	json_object *pHostname = json_object_object_get(NetworkInfo_object, "Hostname");
	rsJsonReadOnvifHostname(pHostname, local_info);

	json_object *pNetworkInterface = json_object_object_get(NetworkInfo_object, "NetworkInterface");
	rsJsonReadOnvifNetworkInterface(pNetworkInterface, local_info);

	pval = json_object_object_get(NetworkInfo_object, "NetInterfaceNum");
	local_info->system_info.network_config.interface_num = json_object_get_int(pval);
	pval = json_object_object_get(NetworkInfo_object, "NetInterfaceSelectedIdx");
	local_info->system_info.network_config.interface_idx = json_object_get_int(pval);
	RS_DBG("\t\tsystem_info.network_config.interface_idx = %d\n", local_info->system_info.network_config.interface_idx);

	pval = json_object_object_get(NetworkInfo_object, "Netmask");
	RS_DBG("\t\tNetmask = %s\n", json_object_get_string(pval));
	inet_aton(json_object_get_string(pval), &local_info->system_info.network_config.netmask);
	pval = json_object_object_get(NetworkInfo_object, "Gateway");
	RS_DBG("\t\tGateway = %s\n", json_object_get_string(pval));
	inet_aton(json_object_get_string(pval), &local_info->system_info.network_config.gateway);

	json_object *pDNS = json_object_object_get(NetworkInfo_object, "DNS");
	rsJsonReadOnvifDNS(pDNS, local_info);
	RS_DBG("\t\tDNs\n");

	json_object *pNTP = json_object_object_get(NetworkInfo_object, "NTP");
	rsJsonReadOnvifNTP(pNTP, local_info);

	json_object *pNetworkProtocol = json_object_object_get(NetworkInfo_object, "NetworkProtocol");
	rsJsonReadOnvifNetworkProtocol(pNetworkProtocol, local_info);
	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadVideoSourceConfig(json_object *pVSC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	RS_DBG("In func\n");
	num = min(MAX_MEDIA_VSC_CONFIG,  json_object_array_length(pVSC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pVSC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.vsc[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.vsc[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "VideoSourceToken");
		strcpy(local_info->system_info.media_config.vsc[idx].source_token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.vsc[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.vsc[idx].use_count = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "BoundsX");
		local_info->system_info.media_config.vsc[idx].bounds_x = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "BoundsY");
		local_info->system_info.media_config.vsc[idx].bounds_y = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "BoundsWidth");
		local_info->system_info.media_config.vsc[idx].bounds_width = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "BoundsHeight");
		local_info->system_info.media_config.vsc[idx].bounds_height = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "RotateMode");
		local_info->system_info.media_config.vsc[idx].rotate_mode = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "RotateDegree");
		local_info->system_info.media_config.vsc[idx].rotate_degree = json_object_get_int(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadVideoEncoderConfig(json_object *pVEC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_VEC_CONFIG,  json_object_array_length(pVEC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pVEC, idx);
		json_object *pval = json_object_object_get(json_node, "Vec_Path");
		strcpy(local_info->system_info.media_config.vec[idx].vec_path, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.vec[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.vec[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.vec[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.vec[idx].use_count = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "EncodingFormat");
		local_info->system_info.media_config.vec[idx].encoding_format = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "EncodingWidth");
		local_info->system_info.media_config.vec[idx].encoding_width = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "EncodingHeight");
		local_info->system_info.media_config.vec[idx].encoding_height = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "Quality");
		local_info->system_info.media_config.vec[idx].quality = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "FrameRateLimit");
		local_info->system_info.media_config.vec[idx].frame_rate_limit = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "EncodingInterval");
		local_info->system_info.media_config.vec[idx].encoding_interval = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "BitrateLimit");
		local_info->system_info.media_config.vec[idx].bitrate_limit = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "H264GovLength");
		local_info->system_info.media_config.vec[idx].h264_gov_length = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "H264Profile");
		local_info->system_info.media_config.vec[idx].h264_profile = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastIPType");
		local_info->system_info.media_config.vec[idx].multicast_ip_type = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastIPAddr");
		strcpy(local_info->system_info.media_config.vec[idx].multicast_ip_addr, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "MulticastIPPort");
		local_info->system_info.media_config.vec[idx].multicast_port = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastAutoStart");
		local_info->system_info.media_config.vec[idx].multicast_auto_start = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastTTL");
		local_info->system_info.media_config.vec[idx].multicast_ttl = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "SessionTimeout");
		local_info->system_info.media_config.vec[idx].session_timeout = json_object_get_int64(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadAudioSourceConfig(json_object *pASC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_ASC_CONFIG,  json_object_array_length(pASC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pASC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.asc[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.asc[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "AudioSourceToken");
		strcpy(local_info->system_info.media_config.asc[idx].source_token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.asc[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.asc[idx].use_count = json_object_get_int(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadAudioEncoderConfig(json_object *pAEC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_AEC_CONFIG,  json_object_array_length(pAEC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pAEC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.aec[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.aec[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.aec[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.aec[idx].use_count = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "EncodingFormat");
		local_info->system_info.media_config.aec[idx].encoding_format = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "Bitrate");
		local_info->system_info.media_config.aec[idx].bit_rate = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "SampleRate");
		local_info->system_info.media_config.aec[idx].sample_rate = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastIPType");
		local_info->system_info.media_config.aec[idx].multicast_ip_type = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastIPAddr");
		strcpy(local_info->system_info.media_config.aec[idx].multicast_ip_addr, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "MulticastIPPort");
		local_info->system_info.media_config.aec[idx].port = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastAutoStart");
		local_info->system_info.media_config.aec[idx].auto_start = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastTTL");
		local_info->system_info.media_config.aec[idx].ttl = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "SessionTimeout");
		local_info->system_info.media_config.aec[idx].session_timeout = json_object_get_int64(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadAudioDecoderConfig(json_object *pADC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_ADC_CONFIG,  json_object_array_length(pADC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pADC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.adc[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.adc[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.adc[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.adc[idx].use_count = json_object_get_int(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadAudioOutputConfig(json_object *pAOC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_AOC_CONFIG,  json_object_array_length(pAOC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pAOC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.aoc[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.aoc[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "OutputToken");
		strcpy(local_info->system_info.media_config.aoc[idx].output_token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.aoc[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.aoc[idx].use_count = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "OutputLevel");
		local_info->system_info.media_config.aoc[idx].output_level = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "SendPrimacy");
		strcpy(local_info->system_info.media_config.aoc[idx].send_primacy, json_object_get_string(pval));
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadPTZConfig(json_object *pPTZC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_PTZC_CONFIG,  json_object_array_length(pPTZC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pPTZC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.ptzc[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.ptzc[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.ptzc[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.ptzc[idx].use_count = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "DefSpeedPtX");
		local_info->system_info.media_config.ptzc[idx].default_speed_pt_x  = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "DefSpeedPtY");
		local_info->system_info.media_config.ptzc[idx].default_speed_pt_y  = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "DefSpeedZoomX");
		local_info->system_info.media_config.ptzc[idx].default_speed_zoom_x  = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "PtLimitsXmax");
		local_info->system_info.media_config.ptzc[idx].pt_limits_x_max = json_object_get_int64(pval);
		pval = json_object_object_get(json_node, "PtLimitsXmin");
		local_info->system_info.media_config.ptzc[idx].pt_limits_x_min = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "PtLimitsYmax");
		local_info->system_info.media_config.ptzc[idx].pt_limits_y_max = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "PtLimitsYmin");
		local_info->system_info.media_config.ptzc[idx].pt_limits_y_min = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "ZoomLimitsXmax");
		local_info->system_info.media_config.ptzc[idx].zoom_limits_x_max = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "ZoomLimitsXmin");
		local_info->system_info.media_config.ptzc[idx].zoom_limits_x_min = (float)json_object_get_double(pval);
		pval = json_object_object_get(json_node, "PTCtrlEFlipMode");
		local_info->system_info.media_config.ptzc[idx].ptctrl_eflip_mode = json_object_get_int(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadVideoAnalyticsConfig(json_object *pVAC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_VAC_CONFIG,  json_object_array_length(pVAC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pVAC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.vac[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.vac[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.vac[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.vac[idx].use_count = json_object_get_int(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadMetadataConfig(json_object *pMetaDataC, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 idx = 0;
	uint8 num = 0;

	num = min(MAX_MEDIA_MC_CONFIG,  json_object_array_length(pMetaDataC));

	for (idx = 0; idx < num; idx++) {
		json_object *json_node = json_object_array_get_idx(pMetaDataC, idx);
		json_object *pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_config.mc[idx].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_config.mc[idx].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "ForcePersistence");
		local_info->system_info.media_config.mc[idx].force_persistence = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "UseCount");
		local_info->system_info.media_config.mc[idx].use_count = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "PTZStatus");
		strcpy(local_info->system_info.media_config.mc[idx].ptz_status, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "PTZPosition");
		strcpy(local_info->system_info.media_config.mc[idx].ptz_position, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "EventFilter");
		strcpy(local_info->system_info.media_config.mc[idx].event_filter, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "EventSubPolicy");
		strcpy(local_info->system_info.media_config.mc[idx].event_subscription_policy, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Analytics");
		strcpy(local_info->system_info.media_config.mc[idx].analytics, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "H264GovLength");
		local_info->system_info.media_config.vec[idx].h264_gov_length = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "H264Profile");
		local_info->system_info.media_config.vec[idx].h264_profile = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastIPType");
		local_info->system_info.media_config.mc[idx].multicast_ip_type = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastIPAddr");
		strcpy(local_info->system_info.media_config.mc[idx].multicast_ip_addr, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "MulticastIPPort");
		local_info->system_info.media_config.mc[idx].multicast_port = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastAutoStart");
		local_info->system_info.media_config.mc[idx].multicast_auto_start = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "MulticastTTL");
		local_info->system_info.media_config.mc[idx].multicast_ttl = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "SessionTimeout");
		local_info->system_info.media_config.mc[idx].session_timeout = json_object_get_int64(pval);
	}

	return 0;
}


static uint8 rsJsonReadOnvifMediaInfo(json_object *MediaInfo_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 i = 0;
	uint8 num = min(MAX_MEDIA_PROFILE, json_object_array_length(MediaInfo_object));
	for (i = 0; i < num; i++) {
		json_object *json_node = json_object_array_get_idx(MediaInfo_object, i);
		json_object *pval = json_object_object_get(json_node, "Token");
		strcpy(local_info->system_info.media_profile[i].token, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Name");
		strcpy(local_info->system_info.media_profile[i].name, json_object_get_string(pval));
		pval = json_object_object_get(json_node, "Fixed");
		local_info->system_info.media_profile[i].fixed = (uint8)json_object_get_int(pval);
		pval = json_object_object_get(json_node, "vsc_index");
		local_info->system_info.media_profile[i].vsc_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "vec_index");
		local_info->system_info.media_profile[i].vec_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "asc_index");
		local_info->system_info.media_profile[i].asc_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "aec_index");
		local_info->system_info.media_profile[i].aec_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "adc_index");
		local_info->system_info.media_profile[i].adc_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "aoc_index");
		local_info->system_info.media_profile[i].aoc_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "ptzc_index");
		local_info->system_info.media_profile[i].ptzc_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "vac_index");
		local_info->system_info.media_profile[i].vac_index = json_object_get_int(pval);
		pval = json_object_object_get(json_node, "mc_index");
		local_info->system_info.media_profile[i].mc_index = json_object_get_int(pval);
	}

	RS_DBG("Leaving Func\n");
	return 0;
}

static uint8 rsJsonReadOnvifMediaConfigInfo(json_object *MediaConfigInfo_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	json_object *pVSC = json_object_object_get(MediaConfigInfo_object, "VideoSourceConfiguraton");
	rsJsonReadVideoSourceConfig(pVSC, local_info);
	json_object *pVEC = json_object_object_get(MediaConfigInfo_object, "VideoEncoderConfiguraton");
	rsJsonReadVideoEncoderConfig(pVEC, local_info);
	json_object *pASC = json_object_object_get(MediaConfigInfo_object, "AudioSourceConfiguraton");
	rsJsonReadAudioSourceConfig(pASC, local_info);
	json_object *pAEC = json_object_object_get(MediaConfigInfo_object, "AudioEncoderConfiguraton");
	rsJsonReadAudioEncoderConfig(pAEC, local_info);
	json_object *pADC = json_object_object_get(MediaConfigInfo_object, "AudioDecoderConfiguraton");
	rsJsonReadAudioDecoderConfig(pADC, local_info);
	json_object *pAOC = json_object_object_get(MediaConfigInfo_object, "AudioOutputConfiguraton");
	rsJsonReadAudioOutputConfig(pAOC, local_info);
	json_object *pPTZC = json_object_object_get(MediaConfigInfo_object, "PTZConfiguraton");
	rsJsonReadPTZConfig(pPTZC, local_info);
	json_object *pVAC = json_object_object_get(MediaConfigInfo_object, "VideoAnalyticsConfiguraton");
	rsJsonReadVideoAnalyticsConfig(pVAC, local_info);
	json_object *pMetaDataC = json_object_object_get(MediaConfigInfo_object, "MetadataConfiguraton");
	rsJsonReadMetadataConfig(pMetaDataC, local_info);

	RS_DBG("Leaving Func\n");
	return 0;
}


static uint8 rsJsonReadOnvifOSDInfo(json_object *OSDInfo_object, RS_LOCAL_SYSTEM_INFO *local_info)
{
	uint8 i = 0;
	uint8 num = min(MAX_OSD_NUM, json_object_array_length(OSDInfo_object));
	for (i = 0; i < num; i++) {
		json_object *pNode = json_object_array_get_idx(OSDInfo_object, i);
		json_object *pval = json_object_object_get(pNode, "Token");
		strcpy(local_info->system_info.osd_config[i].token, json_object_get_string(pval));
		pval = json_object_object_get(pNode, "String");
		strcpy(local_info->system_info.osd_config[i].osd_string_buf, json_object_get_string(pval));
		pval = json_object_object_get(pNode, "OSD_CLR_X");
		local_info->system_info.osd_config[i].osd_color_x = (float)json_object_get_double(pval);

		pval = json_object_object_get(pNode, "OSD_CLR_Y");
		local_info->system_info.osd_config[i].osd_color_y = (float)json_object_get_double(pval);

		pval = json_object_object_get(pNode, "OSD_CLR_Z");
		local_info->system_info.osd_config[i].osd_color_z = (float)json_object_get_double(pval);

		pval = json_object_object_get(pNode, "OSD_CLR_Type");
		local_info->system_info.osd_config[i].osd_color_type = (uint8)json_object_get_int(pval);

		pval = json_object_object_get(pNode, "OSDTransparent");
		local_info->system_info.osd_config[i].osd_transparent = (uint8)json_object_get_int(pval);


		pval = json_object_object_get(pNode, "OSD_BKCLR_X");
		local_info->system_info.osd_config[i].osd_bk_color_x = (float)json_object_get_double(pval);

		pval = json_object_object_get(pNode, "OSD_BKCLR_Y");
		local_info->system_info.osd_config[i].osd_bk_color_y = (float)json_object_get_double(pval);

		pval = json_object_object_get(pNode, "OSD_BKCLR_Z");
		local_info->system_info.osd_config[i].osd_bk_color_z = (float)json_object_get_double(pval);

		pval = json_object_object_get(pNode, "OSD_BKCLR_Type");
		local_info->system_info.osd_config[i].osd_bk_color_type = (uint8)json_object_get_int(pval);

		pval = json_object_object_get(pNode, "OSDBKTransparent");
		local_info->system_info.osd_config[i].osd_bk_transparent = (uint8)json_object_get_int(pval);


		pval = json_object_object_get(pNode, "OSDFontSize");
		local_info->system_info.osd_config[i].osd_font_size = json_object_get_int(pval);

		pval = json_object_object_get(pNode, "OSDType");
		local_info->system_info.osd_config[i].osd_type = json_object_get_int(pval);

		pval = json_object_object_get(pNode, "OSDStartX");
		local_info->system_info.osd_config[i].osd_start_x = json_object_get_int(pval);
		pval = json_object_object_get(pNode, "OSDStartY");
		local_info->system_info.osd_config[i].osd_start_y = json_object_get_int(pval);
		pval = json_object_object_get(pNode, "OSDEndX");
		local_info->system_info.osd_config[i].osd_end_x = json_object_get_int(pval);
		pval = json_object_object_get(pNode, "OSDEndY");
		local_info->system_info.osd_config[i].osd_end_y = json_object_get_int(pval);
	}

	return 0;
}


static uint8 rsJsonReadConfig(json_object *json_system_info, RS_LOCAL_SYSTEM_INFO *local_info)
{

	json_object *pval = NULL;

	pval = json_object_object_get(json_system_info, "UUID");
	strcpy(local_info->system_info.uuid, json_object_get_string(pval));
	RS_DBG("\tUUID is %s\n", local_info->system_info.uuid);

	json_object *SpecialInfo_object = json_object_object_get(json_system_info, "onvif_special_info");
	rsJsonReadOnvifSpecialInfo(SpecialInfo_object, local_info);

	json_object *DeviceInfo_object = json_object_object_get(json_system_info, "onvif_device_info");
	rsJsonReadOnvifDeviceInfo(DeviceInfo_object, local_info);

	json_object *DateTimeInfo_object = json_object_object_get(json_system_info, "onvif_datetime_info");
	rsJsonReadOnvifDateTimeInfo(DateTimeInfo_object, local_info);

	json_object *NetworkInfo_object = json_object_object_get(json_system_info, "onvif_network_info");
	rsJsonReadOnvifNetworkInfo(NetworkInfo_object, local_info);

	json_object *MediaInfo_object = json_object_object_get(json_system_info, "onvif_media_info");
	rsJsonReadOnvifMediaInfo(MediaInfo_object, local_info);

	json_object *MediaConfigInfo_object = json_object_object_get(json_system_info, "onvif_media_config_info");
	rsJsonReadOnvifMediaConfigInfo(MediaConfigInfo_object, local_info);

	json_object *OSDInfo_object = json_object_object_get(json_system_info, "onvif_osd_info");
	rsJsonReadOnvifOSDInfo(OSDInfo_object, local_info);

/*
	json_object_object_add(json_system_info, "onvif_special_info", json_GetOnvifSpecialInfo(local_info));
	json_object_object_add(json_system_info, "onvif_device_info", json_GetOnvifDeviceInfo(local_info));
	json_object_object_add(json_system_info, "onvif_datetime_info", json_GetOnvifDatetimeInfo(local_info));
	json_object_object_add(json_system_info, "onvif_network_info", json_GetOnvifNetworkInfo(local_info));
	json_object_object_add(json_system_info, "onvif_media_info", json_GetOnvifMediaInfo(local_info));
	json_object_object_add(json_system_info, "onvif_osd_info", json_GetOnvifOSDInfo(local_info));
*/

	return 0;
}

uint8 rsOnvifSysInfoReadFromJsonFile(RS_LOCAL_SYSTEM_INFO *local_info, const char *json_fn)
{

	uint8 ret = 0;
	ASSERT(json_fn);
	ASSERT(local_info);
	json_object *root_obj =  json_object_from_file(json_fn);
	if (!root_obj) {
		RS_DBG("cannot find file\n");
		return 1;
	}
	json_object *pval = NULL;

	pval = json_object_object_get(root_obj, "change_counter");
	local_info->change_counter = json_object_get_int(pval);
	RS_DBG("\n *****************************************************************************************************\n");
	RS_DBG("*   The Following info is parsed from config.json   *\n");
	RS_DBG("*****************************************************************************************************\n");
	RS_DBG("change counter is %d\n", local_info->change_counter);
	json_object_put(pval);

	pval = json_object_object_get(root_obj, "state");
	local_info->state = json_object_get_int(pval);
	RS_DBG("state is %d\n", local_info->state);
	json_object_put(pval);

	json_object *config_object = json_object_object_get(root_obj, "system_info");
	rsJsonReadConfig(config_object, local_info);
	RS_DBG("\n *****************************************************************************************************\n");

	json_object_put(root_obj);

	return 0;

}


#ifdef __cplusplus
}
#endif


/*
uint8 main(uint8 argc, char **argv) {
	RS_LOCAL_SYSTEM_INFO*local_info = (RS_LOCAL_SYSTEM_INFO*)malloc(sizeof(RS_LOCAL_SYSTEM_INFO));
	memset(local_info, 0, sizeof(RS_LOCAL_SYSTEM_INFO));
	rsOnvifSysInfoReadFromJsonFile(local_info);

	return 0;
}
*/
