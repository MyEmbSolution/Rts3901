#include <stdio.h>
#include <signal.h>
#include "soapH.h"
#include "rsOnvifDefines.h"
#include "rsOnvifSystemCtrl.h"
#include "wsddapi.h"

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

int SOAP_ENV__Fault(struct soap *soap,
	char *faultcode,
	char *faultstring,
	char *faultactor,
	struct SOAP_ENV__Detail *detail,
	struct SOAP_ENV__Code *SOAP_ENV__Code,
	struct SOAP_ENV__Reason *SOAP_ENV__Reason,
	char *SOAP_ENV__Node,
	char *SOAP_ENV__Role,
	struct SOAP_ENV__Detail *SOAP_ENV__Detail)
{
	return SOAP_OK;
}

static int check_scopes_match(struct soap *soap,
		const char *scopes, onvif_scopes_t *scopes_target)
{
	int scopes_match = 0;
	int len = 0;
	int index = 0;
	int i;

	if (scopes != NULL) {
		if (strcmp(scopes, "") == 0 || strcmp(scopes, "onvif://www.onvif.org") == 0) {
			scopes_match = 1;
		} else {

			char (*list)[][100];

			list = (char (*)[][100])soap_malloc(soap, 1000);
			ASSERT(list);
			memset(list, 0, 1000);
			RS_DBG("list %x, *list %x\n", (int)list, (int)*list);
			explodeitem((char *)scopes, ' ', *list, 10);
			for (i = 0; **((*list) + i) != 0 && !scopes_match; i++)	{
				RS_DBG("i = %d, %s flag %d\n", i, (*list)[i], scopes_match);
				len = strlen("onvif://www.onvif.org/Profile");
				if (!strncmp((*list)[i], "onvif://www.onvif.org/Profile", len)) {
					if ((*list)[i][len] == 0) {
						scopes_match = 1;
						break;
					} else if (!strcmp((*((*list) + i) + len + 1), scopes_target->profile)) {
						scopes_match = 1;
						break;
					}
				}
				len = strlen("onvif://www.onvif.org/location");
				if (!strncmp((*list)[i], "onvif://www.onvif.org/location", len)) {
					if ((*list)[i][len] == 0) {
						scopes_match = 1;
						break;
					} else {
						index = 0;
						while (scopes_target->location[index][0] && index < MAX_SCOPES_LOCATION_NUM) {
							RS_DBG("location: input %s, location %s\n",
									(*((*list) + i) + len), scopes_target->location[index]);
							if (!strcmp((*((*list) + i) + len + 1), scopes_target->location[index])) {
								scopes_match = 1;
								break;
							}
							index++;
						}
					}
				}
				len = strlen("onvif://www.onvif.org/type");
				if (!strncmp((*list)[i], "onvif://www.onvif.org/type", len)) {
					if ((*list)[i][len] == 0) {
						scopes_match = 1;
						break;
					} else {
						index = 0;
						while (scopes_target->type[index][0] && index < MAX_SCOPES_TYPE_NUM) {
							RS_DBG("location: input %s, location %s\n",
									(*((*list) + i) + len), scopes_target->type[index]);
							if (!strcmp((*((*list) + i) + len + 1), scopes_target->type[index])) {
								scopes_match = 1;
								break;
							}
							index++;
						}
					}
				}
				len = strlen("onvif://www.onvif.org/hardware");
				if (!strncmp((*list)[i], "onvif://www.onvif.org/hardware", len)) {
					if ((*list)[i][len] == 0) {
						scopes_match = 1;
						break;
					} else if (!strcmp((*((*list) + i) + len + 1), scopes_target->hardware)) {
						scopes_match = 1;
						break;
					}
				}
				len = strlen("onvif://www.onvif.org/name");
				if (!strncmp((*list)[i], "onvif://www.onvif.org/name", len)) {
					if ((*list)[i][len] == 0) {
						scopes_match = 1;
						break;
					} else if (!strcmp((*((*list) + i) + len + 1), scopes_target->name)) {
						scopes_match = 1;
						break;
					}
				}
				len = strlen("onvif://www.onvif.org");
				if (!strncmp((*list)[i], "onvif://www.onvif.org", len)) {
					for (index = 0; index < MAX_SCOPES_OTHER_NUM; index++) {
						if (!strcmp((*((*list) + i) + len + 1), scopes_target->others[index])) {
							scopes_match = 1;
							break;
						}
					}
				}
			}
		}
	} else {
		scopes_match = 1;
	}

	return scopes_match;
}

