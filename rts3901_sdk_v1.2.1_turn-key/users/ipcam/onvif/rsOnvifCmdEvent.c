#include "soapH.h"
#include "rsOnvifSystemCtrl.h"
#include "rsOnvifMsg.h"
#include "rsOnvifCommonFunc.h"
#include "rsOnvifDevCtrl.h"

/*ctrl device*/
#include "rsOnvifEventMgrLib.h"

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

#define TOPSET_CONTENT  "<tns1:VideoAnalytics >"\
				"<tnsavg:MotionDetection>"\
					"<tns1:Motion wstop:topic=\"true\">"\
						"<tt:MessageDescription IsProperty=\"true\">"\
							"<tt:Source>"\
								"<tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\"/>"\
							"</tt:Source>"\
							"<tt:Data>"\
								"<tt:SimpleItemDescription Name=\"ObjectId\" Type=\"tt:ObjectRefType\"/>"\
							"</tt:Data>"\
						"</tt:MessageDescription>"\
					"</tns1:Motion>"\
				"</tnsavg:MotionDetection>"\
			"</tns1:VideoAnalytics>"

#define SUBMGR_EXE "onvif_events_submgr"
typedef enum e_even_error_code {
	INVALID_FILTER_FAULT,
	INVALID_MESSAGE_CONTENT_EXPRESSION_FAULT,
	INVALID_PRODUCER_PROPERTIES_EXPRESSION_FAULT,
	INVALID_TOPIC_EXPRESSION_FAULT,
	MULTIPLE_TOPICS_SPECIFIED_FAULT,
	NO_CURRENT_MESSAGE_ON_TOPICA_FAULT,
	NOTIFY_MESSAGE_NOT_SUPPORTED_FAULT,
	PAUSE_FAILED_FAULT,
	PULL_MESSAGES_FAULT_RESPONSE,
	RESOURCE_UNKNOWN_FAULT,
	RESUME_FAILED_FAULT,
	OUT_OF_MAX_PRODUCERS_FAULT,
	OUT_OF_MAX_PULLPOINTS_FAULT,
	SUBSCRIBE_CRETATION_FAILED_FAULT,
	TOPIC_EXPRESSION_DIALECT_UNKNOWN_FAULT,
	TOPIC_NOT_SUPPORTED_FAULT,
	UNABLE_TO_CREATE_PULLPOINT_FAULT,
	UNABLE_TO_DESTROY_PULLPOINT_FAULT,
	UNABLE_TO_DESTROY_SUBSCRIPTION_FAULT,
	UNABLE_TO_GET_MESSAGE_FAULT,
	UNACCEPTABLE_INITIAL_TERMINATION_TIME_FAULT,
	UNACCEPTABLE_TERMINATION_TIME_FAULT,
	UNRECOGNIZED_POLICY_REQUEST_FAULT,
	UNSUPPORTED_POLICY_REQUEST_FAULT
} event_error_code_t;

static void remove_blanks_in_string(char *str)
{
	char *p = strchr(str, 0x20);

	if (p == NULL)
		return;

	if (*(p+1)) {
		remove_blanks_in_string(p+1);
	}

	do {
		*p = *(p + 1);
		p++;
	} while (*p);

}

static int split_filter(struct soap *soap, char *str, char **topic_filter, char **content_filter)
{
	int ret = 0;
	char *p_start;
	*topic_filter = NULL;
	*content_filter = NULL;
	/*step 1: retrieve topic expression filer;*/
	p_start = strstr(str, "wsnt:TopicExpression");
	if (p_start) {
		char *p_end;
		while (*p_start++ != '>') {
		}

		p_end = strstr(p_start, "</wsnt:TopicExpression>");

		*topic_filter = (char *)soap_malloc(soap, (int)(p_end - p_start) + 1);
		memset(*topic_filter, 0, (int)(p_end - p_start) + 1);
		memcpy(*topic_filter, p_start, (int)(p_end - p_start));
	}

	/*step 2: retreive content filter;*/
	p_start = strstr(str, "wsnt:MessageContent");
	if (p_start) {
		char *p_end;

		while (*p_start++ != '>') {
		}

		p_end = strstr(p_start, "</wsnt:MessageContent>");

		*content_filter = (char *)soap_malloc(soap, (int)(p_end - p_start) + 1);
		memset(*content_filter, 0, (int)(p_end - p_start) + 1);
		memcpy(*content_filter, p_start, (int)(p_end - p_start));
	}
}

