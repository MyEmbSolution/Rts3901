#include "rsOnvifCommonFunc.h"
#include "rsOnvifDefines.h"
#include "rsOnvifTypes.h"
#include "soapH.h"
#include <time.h>
#include <stdlib.h>

int onvif_not_authorized(struct soap *soap, char *value1, char *value2)
{
	soap->fault = (struct SOAP_ENV__Fault *)soap_malloc(soap, (sizeof(struct SOAP_ENV__Fault)));
	soap->fault->SOAP_ENV__Code = (struct SOAP_ENV__Code *)soap_malloc(soap, (sizeof(struct SOAP_ENV__Code)));
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Value = "SOAP-ENV:Sender";
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode = (struct SOAP_ENV__Code *)soap_malloc(soap, (sizeof(struct SOAP_ENV__Code)));;
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value = value1;
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Subcode = NULL;
	soap->fault->faultcode = NULL;
	soap->fault->faultstring = NULL;
	soap->fault->faultactor = NULL;
	soap->fault->detail = NULL;
	soap->fault->SOAP_ENV__Reason = (struct SOAP_ENV__Reason *)soap_malloc(soap, sizeof(struct SOAP_ENV__Reason));
	soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text = value2;
	soap->fault->SOAP_ENV__Node = NULL;
	soap->fault->SOAP_ENV__Role = NULL;
	soap->fault->SOAP_ENV__Detail = NULL;
}

int onvif_fault(struct soap *soap, int receiver, char *value1, char *value2)
{
	soap->fault = (struct SOAP_ENV__Fault *)soap_malloc(soap, (sizeof(struct SOAP_ENV__Fault)));
	soap->fault->SOAP_ENV__Code = (struct SOAP_ENV__Code *)soap_malloc(soap, (sizeof(struct SOAP_ENV__Code)));
	if (RECEIVER == receiver)
		soap->fault->SOAP_ENV__Code->SOAP_ENV__Value = "SOAP-ENV:Receiver";
	else
		soap->fault->SOAP_ENV__Code->SOAP_ENV__Value = "SOAP-ENV:Sender";
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode =
		(struct SOAP_ENV__Code *)soap_malloc(soap, (sizeof(struct SOAP_ENV__Code)));;
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value = value1;
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Subcode =
		(struct SOAP_ENV__Code *)soap_malloc(soap, (sizeof(struct SOAP_ENV__Code)));
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Subcode->SOAP_ENV__Value = value2;
	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Subcode->SOAP_ENV__Subcode = NULL;
	soap->fault->faultcode = NULL;
	soap->fault->faultstring = NULL;
	soap->fault->faultactor = NULL;
	soap->fault->detail = NULL;
	soap->fault->SOAP_ENV__Reason = NULL;
	soap->fault->SOAP_ENV__Node = NULL;
	soap->fault->SOAP_ENV__Role = NULL;
	soap->fault->SOAP_ENV__Detail = NULL;

	return 0;
}


void explodeitem(char *in, char dl, char list[][100], int row)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int len = strlen(in);
	for (i = 0; i < len && j < row; i++) {
		if (in[i] == dl) {

			*(*(list + j) + k) = 0;
			j++;
			k = 0;
		} else {
			*(*(list + j) + k) = in[i];
			k++;
		}
	}
	if (j < (row - 1)) {
		*(*(list + j) + k) = 0;
		j++;
		**(list + j) = 0;
	}
}

