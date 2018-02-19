#ifndef _RS_ONVIF_COMMON_FUNC_H
#define _RS_ONVIF_COMMON_FUNC_H
#include "rsOnvifTypes.h"
#include "soapH.h"
#include <stdarg.h>
#include <string.h>

/* int onvif_not_authorized(struct soap *soap, char *value1, char *value2); */

int onvif_fault(struct soap *soap, int receiver, char *value1, char *value2);
void rsCommGetDateTime(date_time_t *value);
void explodeitem(char *in, char dl, char list[][100], int row);
void assemble_scopes_string(char *scopes, onvif_special_info_t *info);
int isValidHostname (char *str);
int isValidIP4 (char *str) ;
int check_time_zone(char *TZ);

void rsCommGetDateTime(date_time_t *value);


#endif
