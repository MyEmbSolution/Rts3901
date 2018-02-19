#ifndef RS_ONVIF_CONFIG_H
#define RS_ONVIF_CONFIG_H
/* #include "rsOnvifDefines.h" */
#include "rsOnvifTypes.h"
extern uint16 DEVICE_SERVICE_VERSION          ;
extern uint16 MEDIA_SERVICE_VERSION           ;
extern uint16 DEVICEIO_SERVICE_VERSION        ;
extern uint16 PTZ_SERVICE_VERSION             ;
extern uint16 IMAGING_SERVICE_VERSION         ;
extern uint16 ANALYTICSDEVICE_SERVICE_VERSION ;
extern uint16 EVENTS_SERVICE_VERSION          ;

extern char *DEVICE_TYPE              ;
extern char *SCOPES_TYPE              ;
extern char *SCOPES_PROFILE           ;
extern char *SCOPES_LOCATION          ;
extern char *SCOPES_HARDWARE          ;
extern char *SCOPES_NAME              ;
extern char *SCOPES_OTHERS            ;
extern char *DEVICE_MANUFACTURER      ;
extern char *DEVICE_MODEL             ;
extern char *DEVICE_FIRMWARE_VERSION  ;
extern char *DEVICE_SERIAL_NUMBER     ;
extern char *DEVICE_TIME_ZONE         ;
extern char *SYSTEM_LOG_URI           ;
extern char *SYSTEM_BACKUP_URI        ;
extern char *SUPPORT_INFO_URI         ;
extern char *SYSTEM_RESTORE_URI       ;
extern char *DEFAULT_ACCOUNT_NAME     ;
extern char *DEFAULT_ACCOUNT_PSWD     ;
extern int  DEFAULT_ACCOUNT_LEVEL     ;
extern char *DEFAULT_HOSTNAME         ;
extern char *DEFAULT_SEARCH_DOMAINNAME;
extern char *DEFAULT_NTP_ADDRESS      ;

extern device_service_capabilities_t device_service_capabilities;
extern event_service_capabilities_t event_service_capabilities;
extern media_config_t default_media_config;
extern media_profile_t default_profile_A;
extern media_profile_t default_profile_B;
extern media_profile_t default_profile_C;
extern osd_configuration_t default_OSD_A;
extern osd_configuration_t default_OSD_B;


#endif