static void onvif_event_fault(struct soap *soap, int receiver, event_error_code_t err, char *misc)
{
	time_t tnow;
	if (!soap->header)
		soap->header = (struct SOAP_ENV__Header *)soap_malloc(soap, sizeof(struct SOAP_ENV__Header));
	soap->header->wsa5__Action = "http://docs.oasis-open.org/wsrf/fault";

	soap->fault = (struct SOAP_ENV__Fault *)soap_malloc(soap, sizeof(struct SOAP_ENV__Fault));
	soap->fault->SOAP_ENV__Code = (struct SOAP_ENV__Code *)soap_malloc(soap, sizeof(struct SOAP_ENV__Code));
	if (receiver == 1)
		soap->fault->SOAP_ENV__Code->SOAP_ENV__Value = "SOAP-ENV:Receiver";
	else
		soap->fault->SOAP_ENV__Code->SOAP_ENV__Value = "SOAP-ENV:Sender";

	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode = NULL;
	soap->fault->faultcode = NULL;
	soap->fault->faultstring = NULL;
	soap->fault->faultactor = NULL;
	soap->fault->detail = NULL;
	soap->fault->SOAP_ENV__Reason = NULL;
	soap->fault->SOAP_ENV__Node = NULL;
	soap->fault->SOAP_ENV__Role = NULL;
	soap->fault->SOAP_ENV__Detail = NULL;
	time(&tnow);
	switch (err) {
	case INVALID_MESSAGE_CONTENT_EXPRESSION_FAULT:
		soap->fault->SOAP_ENV__Reason =
			(struct SOAP_ENV__Reason *)soap_malloc(soap, sizeof(struct SOAP_ENV__Reason));
		ASSERT(soap->fault->SOAP_ENV__Reason);
		soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text = "wsnt:InvalidMessageContentExpressionFault";
		soap->fault->SOAP_ENV__Detail =
			(struct SOAP_ENV__Detail *)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail));
		ASSERT(soap->fault->SOAP_ENV__Detail);
		memset(soap->fault->SOAP_ENV__Detail, 0, sizeof(struct SOAP_ENV__Detail));
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidMessageContentExpressionFault =
			(struct wsnt__InvalidMessageContentExpressionFaultType *)soap_malloc(soap,
				sizeof(struct wsnt__InvalidMessageContentExpressionFaultType));
		ASSERT(soap->fault->SOAP_ENV__Detail->wsnt__InvalidMessageContentExpressionFault);
		memset(soap->fault->SOAP_ENV__Detail->wsnt__InvalidMessageContentExpressionFault,
			0, sizeof(struct wsnt__InvalidMessageContentExpressionFaultType));
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidMessageContentExpressionFault->__size = 1;
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidMessageContentExpressionFault->Timestamp = tnow;
		break;
	case INVALID_FILTER_FAULT:
		soap->fault->SOAP_ENV__Reason =
			(struct SOAP_ENV__Reason *)soap_malloc(soap, sizeof(struct SOAP_ENV__Reason));
		ASSERT(soap->fault->SOAP_ENV__Reason);
		soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text = "wsnt:InvalidFilterFault";
		soap->fault->SOAP_ENV__Detail =
			(struct SOAP_ENV__Detail *)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail));
		ASSERT(soap->fault->SOAP_ENV__Detail);
		memset(soap->fault->SOAP_ENV__Detail, 0, sizeof(struct SOAP_ENV__Detail));
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault =
			(struct wsnt__InvalidFilterFaultType *)soap_malloc(soap,
				sizeof(struct wsnt__InvalidFilterFaultType));
		ASSERT(soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault);
		memset(soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault,
			0, sizeof(struct wsnt__InvalidFilterFaultType));
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault->__size = 1;
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault->Timestamp = tnow;
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault->__sizeUnknownFilter = 1;
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault->UnknownFilter =
			(char **)soap_malloc(soap, sizeof(char *) * 1);
		soap->fault->SOAP_ENV__Detail->wsnt__InvalidFilterFault->UnknownFilter[0] = misc;
		break;

	case RESOURCE_UNKNOWN_FAULT:
		soap->fault->SOAP_ENV__Reason =
			(struct SOAP_ENV__Reason *)soap_malloc(soap, sizeof(struct SOAP_ENV__Reason));
		ASSERT(soap->fault->SOAP_ENV__Reason);
		soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text = "wsrfr:ResourceUnknownFault";
		soap->fault->SOAP_ENV__Detail =
			(struct SOAP_ENV__Detail *)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail));
		ASSERT(soap->fault->SOAP_ENV__Detail);
		memset(soap->fault->SOAP_ENV__Detail, 0, sizeof(struct SOAP_ENV__Detail));
		soap->fault->SOAP_ENV__Detail->wsrfr__ResourceUnknownFault =
			(struct wsrfr__ResourceUnknownFaultType *)soap_malloc(soap,
				sizeof(struct wsrfr__ResourceUnknownFaultType));
		ASSERT(soap->fault->SOAP_ENV__Detail->wsrfr__ResourceUnknownFault);
		memset(soap->fault->SOAP_ENV__Detail->wsrfr__ResourceUnknownFault,
			0, sizeof(struct wsrfr__ResourceUnknownFaultType));
		soap->fault->SOAP_ENV__Detail->wsrfr__ResourceUnknownFault->__size = 1;
		soap->fault->SOAP_ENV__Detail->wsrfr__ResourceUnknownFault->Timestamp = tnow;
		break;
	}
}