void wsdd_event_Hello(struct soap *soap,
	unsigned int InstanceId,
	const char *SequenceId,
	unsigned int MessageNumber,
	const char *MessageID,
	const char *RelatesTo,
	const char *EndpointReference,
	const char *Types,
	const char *Scopes,
	const char *MatchBy,
	const char *XAddrs,
	unsigned int MetadataVersion)
{
	RS_DBG("is called!\n");
}

soap_wsdd_mode wsdd_event_Probe(struct soap *soap,
	const char *MessageID,
	const char *ReplyTo,
	const char *Types,
	const char *Scopes,
	const char *MatchBy,
	struct wsdd__ProbeMatchesType *matches)
{
	onvif_special_info_t *info;
	network_config_t *network;
	int ret = 0;
	int idx = 0;
	int scopes_match = 0;

	RS_DBG("MessageID %s\n ReplyTo %s\n Types %s\n Scopes %s\n MatchBy %s\n",
		MessageID, ReplyTo, Types, Scopes, MatchBy);
	RS_DBG("mode 0x%08x\n", soap->mode);
	RS_DBG("input Types len = %d, input Scopes len = %d\n", strlen(Types), strlen(Scopes));

	info = (onvif_special_info_t *)soap_malloc(soap, sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	if (ret)
		goto end;

	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);
	ret = rsOnvifSysInfoGetNetConfig(network);
	if (ret)
		goto end;

