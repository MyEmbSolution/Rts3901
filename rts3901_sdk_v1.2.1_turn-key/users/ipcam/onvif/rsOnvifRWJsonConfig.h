#ifndef __RS_ONVIF_RW_JSON_CONFIG_H
#define __RS_ONVIF_RW_JSON_CONFIG_H

//#include "rsOnvifTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8 rsOnvifSysInfoWrite2JsonFile(RS_LOCAL_SYSTEM_INFO *local_info, const char *json_fn);
uint8 rsOnvifSysInfoReadFromJsonFile(RS_LOCAL_SYSTEM_INFO*local_info, const char *json_fn);

#ifdef __cplusplus
}
#endif


#endif