SOAP_FMAC5 int SOAP_FMAC6  __tev__PullMessages(struct soap *soap,
	struct _tev__PullMessages *tev__PullMessages,
	struct _tev__PullMessagesResponse *tev__PullMessagesResponse)
{

	time_t tnow;
	time_t time_out;
	int message_num = 0;
	int i, ret = 0;
	network_config_t *network;
	onvif_special_info_t *info;
	char *address;
	char *client_address;
	char *response_message = (char *)soap_malloc(soap, 1024);

	/*sprintf later*/
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized", "The action requested requires authorization and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;
	soap->header->wsa5__Action = "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesResponse";

	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);
	info = (onvif_special_info_t *)soap_malloc(soap, sizeof(onvif_special_info_t));
	ASSERT(info);
	address = (char *)soap_malloc(soap, XADDR_BUF_SIZE);
	ASSERT(address);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	ASSERT(!ret);

	sprintf(address, "http://%s/onvif/%s",
		inet_ntoa(network->interfaces[network->interface_idx].ip),
		info->service_name[SERVICE_INDEX_EVENTS]);

	if (soap->header->wsa5__From)
		client_address = soap->header->wsa5__From->Address;
	else
		client_address = NULL;

	time(&tnow);
	/*
	struct _tev__PullMessages
	{
		LONG64 Timeout;
		int MessageLimit;
		int __size;
		char *__any;
	};
	*/
	/*WTH, this is crazy, almost cost me 3 days to debug, this ONVIF bug!!!*/
	char tmp_input_timeout[64] = {0};
	sprintf(tmp_input_timeout, "%s", tev__PullMessages->Timeout);
	LONG64 time_tmp_64;
	soap_s2xsd__duration(soap, tmp_input_timeout, &time_tmp_64);

	time_out = (long)(time_tmp_64 / 1000);/*convert 2 second*/
	if (!time_out)
		time_out += 10;
	RS_DBG("in string time_out = %s\n", tev__PullMessages->Timeout);
	RS_DBG("in tmp_input_time_out = %s\n", tmp_input_timeout);
	RS_DBG("in string d time_out = %d\n", tev__PullMessages->Timeout);
	RS_DBG("in string ld time_out = %ld\n", tev__PullMessages->Timeout);
	RS_DBG("in string lld time_out = %lld\n", tev__PullMessages->Timeout);
	RS_DBG("parsed    time_out = %d\n", time_tmp_64);
	RS_DBG("parseld   time_out = %ld\n", time_tmp_64);
	RS_DBG("parselld  time_out = %lld\n",  time_tmp_64);
	RS_DBG("parse     time_out = %ld\n", time_out);

	/*here get the number of message can be pulled*/
	/*should be replaced later*/
	message_num = 1;

	if (message_num > tev__PullMessages->MessageLimit)
		message_num = tev__PullMessages->MessageLimit;

	int cur_sub_num = 0;
	rs_onvif_events_submgr_info_get_submgr_number(&cur_sub_num);

#if 0
	RS_DBG("submgr num = %d\n", cur_sub_num);
	if (cur_sub_num == 0) {

		return SOAP_FAULT;
	} else if (cur_sub_num > 0)
#endif
	{

		int motion_detect_result = rs_motion_detect(NULL);
		char tmpbuf[64] = {0};
		strftime(tmpbuf, sizeof(tmpbuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&tnow));
		sprintf(response_message, MOTION_DETECT_TEMPLATE,
			tmpbuf,
			(motion_detect_result & 0x02) >> 1,
			(motion_detect_result & 0x04) >> 2,
			(motion_detect_result & 0x08) >> 3
			);

		tev__PullMessagesResponse->CurrentTime = tnow;
		tev__PullMessagesResponse->TerminationTime = tnow + time_out;
		tev__PullMessagesResponse->__sizeNotificationMessage = message_num;

			RS_DBG("output CurrentTime = %ld\n", tev__PullMessagesResponse->CurrentTime);
			RS_DBG("output Terminatio  = %ld\n", tev__PullMessagesResponse->TerminationTime);
		tev__PullMessagesResponse->wsnt__NotificationMessage =
			(struct wsnt__NotificationMessageHolderType *)soap_malloc(soap,
				sizeof(struct wsnt__NotificationMessageHolderType) * message_num);
		ASSERT(tev__PullMessagesResponse->wsnt__NotificationMessage);
		for (i = 0; i < message_num; i++) {
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].SubscriptionReference = NULL;
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].ProducerReference = NULL;

			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Topic =
				(struct wsnt__TopicExpressionType *)soap_malloc(soap,
					sizeof(struct wsnt__TopicExpressionType));
			/*
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Topic->__any =
				"tns1:VideoAnalytics/tns1:MotionDetection/tns1:Motion";
			*/
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Topic->__any =
				"tns1:VideoAnalytics/tnsavg:MotionDetection/tns1:Motion";
			/*
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Topic->__any =
				"tns1:VideoAnalytics/tnsavg:MotionDetection";
			*/
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Topic->Dialect =
				"http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet";
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Topic->__anyAttribute = NULL;
			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Topic->__mixed = NULL;

			tev__PullMessagesResponse->wsnt__NotificationMessage[i].Message.__any = response_message;
		}


		return SOAP_OK;
	}
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__SetSynchronizationPoint(struct soap *soap,
	struct _tev__SetSynchronizationPoint *tev__SetSynchronizationPoint,
	struct _tev__SetSynchronizationPointResponse *tev__SetSynchronizationPointResponse)
{

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;
	soap->header->wsa5__Action =
		"http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/SetSynchronizationPointResponse";

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__GetServiceCapabilities(struct soap *soap,
	struct _tev__GetServiceCapabilities *tev__GetServiceCapabilities,
	struct _tev__GetServiceCapabilitiesResponse *tev__GetServiceCapabilitiesResponse)
{
	int ret;
	onvif_special_info_t *info;
	info = (onvif_special_info_t *)soap_malloc(soap, sizeof(onvif_special_info_t));
	ASSERT(info);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	ASSERT(!ret);