void assemble_scopes_string(char *scopes, onvif_special_info_t *info)
{
	int i;
	if ((strlen(info->scopes.profile) + 30) < SCOPES_BUF_SIZE) {
		if (!strcmp(info->scopes.profile, "ANY")) {
			sprintf(scopes, "onvif://www.onvif.org/Profile");
		} else {
			sprintf(scopes, "onvif://www.onvif.org/Profile/%s", info->scopes.profile);
		}
	}
	for (i = 0; i < MAX_SCOPES_TYPE_NUM; i++) {
		if (info->scopes.type[i][0] && (strlen(info->scopes.type[i]) + strlen(scopes) + 32) < SCOPES_BUF_SIZE) {
			if (!strcmp(info->scopes.type[i], "ANY")) {
				sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/type");
			} else {
				sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/type/%s", info->scopes.type[i]);
			}

		} else
			break;
	}
	for (i = 0; i < MAX_SCOPES_LOCATION_NUM; i++) {
		if (info->scopes.location[i][0] && (strlen(info->scopes.location[i]) + strlen(scopes) + 32) < SCOPES_BUF_SIZE) {
			if (!strcmp(info->scopes.location[i], "ANY")) {
				sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/location");
			} else {
				sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/location/%s", info->scopes.location[i]);
			}

		} else
			break;
	}
	if ((strlen(info->scopes.hardware) + strlen(scopes) + 32) < SCOPES_BUF_SIZE) {
		if (!strcmp(info->scopes.hardware, "ANY")) {
			sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/hardware");
		} else {
			sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/hardware/%s", info->scopes.hardware);
		}

	}
	if ((strlen(info->scopes.name) + strlen(scopes) + 28) < SCOPES_BUF_SIZE) {
		if (!strcmp(info->scopes.name, "ANY")) {
			sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/name");
		} else {
			sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/name/%s", info->scopes.name);
		}
	}

	for (i = 0; i < MAX_SCOPES_OTHER_NUM; i++) {
		if (info->scopes.others[i][0] && (strlen(info->scopes.others[i]) + strlen(scopes) + 24) < SCOPES_BUF_SIZE) {
			sprintf(scopes+strlen(scopes), " onvif://www.onvif.org/%s", info->scopes.others[i]);
		} else
			break;
	}

}

void rsCommGetDateTime(date_time_t *value)
{
	time_t timer;
	struct tm *tblock;
	time(&timer);
	tblock = localtime(&timer);
	value->year = tblock->tm_year + 1900;
	value->month = tblock->tm_mon + 1;
	value->day = tblock->tm_mday;
	value->hour = tblock->tm_hour;
	value->minute = tblock->tm_min;
	value->second = tblock->tm_sec;

}

int isValidIP4 (char *str)
{
	int segs = 0;/* Segment count. */
	int chcnt = 0;/* Character count within segment. */
	int accum = 0;/* Accumulator for segment. */
	/* Catch NULL pointer. */
	if (str == NULL)
		return 0;
	/* Process every character in string. */
	while (*str != '\0') {
		/* Segment changeover. */
		if (*str == '.') {
			/* Must have some digits in segment. */
			if (chcnt == 0)
				return 0;
			/* Limit number of segments. */
			if (++segs == 4)
				return 0;
			/* Reset segment values and restart loop. */
			chcnt = accum = 0;
			str++;
			continue;
		}

		/* Check numeric. */
		if ((*str < '0') || (*str > '9'))
			return 0;
		/* Accumulate and check segment. */
		accum = accum * 10 + *str - '0';
		if (accum > 255)
			return 0;
		/* Advance other segment specific stuff and continue loop. */
		chcnt++;
		str++;
	}
	/* Check enough segments and enough characters in last segment. */
	if (segs != 3)
		return 0;
	if (chcnt == 0)
		return 0;
	/* Address okay. */
	return 1;
}

int isValidHostname (char *str)
{
	/* Catch NULL pointer. */
	if (str == NULL) {
		return 0;
	}
	/* Process every character in string. */
	while (*str != '\0') {
		if ((*str >= 'a' && *str <= 'z') ||
			(*str >= 'A' && *str <= 'Z') ||
			(*str >= '0' && *str <= '9') ||
			(*str == '.') || (*str == '-')) {
			str++;
		} else {
			return 0;
		}
	}
	return 1;
}

