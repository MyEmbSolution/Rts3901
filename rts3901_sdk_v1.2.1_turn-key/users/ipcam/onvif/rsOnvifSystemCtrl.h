#ifndef _RS_ONVIF_SYSTEM_CTRL_H
#define _RS_ONVIF_SYSTEM_CTRL_H
#include "rsOnvifTypes.h"
#include "rsOnvifDefines.h"
#include "rsOnvifDevCtrl.h"


int rsOnvifSysInfoCreate(void);
int rsOnvifSysInfoClose(void);

int rsOnvifSysInfoWrite2Memory(void);
int rsOnvifSysInfoSetDefault(void);

int rsOnvifSysInfoGetUUID(char *uuid);
int rsOnvifSysInfoSetUUID(char *uuid);

int rsOnvifSysInfoGetSpecialInfo(onvif_special_info_t *info);
int rsOnvifSysInfoSetSpecialInfo(onvif_special_info_t *info);

int rsOnvifSysInfoGetDevInfo(device_information_t *info);
int rsOnvifSysInfoSetDevInfo(device_information_t *info);

int rsOnvifSysInfoGetDateTimeSettings(date_time_settings_t *set);
int rsOnvifSysInfoSetDateTimeettings(date_time_settings_t *set);

int rsOnvifSysInfoGetNetConfig(network_config_t *config);
int rsOnvifSysInfoSetNetConfig(network_config_t *config);

int rsOnvifSysInfoSetSendHelloState(int state);
int rsOnvifSysInfoGetSendHelloState(int *state);

int rsOnvifSysInfoGetMediaProfiles(media_profile_t *profiles);
int rsOnvifSysInfoSetMediaProfiles(media_profile_t *profiles);

int rsOnvifSysInfoGetMediaConfigs(media_config_t *configs);
int rsOnvifSysInfoSetMediaConfigs(media_config_t *configs);

int rsOnvifSysInfoGetOSDConfigs(osd_configuration_t *osd);
int rsOnvifSysInfoSetOSDConfigs(osd_configuration_t *osd);


int rsOnvifUsrAuthentication(struct soap *soap, enum tt__UserLevel user_level);

static int rsOnvifNetSearchGateway(char *buf, in_addr_t *gate_addr);
int spawn_new_process(char *cmd);
in_addr_t rsOnvifNetGetIFAddr(char *ifname);
in_addr_t rsOnvifNetGetNetmask(char *ifname);
int rsOnvifNetGetHWAddr(char *ifname, char *mac_addr);
int rsOnvifNetAddGateway(in_addr_t addr);
int rsOnvifNetDelGateway(in_addr_t addr);
in_addr_t rsOnvifNetGetGateway(void);
int rsOnvifNetSetGateway(in_addr_t addr);
int rsOnvifNetSetDNS(struct in_addr addr[], int num);
int rsOnvifNetGetDNS(struct in_addr addr[], int num);
int rsOnvifNetSetSearchDomain(char **domain, int num);
int rsOnvifNetGetNetworkInterfaces(char name[][20], int *interface_num);
int rsOnvifNetSetStaticIPAddr(char *name, struct in_addr ip_addr, struct in_addr netmask, struct in_addr gateway);
int rsOnvifNetEnableDHCP(char *name);
int rsOnvifNetDisableDHCP(char *name);
int rsOnvifNetConfigProtocol(char *name, int enabled, unsigned short port[6]);
void rsOnvifSysGetDevUUID(char *uuid, int length);
void rsOnvifSysGetDevSerialNumber(char *sn, int length);
int rsOnvifSysChangeDstTimeZone(int dst, char *tz);

#endif