	soap->header->wsa5__Action =
		"http://www.onvif.org/ver10/events/wsdl/EventPortType/GetServiceCapabilitiesResponse";
	soap->header->wsse__Security = NULL;
	struct tev__Capabilities *tev_capabilities =
		(struct tev__Capabilities *)soap_malloc(soap, sizeof(struct tev__Capabilities));
	ASSERT(tev_capabilities);

	tev_capabilities->__size = 0;
	tev_capabilities->__any = NULL;

	RS_DBG("\t ws sub policy support = %d\n", info->event_service_capabilities.ws_subscription_policy_support);
	RS_DBG("\t ws pull point support = %d\n", info->event_service_capabilities.ws_pull_point_support);
	RS_DBG("\t ws pausable support = %d\n", info->event_service_capabilities.ws_pausable_submgr_support);
	RS_DBG("\t ws persisten support = %d\n", info->event_service_capabilities.persistentNotificationStorage);
	RS_DBG("\t ws max producers= %d\n", info->event_service_capabilities.max_notificationproducers);
	RS_DBG("\t ws max_pull_point = %d\n", info->event_service_capabilities.max_pull_points);
	tev_capabilities->WSSubscriptionPolicySupport = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
	tev_capabilities->WSSubscriptionPolicySupport[0] =
		info->event_service_capabilities.ws_subscription_policy_support ? xsd__boolean__true_ : xsd__boolean__false_;

	tev_capabilities->WSPullPointSupport = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
	tev_capabilities->WSPullPointSupport[0] =
		info->event_service_capabilities.ws_pull_point_support ? xsd__boolean__true_ : xsd__boolean__false_;