	scopes_match = check_scopes_match(soap, Scopes, &info->scopes);
	RS_DBG("scopes_match %d\n", scopes_match);
	if (scopes_match) {
		char *types;
		char *uuid;
		char *xaddr;
		char *endpoint_reference;
		char *scopes;
		int i;

		types = (char *)soap_malloc(soap, DEVICE_TYPE_BUF_SIZE);
		ASSERT(types);
		memset(types, 0, DEVICE_TYPE_BUF_SIZE);

		uuid = (char *)soap_malloc(soap, UUID_BUF_SIZE);
		ASSERT(uuid);
		memset(uuid, 0, UUID_BUF_SIZE);

		xaddr = (char *)soap_malloc(soap, XADDR_BUF_SIZE);
		ASSERT(xaddr);
		memset(xaddr, 0, XADDR_BUF_SIZE);

		endpoint_reference = (char *)soap_malloc(soap, EDR_BUF_SIZE);
		ASSERT(endpoint_reference);
		memset(endpoint_reference, 0, EDR_BUF_SIZE);

		scopes = (char *)soap_malloc(soap, SCOPES_BUF_SIZE);
		ASSERT(scopes);
		memset(scopes, 0, SCOPES_BUF_SIZE);

		memcpy(types, info->device_type, DEVICE_TYPE_BUF_SIZE);

		sprintf(xaddr,
			"http://%s/onvif/%s",
			inet_ntoa(network->interfaces[network->interface_idx].ip),
			info->service_name[SERVICE_INDEX_DEVICE]);
		RS_DBG("network->interface_idx = %d\n", network->interface_idx);
		RS_DBG("XADDR: %s\n", xaddr);
		ret = rsOnvifSysInfoGetUUID(uuid);
		if (ret)
			goto end;

		sprintf(endpoint_reference, "urn:uuid:%s", uuid);

		RS_DBG("endpoint_reference: %s\n ", endpoint_reference);

		assemble_scopes_string(scopes, info);
		RS_DBG("Scopes: %s\n types %s\n", scopes, types);
		if (MatchBy && !strcmp(MatchBy, "InvalidMatchRule")) {
			int err = 0;
			struct SOAP_ENV__Code *soap_env_code =
				(struct SOAP_ENV__Code *)soap_malloc(soap,
					sizeof(struct SOAP_ENV__Code));
			struct SOAP_ENV__Reason *soap_env_reason =
				(struct SOAP_ENV__Reason *)soap_malloc(soap,
					sizeof(struct SOAP_ENV__Reason));
			struct SOAP_ENV__Detail *detail =
				(struct SOAP_ENV__Detail *)soap_malloc(soap,
					sizeof(struct SOAP_ENV__Detail));
			soap_env_code->SOAP_ENV__Value = "tns:Sender";
			soap_env_code->SOAP_ENV__Subcode =
				(struct SOAP_ENV__Code *)soap_malloc(soap,
					(sizeof(struct SOAP_ENV__Code)));
			soap_env_code->SOAP_ENV__Subcode->SOAP_ENV__Value = "wsdd:MatchingRuleNotSupported";
			soap_env_code->SOAP_ENV__Subcode->SOAP_ENV__Subcode = NULL;

			soap_env_reason->SOAP_ENV__Text = "the matching rule specified is not supported";

			detail->__any = "<wsdd:SupportedMatchingRules>www.realsil.com.cn </wsdd:SupportedMatchingRules>";
			detail->__type = 0;
			detail->fault = NULL;

			soap->header->wsa__Action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/fault";
			soap->header->wsa__To = NULL;
			soap->header->wsa__MessageID =
				(char *)soap_malloc(soap, sizeof(char) * 100);

			strncpy(soap->header->wsa__MessageID,
					endpoint_reference,
					strlen(endpoint_reference));
			soap->header->wsa__RelatesTo =
				(struct wsa__Relationship *)soap_malloc(soap,
					sizeof(struct wsa__Relationship));
			memset(soap->header->wsa__RelatesTo, 0, sizeof(struct wsa__Relationship));
			soap->header->wsa__RelatesTo->__item = (char *)MessageID;

			err = soap_send_SOAP_ENV__Fault(soap, ReplyTo,
					NULL, NULL, NULL, NULL, NULL,
					soap_env_code, soap_env_reason,
					NULL, NULL, detail);
			RS_DBG("soap_send_SOAP_ENV__Fault return value %d\n", err);
		} else {

			/*
			soap_wsdd_add_ProbeMatch(soap,
					matches,
					endpoint_reference,
					(char *)&types,
					scopes,
					NULL,
					xaddr,
					1);
			*/
			soap_wsdd_add_ProbeMatch(soap,
					matches,
					endpoint_reference,
					types,
					scopes,
					NULL,
					xaddr,
					1);/*yafei changed*/
			RS_DBG("before send probe_matches Replyto %s\n", ReplyTo);
			soap_wsdd_ProbeMatches(soap,
					NULL,
					endpoint_reference,
					MessageID,
					ReplyTo,
					matches);
		}
	}


end:
	return SOAP_WSDD_ADHOC;

}

void wsdd_event_ProbeMatches(struct soap *soap,
	unsigned int InstanceId,
	const char *SequenceId,
	unsigned int MessageNumber,
	const char *MessageID,
	const char *RelatesTo,
	struct wsdd__ProbeMatchesType *matches)
{
	RS_DBG("is called!\n");
}

soap_wsdd_mode wsdd_event_Resolve(struct soap *soap,
	const char *MessageID,
	const char *ReplyTo,
	const char *EndpointReference,
	struct wsdd__ResolveMatchType *match)
{
	RS_DBG("is called!\n");
	return SOAP_WSDD_MANAGED;
}

void wsdd_event_ResolveMatches(struct soap *soap,
	unsigned int InstanceId,
	const char *SequenceId,
	unsigned int MessageNumber,
	const char *MessageID,
	const char *RelatesTo,
	struct wsdd__ResolveMatchType *match)
{
	RS_DBG("is called!\n");
}

void wsdd_event_Bye(struct soap *soap,
	unsigned int InstanceId,
	const char *SequenceId,
	unsigned int MessageNumber,
	const char *MessageID,
	const char *RelatesTo,
	const char *EndpointReference,
	const char *Types,
	const char *Scopes,
	const char *MatchBy,
	const char *XAddrs,
	unsigned int *MetadataVersion)
{
	RS_DBG("is called!\n");
}
