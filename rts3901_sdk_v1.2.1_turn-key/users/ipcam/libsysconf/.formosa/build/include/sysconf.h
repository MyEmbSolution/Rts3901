#ifndef _SYSCONF_H
#define _SYSCONF_H

#include <json-c/json.h>

#define CFG_DOMAIN_MULTIMEDIA	"peacock"
#define CFG_DOMAIN_ONVIF	"onvif"
#define CFG_DOMAIN_NETWORK	"network"
#define CFG_DOMAIN_ISP		"isp"
#define CFG_DOMAIN_OSD		"osd"
#define CFG_DOMAIN_TEST		"test"
#define CFG_DOMAIN_BLUETOOTH	"bluetooth"

#define TYPE_INT	0
#define TYPE_STRING	1

enum {
	E_DOMAIN_INVALID	= 0x70000001,
	E_FS_INVALID		= 0x70000002,
	E_KEY_NOT_FOUND		= 0x70000005,
	E_INVALID_OBJ_TYPE	= 0x70000006,
};

int rts_conf_scanf(char *domain, const char *fmt, ...);
int rts_conf_printf(char *domain, const char *fmt, ...);
int rts_conf_reset(char *domain);
void *rts_conf_get_metadata(const char *domain);
int rts_conf_scanf_ex(void *metadata, const char *fmt, ...);
int rts_conf_printf_ex(void *metadata, const char *fmt, ...);
int rts_conf_put_metadata(const char *domain, void *metadata);
void rts_conf_free_metadata(void *metadata);

#endif