	tev_capabilities->WSPausableSubscriptionManagerInterfaceSupport =
		(enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
	tev_capabilities->WSPausableSubscriptionManagerInterfaceSupport[0] =
		info->event_service_capabilities.ws_pausable_submgr_support ? xsd__boolean__true_ : xsd__boolean__false_;

	tev_capabilities->PersistentNotificationStorage = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
	tev_capabilities->PersistentNotificationStorage[0] =
		info->event_service_capabilities.persistentNotificationStorage ? xsd__boolean__true_ : xsd__boolean__false_;

	tev_capabilities->MaxNotificationProducers = (int *)soap_malloc(soap, sizeof(int));
	tev_capabilities->MaxNotificationProducers[0] = info->event_service_capabilities.max_notificationproducers;

	tev_capabilities->MaxPullPoints = (int *)soap_malloc(soap, sizeof(int));
	tev_capabilities->MaxPullPoints[0] = info->event_service_capabilities.max_pull_points;

	tev_capabilities->__anyAttribute = NULL;

	tev__GetServiceCapabilitiesResponse->Capabilities = tev_capabilities;

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__GetEventProperties(struct soap *soap,
	struct _tev__GetEventProperties *tev__GetEventProperties,
	struct _tev__GetEventPropertiesResponse *tev__GetEventPropertiesResponse)
{
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization and the sender is not authorized");
		return SOAP_FAULT;
	}

	soap->header->wsse__Security = NULL;
	soap->header->wsa5__Action = "http://www.onvif.org/ver10/events/wsdl/EventPortType/GetEventPropertiesResponse";


	tev__GetEventPropertiesResponse->__sizeTopicNamespaceLocation = 1;
	tev__GetEventPropertiesResponse->TopicNamespaceLocation = (char **)soap_malloc(soap, sizeof(char *));
	tev__GetEventPropertiesResponse->TopicNamespaceLocation[0] = (char *)soap_malloc(soap, sizeof(char) * 1024);
	strcpy(tev__GetEventPropertiesResponse->TopicNamespaceLocation[0],
		"http://www.onvif.org/onvif/ver10/topics/topicns.xml");
	tev__GetEventPropertiesResponse->wsnt__FixedTopicSet = xsd__boolean__true_;

	tev__GetEventPropertiesResponse->wstop__TopicSet =
		(struct wstop__TopicSetType *)soap_malloc(soap, sizeof(struct wstop__TopicSetType));
	tev__GetEventPropertiesResponse->wstop__TopicSet->documentation =
		(struct wstop__Documentation *)soap_malloc(soap, sizeof(struct wstop__Documentation));
	tev__GetEventPropertiesResponse->wstop__TopicSet->documentation->__size = 0;
	tev__GetEventPropertiesResponse->wstop__TopicSet->documentation->__any = NULL;
	tev__GetEventPropertiesResponse->wstop__TopicSet->documentation->__mixed = NULL;
	tev__GetEventPropertiesResponse->wstop__TopicSet->__anyAttribute = "xsd:string";

	tev__GetEventPropertiesResponse->wstop__TopicSet->__size = strlen(TOPSET_CONTENT) + 1;
	tev__GetEventPropertiesResponse->wstop__TopicSet->__any =
		(char *)soap_malloc(soap, tev__GetEventPropertiesResponse->wstop__TopicSet->__size);
	strcpy(tev__GetEventPropertiesResponse->wstop__TopicSet->__any, TOPSET_CONTENT);

	/*the following topic expressionDialect are mandatory for onvif compliant device*/
	tev__GetEventPropertiesResponse->__sizeTopicExpressionDialect = 2;
	tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect = (char **)soap_malloc(soap, sizeof(char *) * 2);
	tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[0] = (char *)soap_malloc(soap, 100);
	tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[1] = (char *)soap_malloc(soap, 100);
	strcpy(tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[0],
		"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete");
	strcpy(tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[1],
		"http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet");


	tev__GetEventPropertiesResponse->__sizeMessageContentFilterDialect = 1;
	tev__GetEventPropertiesResponse->MessageContentFilterDialect = (char **)soap_malloc(soap, sizeof(char *));
	tev__GetEventPropertiesResponse->MessageContentFilterDialect[0] = (char *)soap_malloc(soap, sizeof(char) * 1024);
	strcpy(tev__GetEventPropertiesResponse->MessageContentFilterDialect[0],
		"http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter");
	tev__GetEventPropertiesResponse->__sizeProducerPropertiesFilterDialect = 0;
	tev__GetEventPropertiesResponse->ProducerPropertiesFilterDialect = NULL;

	tev__GetEventPropertiesResponse->__sizeMessageContentSchemaLocation = 1;
	tev__GetEventPropertiesResponse->MessageContentSchemaLocation = (char **)soap_malloc(soap, sizeof(char *));
	tev__GetEventPropertiesResponse->MessageContentSchemaLocation[0] = (char *)soap_malloc(soap, sizeof(char) * 100);
	strcpy(tev__GetEventPropertiesResponse->MessageContentSchemaLocation[0],
		"http://www.onvif.org/onvif/ver10/schema/onvif.xsd");
	tev__GetEventPropertiesResponse->__size = 0;
	tev__GetEventPropertiesResponse->__any = NULL;

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6  __tev__CreatePullPointSubscription(struct soap *soap,
	struct _tev__CreatePullPointSubscription *tev__CreatePullPointSubscription,
	struct _tev__CreatePullPointSubscriptionResponse *tev__CreatePullPointSubscriptionResponse)
{

	int i = 0;
	char *address;
	time_t tnow;
	int ret = 0;
	long  time_out;
	network_config_t *network;
	onvif_special_info_t *info;
	int cur_submgr_num = -1;

	char *topic_filter = NULL;
	char *content_filter = NULL;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization and the sender is not authorized");
		return SOAP_FAULT;
	}


	soap->header->wsse__Security = NULL;
	soap->header->wsa5__Action =
		"http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse";

	if (tev__CreatePullPointSubscription->Filter) {
		if (tev__CreatePullPointSubscription->Filter->__size) {
			RS_DBG("\nfilter size %d, content: %s\n",
				tev__CreatePullPointSubscription->Filter->__size,
				tev__CreatePullPointSubscription->Filter->__any);

			split_filter(soap, tev__CreatePullPointSubscription->Filter->__any, &topic_filter, &content_filter);
			RS_DBG("topic_filter = %s\n", topic_filter);
			RS_DBG("content_filter = %s\n", content_filter);
			/*samply for test*/
			if (topic_filter) {
				if (!strstr(topic_filter, "VideoAnalytics") && topic_filter[0] != 0) {
					RS_DBG("VideoAnalytics ERROR\n");
					onvif_event_fault(soap, 1, INVALID_FILTER_FAULT, "wsnt:TopicExpression");
					return SOAP_FAULT;
				}
			}

			if (content_filter) {
				if (!strstr(content_filter, "VideoAnalyticsConfigurationToken")) {
					RS_DBG("VideoAnalyticsConfigurationToken ERROR\n");
					onvif_event_fault(soap, 1, INVALID_MESSAGE_CONTENT_EXPRESSION_FAULT, NULL);
					return SOAP_FAULT;
				}
			}
		}
	}


	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);
	info = (onvif_special_info_t *)soap_malloc(soap, sizeof(onvif_special_info_t));
	ASSERT(info);
	address = (char *)soap_malloc(soap, XADDR_BUF_SIZE);
	ASSERT(address);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	ASSERT(!ret);