int check_time_zone(char *TZ)
{
	if (strncmp(TZ, "IDLW", 4) == 0)
		return 0;/* -12 */
	else if (strncmp(TZ, "NT", 2) == 0)
		return 1;/* -11 */
	else if ((strncmp(TZ, "AHST", 4) == 0)
		|| (strncmp(TZ, "CAT", 3) == 0)
		|| (strncmp(TZ, "HST", 3) == 0)
		|| (strncmp(TZ, "HDT", 3) == 0))
		return 2;/* -10 */
	else if ((strncmp(TZ, "YST", 3) == 0)
		|| (strncmp(TZ, "YDT", 3) == 0))
		return 3;/* -9 */
	else if ((strncmp(TZ, "PST", 3) == 0)
		|| (strncmp(TZ, "PDT", 3) == 0))
		return 4;/* -8 */
	else if ((strncmp(TZ, "MST", 3) == 0)
		|| (strncmp(TZ, "MDT", 3) == 0))
		return 5;/* -7 */
	else if ((strncmp(TZ, "CST", 3) == 0)
		|| (strncmp(TZ, "CDT", 3) == 0))
		return 6;/* -6 */
	else if ((strncmp(TZ, "EST", 3) == 0)
		|| (strncmp(TZ, "EDT", 3) == 0))
		return 7;/* -5 */
	else if ((strncmp(TZ, "AST", 3) == 0)
		|| (strncmp(TZ, "ADT", 3) == 0))
		return 8;/* -4 */
	else if (strncmp(TZ, "GMT-03", 6) == 0)
		return 9;/* -3 */
	else if (strncmp(TZ, "AT", 2) == 0)
		return 10;/* -2 */
	else if (strncmp(TZ, "WAT", 3) == 0)
		return 11;/* -1 */
	else if ((strncmp(TZ, "GMT", 3) == 0)
		|| (strncmp(TZ, "UT", 2) == 0)
		|| (strncmp(TZ, "UTC", 3) == 0)
		|| (strncmp(TZ, "BST", 3) == 0))
		return 12;/* -0 */
	else if ((strncmp(TZ, "CET", 3) == 0)
		|| (strncmp(TZ, "FWT", 3) == 0)
		|| (strncmp(TZ, "MET", 3) == 0)
		|| (strncmp(TZ, "MEWT", 4) == 0)
		|| (strncmp(TZ, "SWT", 3) == 0)
		|| (strncmp(TZ, "MEST", 4) == 0)
		|| (strncmp(TZ, "MESZ", 4) == 0)
		|| (strncmp(TZ, "SST", 3) == 0)
		|| (strncmp(TZ, "FST", 3) == 0))
		return 13;/* 1 */
	else if (strncmp(TZ, "EET", 3) == 0)
		return 14;/* 2 */
	else if (strncmp(TZ, "BT", 2) == 0)
		return 15;/* 3 */
	else if (strncmp(TZ, "ZP4", 3) == 0)
		return 16;/* 4 */
	else if (strncmp(TZ, "ZP5", 3) == 0)
		return 17;/* 5 */
	else if (strncmp(TZ, "ZP6", 3) == 0)
		return 18;/* 6 */
	else if (strncmp(TZ, "ZP7", 3) == 0)
		return 19;/* 7 */
	else if ((strncmp(TZ, "WAST", 4) == 0)
		|| (strncmp(TZ, "CST", 3) == 0)
		|| (strncmp(TZ, "CCT", 3) == 0))
		return 20;/* 8 */
	else if (strncmp(TZ, "JST", 3) == 0)
		return 21;/* 9 */
	else if (strncmp(TZ, "ACT", 3) == 0)
		return 22;/* 10 */
	else if (strncmp(TZ, "EAST", 4) == 0)
		return 23;/* 11 */
	else if (strncmp(TZ, "IDLE", 4) == 0)
		return 24;/* 12 */
	else
		return 100;/* ERROR */


}