	RS_DBG("IntialTerminationTime: %s\n", tev__CreatePullPointSubscription->InitialTerminationTime);

	LONG64 time_tmp_64;
	soap_s2xsd__duration(soap, tev__CreatePullPointSubscription->InitialTerminationTime, &time_tmp_64);

	RS_DBG("IntialTerminationTime: %lld\n", time_tmp_64);
	RS_DBG("IntialTerminationTime: %ld\n", (long)(time_tmp_64/1000));
	RS_DBG("size of long: %d\n", sizeof(long));
	RS_DBG("sizeof long long: %d\n", sizeof(long long));
	RS_DBG("sizeof LONG64: %d\n", sizeof(LONG64));
	time_out = (long)(time_tmp_64/1000);/*convert 2 second*/

	RS_DBG("time_out_str: %d\n", tev__CreatePullPointSubscription->InitialTerminationTime);
	RS_DBG("time_out %ld\n", time_out);

	if (tev__CreatePullPointSubscription->Filter != NULL) {
	   /*TODO;*/
	}

	RS_DBG("client address %s, port %s user %s\n ",
		getenv("REMOTE_ADDR"),
		getenv("REMOTE_PORT"),
		getenv("REMOTE_USER"));

	time(&tnow);
	char submgr_id[SUBMGR_ID_BUFFER_LEN];
	ret = rs_onvif_events_submgr_info_get_submgr_number(&cur_submgr_num);
	if (ret)
		sprintf(submgr_id, "Submgr1");
	else {
		/* if cur_submgr_num > max_pullpoint need return false; change later*/
		if (cur_submgr_num >= info->event_service_capabilities.max_pull_points) {
			onvif_event_fault(soap, 1, OUT_OF_MAX_PULLPOINTS_FAULT, NULL);
			return SOAP_FAULT;
		}
		sprintf(submgr_id, "Submgr%d", cur_submgr_num + 1);
	}

	/*get subscription manager id*/
	RS_DBG("submgr id %s\n", submgr_id);

	sprintf(address, "http://%s/onvif/%s?%s",
			inet_ntoa(network->interfaces[network->interface_idx].ip),
			info->service_name[SERVICE_INDEX_EVENTS],
			submgr_id);
	RS_DBG("Address %s\n", address);
	/*events_submgr type id timout notify_target_address*/
	/*lighttpd will make cgi as defunct process */
	/*spawn_new_process(submgr_cmd);*/
	control_message_create_submgr_t *arg =
		(control_message_create_submgr_t *)soap_malloc(soap,
			sizeof(control_message_create_submgr_t));

	ASSERT(arg);
	memset(arg, 0, sizeof(control_message_create_submgr_t));
	sprintf(arg->submgr_cmd, SUBMGR_EXE " %d %s %ld %s",
			SUBMGR_TYPE_PULLPOINT, submgr_id, time_out, "null"); /*pullpoint, no address is needed*/
	rsOnvifMsgCreateSubMgr(arg);
	sleep(2);
	/*this will block cgi need change later*/


	tev__CreatePullPointSubscriptionResponse->SubscriptionReference.Address = address;
	tev__CreatePullPointSubscriptionResponse->__any = NULL;
	tev__CreatePullPointSubscriptionResponse->wsnt__CurrentTime = tnow;
	tev__CreatePullPointSubscriptionResponse->wsnt__TerminationTime = tnow + time_out;

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__Renew(struct soap *soap,
	struct _wsnt__Renew *wsnt__Renew,
	struct _wsnt__RenewResponse *wsnt__RenewResponse)
{

	int i;
	int ret;
	char *query_string;
	long time_out;
	time_t tnow;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;
	soap->header->wsa5__Action = "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewResponse";

	LONG64 time_tmp_64;
	soap_s2xsd__duration(soap, wsnt__Renew->TerminationTime, &time_tmp_64);
	time_out = (long)(time_tmp_64/1000);/*convert 2 second*/
	RS_DBG("time_outstr = %s\n", wsnt__Renew->TerminationTime);
	RS_DBG("time_out = %ld\n", time_out);
	events_submgr_renew_t *arg = (events_submgr_renew_t *)soap_malloc(soap, sizeof(events_submgr_renew_t));
	ASSERT(arg);
	memset(arg, 0, sizeof(events_submgr_renew_t));

	query_string = getenv("QUERY_STRING");
	strncpy(arg->submgr_id, query_string, SUBMGR_ID_BUFFER_LEN);
	arg->time_out = time_out;

	RS_DBG("query string: %s\n", query_string);
	RS_DBG("renew: id %s, timeout %d\n", arg->submgr_id, arg->time_out);
	ret = rs_onvif_events_submgr_renew(arg);
	RS_DBG("renew event return : %d\n", ret);
	if (ret) {
		onvif_event_fault(soap, 1, RESOURCE_UNKNOWN_FAULT, NULL);
		return SOAP_FAULT;
	}

	time(&tnow);

	wsnt__RenewResponse->CurrentTime = (time_t *)soap_malloc(soap, sizeof(time_t));
	ASSERT(wsnt__RenewResponse->CurrentTime);
	*wsnt__RenewResponse->CurrentTime = tnow;
	wsnt__RenewResponse->TerminationTime = tnow + time_out;
	wsnt__RenewResponse->__size = 0;
	wsnt__RenewResponse->__any = NULL;


	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__Unsubscribe(struct soap *soap,
	struct _wsnt__Unsubscribe *wsnt__Unsubscribe,
	struct _wsnt__UnsubscribeResponse *wsnt__UnsubscribeResponse)
{

	int i;
	int ret;
	char *query_string;
	events_submgr_unscribe_t *arg;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;
	soap->header->wsa5__Action = "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeResponse";

	arg = (events_submgr_unscribe_t *)soap_malloc(soap, sizeof(events_submgr_unscribe_t));
	ASSERT(arg);
	memset(arg, 0, sizeof(events_submgr_unscribe_t));

	query_string = getenv("QUERY_STRING");
	strncpy(arg->submgr_id, query_string, SUBMGR_ID_BUFFER_LEN);
	RS_DBG("id %s\n", arg->submgr_id);
	ret = rs_onvif_events_submgr_unsubscribe(arg);
	if (ret) {
		onvif_event_fault(soap, 1, RESOURCE_UNKNOWN_FAULT, NULL);
		return SOAP_FAULT;
	}

	sleep(2);
	/*waiting for a while need change later*/

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__Subscribe(struct soap *soap,
	struct _wsnt__Subscribe *wsnt__Subscribe,
	struct _wsnt__SubscribeResponse *wsnt__SubscribeResponse)
{
	RS_DBG("Step In Func\n");
	char *address;
	char *initial_time = "PT10S";
	char *topic_filter = NULL;
	char *content_filter = NULL;
	time_t t_time;
	time_t tnow;
	int cur_submgr_num = -1;
	int ret = 0;

	network_config_t *network;
	onvif_special_info_t *info;
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization and the sender is not authorized");
		return SOAP_FAULT;
	}


	soap->header->wsse__Security = NULL;
	soap->header->wsa5__Action = "http://docs.oasis-open.org/wsn/bw-2/NotificationProducer/SubscribeResponse";


	if (wsnt__Subscribe->Filter) {
		RS_DBG("using Filter\n");
		RS_DBG("\nfilter size %d, content: %s\n",
			wsnt__Subscribe->Filter->__size,
			wsnt__Subscribe->Filter->__any);
		if (wsnt__Subscribe->Filter->__size) {
			split_filter(soap, wsnt__Subscribe->Filter->__any,
				&topic_filter, &content_filter);

			/*samply for test;*/
			if (topic_filter) {
				if (!strstr(topic_filter, "VideoAnalytics") &&
					topic_filter[0] != 0) {

					onvif_event_fault(soap, 1,
						INVALID_FILTER_FAULT,
						"wsnt:TopicExpression");
					return SOAP_FAULT;
				}
			}

			if (content_filter) {
				if (!strstr(content_filter,
					"VideoAnalyticsConfigurationToken")) {
					onvif_event_fault(soap, 1,
						INVALID_MESSAGE_CONTENT_EXPRESSION_FAULT,
						NULL);
					return SOAP_FAULT;
				}
			}
		}
	}

	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);
	info = (onvif_special_info_t *)soap_malloc(soap, sizeof(onvif_special_info_t));
	ASSERT(info);
	address = (char *)soap_malloc(soap, XADDR_BUF_SIZE);
	ASSERT(address);

	ret = rsOnvifSysInfoGetNetConfig(network);
	ASSERT(!ret);
	ret = rsOnvifSysInfoGetSpecialInfo(info);
	ASSERT(!ret);

	time(&tnow);

	RS_DBG("Termination time %s\n", wsnt__Subscribe->InitialTerminationTime);
	if (wsnt__Subscribe->InitialTerminationTime != NULL) {
		LONG64 time_tmp_64;
		soap_s2xsd__duration(soap, wsnt__Subscribe->InitialTerminationTime, &time_tmp_64);
		t_time = (long)(time_tmp_64 / 1000);/*convert 2 second*/
		RS_DBG("time_outstr = %s", wsnt__Subscribe->InitialTerminationTime);
		RS_DBG("time_out = %ld", t_time);
	} else {
		LONG64 time_tmp_64;
		soap_s2xsd__duration(soap, initial_time, &time_tmp_64);
		t_time = (long)(time_tmp_64 / 1000);/*convert 2 second*/
	}
	/*
	wsnt__SubscribeResponse->SubscriptionReference.Address = address;
	wsnt__SubscribeResponse->SubscriptionReference.Address = "http://172.29.40.79/onvif/device_service?11001100";
	*/
	char submgr_id[SUBMGR_ID_BUFFER_LEN];
	ret = rs_onvif_events_submgr_info_get_submgr_number(&cur_submgr_num);
	RS_DBG("cur_submgr_num = %d ret = %d", cur_submgr_num, ret);
	if (ret)
		sprintf(submgr_id, "Submgr1");
	else {
		/* if cur_submgr_num > max_producesers need return false; change later*/
		if (cur_submgr_num >= info->event_service_capabilities.max_notificationproducers) {
			onvif_event_fault(soap, 1, OUT_OF_MAX_PRODUCERS_FAULT, NULL);
			return SOAP_FAULT;
		}
		sprintf(submgr_id, "Submgr%d", cur_submgr_num+1);
	}

	RS_DBG("submgr id %s\n", submgr_id);
	sprintf(address, "http://%s/onvif/%s?%s",
			inet_ntoa(network->interfaces[network->interface_idx].ip),
			info->service_name[SERVICE_INDEX_EVENTS],
			submgr_id);
	RS_DBG("Address %s\n", address);
	RS_DBG("Consumer Address %s\n", wsnt__Subscribe->ConsumerReference.Address);
	/*events_submgr type id timout notify_target_address*/
	/*spawn_new_process(submgr_cmd);*/
	control_message_create_submgr_t *arg =
		(control_message_create_submgr_t *)soap_malloc(soap,
			sizeof(control_message_create_submgr_t));

	ASSERT(arg);
	memset(arg, 0, sizeof(control_message_create_submgr_t));
	sprintf(arg->submgr_cmd, SUBMGR_EXE " %d %s %ld %s",
			SUBMGR_TYPE_NOTIFICATION,
			submgr_id,
			t_time,
			wsnt__Subscribe->ConsumerReference.Address);
	rsOnvifMsgCreateSubMgr(arg);
	sleep(2);


	wsnt__SubscribeResponse->SubscriptionReference.Address = address;

	wsnt__SubscribeResponse->SubscriptionReference.ReferenceParameters = NULL;
	wsnt__SubscribeResponse->SubscriptionReference.Metadata = NULL;
	wsnt__SubscribeResponse->SubscriptionReference.__size = 0;
	wsnt__SubscribeResponse->SubscriptionReference.__any = NULL;
	wsnt__SubscribeResponse->SubscriptionReference.__anyAttribute = NULL;

	wsnt__SubscribeResponse->CurrentTime = (time_t *)soap_malloc(soap, sizeof(time_t));
	*wsnt__SubscribeResponse->CurrentTime = tnow;
	t_time += tnow;

	wsnt__SubscribeResponse->TerminationTime = (time_t *)soap_malloc(soap, sizeof(time_t));
	*wsnt__SubscribeResponse->TerminationTime = t_time;
	wsnt__SubscribeResponse->__size = 0;
	wsnt__SubscribeResponse->__any = NULL;


	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__GetCurrentMessage(struct soap *soap,
	struct _wsnt__GetCurrentMessage *wsnt__GetCurrentMessage,
	struct _wsnt__GetCurrentMessageResponse *wsnt__GetCurrentMessageResponse)
{

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__Notify(struct soap *soap,
	struct _wsnt__Notify *wsnt__Notify)
{
	RS_DBG("called\n");
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__GetMessages(struct soap *soap,
	struct _wsnt__GetMessages *wsnt__GetMessages,
	struct _wsnt__GetMessagesResponse *wsnt__GetMessagesResponse)
{

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__DestroyPullPoint(struct soap *soap,
	struct _wsnt__DestroyPullPoint *wsnt__DestroyPullPoint,
	struct _wsnt__DestroyPullPointResponse *wsnt__DestroyPullPointResponse)
{

	return SOAP_OK;
}



SOAP_FMAC5 int SOAP_FMAC6  __tev__CreatePullPoint(struct soap *soap,
	struct _wsnt__CreatePullPoint *wsnt__CreatePullPoint,
	struct _wsnt__CreatePullPointResponse *wsnt__CreatePullPointResponse)
{

	return SOAP_OK;
}



SOAP_FMAC5 int SOAP_FMAC6  __tev__PauseSubscription(struct soap *soap,
	struct _wsnt__PauseSubscription *wsnt__PauseSubscription,
	struct _wsnt__PauseSubscriptionResponse *wsnt__PauseSubscriptionResponse)
{

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6  __tev__ResumeSubscription(struct soap *soap,
	struct _wsnt__ResumeSubscription *wsnt__ResumeSubscription,
	struct _wsnt__ResumeSubscriptionResponse *wsnt__ResumeSubscriptionResponse)
{

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Seek(struct soap *soap,
	struct _tev__Seek *tev__Seek,
	struct _tev__SeekResponse *tev__SeekResponces)
{
	return SOAP_OK;
}
