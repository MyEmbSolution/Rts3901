#include <linux/videodev2.h>
#include <octopus/rts_cam.h>
#include "soapH.h"
#include "rsOnvifTypes.h"
#include "rsOnvifDevCtrl.h"

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

#define TMP_AUDIO_PATH "/dev/snd/pcmC0D0p"

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetServiceCapabilities(struct soap *soap,
	struct _trt__GetServiceCapabilities *trt__GetServiceCapabilities,
	struct _trt__GetServiceCapabilitiesResponse *trt__GetServiceCapabilitiesResponse)
{
	soap->header->wsse__Security = NULL;

	struct trt__Capabilities *cap =
		(struct trt__Capabilities *)soap_malloc(soap,
				sizeof(struct trt__Capabilities));
	ASSERT(cap);
	trt__GetServiceCapabilitiesResponse->Capabilities = cap;

	cap->ProfileCapabilities =
		(struct trt__ProfileCapabilities *)soap_malloc(soap,
				sizeof(struct trt__ProfileCapabilities));
	ASSERT(cap->ProfileCapabilities);

	cap->ProfileCapabilities->__size = 0;
	cap->ProfileCapabilities->__any = NULL;
	cap->ProfileCapabilities->MaximumNumberOfProfiles =
		(int *)soap_malloc(soap, sizeof(int));
	ASSERT(cap->ProfileCapabilities->MaximumNumberOfProfiles);
	*cap->ProfileCapabilities->MaximumNumberOfProfiles = MAX_MEDIA_PROFILE;
	cap->ProfileCapabilities->__anyAttribute = NULL;

	cap->StreamingCapabilities =
		(struct trt__StreamingCapabilities *)soap_malloc(soap,
				sizeof(struct trt__StreamingCapabilities));
	ASSERT(cap->StreamingCapabilities);
	cap->StreamingCapabilities->__size = 0;
	cap->StreamingCapabilities->__any = NULL;
	cap->StreamingCapabilities->RTPMulticast =
		(enum xsd__boolean *)soap_malloc(soap,
				sizeof(enum xsd__boolean));
	cap->StreamingCapabilities->RTPMulticast[0] = xsd__boolean__false_;

	cap->StreamingCapabilities->RTP_USCORETCP =
		(enum xsd__boolean *)soap_malloc(soap,
				sizeof(enum xsd__boolean));
	cap->StreamingCapabilities->RTP_USCORETCP[0] = xsd__boolean__false_;

	cap->StreamingCapabilities->RTP_USCORERTSP_USCORETCP =
		(enum xsd__boolean *)soap_malloc(soap,
				sizeof(enum xsd__boolean));
	cap->StreamingCapabilities->RTP_USCORERTSP_USCORETCP[0] =
		xsd__boolean__true_;

	cap->StreamingCapabilities->NonAggregateControl =
		(enum xsd__boolean *)soap_malloc(soap,
				sizeof(enum xsd__boolean));
	cap->StreamingCapabilities->NonAggregateControl[0] =
		xsd__boolean__true_;

	cap->StreamingCapabilities->__anyAttribute = NULL;

	cap->__size = 0;
	cap->__any = NULL;
	cap->SnapshotUri = (enum xsd__boolean *)soap_malloc(soap,
			sizeof(enum xsd__boolean));
	cap->SnapshotUri[0] = xsd__boolean__false_;
	cap->Rotation = (enum xsd__boolean *)soap_malloc(soap,
			sizeof(enum xsd__boolean));
	cap->Rotation[0] = xsd__boolean__false_;
	cap->OSD = (enum xsd__boolean *)soap_malloc(soap,
			sizeof(enum xsd__boolean));
	cap->OSD[0] = xsd__boolean__true_;
	cap->__anyAttribute = NULL;

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSources(struct soap *soap,
	struct _trt__GetVideoSources *trt__GetVideoSources,
	struct _trt__GetVideoSourcesResponse *trt__GetVideoSourcesResponse)
{
	struct _trt__GetVideoSourcesResponse *p_response =
		trt__GetVideoSourcesResponse;

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


	/* Still got problem here,I just response sensor's infomation to client*/
	p_response->__sizeVideoSources = 1;
	p_response->VideoSources =
		(struct tt__VideoSource *)soap_malloc(soap,
				sizeof(struct tt__VideoSource));
	p_response->VideoSources->token = (char *)soap_malloc(soap,
			sizeof(char) * MID_INFO_SIZE);
	strcpy(p_response->VideoSources->token, "VideoSource0");
	/* sprintf(p_response->VideoSources->token, "rts_video_source_%s", SENSOR_NAME); */
	p_response->VideoSources->Framerate = VS_MAX_FPS;
	p_response->VideoSources->Resolution =
		(struct tt__VideoResolution *)soap_malloc(soap,
				sizeof(struct tt__VideoResolution));
	p_response->VideoSources->Resolution->Width = SENSOR_FMT_WIDTH;
	p_response->VideoSources->Resolution->Height = SENSOR_FMT_HEIGHT;
	p_response->VideoSources->Imaging   = NULL;
	p_response->VideoSources->Extension = NULL;

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSources(struct soap *soap,
	struct _trt__GetAudioSources *trt__GetAudioSources,
	struct _trt__GetAudioSourcesResponse *trt__GetAudioSourcesResponse)
{
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

	struct _trt__GetAudioSourcesResponse *p_response =
		trt__GetAudioSourcesResponse;
	p_response->__sizeAudioSources = 1;
	p_response->AudioSources = (struct tt__AudioSource *)soap_malloc(soap,
			sizeof(struct tt__AudioSource));
	p_response->AudioSources->token = (char *)soap_malloc(soap,
			sizeof(char) * MID_INFO_SIZE);
	strcpy(p_response->AudioSources->token, "AudioSource0");
	p_response->AudioSources->Channels = 1;
	p_response->AudioSources->__size = 0;
	p_response->AudioSources->__any = 0;
	p_response->AudioSources->__anyAttribute = 0;
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputs(struct soap *soap,
	struct _trt__GetAudioOutputs *trt__GetAudioOutputs,
	struct _trt__GetAudioOutputsResponse *trt__GetAudioOutputsResponse)
{
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

	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported",
			"ter:AudioOutputNotSupported");
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__CreateProfile(struct soap *soap,
	struct _trt__CreateProfile *trt__CreateProfile,
	struct _trt__CreateProfileResponse *trt__CreateProfileResponse)
{
	int i = 0, ret, num;
	struct _trt__CreateProfileResponse *p_response = trt__CreateProfileResponse;
	media_profile_t *rs_profiles = NULL;
	media_config_t *rs_configs = NULL;
	int profile_count = 0;
	if (trt__CreateProfile->Token == NULL) {
		/*pay attention that here if Token is empty will use the same as Name!!*/
		trt__CreateProfile->Token = trt__CreateProfile->Name;
	}

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
	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	/*Check if ProfileToken Exist or Not*/
	for (i = 0; i < profile_count; i++) {
		if (!strcmp(trt__CreateProfile->Token, rs_profiles[i].token)) {
			onvif_fault(soap, SENDER, "ter:InvalidArgVal",
					"ter:ProfileExists");
			return SOAP_FAULT;
		}
	}

	/*check the limit on number of profiles*/
	if (profile_count >= MAX_MEDIA_PROFILE) {
		onvif_fault(soap, RECEIVER, "ter:Action ",
				"ter:MaxNVTProfiles");
		return SOAP_FAULT;
	} else {
		strcpy(rs_profiles[profile_count].name, trt__CreateProfile->Name);
		strcpy(rs_profiles[profile_count].token, trt__CreateProfile->Token);
		rs_profiles[profile_count].fixed = 0;
	}

	/*save profile*/
	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);

	if (ret == -1) {
		onvif_fault(soap, RECEIVER, "ter:InvalidArgVal", "ter:AddProfileFailed");
		return SOAP_FAULT;
	}

	p_response->Profile = (struct tt__Profile *)soap_malloc(soap,
			sizeof(struct tt__Profile));
	p_response->Profile->Name  = rs_profiles[profile_count].name;
	p_response->Profile->token = rs_profiles[profile_count].token;
	p_response->Profile->fixed = 0;
	p_response->Profile->VideoSourceConfiguration = NULL;
	p_response->Profile->AudioSourceConfiguration = NULL;
	p_response->Profile->VideoEncoderConfiguration = NULL;
	p_response->Profile->AudioEncoderConfiguration = NULL;
	p_response->Profile->VideoAnalyticsConfiguration = NULL;
	p_response->Profile->PTZConfiguration = NULL;
	p_response->Profile->MetadataConfiguration = NULL;
	p_response->Profile->Extension = NULL;

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetProfile(struct soap *soap,
	struct _trt__GetProfile *trt__GetProfile,
	struct _trt__GetProfileResponse *trt__GetProfileResponse)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	media_profile_t *rs_profiles = NULL;
	media_config_t *rs_configs = NULL;
	int profile_count = 0;
	int is_token_exist = 0;
	struct _trt__GetProfileResponse *p_response = trt__GetProfileResponse;

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

	if ((trt__GetProfile->ProfileToken) == NULL ||
		!strcmp(trt__GetProfile->ProfileToken, "")) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal",
				"ter:InvalidInputToken");
		return SOAP_FAULT;
	}
	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap,
			sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;


	/* Check if ProfileToken Exist or Not*/
	for (i = 0; i < profile_count; i++) {
		if (!strcmp(trt__GetProfile->ProfileToken,
					rs_profiles[i].token)) {
			is_token_exist = 1;
			break;
		}
	}

	if (!is_token_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal",
				"ter:NoProfile");
		return SOAP_FAULT;
	}

	p_response->Profile = (struct tt__Profile *)soap_malloc(soap,
			sizeof(struct tt__Profile));
	p_response->Profile->Name  = rs_profiles[i].name;
	p_response->Profile->token = rs_profiles[i].token;
	p_response->Profile->fixed = (enum xsd__boolean *)soap_malloc(soap,
			sizeof(enum xsd__boolean));
	p_response->Profile->fixed[0] =
		rs_profiles[i].fixed  ? xsd__boolean__true_ : xsd__boolean__false_;

	/* VideoSourceConfiguration*/
	p_response->Profile->VideoSourceConfiguration = NULL;
	if (rs_profiles[i].vsc_index) {
		j = rs_profiles[i].vsc_index - 1;

		p_response->Profile->VideoSourceConfiguration =
			(struct tt__VideoSourceConfiguration *)soap_malloc(soap,
					sizeof(struct tt__VideoSourceConfiguration));
		p_response->Profile->VideoSourceConfiguration->Name =
			rs_configs->vsc[j].name;
		p_response->Profile->VideoSourceConfiguration->UseCount =
			rs_configs->vsc[j].use_count;
		p_response->Profile->VideoSourceConfiguration->token =
			rs_configs->vsc[j].token;
		p_response->Profile->VideoSourceConfiguration->SourceToken =
			rs_configs->vsc[j].source_token;
		p_response->Profile->VideoSourceConfiguration->Bounds =
			(struct tt__IntRectangle *)soap_malloc(soap,
					sizeof(struct tt__IntRectangle));
		p_response->Profile->VideoSourceConfiguration->Bounds->y =
			rs_configs->vsc[j].bounds_y;
		p_response->Profile->VideoSourceConfiguration->Bounds->x =
			rs_configs->vsc[j].bounds_x;
		p_response->Profile->VideoSourceConfiguration->Bounds->width =
			rs_configs->vsc[j].bounds_width;
		p_response->Profile->VideoSourceConfiguration->Bounds->height =
			rs_configs->vsc[j].bounds_height;

		p_response->Profile->VideoSourceConfiguration->__size = 0;
		p_response->Profile->VideoSourceConfiguration->__any = NULL;
		p_response->Profile->VideoSourceConfiguration->Extension = NULL;
		p_response->Profile->VideoSourceConfiguration->__anyAttribute = NULL;
	}

	/*VideoEncoderConfiguration*/
	p_response->Profile->VideoEncoderConfiguration = NULL;
	if (rs_profiles[i].vec_index) {
		j = rs_profiles[i].vec_index - 1;

		p_response->Profile->VideoEncoderConfiguration =
			(struct tt__VideoEncoderConfiguration *)soap_malloc(soap,
					sizeof(struct tt__VideoEncoderConfiguration));
		p_response->Profile->VideoEncoderConfiguration->Name =
			rs_configs->vec[j].name;
		p_response->Profile->VideoEncoderConfiguration->UseCount =
			rs_configs->vec[j].use_count;
		p_response->Profile->VideoEncoderConfiguration->token =
			rs_configs->vec[j].token;
		p_response->Profile->VideoEncoderConfiguration->Encoding =
			rs_configs->vec[j].encoding_format;

		p_response->Profile->VideoEncoderConfiguration->Resolution =
			(struct tt__VideoResolution *)soap_malloc(soap,
					sizeof(struct tt__VideoResolution));
		p_response->Profile->VideoEncoderConfiguration->Resolution->Width =
			rs_configs->vec[j].encoding_width;
		p_response->Profile->VideoEncoderConfiguration->Resolution->Height =
			rs_configs->vec[j].encoding_height;
		p_response->Profile->VideoEncoderConfiguration->Quality =
			rs_configs->vec[j].quality;
		p_response->Profile->VideoEncoderConfiguration->RateControl =
			(struct tt__VideoRateControl *)soap_malloc(soap,
					sizeof(struct tt__VideoRateControl));
		p_response->Profile->VideoEncoderConfiguration->RateControl->FrameRateLimit =
			rs_configs->vec[j].frame_rate_limit;
		p_response->Profile->VideoEncoderConfiguration->RateControl->EncodingInterval =
			rs_configs->vec[j].encoding_interval;
		p_response->Profile->VideoEncoderConfiguration->RateControl->BitrateLimit =
			rs_configs->vec[j].bitrate_limit;

		p_response->Profile->VideoEncoderConfiguration->MPEG4 = NULL;

		p_response->Profile->VideoEncoderConfiguration->H264 =
			(struct tt__H264Configuration *)soap_malloc(soap,
					sizeof(struct tt__H264Configuration));
		p_response->Profile->VideoEncoderConfiguration->H264->GovLength =
			rs_configs->vec[j].h264_gov_length;
		p_response->Profile->VideoEncoderConfiguration->H264->H264Profile =
			rs_configs->vec[j].h264_profile;

		p_response->Profile->VideoEncoderConfiguration->Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
					sizeof(struct tt__MulticastConfiguration));
		p_response->Profile->VideoEncoderConfiguration->Multicast->Port =
			rs_configs->vec[j].multicast_port;
		p_response->Profile->VideoEncoderConfiguration->Multicast->TTL =
			rs_configs->vec[j].multicast_ttl;
		p_response->Profile->VideoEncoderConfiguration->Multicast->AutoStart =
			rs_configs->vec[j].multicast_auto_start ? xsd__boolean__true_ : xsd__boolean__false_;
		p_response->Profile->VideoEncoderConfiguration->Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap,
					sizeof(struct tt__IPAddress));
		p_response->Profile->VideoEncoderConfiguration->Multicast->Address->Type =
			rs_configs->vec[j].multicast_ip_type;
		p_response->Profile->VideoEncoderConfiguration->Multicast->Address->IPv4Address =
			rs_configs->vec[j].multicast_ip_addr;
		p_response->Profile->VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;

		p_response->Profile->VideoEncoderConfiguration->Multicast->__size = 0;
		p_response->Profile->VideoEncoderConfiguration->Multicast->__any = NULL;
		p_response->Profile->VideoEncoderConfiguration->Multicast->__anyAttribute = NULL;

		p_response->Profile->VideoEncoderConfiguration->SessionTimeout =
			rs_configs->vec[j].session_timeout;

		p_response->Profile->VideoEncoderConfiguration->__size = 0;
		p_response->Profile->VideoEncoderConfiguration->__any = NULL;
		p_response->Profile->VideoEncoderConfiguration->__anyAttribute = NULL;

		p_response->Profile->VideoEncoderConfiguration->__size = 0;
		p_response->Profile->VideoEncoderConfiguration->__any = NULL;
		p_response->Profile->VideoEncoderConfiguration->__anyAttribute = NULL;
	}

	/* AudioEncoderConfiguration*/
	p_response->Profile->AudioEncoderConfiguration = NULL;
	if (rs_profiles[i].aec_index) {
		p_response->Profile->AudioEncoderConfiguration =
			(struct tt__AudioEncoderConfiguration *)soap_malloc(soap,
					sizeof(struct tt__AudioEncoderConfiguration));
		ASSERT(p_response->Profile->AudioEncoderConfiguration);
		j = rs_profiles[i].aec_index - 1;

		p_response->Profile->AudioEncoderConfiguration->Name =
			rs_configs->aec[j].name;
		p_response->Profile->AudioEncoderConfiguration->UseCount =
			rs_configs->aec[j].use_count;
		p_response->Profile->AudioEncoderConfiguration->token =
			rs_configs->aec[j].token;/*"G711";*/
		p_response->Profile->AudioEncoderConfiguration->Encoding =
			rs_configs->aec[j].encoding_format;/*{tt__AudioEncoding__G711 = 0, tt__AudioEncoding__G726 = 1, tt__AudioEncoding__AAC = 2}*/
		p_response->Profile->AudioEncoderConfiguration->Bitrate =
			rs_configs->aec[j].bit_rate;
		p_response->Profile->AudioEncoderConfiguration->SampleRate =
			rs_configs->aec[j].sample_rate;
		p_response->Profile->AudioEncoderConfiguration->SessionTimeout =
			rs_configs->aec[j].session_timeout;
		p_response->Profile->AudioEncoderConfiguration->Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
					sizeof(struct tt__MulticastConfiguration));
		p_response->Profile->AudioEncoderConfiguration->Multicast->Port =
			rs_configs->aec[j].port;
		p_response->Profile->AudioEncoderConfiguration->Multicast->TTL =
			rs_configs->aec[j].ttl;
		p_response->Profile->AudioEncoderConfiguration->Multicast->AutoStart =
			rs_configs->aec[j].auto_start;
		p_response->Profile->AudioEncoderConfiguration->Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap,
					sizeof(struct tt__IPAddress));
		p_response->Profile->AudioEncoderConfiguration->Multicast->Address->Type = rs_configs->aec[j].multicast_ip_type;
		p_response->Profile->AudioEncoderConfiguration->Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap,
					sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Profile->AudioEncoderConfiguration->Multicast->Address->IPv4Address, rs_configs->aec[j].multicast_ip_addr);
		p_response->Profile->AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
	}
	/* AudioSourceConfiguration*/
	p_response->Profile->AudioSourceConfiguration = NULL;
	if (rs_profiles[i].asc_index) {
		p_response->Profile->AudioSourceConfiguration =
			(struct tt__AudioSourceConfiguration *)soap_malloc(soap,
					sizeof(struct tt__AudioSourceConfiguration));
		ASSERT(p_response->Profile->AudioSourceConfiguration);

		j = rs_profiles[i].asc_index - 1;
		p_response->Profile->AudioSourceConfiguration->Name =
			rs_configs->asc[j].name;
		p_response->Profile->AudioSourceConfiguration->UseCount =
			rs_configs->asc[j].use_count;
		p_response->Profile->AudioSourceConfiguration->token =
			rs_configs->asc[j].token;
		p_response->Profile->AudioSourceConfiguration->SourceToken =
			rs_configs->asc[j].source_token;
	}

	/* PTZConfiguration*/
	p_response->Profile->PTZConfiguration = NULL;
	/* VideoAnalyticsConfiguration*/
	p_response->Profile->VideoAnalyticsConfiguration = NULL;
	/* MetadataConfiguration*/
	p_response->Profile->MetadataConfiguration = NULL;

	p_response->Profile->Extension = NULL;
	p_response->Profile->__anyAttribute = NULL;

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetProfiles(struct soap *soap,
	struct _trt__GetProfiles *trt__GetProfiles,
	struct _trt__GetProfilesResponse *trt__GetProfilesResponse)
{
	int ret;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;
	int i;
	int j = 0;

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

	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap,
			sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	trt__GetProfilesResponse->__sizeProfiles = profile_count;

	struct tt__Profile *tt_profiles;
	if (profile_count)
		tt_profiles = (struct tt__Profile *)soap_malloc(soap,
				profile_count * sizeof(struct tt__Profile));

	for (i = 0; i < profile_count; i++) {
		RS_DBG("profile name %s\n", rs_profiles[i].name);
		tt_profiles[i].Name  = rs_profiles[i].name;
		tt_profiles[i].token = rs_profiles[i].token;
		tt_profiles[i].fixed = (enum xsd__boolean *)soap_malloc(soap,
				sizeof(enum xsd__boolean));
		tt_profiles[i].fixed[0] =
			rs_profiles[i].fixed ? xsd__boolean__true_ : xsd__boolean__false_;

		/* VideoSourceConfiguration*/
		tt_profiles[i].VideoSourceConfiguration = NULL;
		if (rs_profiles[i].vsc_index) {
			RS_DBG("vsc name %s\n", rs_profiles[i].name);
			tt_profiles[i].VideoSourceConfiguration =
				(struct tt__VideoSourceConfiguration *)soap_malloc(soap,
					sizeof(struct tt__VideoSourceConfiguration));
			ASSERT(tt_profiles[i].VideoSourceConfiguration);

			j = rs_profiles[i].vsc_index - 1;
			tt_profiles[i].VideoSourceConfiguration->Name = rs_configs->vsc[j].name;
			tt_profiles[i].VideoSourceConfiguration->token = rs_configs->vsc[j].token;
			tt_profiles[i].VideoSourceConfiguration->UseCount = rs_configs->vsc[j].use_count;
			RS_DBG("source token: %s\n", rs_configs->vsc[j].source_token);
			tt_profiles[i].VideoSourceConfiguration->SourceToken = rs_configs->vsc[j].source_token;
			tt_profiles[i].VideoSourceConfiguration->Bounds =
				(struct tt__IntRectangle *)soap_malloc(soap, sizeof(struct tt__IntRectangle));
			tt_profiles[i].VideoSourceConfiguration->Bounds->y = rs_configs->vsc[j].bounds_y;
			tt_profiles[i].VideoSourceConfiguration->Bounds->x = rs_configs->vsc[j].bounds_x;
			tt_profiles[i].VideoSourceConfiguration->Bounds->width = rs_configs->vsc[j].bounds_width;
			tt_profiles[i].VideoSourceConfiguration->Bounds->height = rs_configs->vsc[j].bounds_height;

			tt_profiles[i].VideoSourceConfiguration->__size = 0;
			tt_profiles[i].VideoSourceConfiguration->__any = NULL;
			tt_profiles[i].VideoSourceConfiguration->Extension = NULL;
			tt_profiles[i].VideoSourceConfiguration->__anyAttribute = NULL;
		}
		/* VideoEncoderConfiguration*/
		tt_profiles[i].VideoEncoderConfiguration = NULL;
		if (rs_profiles[i].vec_index) {
			RS_DBG("vec name %s\n", rs_configs->vec[rs_profiles[i].vec_index - 1].name);
			tt_profiles[i].VideoEncoderConfiguration =
				(struct tt__VideoEncoderConfiguration *)soap_malloc(soap,
					sizeof(struct tt__VideoEncoderConfiguration));
			ASSERT(tt_profiles[i].VideoEncoderConfiguration);

			j = rs_profiles[i].vec_index - 1;
			tt_profiles[i].VideoEncoderConfiguration->Name = rs_configs->vec[j].name;
			tt_profiles[i].VideoEncoderConfiguration->token = rs_configs->vec[j].token;
			tt_profiles[i].VideoEncoderConfiguration->UseCount = rs_configs->vec[j].use_count;
			tt_profiles[i].VideoEncoderConfiguration->Encoding = rs_configs->vec[j].encoding_format;

			tt_profiles[i].VideoEncoderConfiguration->Resolution =
				(struct tt__VideoResolution *)soap_malloc(soap, sizeof(struct tt__VideoResolution));

			tt_profiles[i].VideoEncoderConfiguration->Resolution->Width = rs_configs->vec[j].encoding_width;
			tt_profiles[i].VideoEncoderConfiguration->Resolution->Height = rs_configs->vec[j].encoding_height;
			tt_profiles[i].VideoEncoderConfiguration->Quality = rs_configs->vec[j].quality;

			tt_profiles[i].VideoEncoderConfiguration->RateControl =
				(struct tt__VideoRateControl *)soap_malloc(soap, sizeof(struct tt__VideoRateControl));
			tt_profiles[i].VideoEncoderConfiguration->RateControl->FrameRateLimit =
				rs_configs->vec[j].frame_rate_limit;
			tt_profiles[i].VideoEncoderConfiguration->RateControl->EncodingInterval =
				rs_configs->vec[j].encoding_interval;
			tt_profiles[i].VideoEncoderConfiguration->RateControl->BitrateLimit =
				rs_configs->vec[j].bitrate_limit;

			/*
			RS_DBG("vec %d frame_rate_limit %d\n", i, rs_configs->vec[j].frame_rate_limit);
			RS_DBG("vec %d encoding_interval %d\n", i, rs_configs->vec[j].encoding_interval);
			RS_DBG("vec %d bitrate_limit %d\n", i, rs_configs->vec[j].bitrate_limit);
			*/
			tt_profiles[i].VideoEncoderConfiguration->MPEG4 = NULL;

			tt_profiles[i].VideoEncoderConfiguration->H264 =
				(struct tt__H264Configuration *)soap_malloc(soap, sizeof(struct tt__H264Configuration));
			tt_profiles[i].VideoEncoderConfiguration->H264->GovLength =
				rs_configs->vec[j].h264_gov_length;
			tt_profiles[i].VideoEncoderConfiguration->H264->H264Profile = rs_configs->vec[j].h264_profile;

			tt_profiles[i].VideoEncoderConfiguration->Multicast =
				(struct tt__MulticastConfiguration *)soap_malloc(soap,
					sizeof(struct tt__MulticastConfiguration));
			tt_profiles[i].VideoEncoderConfiguration->Multicast->Port = rs_configs->vec[j].multicast_port;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->TTL  = rs_configs->vec[j].multicast_ttl;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->AutoStart =
				rs_configs->vec[j].multicast_auto_start ? xsd__boolean__true_ : xsd__boolean__false_;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->__size = 0;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->__any = NULL;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->__anyAttribute = NULL;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->Address =
				(struct tt__IPAddress *)soap_malloc(soap, sizeof(struct tt__IPAddress));
			tt_profiles[i].VideoEncoderConfiguration->Multicast->Address->Type =
				rs_configs->vec[j].multicast_ip_type;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->Address->IPv4Address =
				rs_configs->vec[j].multicast_ip_addr;
			tt_profiles[i].VideoEncoderConfiguration->Multicast->Address->IPv6Address = NULL;

			/*tt_profiles[i].VideoEncoderConfiguration->SessionTimeout = (char *)soap_malloc(soap, 16);*/
			/* the new version SessionTimeout is LONG yafei need change later */
			tt_profiles[i].VideoEncoderConfiguration->SessionTimeout = rs_configs->vec[j].session_timeout;

			tt_profiles[i].VideoEncoderConfiguration->__size = 0;
			tt_profiles[i].VideoEncoderConfiguration->__any = NULL;
			tt_profiles[i].VideoEncoderConfiguration->__anyAttribute = NULL;
		}

		/*AudioSourceConfiguration*/
		tt_profiles[i].AudioSourceConfiguration = NULL;
		if (rs_profiles[i].asc_index) {
			tt_profiles[i].AudioSourceConfiguration =
				(struct tt__AudioSourceConfiguration *)soap_malloc(soap,
					sizeof(struct tt__AudioSourceConfiguration));
			ASSERT(tt_profiles[i].AudioSourceConfiguration);

			j = rs_profiles[i].asc_index - 1;
			tt_profiles[i].AudioSourceConfiguration->Name = rs_configs->asc[j].name;
			tt_profiles[i].AudioSourceConfiguration->UseCount = rs_configs->asc[j].use_count;
			tt_profiles[i].AudioSourceConfiguration->token = rs_configs->asc[j].token;
			tt_profiles[i].AudioSourceConfiguration->SourceToken = rs_configs->asc[j].source_token;
		}

		/*AudioEncoderConfiguration*/
		tt_profiles[i].AudioEncoderConfiguration = NULL;
		if (rs_profiles[i].aec_index) {
			tt_profiles[i].AudioEncoderConfiguration =
				(struct tt__AudioEncoderConfiguration *)soap_malloc(soap,
					sizeof(struct tt__AudioEncoderConfiguration));
			ASSERT(tt_profiles[i].AudioEncoderConfiguration);
			j = rs_profiles[i].aec_index - 1;

			tt_profiles[i].AudioEncoderConfiguration->Name = rs_configs->aec[j].name;
			tt_profiles[i].AudioEncoderConfiguration->UseCount = rs_configs->aec[j].use_count;
			tt_profiles[i].AudioEncoderConfiguration->token = rs_configs->aec[j].token;/*"G711";*/
			tt_profiles[i].AudioEncoderConfiguration->Encoding = rs_configs->aec[j].encoding_format;/*{tt__AudioEncoding__G711 = 0, tt__AudioEncoding__G726 = 1, tt__AudioEncoding__AAC = 2}*/
			tt_profiles[i].AudioEncoderConfiguration->Bitrate = rs_configs->aec[j].bit_rate;
			tt_profiles[i].AudioEncoderConfiguration->SampleRate = rs_configs->aec[j].sample_rate;
			tt_profiles[i].AudioEncoderConfiguration->SessionTimeout = rs_configs->aec[j].session_timeout;
			tt_profiles[i].AudioEncoderConfiguration->Multicast =
				(struct tt__MulticastConfiguration *)soap_malloc(soap,
						sizeof(struct tt__MulticastConfiguration));
			tt_profiles[i].AudioEncoderConfiguration->Multicast->Port = rs_configs->aec[j].port;
			tt_profiles[i].AudioEncoderConfiguration->Multicast->TTL = rs_configs->aec[j].ttl;
			tt_profiles[i].AudioEncoderConfiguration->Multicast->AutoStart = rs_configs->aec[j].auto_start;
			tt_profiles[i].AudioEncoderConfiguration->Multicast->Address =
				(struct tt__IPAddress *)soap_malloc(soap,
						sizeof(struct tt__IPAddress));
			tt_profiles[i].AudioEncoderConfiguration->Multicast->Address->Type = rs_configs->aec[j].multicast_ip_type;
			tt_profiles[i].AudioEncoderConfiguration->Multicast->Address->IPv4Address =
				(char *)soap_malloc(soap, sizeof(char) * MAX_IPADDR_STR_LENGTH);
			strcpy(tt_profiles[i].AudioEncoderConfiguration->Multicast->Address->IPv4Address, rs_configs->aec[j].multicast_ip_addr);
			tt_profiles[i].AudioEncoderConfiguration->Multicast->Address->IPv6Address = NULL;
		}

		/* PTZConfiguration*/
		tt_profiles[i].PTZConfiguration = NULL;

		/*VideoAnalyticsConfiguration*/
		tt_profiles[i].VideoAnalyticsConfiguration = NULL;
		/* MetadataConfiguration*/
		tt_profiles[i].MetadataConfiguration = NULL;

		tt_profiles[i].__anyAttribute = NULL;
	}
	trt__GetProfilesResponse->Profiles = tt_profiles;

	return SOAP_OK;
}

/*adds a VideoEncoderConfiguration to an existing media profile. If a configuration exists in the media profile, it will be replaced.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoEncoderConfiguration(struct soap *soap,
	struct _trt__AddVideoEncoderConfiguration *trt__AddVideoEncoderConfiguration,
	struct _trt__AddVideoEncoderConfigurationResponse *trt__AddVideoEncoderConfigurationResponse)
{
	struct _trt__AddVideoEncoderConfiguration *p_request = trt__AddVideoEncoderConfiguration;
	int is_profile_exist = 0;
	int is_vec_exist = 0;
	int i = 0;
	int j = 0;

	int ret = 0;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;
	int vec_count = 0;

	struct tt__Profile *tt_profiles;

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

	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap,
			sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken,
						rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	vec_count = 0;
	while (vec_count < MAX_MEDIA_VEC_CONFIG &&
			rs_configs->vec[vec_count].name[0] != '\0')
		vec_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < vec_count; j++) {
			if (!strcmp(p_request->ConfigurationToken,
						rs_configs->vec[j].token)) {
				is_vec_exist = 1;
				break;
			}
		}
	}

	if (!is_profile_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (!is_vec_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		if (rs_profiles[i].vec_index) {
			rs_configs->vec[rs_profiles[i].vec_index - 1].use_count--;
		}
		rs_configs->vec[j].use_count++;
		rs_profiles[i].vec_index = j + 1;
	}

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	return SOAP_OK;
}

/**
* adds a VideoEncoderConfiguration to an existing media profile. if such a configuration exists,it will be replaced.
*
*
*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoSourceConfiguration(struct soap *soap,
	struct _trt__AddVideoSourceConfiguration *trt__AddVideoSourceConfiguration,
	struct _trt__AddVideoSourceConfigurationResponse *trt__AddVideoSourceConfigurationResponse)
{
	int is_profile_exist = 0;
	int is_vsc_exist = 0;
	struct _trt__AddVideoSourceConfiguration *p_request = trt__AddVideoSourceConfiguration;
	int i = 0;
	int j = 0;
	int ret = 0;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;
	int vsc_count = 0;

	struct tt__Profile *tt_profiles;

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

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	vsc_count = 0;
	while (vsc_count < MAX_MEDIA_VSC_CONFIG &&
		rs_configs->vsc[vsc_count].name[0] != '\0')
		vsc_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < vsc_count; j++) {
			if (!strcmp(p_request->ConfigurationToken,
						rs_configs->vsc[j].token)) {
				is_vsc_exist = 1;
				break;
			}
		}
	}

	if (!is_profile_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (!is_vsc_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		if (rs_profiles[i].vsc_index) {
			rs_configs->vsc[rs_profiles[i].vsc_index - 1].use_count--;
		}
		rs_profiles[i].vsc_index = j + 1;
		rs_configs->vsc[j].use_count++;
	}

	/*save profile video source*/
	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioEncoderConfiguration(struct soap *soap,
	struct _trt__AddAudioEncoderConfiguration *trt__AddAudioEncoderConfiguration,
	struct _trt__AddAudioEncoderConfigurationResponse *trt__AddAudioEncoderConfigurationResponse)
{

	int is_profile_exist = 0;
	int is_aec_exist = 0;
	int i = 0;
	int j = 0;
	int ret;
	struct _trt__AddAudioEncoderConfiguration *p_request = trt__AddAudioEncoderConfiguration;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;
	int aec_count = 0;
	uint8 _Encoding;

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

	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap,
			sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken,
						rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	aec_count = 0;
	while (aec_count < MAX_MEDIA_AEC_CONFIG &&
			rs_profiles[aec_count].name[0] != '\0')
		aec_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < aec_count; j++) {
			if (!strcmp(p_request->ConfigurationToken,
						rs_configs->aec[j].token)) {
				is_aec_exist = 1;
				break;
			}
		}
	}

	if (!is_profile_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (!is_aec_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		if (rs_profiles[i].aec_index) {
			rs_configs->aec[rs_profiles[i].aec_index - 1].use_count--;
		}
		rs_profiles[i].aec_index = j + 1;
		rs_configs->aec[j].use_count++;
	}

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioSourceConfiguration(struct soap *soap,
	struct _trt__AddAudioSourceConfiguration *trt__AddAudioSourceConfiguration,
	struct _trt__AddAudioSourceConfigurationResponse *trt__AddAudioSourceConfigurationResponse)
{
	int is_profile_exist = 0;
	int is_asc_exist = 0;
	struct _trt__AddAudioSourceConfiguration *p_request = trt__AddAudioSourceConfiguration;
	int i = 0;
	int j = 0;
	int ret = 0;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;
	int asc_count = 0;

	struct tt__Profile *tt_profiles;

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

	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap,
			sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	asc_count = 0;
	while (asc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[asc_count].name[0] != '\0')
		asc_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < asc_count; j++) {
			if (!strcmp(p_request->ConfigurationToken,
						rs_configs->asc[j].token)) {
				is_asc_exist = 1;
				break;
			}
		}
	}

	if (!is_profile_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (!is_asc_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		if (rs_profiles[i].asc_index) {
			rs_configs->asc[rs_profiles[i].asc_index - 1].use_count--;
		}
		rs_profiles[i].asc_index = j + 1;
		rs_configs->asc[j].use_count++;
	}

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__AddPTZConfiguration(struct soap *soap,
	struct _trt__AddPTZConfiguration *trt__AddPTZConfiguration,
	struct _trt__AddPTZConfigurationResponse *trt__AddPTZConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoAnalyticsConfiguration(struct soap *soap,
	struct _trt__AddVideoAnalyticsConfiguration *trt__AddVideoAnalyticsConfiguration,
	struct _trt__AddVideoAnalyticsConfigurationResponse *trt__AddVideoAnalyticsConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__AddMetadataConfiguration(struct soap *soap,
	struct _trt__AddMetadataConfiguration *trt__AddMetadataConfiguration,
	struct _trt__AddMetadataConfigurationResponse *trt__AddMetadataConfigurationResponse)
{

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioOutputConfiguration(struct soap *soap,
	struct _trt__AddAudioOutputConfiguration *trt__AddAudioOutputConfiguration,
	struct _trt__AddAudioOutputConfigurationResponse *trt__AddAudioOutputConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioDecoderConfiguration(struct soap *soap,
	struct _trt__AddAudioDecoderConfiguration *trt__AddAudioDecoderConfiguration,
	struct _trt__AddAudioDecoderConfigurationResponse *trt__AddAudioDecoderConfigurationResponse)
{
	return SOAP_OK;
}

/*removes a VideoEncoderConfiguration from an existing media profile. If the media profile does not contain a VideoEncoderConfiguration, the operation takes no action.*/

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoEncoderConfiguration(struct soap *soap,
	struct _trt__RemoveVideoEncoderConfiguration *trt__RemoveVideoEncoderConfiguration,
	struct _trt__RemoveVideoEncoderConfigurationResponse *trt__RemoveVideoEncoderConfigurationResponse)
{
	int is_token_exist = 0;
	int i = 0;

	int ret;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;


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

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (trt__RemoveVideoEncoderConfiguration->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(trt__RemoveVideoEncoderConfiguration->ProfileToken,
						rs_profiles[i].token)) {
				is_token_exist = 1;
				break;
			}
		}
	}

	if (!is_token_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if ((1 == is_token_exist)) {
		if (rs_profiles[i].vec_index) {
			rs_configs->vec[rs_profiles[i].vec_index - 1].use_count--;
		}
		rs_profiles[i].vec_index = 0;
	}

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	return SOAP_OK;
}

/* removes a VideoSourceConfiguration from an existing media profile.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoSourceConfiguration(struct soap *soap,
	struct _trt__RemoveVideoSourceConfiguration *trt__RemoveVideoSourceConfiguration,
	struct _trt__RemoveVideoSourceConfigurationResponse *trt__RemoveVideoSourceConfigurationResponse)
{
	struct _trt__RemoveVideoSourceConfiguration *p_request = trt__RemoveVideoSourceConfiguration;
	int is_token_exist = 0;
	int i = 0;

	int ret;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;


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

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_token_exist = 1;
				break;
			}
		}
	}

	if (!is_token_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if ((1 == is_token_exist)) {
		if (rs_profiles[i].vsc_index) {
			rs_configs->vsc[rs_profiles[i].vsc_index - 1].use_count--;
		}
		rs_profiles[i].vsc_index = 0;
	}

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioEncoderConfiguration(struct soap *soap,
	struct _trt__RemoveAudioEncoderConfiguration *trt__RemoveAudioEncoderConfiguration,
	struct _trt__RemoveAudioEncoderConfigurationResponse *trt__RemoveAudioEncoderConfigurationResponse)
{
	struct _trt__RemoveAudioEncoderConfiguration *p_request = trt__RemoveAudioEncoderConfiguration;
	int is_token_exist = 0;
	int i = 0;

	int ret;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;


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

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_token_exist = 1;
				break;
			}
		}
	}

	if (!is_token_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else {
		if (rs_profiles[i].aec_index) {
			rs_configs->aec[rs_profiles[i].aec_index - 1].use_count--;
		}
		rs_profiles[i].aec_index = 0;
	}

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioSourceConfiguration(struct soap *soap,
	struct _trt__RemoveAudioSourceConfiguration *trt__RemoveAudioSourceConfiguration,
	struct _trt__RemoveAudioSourceConfigurationResponse *trt__RemoveAudioSourceConfigurationResponse)
{
	struct _trt__RemoveAudioSourceConfiguration *p_request = trt__RemoveAudioSourceConfiguration;
	int is_token_exist = 0;
	int i = 0;

	int ret;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count;


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

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_token_exist = 1;
				break;
			}
		}
	}

	if (!is_token_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if ((1 == is_token_exist)) {
		if (rs_profiles[i].asc_index) {
			rs_configs->asc[rs_profiles[i].asc_index - 1].use_count--;
		}
		rs_profiles[i].asc_index = 0;
	}

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__RemovePTZConfiguration(struct soap *soap,
	struct _trt__RemovePTZConfiguration *trt__RemovePTZConfiguration,
	struct _trt__RemovePTZConfigurationResponse *trt__RemovePTZConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoAnalyticsConfiguration(struct soap *soap,
	struct _trt__RemoveVideoAnalyticsConfiguration *trt__RemoveVideoAnalyticsConfiguration,
	struct _trt__RemoveVideoAnalyticsConfigurationResponse *trt__RemoveVideoAnalyticsConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveMetadataConfiguration(struct soap *soap,
	struct _trt__RemoveMetadataConfiguration *trt__RemoveMetadataConfiguration,
	struct _trt__RemoveMetadataConfigurationResponse *trt__RemoveMetadataConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioOutputConfiguration(struct soap *soap,
	struct _trt__RemoveAudioOutputConfiguration *trt__RemoveAudioOutputConfiguration,
	struct _trt__RemoveAudioOutputConfigurationResponse *trt__RemoveAudioOutputConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioDecoderConfiguration(struct soap *soap,
	struct _trt__RemoveAudioDecoderConfiguration *trt__RemoveAudioDecoderConfiguration,
	struct _trt__RemoveAudioDecoderConfigurationResponse *trt__RemoveAudioDecoderConfigurationResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__DeleteProfile(struct soap *soap,
	struct _trt__DeleteProfile *trt__DeleteProfile,
	struct _trt__DeleteProfileResponse *trt__DeleteProfileResponse)
{
	int is_token_exist = 0;
	int i = 0, ret = 0, index = 0;

	media_profile_t *rs_profiles = NULL;
	media_config_t *rs_configs = NULL;
	int profile_count = 0;

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
	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	/* Check if ProfileToken Exist or Not*/
	for (i = 0; i < profile_count; i++) {
		if (!strcmp(trt__DeleteProfile->ProfileToken, rs_profiles[i].token)) {
			is_token_exist = 1;
			break;
		}
	}


	if (!is_token_exist) {
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	if (rs_profiles[i].fixed) {
		onvif_fault(soap, "ter:InvalidArgVal", "ter:DeletionOfFixedProfile");
		return SOAP_FAULT;
	}

	if (rs_profiles[i].vsc_index) {
		rs_configs->vsc[rs_profiles[i].vsc_index - 1].use_count--;
	}
	if (rs_profiles[i].vec_index) {
		rs_configs->vec[rs_profiles[i].vec_index - 1].use_count--;
	}
	if (rs_profiles[i].asc_index) {
		rs_configs->asc[rs_profiles[i].asc_index - 1].use_count--;
	}
	if (rs_profiles[i].aec_index) {
		rs_configs->aec[rs_profiles[i].aec_index - 1].use_count--;
	}
	if (rs_profiles[i].adc_index) {
		rs_configs->adc[rs_profiles[i].adc_index - 1].use_count--;
	}
	if (rs_profiles[i].aoc_index) {
		rs_configs->aoc[rs_profiles[i].aoc_index - 1].use_count--;
	}
	if (rs_profiles[i].ptzc_index) {
		rs_configs->ptzc[rs_profiles[i].ptzc_index - 1].use_count--;
	}
	if (rs_profiles[i].vac_index) {
		rs_configs->vac[rs_profiles[i].vac_index - 1].use_count--;
	}
	if (rs_profiles[i].mc_index) {
		rs_configs->mc[rs_profiles[i].mc_index - 1].use_count--;
	}

	for (index = i; index < MAX_MEDIA_PROFILE - 1; index++) {
		memcpy(&rs_profiles[index], &rs_profiles[index + 1], sizeof(media_profile_t));
	}
	memset(&rs_profiles[MAX_MEDIA_PROFILE - 1], 0, sizeof(media_profile_t));

	ret = rsOnvifSysInfoSetMediaProfiles(rs_profiles);
	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	ASSERT(!ret);

	return SOAP_OK;
}

	/*lists the video source configurations of a VideoSource.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfigurations(struct soap *soap,
	struct _trt__GetVideoSourceConfigurations *trt__GetVideoSourceConfigurations,
	struct _trt__GetVideoSourceConfigurationsResponse *trt__GetVideoSourceConfigurationsResponse)
{
	struct _trt__GetVideoSourceConfigurationsResponse *p_response =
		trt__GetVideoSourceConfigurationsResponse;
	int i = 0, j = 0, k = 0, ret = 0;
	int vsc_count = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (vsc_count < MAX_MEDIA_VSC_CONFIG &&
			rs_configs->vsc[vsc_count].name[0] != '\0')
		vsc_count++;

	p_response->Configurations =
		(struct tt__VideoSourceConfiguration *)soap_malloc(soap,
				vsc_count * sizeof(struct tt__VideoSourceConfiguration));

	for (i = 0; i < vsc_count; i++) {
		p_response->Configurations[i].Name = rs_configs->vsc[i].name;
		p_response->Configurations[i].UseCount = rs_configs->vsc[i].use_count;
		p_response->Configurations[i].token = rs_configs->vsc[i].token;
		p_response->Configurations[i].SourceToken = rs_configs->vsc[i].source_token;
		p_response->Configurations[i].Bounds =
			(struct tt__IntRectangle *)soap_malloc(soap, sizeof(struct tt__IntRectangle));
		p_response->Configurations[i].Bounds->x = rs_configs->vsc[i].bounds_x;
		p_response->Configurations[i].Bounds->y = rs_configs->vsc[i].bounds_y;
		p_response->Configurations[i].Bounds->width = rs_configs->vsc[i].bounds_width;
		p_response->Configurations[i].Bounds->height = rs_configs->vsc[i].bounds_height;

		p_response->Configurations[i].__size = 0;
		p_response->Configurations[i].__any = NULL;
		p_response->Configurations[i].Extension = NULL;
		p_response->Configurations[i].__anyAttribute = NULL;
	}
	p_response->__sizeConfigurations = vsc_count;

	return SOAP_OK;
}

/*lists all existing video encoder configurations of an NVT.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfigurations(struct soap *soap,
	struct _trt__GetVideoEncoderConfigurations *trt__GetVideoEncoderConfigurations,
	struct _trt__GetVideoEncoderConfigurationsResponse *trt__GetVideoEncoderConfigurationsResponse)
{
	struct _trt__GetVideoEncoderConfigurationsResponse *p_response =
		trt__GetVideoEncoderConfigurationsResponse;
	int i = 0, j = 0, k = 0, ret = 0;
	media_config_t *rs_configs = NULL;
	int vec_count = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap,
			sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (vec_count < MAX_MEDIA_VEC_CONFIG &&
		rs_configs->vec[vec_count].name[0] != '\0')
		vec_count++;

	p_response->Configurations =
		(struct tt__VideoEncoderConfiguration *)soap_malloc(soap,
			(vec_count * sizeof(struct tt__VideoEncoderConfiguration)));

	for (i = 0; i < vec_count; i++) {

		p_response->Configurations[i].Name = rs_configs->vec[i].name;
		p_response->Configurations[i].UseCount =
			rs_configs->vec[i].use_count;
		p_response->Configurations[i].token = rs_configs->vec[i].token;
		p_response->Configurations[i].Encoding =
			rs_configs->vec[i].encoding_format;
		p_response->Configurations[i].Quality =
			rs_configs->vec[i].quality;
		p_response->Configurations[i].SessionTimeout =
			rs_configs->vec[i].session_timeout;

		p_response->Configurations[i].Resolution =
			(struct tt__VideoResolution *)soap_malloc(soap,
					sizeof(struct tt__VideoResolution));
		p_response->Configurations[i].Resolution->Width =
			rs_configs->vec[i].encoding_width;
		p_response->Configurations[i].Resolution->Height =
			rs_configs->vec[i].encoding_height;

		p_response->Configurations[i].RateControl =
			(struct tt__VideoRateControl *)soap_malloc(soap,
					sizeof(struct tt__VideoRateControl));
		p_response->Configurations[i].RateControl->FrameRateLimit =
			rs_configs->vec[i].frame_rate_limit;
		p_response->Configurations[i].RateControl->EncodingInterval =
			rs_configs->vec[i].encoding_interval;
		p_response->Configurations[i].RateControl->BitrateLimit =
			rs_configs->vec[i].bitrate_limit;

		p_response->Configurations[i].MPEG4 = NULL;

		p_response->Configurations[i].H264 =
			(struct tt__H264Configuration *)soap_malloc(soap,
				sizeof(struct tt__H264Configuration));
		p_response->Configurations[i].H264->GovLength =
			rs_configs->vec[i].h264_gov_length;
		p_response->Configurations[i].H264->H264Profile =
			rs_configs->vec[i].h264_profile;

		p_response->Configurations[i].Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
				sizeof(struct tt__MulticastConfiguration));
		p_response->Configurations[i].Multicast->Port =
			rs_configs->vec[i].multicast_port;
		p_response->Configurations[i].Multicast->TTL =
			rs_configs->vec[i].multicast_ttl;
		p_response->Configurations[i].Multicast->AutoStart =
			rs_configs->vec[i].multicast_auto_start;
		p_response->Configurations[i].Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap,
				sizeof(struct tt__IPAddress));
		p_response->Configurations[i].Multicast->Address->Type =
			rs_configs->vec[i].multicast_ip_type;
		p_response->Configurations[i].Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap,
				sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configurations[i].Multicast->Address->IPv4Address,
			rs_configs->vec[i].multicast_ip_addr);

		p_response->Configurations[i].Multicast->Address->IPv6Address = NULL;
	}

	p_response->__sizeConfigurations = vec_count; /* JPEG | H264*/
	RS_DBG("video encoder num is %d\n", vec_count);
	return SOAP_OK;
}

/*lists all existing audio source configurations of an NVT.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfigurations(struct soap *soap,
	struct _trt__GetAudioSourceConfigurations *trt__GetAudioSourceConfigurations,
	struct _trt__GetAudioSourceConfigurationsResponse *trt__GetAudioSourceConfigurationsResponse)
{
	struct _trt__GetAudioSourceConfigurationsResponse *p_response =
		trt__GetAudioSourceConfigurationsResponse;

	int i = 0, j = 0, ret = 0;
	int num_tokens = 0;
	int flag = 0;
	int num = 0;

	media_config_t *rs_configs = NULL;
	int asc_count = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (asc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[asc_count].name[0] != 0)
		asc_count++;

	p_response->Configurations =
		(struct tt__AudioSourceConfiguration *)soap_malloc(soap,
			asc_count * sizeof(struct tt__AudioSourceConfiguration));

	for (i = 0; i < asc_count; i++) {
		p_response->Configurations[i].Name = rs_configs->asc[i].name;
		p_response->Configurations[i].UseCount =
			rs_configs->asc[i].use_count;
		p_response->Configurations[i].token = rs_configs->asc[i].token;
		p_response->Configurations[i].SourceToken =
			rs_configs->asc[i].source_token;
	}
	p_response->__sizeConfigurations = asc_count;

	return SOAP_OK;
}

/*lists all existing device audio encoder configurations.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfigurations(struct soap *soap,
	struct _trt__GetAudioEncoderConfigurations *trt__GetAudioEncoderConfigurations,
	struct _trt__GetAudioEncoderConfigurationsResponse *trt__GetAudioEncoderConfigurationsResponse)
{
	struct _trt__GetAudioEncoderConfigurationsResponse *p_response =
		trt__GetAudioEncoderConfigurationsResponse;
	int i = 0, j = 0, k = 0, ret = 0;

	media_config_t *rs_configs = NULL;
	int aec_count = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	aec_count = 0;
	while (aec_count < MAX_MEDIA_AEC_CONFIG &&
			rs_configs->aec[aec_count].name[0] != '\0')
		aec_count++;

	p_response->__sizeConfigurations = aec_count;
	p_response->Configurations =
		(struct tt__AudioEncoderConfiguration *)soap_malloc(soap,
			aec_count * sizeof(struct tt__AudioEncoderConfiguration));

	for (i = 0; i < aec_count; i++) {
		p_response->Configurations[i].Name = rs_configs->aec[i].name;
		p_response->Configurations[i].UseCount = rs_configs->aec[i].use_count;
		p_response->Configurations[i].token = rs_configs->aec[i].token;/*"G711";*/
		p_response->Configurations[i].Encoding = rs_configs->aec[i].encoding_format;/*{tt__AudioEncoding__G711 = 0, tt__AudioEncoding__G726 = 1, tt__AudioEncoding__AAC = 2}*/
		p_response->Configurations[i].Bitrate = rs_configs->aec[i].bit_rate;
		p_response->Configurations[i].SampleRate = rs_configs->aec[i].sample_rate;
		p_response->Configurations[i].SessionTimeout = rs_configs->aec[i].session_timeout;
		p_response->Configurations[i].Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
				sizeof(struct tt__MulticastConfiguration));
		p_response->Configurations[i].Multicast->Port = rs_configs->aec[i].port;
		p_response->Configurations[i].Multicast->TTL = rs_configs->aec[i].ttl;
		p_response->Configurations[i].Multicast->AutoStart = rs_configs->aec[i].auto_start;
		p_response->Configurations[i].Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap,
				sizeof(struct tt__IPAddress));
		p_response->Configurations[i].Multicast->Address->Type = rs_configs->aec[i].multicast_ip_type;
		p_response->Configurations[i].Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap, sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configurations[i].Multicast->Address->IPv4Address, rs_configs->aec[i].multicast_ip_addr);
		p_response->Configurations[i].Multicast->Address->IPv6Address = NULL;
		p_response->Configurations[i].Multicast->__size = NULL;
		p_response->Configurations[i].Multicast->__any = NULL;
		p_response->Configurations[i].Multicast->__anyAttribute = NULL;

		p_response->Configurations[i].__size = 0;
		p_response->Configurations[i].__any = NULL;
		p_response->Configurations[i].__anyAttribute = NULL;
	}

	return SOAP_OK;
}

/*lists all video analytics configurations of a device.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoAnalyticsConfigurations(struct soap *soap,
	struct _trt__GetVideoAnalyticsConfigurations *trt__GetVideoAnalyticsConfigurations,
	struct _trt__GetVideoAnalyticsConfigurationsResponse *trt__GetVideoAnalyticsConfigurationsResponse)
{

	onvif_fault(soap, SENDER, "ter:ActionNotSupported", "ter:VideoAnalyticsNotSupported");
	return SOAP_FAULT;
}

/*lists all existing metadata configurations.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfigurations(struct soap *soap,
	struct _trt__GetMetadataConfigurations *trt__GetMetadataConfigurations,
	struct _trt__GetMetadataConfigurationsResponse *trt__GetMetadataConfigurationsResponse)
{
	struct _trt__GetMetadataConfigurationsResponse *p_response = trt__GetMetadataConfigurationsResponse;
	int token_exist = 0;
	int num = 0;
	int i = 0, j = 0;
	int index = 0;
	int num_tokens = 0;

	int ret = 0;
	media_profile_t *rs_profiles = NULL;
	int mc_count = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (mc_count < MAX_MEDIA_MC_CONFIG &&
			rs_configs->mc[mc_count].name[0] != '\0')
		mc_count++;

	p_response->__sizeConfigurations = mc_count;
	p_response->Configurations =
		(struct tt__MetadataConfiguration *)soap_malloc(soap,
			mc_count * sizeof(struct tt__MetadataConfiguration));

	for (i = 0; i < mc_count; i++) {
		p_response->Configurations[i].Name = rs_configs->mc[i].name;
		p_response->Configurations[i].UseCount = rs_configs->mc[i].use_count;
		p_response->Configurations[i].token = rs_configs->mc[i].token;
		p_response->Configurations[i].SessionTimeout = rs_configs->mc[i].session_timeout;
		p_response->Configurations[i].PTZStatus = NULL;
		p_response->Configurations[i].Events = NULL;
		p_response->Configurations[i].Analytics = NULL;

		p_response->Configurations[i].Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
				sizeof(struct tt__MulticastConfiguration));
		p_response->Configurations[i].Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap, sizeof(struct tt__IPAddress));
		p_response->Configurations[i].Multicast->Address->Type = rs_configs->mc[i].multicast_ip_type;
		p_response->Configurations[i].Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap, sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configurations[i].Multicast->Address->IPv4Address,
			rs_configs->mc[i].multicast_ip_addr);
		p_response->Configurations[i].Multicast->Address->IPv6Address = NULL;
		p_response->Configurations[i].Multicast->Port = rs_configs->mc[i].multicast_port;
		p_response->Configurations[i].Multicast->TTL = rs_configs->mc[i].multicast_ttl;
		p_response->Configurations[i].Multicast->AutoStart = rs_configs->mc[i].multicast_auto_start;

		p_response->Configurations[i].Multicast->__size = 0;
		p_response->Configurations[i].Multicast->__any = NULL;
		p_response->Configurations[i].Multicast->__anyAttribute = NULL;

		p_response->Configurations[i].__size = 0;
		p_response->Configurations[i].__any = NULL;
		p_response->Configurations[i].AnalyticsEngineConfiguration = NULL;
		p_response->Configurations[i].Extension = NULL;
		p_response->Configurations[i].__anyAttribute = NULL;
	}

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfigurations(struct soap *soap,
	struct _trt__GetAudioOutputConfigurations *trt__GetAudioOutputConfigurations,
	struct _trt__GetAudioOutputConfigurationsResponse *trt__GetAudioOutputConfigurationsResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:AudioIn/OutNotSupported");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfigurations(struct soap *soap,
	struct _trt__GetAudioDecoderConfigurations *trt__GetAudioDecoderConfigurations,
	struct _trt__GetAudioDecoderConfigurationsResponse *trt__GetAudioDecoderConfigurationsResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:AudioIn/OutNotSupported");
	return SOAP_FAULT;
}

/*lists the video source configurations of a VideoSource.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfiguration(struct soap *soap,
	struct _trt__GetVideoSourceConfiguration *trt__GetVideoSourceConfiguration,
	struct _trt__GetVideoSourceConfigurationResponse *trt__GetVideoSourceConfigurationResponse)
{
	struct _trt__GetVideoSourceConfiguration *p_request = trt__GetVideoSourceConfiguration;
	int i = 0, j = 0;
	int flag = 0;
	int num_tokens = 0;
	int is_token_exist = 0;

	int ret = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	int vsc_count = 0;
	while (vsc_count < MAX_MEDIA_VSC_CONFIG &&
			rs_configs->vsc[vsc_count].name[0] != '\0')
		vsc_count++;

	if (p_request->ConfigurationToken) {
		for (i = 0; i < vsc_count; i++) {
			if (!strcmp(p_request->ConfigurationToken, rs_configs->vsc[i].token)) {
				is_token_exist = 1;
				break;
			}
		}
	}


	/*if ConfigurationToken does not exist*/
	if (!is_token_exist) {
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		trt__GetVideoSourceConfigurationResponse->Configuration =
			(struct tt__VideoSourceConfiguration *)soap_malloc(soap,
				sizeof(struct tt__VideoSourceConfiguration));
		trt__GetVideoSourceConfigurationResponse->Configuration->Name = rs_configs->vsc[i].name;
		trt__GetVideoSourceConfigurationResponse->Configuration->UseCount = rs_configs->vsc[i].use_count;
		trt__GetVideoSourceConfigurationResponse->Configuration->token = rs_configs->vsc[i].token;
		trt__GetVideoSourceConfigurationResponse->Configuration->SourceToken = rs_configs->vsc[i].source_token;
		trt__GetVideoSourceConfigurationResponse->Configuration->Bounds =
			(struct tt__IntRectangle *)soap_malloc(soap, sizeof(struct tt__IntRectangle));
		trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->x = rs_configs->vsc[i].bounds_x;
		trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->y = rs_configs->vsc[i].bounds_y;
		trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->width = rs_configs->vsc[i].bounds_width;
		trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->height = rs_configs->vsc[i].bounds_height;
	}

	return SOAP_OK;
}

/*lists all existing video encoder configurations of anNVT.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfiguration(struct soap *soap,
	struct _trt__GetVideoEncoderConfiguration *trt__GetVideoEncoderConfiguration,
	struct _trt__GetVideoEncoderConfigurationResponse *trt__GetVideoEncoderConfigurationResponse)
{
	struct _trt__GetVideoEncoderConfigurationResponse *p_response = trt__GetVideoEncoderConfigurationResponse;
	int i;
	int num_tokens = 0;
	int is_token_exist = 0;

	int ret = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	int vec_count = 0;
	while (vec_count < MAX_MEDIA_VEC_CONFIG &&
			rs_configs->vec[vec_count].name[0] != '\0')
		vec_count++;

	if (trt__GetVideoEncoderConfiguration->ConfigurationToken) {
		for (i = 0; i < vec_count; i++) {
			if (!strcmp(trt__GetVideoEncoderConfiguration->ConfigurationToken, rs_configs->vec[i].token)) {
				is_token_exist = 1;
				break;
			}
		}
	}

	/* if ConfigurationToken does not exist*/
	if (!is_token_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		p_response->Configuration =
			(struct tt__VideoEncoderConfiguration *)soap_malloc(soap,
				sizeof(struct tt__VideoEncoderConfiguration));
		p_response->Configuration->Name = rs_configs->vec[i].name;
		p_response->Configuration->UseCount = rs_configs->vec[i].use_count;
		p_response->Configuration->token = rs_configs->vec[i].token;
		p_response->Configuration->Encoding = rs_configs->vec[i].encoding_format;
		p_response->Configuration->Resolution =
			(struct tt__VideoResolution *)soap_malloc(soap,
				sizeof(struct tt__VideoResolution));
		p_response->Configuration->Resolution->Width = rs_configs->vec[i].encoding_width;
		p_response->Configuration->Resolution->Height = rs_configs->vec[i].encoding_height;
		p_response->Configuration->Quality = rs_configs->vec[i].quality;
		p_response->Configuration->RateControl =
			(struct tt__VideoRateControl *)soap_malloc(soap,
				sizeof(struct tt__VideoRateControl));
		p_response->Configuration->RateControl->FrameRateLimit = rs_configs->vec[i].frame_rate_limit;
		p_response->Configuration->RateControl->EncodingInterval = rs_configs->vec[i].encoding_interval;
		p_response->Configuration->RateControl->BitrateLimit = rs_configs->vec[i].bitrate_limit;
		p_response->Configuration->MPEG4 = NULL;
		p_response->Configuration->H264 =
			(struct tt__H264Configuration *)soap_malloc(soap,
				sizeof(struct tt__H264Configuration));
		p_response->Configuration->H264->GovLength = rs_configs->vec[i].h264_gov_length;
		p_response->Configuration->H264->H264Profile = rs_configs->vec[i].h264_profile;/*Baseline = 0, Main = 1, High = 3*/
		p_response->Configuration->Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
				sizeof(struct tt__MulticastConfiguration));
		p_response->Configuration->Multicast->Port = rs_configs->vec[i].multicast_port;
		p_response->Configuration->Multicast->TTL = rs_configs->vec[i].multicast_ttl;
		p_response->Configuration->Multicast->AutoStart = rs_configs->vec[i].multicast_auto_start;
		p_response->Configuration->Multicast->Address  =
			(struct tt__IPAddress *)soap_malloc(soap, sizeof(struct tt__IPAddress));
		p_response->Configuration->Multicast->Address->Type = rs_configs->vec[i].multicast_ip_type;
		p_response->Configuration->Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap,
				sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configuration->Multicast->Address->IPv4Address,
			rs_configs->vec[i].multicast_ip_addr);
		p_response->Configuration->Multicast->Address->IPv6Address = NULL;
		p_response->Configuration->SessionTimeout = rs_configs->vec[i].session_timeout;
	}

	return SOAP_OK;
}

/*An AudioSourceConfiguration contains a reference to an AudioSource that is to be used for input in a media profile. This operation lists all existing audio source configurations of an NVT*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfiguration(struct soap *soap,
	struct _trt__GetAudioSourceConfiguration *trt__GetAudioSourceConfiguration,
	struct _trt__GetAudioSourceConfigurationResponse *trt__GetAudioSourceConfigurationResponse)
{
	struct _trt__GetAudioSourceConfiguration *p_request = trt__GetAudioSourceConfiguration;
	struct _trt__GetAudioSourceConfigurationResponse *p_response = trt__GetAudioSourceConfigurationResponse;
	int i;
	int flag = 0;
	int use = 0;
	int k = 0;
	int is_token_exist = 0;

	int ret = 0;
	media_config_t *rs_configs = NULL;
	int asc_count = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (asc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[asc_count].name[0] != '\0')
		asc_count++;

	if (p_request->ConfigurationToken) {
		for (i = 0; i < asc_count; i++) {
			if (!strcmp(p_request->ConfigurationToken,
					rs_configs->asc[i].token)) {
				is_token_exist = 1;
				break;
			}
		}
	}


	/*if ConfigurationToken does not exist*/
	if (!is_token_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		p_response->Configuration =
			(struct tt__AudioSourceConfiguration *)soap_malloc(soap,
				sizeof(struct tt__AudioSourceConfiguration));
		p_response->Configuration->Name = rs_configs->asc[i].name;
		p_response->Configuration->UseCount = rs_configs->asc[i].use_count;
		p_response->Configuration->token = rs_configs->asc[i].token;
		p_response->Configuration->SourceToken = rs_configs->asc[i].source_token;
	}

	return SOAP_OK;
}

/*This operation lists all existing device audio encoder configurations.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfiguration(struct soap *soap,
	struct _trt__GetAudioEncoderConfiguration *trt__GetAudioEncoderConfiguration,
	struct _trt__GetAudioEncoderConfigurationResponse *trt__GetAudioEncoderConfigurationResponse)
{
		struct _trt__GetAudioEncoderConfiguration *p_request = trt__GetAudioEncoderConfiguration;
	struct _trt__GetAudioEncoderConfigurationResponse *p_response = trt__GetAudioEncoderConfigurationResponse;

	int j = 0;
	int is_aec_exist = 0;
	int k = 0;

	int ret = 0;
	media_profile_t *rs_profiles = NULL;
	int aec_count = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	aec_count = 0;
	while (aec_count < MAX_MEDIA_AEC_CONFIG &&
			rs_configs->aec[aec_count].name[0] != '\0')
		aec_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < aec_count; j++) {
			if (!strcmp(p_request->ConfigurationToken, rs_configs->aec[j].token)) {
				is_aec_exist = 1;
				break;
			}
		}
	}

	if (!is_aec_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		p_response->Configuration =
			(struct tt__AudioEncoderConfiguration *)soap_malloc(soap,
				sizeof(struct tt__AudioEncoderConfiguration));

		p_response->Configuration->Name =
			(char *) soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
		strcpy(p_response->Configuration->Name, rs_configs->aec[j].name);
		p_response->Configuration->UseCount = rs_configs->aec[j].use_count;
		p_response->Configuration->token =
			(char *) soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
		strcpy(p_response->Configuration->token, rs_configs->aec[j].token);

		p_response->Configuration->Encoding = rs_configs->aec[j].encoding_format;
		p_response->Configuration->Bitrate = rs_configs->aec[j].bit_rate;
		p_response->Configuration->SampleRate = rs_configs->aec[j].sample_rate;

		p_response->Configuration->Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap, sizeof(struct tt__MulticastConfiguration));
		p_response->Configuration->Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap, sizeof(struct tt__IPAddress));
		p_response->Configuration->Multicast->Address->Type = rs_configs->aec[j].multicast_ip_type;
		p_response->Configuration->Multicast->Address->IPv4Address = (char *)soap_malloc(soap, sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configuration->Multicast->Address->IPv4Address, rs_configs->aec[j].multicast_ip_addr);
		p_response->Configuration->Multicast->Address->IPv6Address = NULL;
		p_response->Configuration->Multicast->Port = rs_configs->aec[j].port;
		p_response->Configuration->Multicast->TTL = rs_configs->aec[j].ttl;
		p_response->Configuration->Multicast->AutoStart = rs_configs->aec[j].auto_start;
		p_response->Configuration->Multicast->__size = NULL;
		p_response->Configuration->Multicast->__any = NULL;
		p_response->Configuration->Multicast->__anyAttribute = NULL;
		p_response->Configuration->SessionTimeout = rs_configs->aec[j].session_timeout;

		p_response->Configuration->__size = 0;
		p_response->Configuration->__any = NULL;
		p_response->Configuration->__anyAttribute = NULL;
	}

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoAnalyticsConfiguration(struct soap *soap,
	struct _trt__GetVideoAnalyticsConfiguration *trt__GetVideoAnalyticsConfiguration,
	struct _trt__GetVideoAnalyticsConfigurationResponse *trt__GetVideoAnalyticsConfigurationResponse)
{
	onvif_fault(soap, SENDER, "ter:ActionNotSupported", "ter:VideoAnalyticsNotSupported");
	return SOAP_FAULT;
}

/*This operation fetches the metadata configurations of a device if the metadata token is known.*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfiguration(struct soap *soap,
	struct _trt__GetMetadataConfiguration *trt__GetMetadataConfiguration,
	struct _trt__GetMetadataConfigurationResponse *trt__GetMetadataConfigurationResponse)
{
	struct _trt__GetMetadataConfiguration *p_request = trt__GetMetadataConfiguration;
	struct _trt__GetMetadataConfigurationResponse *p_response = trt__GetMetadataConfigurationResponse;
	int token_exist = 0;
	int i = 0, j = 0;

	int ret = 0;
	media_config_t *rs_configs = NULL;
	int mc_count = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (mc_count < MAX_MEDIA_MC_CONFIG &&
		rs_configs->mc[mc_count].name[0] != '\0')
		mc_count++;

	if (p_request->ConfigurationToken) {
		for (i = 0; i < mc_count; i++) {
			if (!strcmp(p_request->ConfigurationToken,
						rs_configs->mc[i].token)) {
				token_exist = 1;
			}
		}
	}

	if (!token_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else {
		p_response->Configuration =
			(struct tt__MetadataConfiguration *)soap_malloc(soap,
					sizeof(struct tt__MetadataConfiguration));
		p_response->Configuration[0].Name = rs_configs->mc[i].name;
		p_response->Configuration[0].UseCount = rs_configs->mc[i].use_count;
		p_response->Configuration[0].token = rs_configs->mc[i].token;
		p_response->Configuration[0].SessionTimeout = rs_configs->mc[i].session_timeout;
		p_response->Configuration[0].PTZStatus = NULL;
		p_response->Configuration[0].Events = NULL;
		p_response->Configuration[0].Analytics = NULL;
		p_response->Configuration[0].Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap, sizeof(struct tt__MulticastConfiguration));
		p_response->Configuration[0].Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap, sizeof(struct tt__IPAddress));
		p_response->Configuration[0].Multicast->Address->Type = rs_configs->mc[i].multicast_ip_type;
		p_response->Configuration[0].Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap, sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configuration[0].Multicast->Address->IPv4Address,
			rs_configs->mc[i].multicast_ip_addr);
		p_response->Configuration[0].Multicast->Address->IPv6Address = NULL;
		p_response->Configuration[0].Multicast->Port = rs_configs->mc[i].multicast_port;
		p_response->Configuration[0].Multicast->TTL = rs_configs->mc[i].multicast_ttl;
		p_response->Configuration[0].Multicast->AutoStart = rs_configs->mc[i].multicast_auto_start;
	}

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfiguration(struct soap *soap,
	struct _trt__GetAudioOutputConfiguration *trt__GetAudioOutputConfiguration,
	struct _trt__GetAudioOutputConfigurationResponse *trt__GetAudioOutputConfigurationResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:AudioOutputNotSupported");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfiguration(struct soap *soap,
	struct _trt__GetAudioDecoderConfiguration *trt__GetAudioDecoderConfiguration,
	struct _trt__GetAudioDecoderConfigurationResponse *trt__GetAudioDecoderConfigurationResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:AudioIn/OutNotSupported");
	return SOAP_FAULT;
}


/* this operation lists all the video encoder configurations of the device that are compatible with a certain media profile*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoEncoderConfigurations(struct soap *soap,
	struct _trt__GetCompatibleVideoEncoderConfigurations *trt__GetCompatibleVideoEncoderConfigurations,
	struct _trt__GetCompatibleVideoEncoderConfigurationsResponse *trt__GetCompatibleVideoEncoderConfigurationsResponse)
{
	struct _trt__GetCompatibleVideoEncoderConfigurationsResponse *p_response =
		trt__GetCompatibleVideoEncoderConfigurationsResponse;
	int i = 0, j = 0, k = 0, ret = 0;
	media_config_t *rs_configs = NULL;
	int vec_count = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (vec_count < MAX_MEDIA_VEC_CONFIG &&
			rs_configs->vec[vec_count].name[0] != '\0')
		vec_count++;


	p_response->Configurations =
		(struct tt__VideoEncoderConfiguration *)soap_malloc(soap,
			(vec_count * sizeof(struct tt__VideoEncoderConfiguration)));

	for (i = 0; i < vec_count; i++) {
		p_response->Configurations[i].Name = rs_configs->vec[i].name;
		p_response->Configurations[i].UseCount = rs_configs->vec[i].use_count;
		p_response->Configurations[i].token = rs_configs->vec[i].token;
		p_response->Configurations[i].Encoding = rs_configs->vec[i].encoding_format;/* JPEG = 0,  H264 = 2*/
		p_response->Configurations[i].Quality = rs_configs->vec[i].quality;
		p_response->Configurations[i].SessionTimeout = rs_configs->vec[i].session_timeout;
		p_response->Configurations[i].Resolution =
			(struct tt__VideoResolution *)soap_malloc(soap, sizeof(struct tt__VideoResolution));
		p_response->Configurations[i].Resolution->Width = rs_configs->vec[i].encoding_width;
		p_response->Configurations[i].Resolution->Height = rs_configs->vec[i].encoding_height;
		p_response->Configurations[i].RateControl =
			(struct tt__VideoRateControl *)soap_malloc(soap, sizeof(struct tt__VideoRateControl));
		p_response->Configurations[i].RateControl->FrameRateLimit = rs_configs->vec[i].frame_rate_limit;
		p_response->Configurations[i].RateControl->EncodingInterval = rs_configs->vec[i].encoding_interval; /* dummy*/
		p_response->Configurations[i].RateControl->BitrateLimit = rs_configs->vec[i].bitrate_limit;
		p_response->Configurations[i].MPEG4 = NULL;
		p_response->Configurations[i].H264 =
			(struct tt__H264Configuration *)soap_malloc(soap,
				sizeof(struct tt__H264Configuration));
		p_response->Configurations[i].H264->GovLength = rs_configs->vec[i].h264_gov_length;
		p_response->Configurations[i].H264->H264Profile = rs_configs->vec[i].h264_profile;/*Baseline = 0, Main = 1, High = 3;*/
		p_response->Configurations[i].Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
				sizeof(struct tt__MulticastConfiguration));
		p_response->Configurations[i].Multicast->Port = rs_configs->vec[i].multicast_port;
		p_response->Configurations[i].Multicast->TTL = rs_configs->vec[i].multicast_ttl;
		p_response->Configurations[i].Multicast->AutoStart = rs_configs->vec[i].multicast_auto_start;
		p_response->Configurations[i].Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap,
				sizeof(struct tt__IPAddress));
		p_response->Configurations[i].Multicast->Address->Type = rs_configs->vec[i].multicast_ip_type;
		p_response->Configurations[i].Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap, sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configurations[i].Multicast->Address->IPv4Address,
			rs_configs->vec[i].multicast_ip_addr);
		p_response->Configurations[i].Multicast->Address->IPv6Address = NULL;
	}

	p_response->__sizeConfigurations = vec_count; /* JPEG | H264*/

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoSourceConfigurations(struct soap *soap,
	struct _trt__GetCompatibleVideoSourceConfigurations *trt__GetCompatibleVideoSourceConfigurations,
	struct _trt__GetCompatibleVideoSourceConfigurationsResponse *trt__GetCompatibleVideoSourceConfigurationsResponse)
{
	struct _trt__GetCompatibleVideoSourceConfigurations *p_request =
		trt__GetCompatibleVideoSourceConfigurations;
	struct _trt__GetCompatibleVideoSourceConfigurationsResponse *p_response =
		trt__GetCompatibleVideoSourceConfigurationsResponse;
	int i = 0, j = 0, k = 0, ret = 0;
	int vsc_count = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	while (vsc_count < MAX_MEDIA_VSC_CONFIG &&
			rs_configs->vsc[vsc_count].name[0] != '\0')
		vsc_count++;

	p_response->Configurations =
		(struct tt__VideoEncoderConfiguration *)soap_malloc(soap,
				vsc_count * sizeof(struct tt__VideoEncoderConfiguration));

	for (i = 0; i < vsc_count; i++) {
		p_response->Configurations[i].Name = rs_configs->vsc[i].name;
		p_response->Configurations[i].UseCount = rs_configs->vsc[i].use_count;
		p_response->Configurations[i].token = rs_configs->vsc[i].token;
		p_response->Configurations[i].SourceToken = rs_configs->vsc[i].source_token;
		p_response->Configurations[i].Bounds =
			(struct tt__IntRectangle *)soap_malloc(soap, sizeof(struct tt__IntRectangle));
		p_response->Configurations[i].Bounds->x = rs_configs->vsc[i].bounds_x;
		p_response->Configurations[i].Bounds->y = rs_configs->vsc[i].bounds_y;
		p_response->Configurations[i].Bounds->width = rs_configs->vsc[i].bounds_width;
		p_response->Configurations[i].Bounds->height = rs_configs->vsc[i].bounds_height;
	}
	p_response->__sizeConfigurations = vsc_count;

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioEncoderConfigurations(struct soap *soap,
	struct _trt__GetCompatibleAudioEncoderConfigurations *trt__GetCompatibleAudioEncoderConfigurations,
	struct _trt__GetCompatibleAudioEncoderConfigurationsResponse *trt__GetCompatibleAudioEncoderConfigurationsResponse)
{
	struct _trt__GetCompatibleAudioEncoderConfigurations *p_request =
		trt__GetCompatibleAudioEncoderConfigurations;
	struct _trt__GetCompatibleAudioEncoderConfigurationsResponse *p_response =
		trt__GetCompatibleAudioEncoderConfigurationsResponse;
	int i = 0, j = 0, k = 0, ret = 0;

	media_profile_t *rs_profiles = NULL;
	int profile_count = 0;
	int is_profile_exist = 0;
	media_config_t *rs_configs = NULL;
	int aec_count = 0;

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

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	if (!is_profile_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	while (aec_count < MAX_MEDIA_AEC_CONFIG &&
			rs_configs->aec[aec_count].name[0] != '\0')
		aec_count++;

	p_response->__sizeConfigurations = aec_count;
	p_response->Configurations =
		(struct tt__AudioEncoderConfiguration *)soap_malloc(soap,
			aec_count * sizeof(struct tt__AudioEncoderConfiguration));

	for (i = 0; i < aec_count; i++) {
		p_response->Configurations[i].Name = rs_configs->aec[i].name;
		p_response->Configurations[i].UseCount = rs_configs->aec[i].use_count;
		p_response->Configurations[i].token = rs_configs->aec[i].token;/*"G711";*/
		p_response->Configurations[i].Encoding = rs_configs->aec[i].encoding_format;
		p_response->Configurations[i].Bitrate = rs_configs->aec[i].bit_rate;
		p_response->Configurations[i].SampleRate = rs_configs->aec[i].sample_rate;
		p_response->Configurations[i].SessionTimeout = rs_configs->aec[i].session_timeout;
		p_response->Configurations[i].Multicast =
			(struct tt__MulticastConfiguration *)soap_malloc(soap,
				sizeof(struct tt__MulticastConfiguration));
		p_response->Configurations[i].Multicast->Port = rs_configs->aec[i].port;
		p_response->Configurations[i].Multicast->TTL = rs_configs->aec[i].ttl;
		p_response->Configurations[i].Multicast->AutoStart = rs_configs->aec[i].auto_start;
		p_response->Configurations[i].Multicast->Address =
			(struct tt__IPAddress *)soap_malloc(soap,
				sizeof(struct tt__IPAddress));
		p_response->Configurations[i].Multicast->Address->Type = rs_configs->aec[i].multicast_ip_type;
		p_response->Configurations[i].Multicast->Address->IPv4Address =
			(char *)soap_malloc(soap, sizeof(char) * MAX_IPADDR_STR_LENGTH);
		strcpy(p_response->Configurations[i].Multicast->Address->IPv4Address, rs_configs->aec[i].multicast_ip_addr);
		p_response->Configurations[i].Multicast->Address->IPv6Address = NULL;
		p_response->Configurations[i].Multicast->__size = NULL;
		p_response->Configurations[i].Multicast->__any = NULL;
		p_response->Configurations[i].Multicast->__anyAttribute = NULL;

		p_response->Configurations[i].__size = 0;
		p_response->Configurations[i].__any = NULL;
		p_response->Configurations[i].__anyAttribute = NULL;
	}

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioSourceConfigurations(struct soap *soap,
	struct _trt__GetCompatibleAudioSourceConfigurations *trt__GetCompatibleAudioSourceConfigurations,
	struct _trt__GetCompatibleAudioSourceConfigurationsResponse *trt__GetCompatibleAudioSourceConfigurationsResponse)
{
	struct _trt__GetCompatibleAudioSourceConfigurations *p_request =
		trt__GetCompatibleAudioSourceConfigurations;
	struct _trt__GetCompatibleAudioSourceConfigurationsResponse *p_response =
		trt__GetCompatibleAudioSourceConfigurationsResponse;

	int i = 0, j = 0, ret = 0;
	int num_tokens = 0;
	int flag = 0;
	int num = 0;

	media_profile_t *rs_profiles = NULL;
	int profile_count = 0;
	int is_profile_exist = 0;
	media_config_t *rs_configs = NULL;
	int asc_count = 0;

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

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
		rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	if (!is_profile_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	while (asc_count < MAX_MEDIA_ASC_CONFIG &&
		rs_configs->asc[asc_count].name[0] != '\0')
		asc_count++;

	p_response->Configurations =
		(struct tt__AudioSourceConfiguration *)soap_malloc(soap,
			asc_count * sizeof(struct tt__AudioSourceConfiguration));

	for (i = 0; i < asc_count; i++) {
		p_response->Configurations[i].Name = rs_configs->asc[i].name;
		p_response->Configurations[i].UseCount = rs_configs->asc[i].use_count;
		p_response->Configurations[i].token = rs_configs->asc[i].token;
		p_response->Configurations[i].SourceToken = rs_configs->asc[i].source_token;
	}
	p_response->__sizeConfigurations = asc_count;

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoAnalyticsConfigurations(struct soap *soap,
	struct _trt__GetCompatibleVideoAnalyticsConfigurations *trt__GetCompatibleVideoAnalyticsConfigurations,
	struct _trt__GetCompatibleVideoAnalyticsConfigurationsResponse *trt__GetCompatibleVideoAnalyticsConfigurationsResponse)
{

	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:VideoAnalytiscsNotSuppoted");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleMetadataConfigurations(struct soap *soap,
	struct _trt__GetCompatibleMetadataConfigurations *trt__GetCompatibleMetadataConfigurations,
	struct _trt__GetCompatibleMetadataConfigurationsResponse *trt__GetCompatibleMetadataConfigurationsResponse)
{

	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:GetCompatibleMetadataConfigurationsNotSuppoted");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioOutputConfigurations(struct soap *soap,
	struct _trt__GetCompatibleAudioOutputConfigurations *trt__GetCompatibleAudioOutputConfigurations,
	struct _trt__GetCompatibleAudioOutputConfigurationsResponse *trt__GetCompatibleAudioOutputConfigurationsResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:GetCompatibleAudioOutputConfigurationsNotSuppoted");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioDecoderConfigurations(struct soap *soap,
	struct _trt__GetCompatibleAudioDecoderConfigurations *trt__GetCompatibleAudioDecoderConfigurations,
	struct _trt__GetCompatibleAudioDecoderConfigurationsResponse *trt__GetCompatibleAudioDecoderConfigurationsResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:GetCompatibleAudioDecoderConfigurationsNotSuppoted");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoSourceConfiguration(struct soap *soap,
	struct _trt__SetVideoSourceConfiguration *trt__SetVideoSourceConfiguration,
	struct _trt__SetVideoSourceConfigurationResponse *trt__SetVideoSourceConfigurationResponse)
{
	struct _trt__SetVideoSourceConfiguration *p_request = trt__SetVideoSourceConfiguration;
	int i = 0;
	int j = 0;
	int is_vsc_exist = 0;
	int ret;
	uint8 codec_combo = 0;
	uint8 codec_res = 0;
	uint8 mode = 0;
	uint8 encodinginterval = 0;
	uint8 frameratelimit = 0;
	int _width = 0;
	int _height = 0;
	int bitrate = 0;
	int govlength = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);


	if (p_request->Configuration->Bounds != NULL) {
		_width = p_request->Configuration->Bounds->width;
		_height = p_request->Configuration->Bounds->height;
	}


	int vsc_count = 0;
	while (vsc_count < MAX_MEDIA_VSC_CONFIG &&
		rs_configs->vsc[vsc_count].name[0] != '\0')
		vsc_count++;

	for (j = 0; j < vsc_count; j++) {
		if (!strcmp(trt__SetVideoSourceConfiguration->Configuration->token, rs_configs->vsc[j].token)) {
			is_vsc_exist = 1;
			break;
		}
	}

	if (!is_vsc_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	/* check for width and height*/
	if (p_request->Configuration->Bounds) {
		if (_width != SENSOR_FMT_WIDTH || _height != SENSOR_FMT_HEIGHT) {
			onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
			return SOAP_FAULT;
		}
	}

	/*
	 * to pass conformance test MEDIA-2-3-12.
	 * the spec says the ForcePersistence is obsolete and should always assumed to be ture.
	 */
	/* if (xsd__boolean__true_ == p_request->ForcePersistence) { */
		rs_configs->vsc[j].force_persistence = xsd__boolean__true_;

		if (p_request->Configuration->Bounds) {
			rs_configs->vsc[j].bounds_x = p_request->Configuration->Bounds->x;
			rs_configs->vsc[j].bounds_y = p_request->Configuration->Bounds->y;
			rs_configs->vsc[j].bounds_width  = p_request->Configuration->Bounds->width;
			rs_configs->vsc[j].bounds_height = p_request->Configuration->Bounds->height;
		}

		/*need rewrite*/
		if (p_request->Configuration->Name) {
			strcpy(rs_configs->vsc[j].name, p_request->Configuration->Name);
		}
		if (p_request->Configuration->token) {
			strcpy(rs_configs->vsc[j].token, p_request->Configuration->token);
		}
		if (p_request->Configuration->SourceToken) {
			strcpy(rs_configs->vsc[j].source_token, p_request->Configuration->SourceToken);
		}

		if (p_request->Configuration->Extension) {
			rs_configs->vsc[j].rotate_mode = p_request->Configuration->Extension->Rotate->Mode;
			rs_configs->vsc[j].rotate_degree = *(p_request->Configuration->Extension->Rotate->Degree);
		}
	/* } */

	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);


	/*maybe we need send a message, ask FW to load this setting and take action right now*/
	/*implement later*/

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoEncoderConfiguration(struct soap *soap,
	struct _trt__SetVideoEncoderConfiguration *trt__SetVideoEncoderConfiguration,
	struct _trt__SetVideoEncoderConfigurationResponse *trt__SetVideoEncoderConfigurationResponse)
{
	struct _trt__SetVideoEncoderConfiguration *p_request = trt__SetVideoEncoderConfiguration;
	int i = 0;
	int j = 0;
	int is_vec_exist = 0;
	int ret;
	uint8 codec_combo = 0;
	uint8 codec_res = 0;
	uint8 mode = 0;
	uint8 encodinginterval = 0;
	int frameratelimit = -1;
	int width = -1;
	int height = -1;
	int bitrate = -1;
	int gop = -1;
	int qp = -1;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	int vec_count = 0;
	while (vec_count < MAX_MEDIA_VEC_CONFIG &&
			rs_configs->vec[vec_count].name[0] != '\0')
		vec_count++;

	if (p_request->Configuration->token) {
		for (j = 0; j < vec_count; j++) {
			if (!strcmp(p_request->Configuration->token, rs_configs->vec[j].token)) {
				is_vec_exist = 1;
				break;
			}
		}
	}

	if (!is_vec_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	int fd = 0;
	fd = rts_cam_open(rs_configs->vec[j].vec_path);
	if (fd < 0) {
		onvif_fault(soap, "ter:NoCamDevice", "ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (rts_cam_lock(fd, 1000)) {
		rts_cam_close(fd);
		onvif_fault(soap, "ter:LockCamDev", "ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	struct attr_stream *p_attr_stream = NULL;
	ret = rts_cam_get_stream_attr(fd, &p_attr_stream);

	struct attr_video_encoder *p_attr_vec = NULL;
	ret = rts_cam_get_video_encoder_attr(fd, &p_attr_vec);

	/* to judge whether the parameters are valid or not */
	if (p_request->Configuration != NULL) {
		qp = (int)p_request->Configuration->Quality;
		if (qp > p_attr_vec->max_qp || qp < p_attr_vec->min_qp) {
			RS_DBG("Quality = %f is invalid\n", p_request->Configuration->Quality);
			RS_DBG("qp = %d is invalid, min %d, max %d\n", qp, p_attr_vec->min_qp, p_attr_vec->max_qp);
			onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
			return SOAP_FAULT;
		}

		if (p_request->Configuration->Resolution != NULL) {
			width = p_request->Configuration->Resolution->Width;
			height = p_request->Configuration->Resolution->Height;

			int res_num = p_attr_stream->formats->number;
			for (i = 0; i < p_attr_stream->formats->number; i++) {
				if (width == p_attr_stream->formats->resolutions[i].discrete.width &&
						height == p_attr_stream->formats->resolutions[i].discrete.height) {
					break;
				}
			}
			if (i == res_num) {
				RS_DBG("resolution is invalid\n");
				onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
				return SOAP_FAULT;
			}
		}

		if (p_request->Configuration->H264 != NULL) {
			gop = p_request->Configuration->H264->GovLength;
			if (gop > (int)p_attr_vec->max_gop || gop < (int)p_attr_vec->min_gop) {
				RS_DBG("gop %d is invalid, min %d, max %d\n", gop, p_attr_vec->min_gop, p_attr_vec->max_gop);
				onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
				return SOAP_FAULT;
			}
		}

		if (p_request->Configuration->RateControl != NULL) {
			encodinginterval = p_request->Configuration->RateControl->EncodingInterval;
			if (encodinginterval != 1) {
				RS_DBG("encodinginterval is invalid\n");
				onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
				return SOAP_FAULT;
			}

			bitrate = p_request->Configuration->RateControl->BitrateLimit;
			bitrate <<= 10; /*onvif bitrate unit is kbps*/
			/*
			 * MEDIA-2-1-11 set bitrate to 64000, no matter what we responsed in
			 * GetVideoEncoderConfigurationOptions, however, 64000 is invalid
			 */
			/*
			if (bitrate > p_attr_vec->max_bitrate || bitrate < p_attr_vec->min_bitrate) {
				RS_DBG("bitrate %d is invalid,min=%d, max=%d\n", bitrate, p_attr_vec->min_bitrate, p_attr_vec->max_bitrate);
				onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
				return SOAP_FAULT;
			}
			*/

			frameratelimit = p_request->Configuration->RateControl->FrameRateLimit;
			if (frameratelimit > RTS5826_FPS_MAX || frameratelimit < RTS5826_FPS_MIN) {
				RS_DBG("frameratelimit %d is invalid, min %d, max %d\n", frameratelimit, RTS5826_FPS_MIN, RTS5826_FPS_MAX);
				onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
				return SOAP_FAULT;
			}
		}
	}

	/*set resolution*/
	p_attr_stream->current.resolution.discrete.width = width;
	p_attr_stream->current.resolution.discrete.height = height;
	/*set fps*/
	p_attr_stream->current.frmival.discrete.denominator = frameratelimit;

	ret = rts_cam_set_stream_attr(fd, p_attr_stream);
	rts_cam_free_attr(p_attr_stream);

	p_attr_vec->bitrate = bitrate;
	p_attr_vec->gop = gop;
	p_attr_vec->qp = qp;
	ret = rts_cam_set_video_encoder_attr(fd, p_attr_vec);
	rts_cam_free_attr(p_attr_vec);

	rts_cam_unlock(fd);
	rts_cam_close(fd);


	/*set 2 media_profile*/
	/* if (xsd__boolean__true_ == p_request->ForcePersistence) { */
		rs_configs->vec[j].force_persistence = xsd__boolean__true_;
		rs_configs->vec[j].encoding_format = p_request->Configuration->Encoding;
		rs_configs->vec[j].encoding_width = width;
		rs_configs->vec[j].encoding_height = height;
		rs_configs->vec[j].quality = p_request->Configuration->Quality;

		if (p_request->Configuration->RateControl) {
			rs_configs->vec[j].frame_rate_limit = p_request->Configuration->RateControl->FrameRateLimit;
			rs_configs->vec[j].encoding_interval = p_request->Configuration->RateControl->EncodingInterval;
			rs_configs->vec[j].bitrate_limit = p_request->Configuration->RateControl->BitrateLimit;
		}


		if (p_request->Configuration->H264) {
			rs_configs->vec[j].h264_gov_length =  p_request->Configuration->H264->GovLength;
			rs_configs->vec[j].h264_profile =  p_request->Configuration->H264->H264Profile;
		}
	/* } */

	/* Changes in the Multicast settings shall always be persistent */
	if (p_request->Configuration->Multicast) {
		if (p_request->Configuration->Multicast->Address) {
			rs_configs->vec[j].multicast_ip_type =  p_request->Configuration->Multicast->Address->Type;
			strcpy(rs_configs->vec[j].multicast_ip_addr,
					p_request->Configuration->Multicast->Address->Type == tt__IPType__IPv4 ?
					p_request->Configuration->Multicast->Address->IPv4Address :
					p_request->Configuration->Multicast->Address->IPv6Address);
		}
		rs_configs->vec[j].multicast_port = p_request->Configuration->Multicast->Port;
		rs_configs->vec[j].multicast_auto_start = p_request->Configuration->Multicast->AutoStart;
		rs_configs->vec[j].multicast_ttl = p_request->Configuration->Multicast->TTL;
		rs_configs->vec[j].session_timeout = p_request->Configuration->SessionTimeout;
	}

	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);


/*	ret = rsOnvifDevCtrlChangeVECSettings(frameratelimit, width, height, bitrate, gop, qp);*/
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioSourceConfiguration(struct soap *soap,
	struct _trt__SetAudioSourceConfiguration *trt__SetAudioSourceConfiguration,
	struct _trt__SetAudioSourceConfigurationResponse *trt__SetAudioSourceConfigurationResponse)
{
	struct _trt__SetAudioSourceConfiguration *p_request = trt__SetAudioSourceConfiguration;
	int i;
	int j = 0;
	int flag = 0;

	uint8 _Encoding;

	int ret = 0;
	int asc_count = 0;
	media_config_t *rs_configs = NULL;

	int is_asc_exist = 0;
	int is_as_exist = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	asc_count = 0;
	while (asc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[asc_count].name[0] != '\0')
		asc_count++;

	if (p_request->Configuration->token) {
		for (j = 0; j < asc_count; j++) {
			if (!strcmp(p_request->Configuration->token, rs_configs->asc[j].token)) {
				is_asc_exist = 1;
				break;
			}
		}
	}


	if (!is_asc_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	/* TODO should test whether the parameters are valid */
	for (i = 0; i < asc_count; i++) {
		if (!strcmp(p_request->Configuration->SourceToken, rs_configs->asc[j].source_token)) {
			is_as_exist = 1;
			break;
		}
	}

	if (!is_as_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
		return SOAP_FAULT;
	}

	/* if (xsd__boolean__true_ == p_request->ForcePersistence) { */
		if (p_request->Configuration->Name) {
			strcpy(rs_configs->asc[j].name, p_request->Configuration->Name);
		}
		if (p_request->Configuration->Name) {
			strcpy(rs_configs->asc[j].token, p_request->Configuration->token);
		}
		if (p_request->Configuration->Name) {
			strcpy(rs_configs->asc[j].source_token, p_request->Configuration->SourceToken);
		}

		rs_configs->asc[j].force_persistence = 1;
	/* } */

	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);


	/*maybe we need send a message, ask FW to load this setting and take action right now*/
	/*implement later*/

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioEncoderConfiguration(struct soap *soap,
	struct _trt__SetAudioEncoderConfiguration *trt__SetAudioEncoderConfiguration,
	struct _trt__SetAudioEncoderConfigurationResponse *trt__SetAudioEncoderConfigurationResponse)
{
	struct _trt__SetAudioEncoderConfiguration *p_request = trt__SetAudioEncoderConfiguration;
	struct _trt__SetAudioEncoderConfigurationResponse *p_response = trt__SetAudioEncoderConfigurationResponse;
	int i = 0;
	int j = 0;
	int k = 0;
	int ret = 0;
	int is_aec_exist = 0;
	media_config_t *rs_configs = NULL;

	int bit_rate = 0;
	int sample_rate = 0;

	int fd = 0;

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

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);


	int aec_count = 0;
	while (aec_count < MAX_MEDIA_VEC_CONFIG &&
			rs_configs->aec[aec_count].name[0] != '\0')
		aec_count++;

	if (p_request->Configuration && p_request->Configuration->token) {
		for (j = 0; j < aec_count; j++) {
			if (!strcmp(p_request->Configuration->token, rs_configs->aec[j].token)) {
				is_aec_exist = 1;
				break;
			}
		}
	}

	if (!is_aec_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	if (p_request->Configuration != NULL) {
		fd = rts_cam_open(TMP_AUDIO_PATH);
		if (fd < 0) {
			onvif_fault(soap, SENDER, "ter:NoCamDevice", "ter:InvalidArgValue");
			return SOAP_FAULT;
		}

		if (rts_cam_lock(fd, 1000)) {
			rts_cam_close(fd);
			onvif_fault(soap, SENDER, "ter:LockCamDev", "ter:InvalidArgValue");
			return SOAP_FAULT;
		}

		struct attr_audio_encoder *p_attr_aec = NULL;
		ret = rts_cam_get_audio_encoder_attr(fd, &p_attr_aec);

		bit_rate = p_request->Configuration->Bitrate;
		bit_rate *= 1000;

		sample_rate = p_request->Configuration->SampleRate;
		sample_rate *= 1000;

		for (i = 0; i < p_attr_aec->number; i++) {
			if (p_request->Configuration->Encoding == p_attr_aec->options[i].codec_id) {
				for (k = 0; k < p_attr_aec->options[i].bitrate_num; k++) {
					if (bit_rate == p_attr_aec->options[i].bitrate[k]) {
						break;
					}
				}
				if (k == p_attr_aec->options[i].bitrate_num) {
					onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
					return SOAP_FAULT;
				}

				for (k = 0; k < p_attr_aec->options[i].sample_rate_num; k++) {
					if (sample_rate == p_attr_aec->options[i].sample_rate[k]) {
						break;
					}
				}
				if (k == p_attr_aec->options[i].sample_rate_num) {
					onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
					return SOAP_FAULT;
				}

				break;
			}
		}

		/* encoding is invalid */
		if (i == p_attr_aec->number) {
			onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:ConfigModify");
			return SOAP_FAULT;
		}

		p_attr_aec->current.codec_id = p_request->Configuration->Encoding;
		p_attr_aec->current.bitrate = bit_rate;
		p_attr_aec->current.sample_rate = sample_rate;

		ret = rts_cam_set_audio_encoder_attr(fd, &p_attr_aec);

		rts_cam_free_attr(p_attr_aec);
		rts_cam_unlock(fd);
		rts_cam_close(fd);
	} else {
		return SOAP_OK;
	}


	/* if (xsd__boolean__true_ == p_request->ForcePersistence) { */
	strcpy(rs_configs->aec[j].name, p_request->Configuration->Name);
	rs_configs->aec[j].force_persistence = xsd__boolean__true_;
	rs_configs->aec[j].encoding_format = p_request->Configuration->Encoding;
	rs_configs->aec[j].bit_rate = p_request->Configuration->Bitrate;
	rs_configs->aec[j].sample_rate = p_request->Configuration->SampleRate;
	rs_configs->aec[j].session_timeout = p_request->Configuration->SessionTimeout;
	/* } */

	/* Changes in the Multicast settings shall always be persistent */
	if (p_request->Configuration->Multicast) {
		if (p_request->Configuration->Multicast->Address) {
			rs_configs->aec[j].multicast_ip_type =  p_request->Configuration->Multicast->Address->Type;
			strcpy(rs_configs->aec[j].multicast_ip_addr,
					p_request->Configuration->Multicast->Address->Type == tt__IPType__IPv4 ?
					p_request->Configuration->Multicast->Address->IPv4Address :
					p_request->Configuration->Multicast->Address->IPv6Address);
		}
		rs_configs->aec[j].port = p_request->Configuration->Multicast->Port;
		rs_configs->aec[j].auto_start = p_request->Configuration->Multicast->AutoStart;
		rs_configs->aec[j].ttl = p_request->Configuration->Multicast->TTL;
	}

	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoAnalyticsConfiguration(struct soap *soap,
	struct _trt__SetVideoAnalyticsConfiguration *trt__SetVideoAnalyticsConfiguration,
	struct _trt__SetVideoAnalyticsConfigurationResponse *trt__SetVideoAnalyticsConfigurationResponse)
{

	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:SetVideoAnalyticsNotSupported");
	return SOAP_FAULT;

}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetMetadataConfiguration(struct soap *soap,
	struct _trt__SetMetadataConfiguration *trt__SetMetadataConfiguration,
	struct _trt__SetMetadataConfigurationResponse *trt__SetMetadataConfigurationResponse)
{
	struct _trt__SetMetadataConfiguration *p_request = trt__SetMetadataConfiguration;

	int i;
	int j = 0;
	int is_token_exist = 0;
	int is_mc_exist = 0;

	int ret = 0;
	int mc_count = 0;

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

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	mc_count = 0;
	while (mc_count < MAX_MEDIA_MC_CONFIG &&
			rs_configs->mc[mc_count].name[0] != '\0')
		mc_count++;

	if (p_request->Configuration->token != NULL) {
		for (j = 0; j < mc_count; j++) {
			if (!strcmp(p_request->Configuration->token, rs_configs->mc[j].token)) {
				is_mc_exist = 1;
				break;
			}
		}
	}
	/* if ConfigurationToken does not exist*/
	if (!is_mc_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	/* if (p_request->ForcePersistence) { */
		if (p_request->Configuration->Name) {
			strcpy(rs_configs->mc[j].name, p_request->Configuration->Name);
		}
		if (p_request->Configuration->token) {
			strcpy(rs_configs->mc[j].token, p_request->Configuration->token);
		}

		if (p_request->Configuration->Analytics) {
			strcpy(rs_configs->mc[j].analytics, p_request->Configuration->Analytics);
		}

		if (p_request->Configuration->PTZStatus) {
			strcpy(rs_configs->mc[j].ptz_status, p_request->Configuration->PTZStatus->Status ? "true" : "false");
		}

		if (p_request->Configuration->Extension) {
			strcpy(rs_configs->mc[j].ptz_position, p_request->Configuration->PTZStatus->Position ? "true" : "false");
		}

		if (p_request->Configuration->Events
			&& p_request->Configuration->Events->Filter
			&& p_request->Configuration->Events->Filter->__any) {
			strcpy(rs_configs->mc[j].event_filter, p_request->Configuration->Events->Filter->__any);
		}

		if (p_request->Configuration->Events
			&& p_request->Configuration->Events
			&& p_request->Configuration->Events->SubscriptionPolicy
			&& p_request->Configuration->Events->SubscriptionPolicy->__any
			) {
			strcpy(rs_configs->mc[j].event_subscription_policy,
				p_request->Configuration->Events->SubscriptionPolicy->__any);
		}


		rs_configs->mc[j].force_persistence = 1;
		rs_configs->mc[j].session_timeout  = p_request->Configuration->SessionTimeout;
		rs_configs->mc[j].multicast_ip_type = p_request->Configuration->Multicast->Address->Type;

		if (p_request->Configuration->Multicast) {
			if (p_request->Configuration->Multicast->Address
					&& p_request->Configuration->Multicast->Address->IPv4Address) {
				strcpy(rs_configs->mc[j].multicast_ip_addr, p_request->Configuration->Multicast->Address->IPv4Address);
			}

			rs_configs->mc[j].multicast_port  = p_request->Configuration->Multicast->Port;
			rs_configs->mc[j].multicast_ttl = p_request->Configuration->Multicast->TTL;
			rs_configs->mc[j].multicast_auto_start = p_request->Configuration->Multicast->AutoStart;
		}
	/* } */


	ret = rsOnvifSysInfoSetMediaConfigs(rs_configs);
	/*may be send msg to take action*/



	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioOutputConfiguration(struct soap *soap,
	struct _trt__SetAudioOutputConfiguration *trt__SetAudioOutputConfiguration,
	struct _trt__SetAudioOutputConfigurationResponse *trt__SetAudioOutputConfigurationResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:AudioIn/OutNotSupported");
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioDecoderConfiguration(struct soap *soap,
	struct _trt__SetAudioDecoderConfiguration *trt__SetAudioDecoderConfiguration,
	struct _trt__SetAudioDecoderConfigurationResponse *trt__SetAudioDecoderConfigurationResponse)
{
	onvif_fault(soap, RECEIVER, "ter:ActionNotSupported", "ter:AudioIn/OutNotSupported");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfigurationOptions(struct soap *soap,
	struct _trt__GetVideoSourceConfigurationOptions *trt__GetVideoSourceConfigurationOptions,
	struct _trt__GetVideoSourceConfigurationOptionsResponse *trt__GetVideoSourceConfigurationOptionsResponse)
{
	struct _trt__GetVideoSourceConfigurationOptions *p_request =
		trt__GetVideoSourceConfigurationOptions;
	struct _trt__GetVideoSourceConfigurationOptionsResponse *p_response =
		trt__GetVideoSourceConfigurationOptionsResponse;

	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	int flag = 0;
	int index = 0;

	int profile_count = 0, ret = 0;
	media_profile_t *rs_profiles = NULL;
	int is_profile_exist = 0;

	int is_vsc_exist = 0;
	int vsc_count = 0;
	media_config_t *rs_configs = NULL;

	int source_token_count = 0;

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
	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	vsc_count = 0;
	while (vsc_count < MAX_MEDIA_VSC_CONFIG &&
			rs_configs->vsc[vsc_count].name[0] != '\0')
		vsc_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	if (p_request->ConfigurationToken) {
		for (j = 0; j < vsc_count; j++) {
			if (!strcmp(p_request->ConfigurationToken, rs_configs->vsc[j].token)) {
				is_vsc_exist = 1;
				break;
			}
		}
	}


	if (p_request->ProfileToken && !is_profile_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (p_request->ConfigurationToken && !is_vsc_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		p_response->Options =
			(struct tt__VideoSourceConfigurationOptions *)soap_malloc(soap,
				sizeof(struct tt__VideoSourceConfigurationOptions));

		/* caputre area of sensor can't be changed */
		p_response->Options->BoundsRange =
			(struct tt__IntRectangleRange *)soap_malloc(soap, sizeof(struct tt__IntRectangleRange));

		p_response->Options->BoundsRange->XRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->BoundsRange->XRange->Min = 0;
		p_response->Options->BoundsRange->XRange->Max = 0;

		p_response->Options->BoundsRange->YRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->BoundsRange->YRange->Min = 0;
		p_response->Options->BoundsRange->YRange->Max = 0;

		p_response->Options->BoundsRange->WidthRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->BoundsRange->WidthRange->Min = SENSOR_FMT_WIDTH;
		p_response->Options->BoundsRange->WidthRange->Max = SENSOR_FMT_WIDTH;

		p_response->Options->BoundsRange->HeightRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->BoundsRange->HeightRange->Min = SENSOR_FMT_HEIGHT;
		p_response->Options->BoundsRange->HeightRange->Max = SENSOR_FMT_HEIGHT;

		if (is_vsc_exist) {
			p_response->Options->__sizeVideoSourceTokensAvailable = 1;
			p_response->Options->VideoSourceTokensAvailable =
				(char **)soap_malloc(soap, sizeof(char *) * p_response->Options->__sizeVideoSourceTokensAvailable);
			p_response->Options->VideoSourceTokensAvailable[0] =
				(char *)soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
			strcpy(p_response->Options->VideoSourceTokensAvailable[0], rs_configs->vsc[j].source_token);
		} else if (is_profile_exist) {
			p_response->Options->__sizeVideoSourceTokensAvailable = 1;
			p_response->Options->VideoSourceTokensAvailable =
				(char **)soap_malloc(soap, sizeof(char *) * p_response->Options->__sizeVideoSourceTokensAvailable);
			p_response->Options->VideoSourceTokensAvailable[0] =
				(char *)soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
			strcpy(p_response->Options->VideoSourceTokensAvailable[0], rs_configs->vsc[rs_profiles[i].vsc_index - 1].source_token);
		} else {
			/* if no vsc and profile is specified, then return all video source token */
			/* count the num of different video source tokens */
			source_token_count = 0;
			for (k = 0; k < vsc_count; k++) {
				for (l = 0; l < k; l++) {
					if (!strcmp(rs_configs->vsc[k].source_token, rs_configs->vsc[l].source_token)) {
						++source_token_count;
						break;
					}
				}
			}

			p_response->Options->__sizeVideoSourceTokensAvailable = source_token_count;
			p_response->Options->VideoSourceTokensAvailable =
				(char **)soap_malloc(soap, sizeof(char *) * p_response->Options->__sizeVideoSourceTokensAvailable);

			source_token_count = 0;
			for (k = 0; k < vsc_count; k++) {
				flag = 0;
				/* find different video source tokens */
				for (l = 0; l < k; l++) {
					if (!strcmp(rs_configs->vsc[k].source_token, rs_configs->vsc[l].source_token)) {
						flag = 1;
						break;
					}

				}

				if (!flag) {
					p_response->Options->VideoSourceTokensAvailable[source_token_count] =
						(char *)soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
					strcpy(p_response->Options->VideoSourceTokensAvailable[source_token_count], rs_configs->vsc[k].source_token);
					source_token_count++;

				}
			}
			p_response->Options->__sizeVideoSourceTokensAvailable = source_token_count;
		}
	}

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfigurationOptions(struct soap *soap,
	struct _trt__GetVideoEncoderConfigurationOptions *trt__GetVideoEncoderConfigurationOptions,
	struct _trt__GetVideoEncoderConfigurationOptionsResponse *trt__GetVideoEncoderConfigurationOptionsResponse)
{
	struct _trt__GetVideoEncoderConfigurationOptions *p_request =
		trt__GetVideoEncoderConfigurationOptions;
	struct _trt__GetVideoEncoderConfigurationOptionsResponse *p_response =
		trt__GetVideoEncoderConfigurationOptionsResponse;

	int i = 0, j = 0, ret = 0;
	int k = 0;
	int m = 0;
	int fd = 0;
	char *profile_token = NULL;
	char *vec_token = NULL;
	int profile_count = 0;
	media_profile_t *rs_profiles = NULL;
	int vec_count = 0;
	media_config_t *rs_configs = NULL;
	int is_profile_exist = 0;
	int is_vec_exist = 0;
	int res_num = 0;
	int stepwise_res_num = 0;
	int min_width = 0;
	int max_width = 0;
	int step_width = 0;
	int min_height = 0;
	int max_height = 0;
	int step_height = 0;

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
	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	vec_count = 0;
	while (vec_count < MAX_MEDIA_VEC_CONFIG &&
			rs_configs->vec[vec_count].name[0] != '\0')
		vec_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < vec_count; j++) {
			if (!strcmp(p_request->ConfigurationToken, rs_configs->vec[j].token)) {
				is_vec_exist = 1;
				break;
			}
		}
	}

	p_response->Options =
		(struct tt__VideoEncoderConfigurationOptions *)soap_malloc(soap,
			sizeof(struct tt__VideoEncoderConfigurationOptions));
	p_response->Options->JPEG  = NULL;
	p_response->Options->MPEG4 = NULL;
	p_response->Options->H264  = NULL;
	p_response->Options->Extension  = NULL;
	p_response->Options->Extension =
		(struct  tt__VideoEncoderOptionsExtension *)soap_malloc(soap,
			sizeof(struct tt__VideoEncoderOptionsExtension));
	p_response->Options->Extension->JPEG = NULL;
	p_response->Options->Extension->H264 = NULL;
	p_response->Options->Extension->MPEG4 = NULL;

	p_response->Options->QualityRange =
		(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
	p_response->Options->QualityRange->Min = RTS5826_QP_MIN;
	p_response->Options->QualityRange->Max = RTS5826_QP_MAX;

	if (p_request->ProfileToken && !is_profile_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (p_request->ConfigurationToken && !is_vec_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else if (is_vec_exist) {
		RS_DBG("get vec options 1: vec token= %s\n", rs_configs->vec[j].token);
		fd = rts_cam_open(rs_configs->vec[j].vec_path);
	} else if (is_profile_exist) {
		RS_DBG("get vec options 2: vec token= %s\n", rs_configs->vec[rs_profiles[i].vec_index - 1].token);
		fd = rts_cam_open(rs_configs->vec[rs_profiles[i].vec_index - 1].vec_path);
	} else {
		/*No profile and vec is specified, neil_yan has no idea now, implement later*/
		RS_DBG("get vec options 3: vec token= %s\n", rs_configs->vec[rs_profiles[0].vec_index].token);
		fd = rts_cam_open(rs_configs->vec[0].vec_path);
		/*
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfileAndConfig");
		return SOAP_FAULT;
		*/
	}

	if (fd < 0) {
		onvif_fault(soap, SENDER, "ter:NoCamDevice", "ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (rts_cam_lock(fd, 1000)) {
		rts_cam_close(fd);
		onvif_fault(soap, SENDER, "ter:LockCamDev", "ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	struct attr_stream *p_attr_stream = NULL;
	struct attr_video_encoder *p_attr_vec = NULL;
	ret = rts_cam_get_stream_attr(fd, &p_attr_stream);


	for (i = 0; i < p_attr_stream->number; i++) {
		if (p_attr_stream->formats[i].format == V4L2_PIX_FMT_MJPEG) {
			RS_DBG("format number is %d\n", p_attr_stream->number);
			RS_DBG("format is JPEG\n");
			break;
		}
	}

	if (i != p_attr_stream->number) {
		p_response->Options->JPEG =
			(struct tt__JpegOptions *)soap_malloc(soap, sizeof(struct tt__JpegOptions));

		res_num = 0;
		for (j = 0; j < p_attr_stream->formats[i].number; j++) {
			if (VIDEO_FRMSIZE_TYPE_DISCRETE == p_attr_stream->formats[i].resolutions[j].type) {
				res_num++;
			} else {
				min_width = p_attr_stream->formats[i].resolutions[j].stepwise.min_width;
				max_width = p_attr_stream->formats[i].resolutions[j].stepwise.max_width;
				step_width = p_attr_stream->formats[i].resolutions[j].stepwise.step_width;
				res_num += (max_width - min_width) / step_width + 1;
			}
		}

		p_response->Options->JPEG->__sizeResolutionsAvailable = res_num;
		p_response->Options->JPEG->ResolutionsAvailable  =
			(struct tt__VideoResolution *)soap_malloc(soap, res_num * sizeof(struct tt__VideoResolution));
		k = 0;
		for (j = 0; j < p_attr_stream->formats[i].number; j++) {
			if (VIDEO_FRMSIZE_TYPE_DISCRETE == p_attr_stream->formats[i].resolutions[j].type) {
				p_response->Options->JPEG->ResolutionsAvailable[k].Width  =
					p_attr_stream->formats[i].resolutions[j].discrete.width;
				p_response->Options->JPEG->ResolutionsAvailable[k].Height =
					p_attr_stream->formats[i].resolutions[j].discrete.height;
				++k;
			} else {
				min_width = p_attr_stream->formats[i].resolutions[j].stepwise.min_width;
				max_width = p_attr_stream->formats[i].resolutions[j].stepwise.max_width;
				step_width = p_attr_stream->formats[i].resolutions[j].stepwise.step_width;
				min_height = p_attr_stream->formats[i].resolutions[j].stepwise.min_height;
				max_height = p_attr_stream->formats[i].resolutions[j].stepwise.max_height;
				step_height = p_attr_stream->formats[i].resolutions[j].stepwise.step_height;
				stepwise_res_num = (max_width - min_width) / step_width + 1;
				for (m = 0; m < stepwise_res_num; m++) {
					p_response->Options->JPEG->ResolutionsAvailable[k].Width  =
						min_width + m * step_width;
					p_response->Options->JPEG->ResolutionsAvailable[k].Height =
						min_height + m * step_height;
					++k;
				}
			}
		}

		p_response->Options->JPEG->FrameRateRange  =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->JPEG->FrameRateRange->Min = 8;
		p_response->Options->JPEG->FrameRateRange->Max = 30;
		p_response->Options->JPEG->EncodingIntervalRange  =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->JPEG->EncodingIntervalRange->Min = 1;
		p_response->Options->JPEG->EncodingIntervalRange->Max = 1;
	} else {
		p_response->Options->JPEG = NULL;
	}

	p_response->Options->MPEG4 = NULL;


	for (i = 0; i < p_attr_stream->number; i++) {
		if (p_attr_stream->formats[i].format == V4L2_PIX_FMT_NV12) {
			RS_DBG("format number is %d\n", p_attr_stream->number);
			RS_DBG("format is H264\n");
			break;
		}
	}

	if (i != p_attr_stream->number) {
		p_response->Options->H264 =
			(struct tt__H264Options *)soap_malloc(soap,
				sizeof(struct tt__H264Options));

		res_num = 0;
		for (j = 0; j < p_attr_stream->formats[i].number; j++) {
			if (VIDEO_FRMSIZE_TYPE_DISCRETE == p_attr_stream->formats[i].resolutions[j].type) {
				res_num++;
			} else {
				min_width = p_attr_stream->formats[i].resolutions[j].stepwise.min_width;
				max_width = p_attr_stream->formats[i].resolutions[j].stepwise.max_width;
				step_width = p_attr_stream->formats[i].resolutions[j].stepwise.step_width;
				res_num += (max_width - min_width) / step_width + 1;
			}
		}

		p_response->Options->H264->__sizeResolutionsAvailable = res_num;
		p_response->Options->H264->ResolutionsAvailable  =
			(struct tt__VideoResolution *)soap_malloc(soap, res_num * sizeof(struct tt__VideoResolution));
		k = 0;
		for (j = 0; j < p_attr_stream->formats[i].number; j++) {
			if (VIDEO_FRMSIZE_TYPE_DISCRETE == p_attr_stream->formats[i].resolutions[j].type) {
				p_response->Options->H264->ResolutionsAvailable[k].Width  =
					p_attr_stream->formats[i].resolutions[j].discrete.width;
				p_response->Options->H264->ResolutionsAvailable[k].Height =
					p_attr_stream->formats[i].resolutions[j].discrete.height;
				++k;
			} else {
				min_width = p_attr_stream->formats[i].resolutions[j].stepwise.min_width;
				max_width = p_attr_stream->formats[i].resolutions[j].stepwise.max_width;
				step_width = p_attr_stream->formats[i].resolutions[j].stepwise.step_width;
				min_height = p_attr_stream->formats[i].resolutions[j].stepwise.min_height;
				max_height = p_attr_stream->formats[i].resolutions[j].stepwise.max_height;
				step_height = p_attr_stream->formats[i].resolutions[j].stepwise.step_height;
				stepwise_res_num = (max_width - min_width) / step_width + 1;
				for (m = 0; m < stepwise_res_num; m++) {
					p_response->Options->H264->ResolutionsAvailable[k].Width  =
						min_width + m * step_width;
					p_response->Options->H264->ResolutionsAvailable[k].Height =
						min_height + m * step_height;
					++k;
				}
			}
		}

		ret = rts_cam_get_video_encoder_attr(fd, &p_attr_vec);
		p_response->Options->QualityRange->Min = p_attr_vec->min_qp;
		p_response->Options->QualityRange->Max = p_attr_vec->max_qp;

		p_response->Options->H264->GovLengthRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->H264->GovLengthRange->Min = p_attr_vec->min_gop;
		p_response->Options->H264->GovLengthRange->Max = p_attr_vec->max_gop;
		/*wth, we only support discrete fps yafei*/
		p_response->Options->H264->FrameRateRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->H264->FrameRateRange->Min = RTS5826_FPS_MIN;
		p_response->Options->H264->FrameRateRange->Max = RTS5826_FPS_MAX;

		p_response->Options->H264->EncodingIntervalRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->H264->EncodingIntervalRange->Min = RTS5826_ENCODING_INTERVAL;
		p_response->Options->H264->EncodingIntervalRange->Max = RTS5826_ENCODING_INTERVAL;

		p_response->Options->H264->__sizeH264ProfilesSupported = RTS5826_SUPPORTED_PROFILE_NUM;
		p_response->Options->H264->H264ProfilesSupported =
			(int *)soap_malloc(soap, RTS5826_SUPPORTED_PROFILE_NUM * sizeof(int));
		/*Baseline = 0, Main = 1, High = 3*/
		p_response->Options->H264->H264ProfilesSupported[0] = 0;
		p_response->Options->Extension->H264 =
			(struct tt__H264Options2 *)soap_malloc(soap, sizeof(struct tt__H264Options2));
		res_num = p_response->Options->H264->__sizeResolutionsAvailable;
		p_response->Options->Extension->H264->__sizeResolutionsAvailable = res_num;
		p_response->Options->Extension->H264->ResolutionsAvailable =
			(struct tt__VideoResolution *)soap_malloc(soap,
				res_num * sizeof(struct tt__VideoResolution));
		for (j = 0; j < res_num; j++) {
			p_response->Options->Extension->H264->ResolutionsAvailable[j] =
				p_response->Options->H264->ResolutionsAvailable[j];
		}

		p_response->Options->Extension->H264->GovLengthRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->Extension->H264->GovLengthRange->Max =
			p_response->Options->H264->GovLengthRange->Max;
		p_response->Options->Extension->H264->GovLengthRange->Min =
			p_response->Options->H264->GovLengthRange->Min;

		p_response->Options->Extension->H264->FrameRateRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->Extension->H264->FrameRateRange->Max =
			p_response->Options->H264->FrameRateRange->Max;
		p_response->Options->Extension->H264->FrameRateRange->Min =
			p_response->Options->H264->FrameRateRange->Min;

		p_response->Options->Extension->H264->EncodingIntervalRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->Extension->H264->EncodingIntervalRange->Max =
			p_response->Options->H264->EncodingIntervalRange->Max;
		p_response->Options->Extension->H264->EncodingIntervalRange->Min =
			p_response->Options->H264->EncodingIntervalRange->Min;

		p_response->Options->Extension->H264->BitrateRange =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		p_response->Options->Extension->H264->BitrateRange->Min =
			p_attr_vec->min_bitrate >> 10; /*change to kbps*/
		p_response->Options->Extension->H264->BitrateRange->Max =
			p_attr_vec->max_bitrate >> 10;

		p_response->Options->Extension->H264->__sizeH264ProfilesSupported =
			p_response->Options->H264->__sizeH264ProfilesSupported;
		p_response->Options->Extension->H264->H264ProfilesSupported =
			(int *)soap_malloc(soap,
				p_response->Options->H264->__sizeH264ProfilesSupported);
		for (j = 0; j < p_response->Options->H264->__sizeH264ProfilesSupported; j++) {
			p_response->Options->Extension->H264->H264ProfilesSupported[j] =
				p_response->Options->H264->H264ProfilesSupported[j];
		}

		rts_cam_free_attr(p_attr_vec);
	} else {
		p_response->Options->H264 = NULL;
	}

	rts_cam_free_attr(p_attr_stream);

	rts_cam_unlock(fd);
	rts_cam_close(fd);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfigurationOptions(struct soap *soap,
	struct _trt__GetAudioSourceConfigurationOptions *trt__GetAudioSourceConfigurationOptions,
	struct _trt__GetAudioSourceConfigurationOptionsResponse *trt__GetAudioSourceConfigurationOptionsResponse)
{

	struct _trt__GetAudioSourceConfigurationOptions *p_request =
		trt__GetAudioSourceConfigurationOptions;
	struct _trt__GetAudioSourceConfigurationOptionsResponse *p_response =
		trt__GetAudioSourceConfigurationOptionsResponse;

	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	int flag = 0;
	int index = 0;

	int profile_count = 0, ret = 0;
	media_profile_t *rs_profiles = NULL;
	int is_profile_exist = 0;

	int is_asc_exist = 0;
	int asc_count = 0;
	media_config_t *rs_configs = NULL;

	int source_token_count = 0;

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
	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	asc_count = 0;
	while (asc_count < MAX_MEDIA_VSC_CONFIG &&
			rs_configs->asc[asc_count].name[0] != '\0')
		asc_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	if (p_request->ConfigurationToken) {
		for (j = 0; j < asc_count; j++) {
			if (!strcmp(p_request->ConfigurationToken, rs_configs->asc[j].token)) {
				is_asc_exist = 1;
				break;
			}
		}
	}

	if (p_request->ProfileToken && !is_profile_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (p_request->ConfigurationToken && !is_asc_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		p_response->Options =
			(struct tt__VideoSourceConfigurationOptions *)soap_malloc(soap,
				sizeof(struct tt__VideoSourceConfigurationOptions));

		p_response->Options->__sizeInputTokensAvailable = 1;

		if (is_asc_exist) {
			p_response->Options->__sizeInputTokensAvailable = 1;
			p_response->Options->InputTokensAvailable =
				(char **)soap_malloc(soap, sizeof(char *) * p_response->Options->__sizeInputTokensAvailable);
			p_response->Options->InputTokensAvailable[0] =
				(char *)soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
			strcpy(p_response->Options->InputTokensAvailable[0], rs_configs->asc[j].source_token);
		} else if (is_profile_exist) {
			p_response->Options->__sizeInputTokensAvailable = 1;
			p_response->Options->InputTokensAvailable =
				(char **)soap_malloc(soap, sizeof(char *) * p_response->Options->__sizeInputTokensAvailable);
			p_response->Options->InputTokensAvailable[0] =
				(char *)soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
			strcpy(p_response->Options->InputTokensAvailable[0], rs_configs->asc[rs_profiles[i].asc_index - 1].source_token);
		} else {
			/* if no asc and profile is specified, then return all video source token */

			/* count the num of different video source tokens */
			source_token_count = 0;
			for (k = 0; k < asc_count; k++) {
				for (l = 0; l < k; l++) {
					if (!strcmp(rs_configs->asc[k].source_token, rs_configs->asc[l].source_token)) {
						++source_token_count;
						break;
					}
				}
			}
			p_response->Options->__sizeInputTokensAvailable = source_token_count;
			p_response->Options->InputTokensAvailable =
				(char **)soap_malloc(soap, sizeof(char *) * p_response->Options->__sizeInputTokensAvailable);

			source_token_count = 0;
			for (k = 0; k < asc_count; k++) {
				flag = 0;
				/* find different video source tokens */
				for (l = 0; l < k; l++) {
					if (!strcmp(rs_configs->asc[k].source_token, rs_configs->asc[l].source_token)) {
						flag = 1;
						break;
					}

				}

				if (!flag) {
					p_response->Options->InputTokensAvailable[source_token_count] =
						(char *)soap_malloc(soap, sizeof(char) * MEDIA_NAME_LENGTH);
					strcpy(p_response->Options->InputTokensAvailable[source_token_count], rs_configs->asc[k].source_token);
					source_token_count++;

				}
			}
		}
		p_response->Options->Extension = NULL;
		p_response->Options->__anyAttribute = NULL;
	}


	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfigurationOptions(struct soap *soap,
	struct _trt__GetAudioEncoderConfigurationOptions *trt__GetAudioEncoderConfigurationOptions,
	struct _trt__GetAudioEncoderConfigurationOptionsResponse *trt__GetAudioEncoderConfigurationOptionsResponse)
{
	struct _trt__GetAudioEncoderConfigurationOptions *p_request =
		trt__GetAudioEncoderConfigurationOptions;
	struct _trt__GetAudioEncoderConfigurationOptionsResponse *p_response =
		trt__GetAudioEncoderConfigurationOptionsResponse;

	int is_profile_exist = 0;
	int is_aec_exist = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int index = 0;
	int _Encoding;
	int default_Bitrate = 64;
	int default_Sample_Rate = 8;

	int profile_count = 0, ret = 0;
	media_profile_t *rs_profiles = NULL;

	int aec_count = 0;
	media_config_t *rs_configs = NULL;

	int fd = 0;

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
	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	aec_count = 0;
	while (aec_count < MAX_MEDIA_AEC_CONFIG &&
			rs_configs->aec[aec_count].name[0] != '\0')
		aec_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < aec_count; j++) {
			if (!strcmp(p_request->ConfigurationToken, rs_configs->aec[j].token)) {
				is_aec_exist = 1;
				break;
			}
		}
	}

	/* need to rewrite according to audio lib */
	if (p_request->ProfileToken && !is_profile_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else if (p_request->ConfigurationToken && !is_aec_exist) {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	} else {
		fd = rts_cam_open(TMP_AUDIO_PATH);
		if (fd < 0) {
			onvif_fault(soap, SENDER, "ter:NoCamDevice", "ter:InvalidArgValue");
			return SOAP_FAULT;
		}

		if (rts_cam_lock(fd, 1000)) {
			rts_cam_close(fd);
			onvif_fault(soap, SENDER, "ter:LockCamDev", "ter:InvalidArgValue");
			return SOAP_FAULT;
		}

		struct attr_audio_encoder *p_attr_aec = NULL;
		ret = rts_cam_get_audio_encoder_attr(fd, &p_attr_aec);

		p_response->Options =
			(struct tt__AudioEncoderConfigurationOptions *)soap_malloc(soap,
					sizeof(struct tt__AudioEncoderConfigurationOptions));

		p_response->Options->__sizeOptions = p_attr_aec->number;
		p_response->Options->Options =
			(struct tt__AudioEncoderConfigurationOption *)soap_malloc(soap,
					p_attr_aec->number * sizeof(struct tt__AudioEncoderConfigurationOption));

		for (i = 0; i < p_attr_aec->number; i++) {
			p_response->Options->Options[i].Encoding = p_attr_aec->options[i].codec_id;

			p_response->Options->Options[i].BitrateList =
				(struct tt__IntList *)soap_malloc(soap, sizeof(struct tt__IntList));

			p_response->Options->Options[i].BitrateList->__sizeItems = p_attr_aec->options[i].bitrate_num;
			p_response->Options->Options[i].BitrateList->Items =
				(int *)soap_malloc(soap, p_attr_aec->options[i].bitrate_num * sizeof(int));
			for (k = 0; k < p_attr_aec->options[i].bitrate_num; k++) {
				p_response->Options->Options[i].BitrateList->Items[k] = p_attr_aec->options[i].bitrate[k] / 1000;
			}

			p_response->Options->Options[i].SampleRateList =
				(struct tt__IntList *)soap_malloc(soap, sizeof(struct tt__IntList));

			p_response->Options->Options[i].SampleRateList->__sizeItems = p_attr_aec->options[i].sample_rate_num;
			p_response->Options->Options[i].SampleRateList->Items =
				(int *)soap_malloc(soap, p_attr_aec->options[i].sample_rate_num * sizeof(int));
			for (k = 0; k < p_attr_aec->options[i].sample_rate_num; k++) {
				p_response->Options->Options[i].SampleRateList->Items[k] = p_attr_aec->options[i].sample_rate[k] / 1000;
			}

		}

		rts_cam_free_attr(p_attr_aec);
		rts_cam_unlock(fd);
		rts_cam_close(fd);
	}

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfigurationOptions(struct soap *soap,
	struct _trt__GetMetadataConfigurationOptions *trt__GetMetadataConfigurationOptions,
	struct _trt__GetMetadataConfigurationOptionsResponse *trt__GetMetadataConfigurationOptionsResponse)
{

	struct _trt__GetMetadataConfigurationOptionsResponse *p_response =  trt__GetMetadataConfigurationOptionsResponse;
	int i;
	int num_tokens = 0;
	int j;
	int flag = 0;
	int index = 0;
	int num = 0;
	int profile_count = 0, ret = 0;
	media_profile_t *rs_profiles = NULL;

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
	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	for (i = 0; i < profile_count; i++) {
		for (j = 0; j <= i; j++) {
			if (!strcmp(rs_configs->mc[rs_profiles[j].mc_index - 1].token, rs_configs->mc[rs_profiles[i].mc_index - 1].token)) {
				flag = 1;
				break;
			}
		}
		if (!flag) {
			num_tokens++;
		}
	}

	flag = 0;
	if (trt__GetMetadataConfigurationOptions->ConfigurationToken != NULL) {
		for (i = 0; i <= profile_count ; i++) {
			if (!strcmp(trt__GetMetadataConfigurationOptions->ConfigurationToken, rs_configs->mc[rs_profiles[i].mc_index - 1].token)) {
				flag = 1;
				index = j;
				break;
			}
		}
	}

	if (trt__GetMetadataConfigurationOptions->ProfileToken != NULL) {
		for (i = 0; i <= profile_count ; i++) {
			if (!strcmp(trt__GetMetadataConfigurationOptions->ProfileToken, rs_profiles[i].token)) {
				flag = 1;
				index = j;
				break;
			}
		}
	}

	if (!flag) {
		return SOAP_FAULT;
	} else {
		p_response->Options =
			(struct tt__MetadataConfigurationOptions *)soap_malloc(soap,
				sizeof(struct tt__MetadataConfigurationOptions));
		p_response->Options->PTZStatusFilterOptions =
			(struct tt__PTZStatusFilterOptions *)soap_malloc(soap,
				sizeof(struct tt__PTZStatusFilterOptions));
		p_response->Options->PTZStatusFilterOptions->PanTiltStatusSupported = 0;
		p_response->Options->PTZStatusFilterOptions->ZoomStatusSupported = 0;

		p_response->Options->PTZStatusFilterOptions->PanTiltPositionSupported = (char *)soap_malloc(soap, sizeof(int));
		p_response->Options->PTZStatusFilterOptions->PanTiltPositionSupported[0] = 0;
		p_response->Options->PTZStatusFilterOptions->ZoomPositionSupported = (char *)soap_malloc(soap, sizeof(int));
		p_response->Options->PTZStatusFilterOptions->ZoomPositionSupported[0] = 0;
		p_response->Options->PTZStatusFilterOptions->Extension =
			(struct tt__PTZStatusFilterOptionsExtension *)soap_malloc(soap,
				sizeof(struct tt__PTZStatusFilterOptionsExtension));
		p_response->Options->PTZStatusFilterOptions->Extension = NULL;
	}

	return SOAP_OK;
}




SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfigurationOptions(struct soap *soap,
	struct _trt__GetAudioOutputConfigurationOptions *trt__GetAudioOutputConfigurationOptions,
	struct _trt__GetAudioOutputConfigurationOptionsResponse *trt__GetAudioOutputConfigurationOptionsResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfigurationOptions(struct soap *soap,
	struct _trt__GetAudioDecoderConfigurationOptions *trt__GetAudioDecoderConfigurationOptions,
	struct _trt__GetAudioDecoderConfigurationOptionsResponse *trt__GetAudioDecoderConfigurationOptionsResponse)
{
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetGuaranteedNumberOfVideoEncoderInstances(struct soap *soap,
	struct _trt__GetGuaranteedNumberOfVideoEncoderInstances *trt__GetGuaranteedNumberOfVideoEncoderInstances,
	struct _trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse *trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse)
{
	struct _trt__GetGuaranteedNumberOfVideoEncoderInstances *p_request =
		trt__GetGuaranteedNumberOfVideoEncoderInstances;
	int j = 0;
	int ret = 0;
	int is_vsc_exist = 0;
	int vsc_count = 0;
	media_config_t *rs_configs = NULL;

	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap, 0,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	soap->header->wsse__Security = NULL;

	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	vsc_count = 0;
	while (vsc_count < MAX_MEDIA_VSC_CONFIG &&
			rs_configs->vsc[vsc_count].name[0] != '\0')
		vsc_count++;

	if (p_request->ConfigurationToken) {
		for (j = 0; j < vsc_count; j++) {
			if (!strcmp(p_request->ConfigurationToken, rs_configs->vsc[j].token)) {
				is_vsc_exist = 1;
				break;
			}
		}
	}


	if (!is_vsc_exist) {
		onvif_fault(soap, 0, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->TotalNumber = (int *) soap_malloc(soap, sizeof(int));
	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->TotalNumber = 3;
	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->JPEG = (int *)soap_malloc(soap, sizeof(int));
	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->JPEG[0] = 0;
	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->H264 = (int *)soap_malloc(soap, sizeof(int));
	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->H264[0] = 3;

	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->MPEG4 = NULL;

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetStreamUri(struct soap *soap,
	struct _trt__GetStreamUri *trt__GetStreamUri,
	struct _trt__GetStreamUriResponse *trt__GetStreamUriResponse)
{
	int ret = 0, i;
	int j = 0;
	int profile_count;
	char *in_profile_token = NULL;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	network_config_t *network;
	network = (network_config_t *)soap_malloc(soap,
			sizeof(network_config_t));
	ASSERT(network);
	ret = rsOnvifSysInfoGetNetConfig(network);

	char *uri;
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

	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	rs_configs = (media_config_t *)soap_malloc(soap,
			sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;


	in_profile_token = trt__GetStreamUri->ProfileToken;
	for (i = 0; i < profile_count; i++) {
		if (in_profile_token && rs_profiles[i].token &&
			strcmp(in_profile_token, rs_profiles[i].token) != 0) {
			continue;
		} else {
			break;
		}
	}


	if (i == profile_count) {
		RS_DBG("get stream uri 2 failed\n");
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	} else {
		RS_DBG("get stream uri 3 ok\n");
		uri = (char *)soap_malloc(soap, 200);
		ASSERT(uri);
		if (rs_profiles[i].vec_index) {
			j = rs_profiles[i].vec_index - 1;
			char tmp_buf[128] = {0};
			parse_peacock_profile_name(tmp_buf, rs_configs->vec[j].vec_path);
			if (tmp_buf[0] == '\0') {
				onvif_fault(soap,
					RECEIVER,
					"ter:InvalidArgVal",
					"ter:CannotParsePeacockProfile");
				return SOAP_FAULT;
			}
			if (trt__GetStreamUri->StreamSetup->Transport->Protocol ==
					tt__TransportProtocol__HTTP) {
				sprintf(uri,
					"rtsp://%s/cgi-bin/%s.cgi",
					inet_ntoa(network->interfaces[network->interface_idx].ip),
					tmp_buf);
			} else {
				sprintf(uri,
					"rtsp://%s:%d/%s",
					inet_ntoa(network->interfaces[network->interface_idx].ip),
					rs_configs->vec[j].multicast_port, tmp_buf);
			}

			RS_DBG("%s\n", uri);
		} else if (rs_profiles[i].aec_index) {
			RS_DBG("get stream uri 5 ok\n");
			j = rs_profiles[i].aec_index - 1;
			if (trt__GetStreamUri->StreamSetup->Transport->Protocol ==
					tt__TransportProtocol__HTTP) {
				sprintf(uri,
					"rtsp://%s/cgi-bin/%s.cgi",
					inet_ntoa(network->interfaces[network->interface_idx].ip),
					"profile1");
			} else {
				sprintf(uri,
					"rtsp://%s:%d/%s",
					inet_ntoa(network->interfaces[network->interface_idx].ip),
					rs_configs->vec[j].multicast_port,
					"profile1");
			}
		} else {
			onvif_fault(soap,
				RECEIVER,
				"ter:Action",
				"ter:IncompleteConfiguration");
			return SOAP_FAULT;
		}
	}
	RS_DBG("get stream uri 6\n");


	if (trt__GetStreamUri->StreamSetup != NULL) {
		if (1 == trt__GetStreamUri->StreamSetup->Stream) {
			onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:InvalidStreamSetup");
			return SOAP_FAULT;
		}
	}

	/*Response*/
	trt__GetStreamUriResponse->MediaUri =
		(struct tt__MediaUri *)soap_malloc(soap,
				sizeof(struct tt__MediaUri));
	trt__GetStreamUriResponse->MediaUri->Uri = uri;
	trt__GetStreamUriResponse->MediaUri->InvalidAfterConnect = xsd__boolean__false_;
	trt__GetStreamUriResponse->MediaUri->InvalidAfterReboot  = xsd__boolean__false_;
	trt__GetStreamUriResponse->MediaUri->Timeout = 200;
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__StartMulticastStreaming(struct soap *soap,
	struct _trt__StartMulticastStreaming *trt__StartMulticastStreaming,
	struct _trt__StartMulticastStreamingResponse *trt__StartMulticastStreamingResponse)
{
	struct _trt__StartMulticastStreaming *p_request = trt__StartMulticastStreaming;
	int is_profile_exist = 0;
	int i = 0;

	int ret = 0;
	media_profile_t *rs_profiles;
	media_config_t *rs_configs = NULL;
	int profile_count = 0;

	soap->header->wsse__Security = NULL;

	rs_profiles = (media_profile_t *)soap_malloc(soap, sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	if (p_request->ProfileToken) {
		for (i = 0; i < profile_count; i++) {
			if (!strcmp(p_request->ProfileToken, rs_profiles[i].token)) {
				is_profile_exist = 1;
				break;
			}
		}
	}

	if (is_profile_exist) {
		return SOAP_OK;
	} else {
		onvif_fault(soap, SENDER,  "ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__StopMulticastStreaming(struct soap *soap,
	struct _trt__StopMulticastStreaming *trt__StopMulticastStreaming,
	struct _trt__StopMulticastStreamingResponse *trt__StopMulticastStreamingResponse)
{
	soap->header->wsse__Security = NULL;
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__SetSynchronizationPoint(struct soap *soap,
	struct _trt__SetSynchronizationPoint *trt__SetSynchronizationPoint,
	struct _trt__SetSynchronizationPointResponse *trt__SetSynchronizationPointResponse)
{
	/* insert I frame*/
	soap->header->wsse__Security = NULL;
	/*generate IDR for all profiles, will change later yafei*/
	rsOnvifMsgGenerateIntraFrame();
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __trt__GetSnapshotUri(struct soap *soap,
	struct _trt__GetSnapshotUri *trt__GetSnapshotUri,
	struct _trt__GetSnapshotUriResponse *trt__GetSnapshotUriResponse)
{
	int ret = 0, i;
	int profile_count;
	char *in_profile_token = NULL;
	media_profile_t *rs_profiles;
	network_config_t *network;
	int width = 0;
	int height = 0;
	network = (network_config_t *)soap_malloc(soap, sizeof(network_config_t));
	ASSERT(network);
	ret = rsOnvifSysInfoGetNetConfig(network);

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

	rs_profiles = (media_profile_t *)soap_malloc(soap,
			sizeof(media_profile_t) * MAX_MEDIA_PROFILE);
	ret = rsOnvifSysInfoGetMediaProfiles(rs_profiles);
	ASSERT(!ret);

	media_config_t *rs_configs = NULL;
	rs_configs = (media_config_t *)soap_malloc(soap, sizeof(media_config_t));
	ret = rsOnvifSysInfoGetMediaConfigs(rs_configs);
	ASSERT(!ret);

	profile_count = 0;
	while (profile_count < MAX_MEDIA_PROFILE &&
			rs_profiles[profile_count].name[0] != '\0')
		profile_count++;

	in_profile_token = trt__GetSnapshotUri->ProfileToken;
	for (i = 0; i < profile_count; i++) {
		if (in_profile_token && rs_profiles[i].token &&
			strcmp(in_profile_token, rs_profiles[i].token) != 0) {
			continue;
		} else {
			break;
		}
	}


	if (i == profile_count) {

		RS_DBG("get snapshot uri failed\n");
		onvif_fault(soap,
			SENDER,
			"ter:InvalidArgVal",
			"ter:NoProfile");
		return SOAP_FAULT;
	} else if (rs_profiles[i].vec_index == 0 ||
			rs_profiles[i].vsc_index == 0) {

		onvif_fault(soap,
			RECEIVER,
			"ter:Action",
			"ter:InCompleteConfiguration");
		return SOAP_FAULT;
	} else {
		char *snapshot_uri = (char *)soap_malloc(soap, 256);

		sprintf(snapshot_uri,
			"http://%s/cgi-bin/snapshot.cgi",
			inet_ntoa(network->interfaces[network->interface_idx].ip));

		trt__GetSnapshotUriResponse->MediaUri =
			(struct tt__MediaUri *)soap_malloc(soap,
					sizeof(struct tt__MediaUri));
		trt__GetSnapshotUriResponse->MediaUri->Uri = snapshot_uri;
		trt__GetSnapshotUriResponse->MediaUri->InvalidAfterConnect = 0;
		trt__GetSnapshotUriResponse->MediaUri->InvalidAfterReboot = 0;
		trt__GetSnapshotUriResponse->MediaUri->Timeout = 720000;
	}

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSDs(struct soap *soap,
	struct _trt__GetOSDs *trt__GetOSDs,
	struct _trt__GetOSDsResponse *trt__GetOSDsResponse)
{
	int i = 0;
	int osd_items = 0, ret = 0;
	osd_configuration_t *rs_osds = NULL;
	/*
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	*/
	soap->header->wsse__Security = NULL;
	rs_osds = (osd_configuration_t *)soap_malloc(soap, sizeof(osd_configuration_t) * MAX_OSD_NUM);
	ret = rsOnvifSysInfoGetOSDConfigs(rs_osds);
	ASSERT(!ret);

	osd_items = 0;
	while (osd_items < MAX_OSD_NUM &&
			rs_osds[osd_items].token[0] != '\0')
		osd_items++;

	/*chekc video source configuration token*/
	if (trt__GetOSDs->ConfigurationToken == NULL) {
	}
	if (0 == osd_items) {
		onvif_fault(soap, RECEIVER, "ter:NoConfigFile", "NO OSD Token files Setted.");
		return SOAP_FAULT;
	}

	trt__GetOSDsResponse->__sizeOSDs = osd_items;
	struct tt__OSDConfiguration *OSDs =
		(struct tt__OSDConfiguration *)soap_malloc(soap,
			osd_items * sizeof(struct tt__OSDConfiguration));
	memset(OSDs, 0, osd_items * sizeof(struct tt__OSDConfiguration));
	trt__GetOSDsResponse->OSDs = OSDs;

	for (i = 0; i < osd_items; i++) {
		OSDs[i].token = (char *)soap_malloc(soap, sizeof(char) * OSD_TOKEN_LENGTH);
		strcpy(OSDs[i].token, rs_osds[i].token);
		OSDs[i].Type = rs_osds[i].osd_type;/*tt__OSDType__Text = 0, tt__OSDType__Image = 1, tt__OSDType__Extended = 2*/

		OSDs[i].Position = (struct tt__OSDPosConfiguration *)soap_malloc(soap, sizeof(struct tt__OSDPosConfiguration));
		OSDs[i].Position->Type = (char *)soap_malloc(soap, sizeof(char) * 10);
		strcpy(OSDs[i].Position->Type, "Custom");
		OSDs[i].Position->Pos = (struct tt__Vector *)soap_malloc(soap, sizeof(struct tt__Vector));
		OSDs[i].Position->Pos->x = (float *)soap_malloc(soap, 2 * sizeof(float));
		OSDs[i].Position->Pos->y = (float *)soap_malloc(soap, 2 * sizeof(float));
		OSDs[i].Position->Pos->x[0] = rs_osds[i].osd_start_x;
		OSDs[i].Position->Pos->x[1] = rs_osds[i].osd_end_x;
		OSDs[i].Position->Pos->y[0] = rs_osds[i].osd_start_y;
		OSDs[i].Position->Pos->y[1] = rs_osds[i].osd_end_y;

		OSDs[i].TextString = (struct tt__OSDTextConfiguration *)soap_malloc(soap, sizeof(struct tt__OSDTextConfiguration));
		OSDs[i].TextString->Type = (char *)soap_malloc(soap , 10 * sizeof(char));
		strcpy(OSDs[i].TextString->Type, "Arial");
		OSDs[i].TextString->DateFormat = (char *)soap_malloc(soap , 10 * sizeof(char));
		OSDs[i].TextString->TimeFormat = (char *)soap_malloc(soap , 10 * sizeof(char));
		strcpy(OSDs[i].TextString->DateFormat, "YYYYMMDD");
		strcpy(OSDs[i].TextString->TimeFormat, "HHMMSS");
		OSDs[i].TextString->FontSize = (int *)soap_malloc(soap, sizeof(int));
		OSDs[i].TextString->FontSize[0] = rs_osds[i].osd_font_size;
		OSDs[i].TextString->PlainText = (char *)soap_malloc(soap, sizeof(char) * OSD_STRING_LENGTH);
		strcpy(OSDs[i].TextString->PlainText, rs_osds[i].osd_string_buf);


		OSDs[i].TextString->FontColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
		OSDs[i].TextString->FontColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
		OSDs[i].TextString->FontColor->Transparent[0] = rs_osds[i].osd_transparent;
		OSDs[i].TextString->FontColor->Color = (struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
		OSDs[i].TextString->FontColor->Color->X = rs_osds[i].osd_color_x;
		OSDs[i].TextString->FontColor->Color->Y = rs_osds[i].osd_color_y;
		OSDs[i].TextString->FontColor->Color->Z = rs_osds[i].osd_color_z;
		OSDs[i].TextString->FontColor->Color->Colorspace = (char *)soap_malloc(soap , 10 * sizeof(char));
		if (YCbCr == rs_osds[i].osd_color_type)
			strcpy(OSDs[i].TextString->FontColor->Color->Colorspace, "YCbCr");
		else if (CIELuv == rs_osds[i].osd_color_type)
			strcpy(OSDs[i].TextString->FontColor->Color->Colorspace, "CIELuv");
		else if (CIELab == rs_osds[i].osd_color_type)
			strcpy(OSDs[i].TextString->FontColor->Color->Colorspace, "CIELab");
		else if (HSV == rs_osds[i].osd_color_type)
			strcpy(OSDs[i].TextString->FontColor->Color->Colorspace, "HSV");

		OSDs[i].TextString->BackgroundColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
		OSDs[i].TextString->BackgroundColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
		OSDs[i].TextString->BackgroundColor->Transparent[0] = rs_osds[i].osd_bk_transparent;
		OSDs[i].TextString->BackgroundColor->Color = (struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
		OSDs[i].TextString->BackgroundColor->Color->X = rs_osds[i].osd_bk_color_x;
		OSDs[i].TextString->BackgroundColor->Color->Y = rs_osds[i].osd_bk_color_y;
		OSDs[i].TextString->BackgroundColor->Color->Z = rs_osds[i].osd_bk_color_z;
		OSDs[i].TextString->BackgroundColor->Color->Colorspace = (char *)soap_malloc(soap , 10 * sizeof(char));
		if (YCbCr == rs_osds[i].osd_bk_color_type)
			strcpy(OSDs[i].TextString->BackgroundColor->Color->Colorspace, "YCbCr");
		else if (CIELuv == rs_osds[i].osd_bk_color_type)
			strcpy(OSDs[i].TextString->BackgroundColor->Color->Colorspace, "CIELuv");
		else if (CIELab == rs_osds[i].osd_bk_color_type)
			strcpy(OSDs[i].TextString->BackgroundColor->Color->Colorspace, "CIELab");
		else if (HSV == rs_osds[i].osd_bk_color_type)
			strcpy(OSDs[i].TextString->BackgroundColor->Color->Colorspace, "HSV");

	}


	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSD(struct soap *soap,
	struct _trt__GetOSD *trt__GetOSD,
	struct _trt__GetOSDResponse *trt__GetOSDResponse)
{

	uint8 i = 0;
	uint8 is_token_exist = 0;
	uint8 osd_items = 0;
	int ret = 0;
	osd_configuration_t *rs_osds = NULL;
	/*
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	*/
	soap->header->wsse__Security = NULL;
	rs_osds = (osd_configuration_t *)soap_malloc(soap, sizeof(osd_configuration_t) * MAX_OSD_NUM);
	ret = rsOnvifSysInfoGetOSDConfigs(rs_osds);
	ASSERT(!ret);

	osd_items = 0;
	while (osd_items < MAX_OSD_NUM &&
			rs_osds[osd_items].token[0] != '\0')
		osd_items++;

	for (i = 0; i < osd_items; i++) {
		if (!strcmp(trt__GetOSD->OSDToken, rs_osds[i].token)) {
			is_token_exist = 1;
			break;
		}
	}
	/*if token does not exist*/
	if (!is_token_exist) {
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}

	trt__GetOSDResponse->__size = 0;
	trt__GetOSDResponse->__any = NULL;

	trt__GetOSDResponse->OSD = (struct tt__OSDConfiguration *)soap_malloc(soap, sizeof(struct tt__OSDConfiguration));
	memset(trt__GetOSDResponse->OSD, 0, sizeof(struct tt__OSDConfiguration));
	trt__GetOSDResponse->OSD->token = (char *)soap_malloc(soap, sizeof(char) * OSD_TOKEN_LENGTH);
	strcpy(trt__GetOSDResponse->OSD->token, rs_osds[i].token);
	trt__GetOSDResponse->OSD->VideoSourceConfigurationToken = NULL;/*used for what  ?yafei problem here.*/
	trt__GetOSDResponse->OSD->Type = 0;/*only support osd text.*/

	trt__GetOSDResponse->OSD->Position = (struct tt__OSDPosConfiguration *)soap_malloc(soap, sizeof(struct tt__OSDPosConfiguration));
	memset(trt__GetOSDResponse->OSD->Position, 0, sizeof(struct tt__OSDPosConfiguration));
	trt__GetOSDResponse->OSD->Position->Type = (char *)soap_malloc(soap, sizeof(char) * 10);
	strcpy(trt__GetOSDResponse->OSD->Position->Type, "Custom");
	trt__GetOSDResponse->OSD->Position->Pos = (struct tt__Vector *)soap_malloc(soap, sizeof(struct tt__Vector));
	trt__GetOSDResponse->OSD->Position->Pos->x = (float *)soap_malloc(soap, 2 * sizeof(float));
	trt__GetOSDResponse->OSD->Position->Pos->y = (float *)soap_malloc(soap, 2 * sizeof(float));
	trt__GetOSDResponse->OSD->Position->Pos->x[0] = rs_osds[i].osd_start_x;
	trt__GetOSDResponse->OSD->Position->Pos->x[1] = rs_osds[i].osd_end_x;
	trt__GetOSDResponse->OSD->Position->Pos->y[0] = rs_osds[i].osd_start_y;
	trt__GetOSDResponse->OSD->Position->Pos->y[1] = rs_osds[i].osd_end_y;


	trt__GetOSDResponse->OSD->TextString = (struct tt__OSDTextConfiguration *)soap_malloc(soap, sizeof(struct tt__OSDTextConfiguration));
	memset(trt__GetOSDResponse->OSD->TextString, 0, sizeof(struct tt__OSDTextConfiguration));

	trt__GetOSDResponse->OSD->TextString->Type = (char *)soap_malloc(soap , 10 * sizeof(char));
	strcpy(trt__GetOSDResponse->OSD->TextString->Type, "Arial");
	trt__GetOSDResponse->OSD->TextString->DateFormat = (char *)soap_malloc(soap , 10 * sizeof(char));
	trt__GetOSDResponse->OSD->TextString->TimeFormat = (char *)soap_malloc(soap , 10 * sizeof(char));
	strcpy(trt__GetOSDResponse->OSD->TextString->DateFormat, "YYYYMMDD");
	strcpy(trt__GetOSDResponse->OSD->TextString->TimeFormat, "HHMMSS");
	trt__GetOSDResponse->OSD->TextString->FontSize = (int *)soap_malloc(soap, sizeof(int));
	trt__GetOSDResponse->OSD->TextString->FontSize[0] = rs_osds[i].osd_font_size;
	trt__GetOSDResponse->OSD->TextString->PlainText = (char *)soap_malloc(soap, sizeof(char) * OSD_STRING_LENGTH);
	strcpy(trt__GetOSDResponse->OSD->TextString->PlainText, rs_osds[i].osd_string_buf);


	trt__GetOSDResponse->OSD->TextString->FontColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
	trt__GetOSDResponse->OSD->TextString->FontColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
	trt__GetOSDResponse->OSD->TextString->FontColor->Transparent[0] = rs_osds[i].osd_transparent;
	trt__GetOSDResponse->OSD->TextString->FontColor->Color = (struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
	trt__GetOSDResponse->OSD->TextString->FontColor->Color->X = rs_osds[i].osd_color_x;
	trt__GetOSDResponse->OSD->TextString->FontColor->Color->Y = rs_osds[i].osd_color_y;
	trt__GetOSDResponse->OSD->TextString->FontColor->Color->Z = rs_osds[i].osd_color_z;
	trt__GetOSDResponse->OSD->TextString->FontColor->Color->Colorspace = (char *)soap_malloc(soap , 10 * sizeof(char));
	if (YCbCr == rs_osds[i].osd_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->FontColor->Color->Colorspace, "YCbCr");
	else if (CIELuv == rs_osds[i].osd_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->FontColor->Color->Colorspace, "CIELuv");
	else if (CIELab == rs_osds[i].osd_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->FontColor->Color->Colorspace, "CIELab");
	else if (HSV == rs_osds[i].osd_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->FontColor->Color->Colorspace, "HSV");

	trt__GetOSDResponse->OSD->TextString->BackgroundColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
	trt__GetOSDResponse->OSD->TextString->BackgroundColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
	trt__GetOSDResponse->OSD->TextString->BackgroundColor->Transparent[0] = rs_osds[i].osd_bk_transparent;
	trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color = (struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
	trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->X = rs_osds[i].osd_bk_color_x;
	trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->Y = rs_osds[i].osd_bk_color_y;
	trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->Z = rs_osds[i].osd_bk_color_z;
	trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->Colorspace = (char *)soap_malloc(soap , 10 * sizeof(char));
	if (YCbCr == rs_osds[i].osd_bk_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->Colorspace, "YCbCr");
	else if (CIELuv == rs_osds[i].osd_bk_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->Colorspace, "CIELuv");
	else if (CIELab == rs_osds[i].osd_bk_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->Colorspace, "CIELab");
	else if (HSV == rs_osds[i].osd_bk_color_type)
		strcpy(trt__GetOSDResponse->OSD->TextString->BackgroundColor->Color->Colorspace, "HSV");

	return SOAP_OK;
}

/*listing of available OSD Parameter options for a given video source configuration*/
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSDOptions(struct soap *soap,
	struct _trt__GetOSDOptions *trt__GetOSDOptions,
	struct _trt__GetOSDOptionsResponse *trt__GetOSDOptionsResponse)
{
	int i = 0;
	int osd_items = 0, ret = 0;
	osd_configuration_t *rs_osds = NULL;
	/*
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	*/
	soap->header->wsse__Security = NULL;
	rs_osds = (osd_configuration_t *)soap_malloc(soap, sizeof(osd_configuration_t) * MAX_OSD_NUM);
	ret = rsOnvifSysInfoGetOSDConfigs(rs_osds);
	ASSERT(!ret);

	osd_items = 0;
	while (osd_items < MAX_OSD_NUM &&
			rs_osds[osd_items].token[0] != '\0')
		osd_items++;

	/*chekc video source configuration token*/
	if (trt__GetOSDOptions->ConfigurationToken == NULL) {
	}

	if (0 == osd_items) {
		onvif_fault(soap, RECEIVER, "ter:NoConfigFile", "NO OSD Token files Setted.");
		return SOAP_FAULT;
	}
	trt__GetOSDOptionsResponse->__size = osd_items;
	trt__GetOSDOptionsResponse->__any = NULL;
	struct tt__OSDConfigurationOptions *osd_options =
		(struct tt__OSDConfigurationOptions *)soap_malloc(soap,
			osd_items * sizeof(struct tt__OSDConfigurationOptions));
	memset(osd_options, 0, osd_items * sizeof(struct tt__OSDConfigurationOptions));
	trt__GetOSDOptionsResponse->OSDOptions = osd_options;

	for (i = 0; i < osd_items; i++) {
		osd_options[i].__sizePositionOption = 5;
		osd_options[i].PositionOption = (char **)soap_malloc(soap, 5 * sizeof(char *));
		osd_options[i].PositionOption[0] = (char *)soap_malloc(soap , 10 * sizeof(char));
		osd_options[i].PositionOption[1] = (char *)soap_malloc(soap , 10 * sizeof(char));
		osd_options[i].PositionOption[2] = (char *)soap_malloc(soap , 10 * sizeof(char));
		osd_options[i].PositionOption[3] = (char *)soap_malloc(soap , 10 * sizeof(char));
		osd_options[i].PositionOption[4] = (char *)soap_malloc(soap , 10 * sizeof(char));
		strcpy(osd_options[i].PositionOption[0], "Custom");

		sprintf(osd_options[i].PositionOption[1], "x0=%d", rs_osds[i].osd_start_x);
		sprintf(osd_options[i].PositionOption[2], "x1=%d", rs_osds[i].osd_end_x);
		sprintf(osd_options[i].PositionOption[3], "y0=%d", rs_osds[i].osd_start_y);
		sprintf(osd_options[i].PositionOption[4], "y1=%d", rs_osds[i].osd_end_y);
		osd_options[i].MaximumNumberOfOSDs = (struct tt__MaximumNumberOfOSDs *)soap_malloc(soap, sizeof(struct tt__MaximumNumberOfOSDs));
		osd_options[i].MaximumNumberOfOSDs->Total = 1;
		osd_options[i].MaximumNumberOfOSDs->PlainText = (int *)soap_malloc(soap, sizeof(int));
		osd_options[i].MaximumNumberOfOSDs->PlainText[0] = OSD_STRING_LENGTH;
		osd_options[i].__sizeType = 1;
		osd_options[i].Type = (enum tt__OSDType *)soap_malloc(soap, sizeof(enum tt__OSDType));
		osd_options[i].Type[0] = 0;/*text*/


		osd_options[i].TextOption = (struct tt__OSDTextOptions *)soap_malloc(soap, sizeof(struct tt__OSDTextOptions));
		memset(osd_options[i].TextOption, 0, sizeof(struct tt__OSDTextOptions));
		osd_options[i].TextOption->__sizeType = 1;
		osd_options[i].TextOption->Type = (char **)soap_malloc(soap, sizeof(char *));
		osd_options[i].TextOption->Type[0] = (char *)soap_malloc(soap, sizeof(char) * 10);
		strcpy(osd_options[i].TextOption->Type[0], "Arial");

		osd_options[i].TextOption->FontSizeRange = (struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		osd_options[i].TextOption->FontSizeRange->Min = 8;
		osd_options[i].TextOption->FontSizeRange->Max = 58;

		osd_options[i].TextOption->__sizeDateFormat = 1;
		osd_options[i].TextOption->DateFormat = (char **)soap_malloc(soap, sizeof(char *));
		osd_options[i].TextOption->DateFormat[0] = (char *)soap_malloc(soap, sizeof(char) * 10);
		strcpy(osd_options[i].TextOption->DateFormat[0], "YYYYMMDD");

		osd_options[i].TextOption->__sizeTimeFormat = 1;
		osd_options[i].TextOption->TimeFormat = (char **)soap_malloc(soap, sizeof(char *));
		osd_options[i].TextOption->TimeFormat[0] = (char *)soap_malloc(soap, sizeof(char) * 10);
		strcpy(osd_options[i].TextOption->TimeFormat[0], "HHMMSS");

		osd_options[i].TextOption->FontColor = (struct tt__OSDColorOptions *)soap_malloc(soap, sizeof(struct tt__OSDColorOptions));
		osd_options[i].TextOption->FontColor->Color = (struct tt__ColorOptions *)soap_malloc(soap, sizeof(struct tt__ColorOptions));
		osd_options[i].TextOption->FontColor->Color->__sizeColorList = 1;
		osd_options[i].TextOption->FontColor->Color->ColorList = (struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
		osd_options[i].TextOption->FontColor->Color->ColorList->X = rs_osds[i].osd_color_x;
		osd_options[i].TextOption->FontColor->Color->ColorList->Y = rs_osds[i].osd_color_y;
		osd_options[i].TextOption->FontColor->Color->ColorList->Z = rs_osds[i].osd_color_z;
		osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace = (char *)soap_malloc(soap, sizeof(char) * 10);
		if (YCbCr == rs_osds[i].osd_color_type)
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "YCbCr");
		else if (CIELuv == rs_osds[i].osd_color_type)
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "CIELuv");
		else if (CIELab == rs_osds[i].osd_color_type)
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "CIELab");
		else if (HSV == rs_osds[i].osd_color_type)
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "HSV");

		osd_options[i].TextOption->FontColor->Color->__sizeColorspaceRange = 1;
		osd_options[i].TextOption->FontColor->Color->ColorspaceRange =
			(struct tt__ColorspaceRange *)soap_malloc(soap, sizeof(struct tt__ColorspaceRange));
		osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X =
			(struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
		osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y =
			(struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
		osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z =
			(struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
		osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Colorspace =
			(char *)soap_malloc(soap, sizeof(char) * 10);
		if (YCbCr == rs_osds[i].osd_color_type) {
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "YCbCr");
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Min = 0.;
		} else if (CIELuv == rs_osds[i].osd_color_type) {
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "CIELuv");
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Min = 0.;
		} else if (CIELab == rs_osds[i].osd_color_type) {
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "CIELab");
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Min = 0.;
		} else if (HSV == rs_osds[i].osd_color_type) {
			strcpy(osd_options[i].TextOption->FontColor->Color->ColorList->Colorspace, "HSV");
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->FontColor->Color->ColorspaceRange->Z->Min = 0.;
		}
		osd_options[i].TextOption->FontColor->Color->ColorspaceRange->__anyAttribute = NULL;
		osd_options[i].TextOption->FontColor->Color->__anyAttribute = NULL;
		osd_options[i].TextOption->FontColor->Transparent =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		osd_options[i].TextOption->FontColor->Transparent->Max = 16;
		osd_options[i].TextOption->FontColor->Transparent->Min = 0;
		osd_options[i].TextOption->FontColor->Extension = NULL;
		osd_options[i].TextOption->FontColor->__anyAttribute = NULL;

		/*bk ground color*/
		osd_options[i].TextOption->BackgroundColor =
			(struct tt__OSDColorOptions *)soap_malloc(soap, sizeof(struct tt__OSDColorOptions));
		osd_options[i].TextOption->BackgroundColor->Color =
			(struct tt__ColorOptions *)soap_malloc(soap, sizeof(struct tt__ColorOptions));
		osd_options[i].TextOption->BackgroundColor->Color->__sizeColorList = 1;
		osd_options[i].TextOption->BackgroundColor->Color->ColorList =
			(struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
		osd_options[i].TextOption->BackgroundColor->Color->ColorList->X = rs_osds[i].osd_bk_color_x;
		osd_options[i].TextOption->BackgroundColor->Color->ColorList->Y = rs_osds[i].osd_bk_color_y;
		osd_options[i].TextOption->BackgroundColor->Color->ColorList->Z = rs_osds[i].osd_bk_color_z;
		osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace =
			(char *)soap_malloc(soap, sizeof(char) * 10);
		if (YCbCr == rs_osds[i].osd_bk_color_type)
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "YCbCr");
		else if (CIELuv == rs_osds[i].osd_bk_color_type)
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "CIELuv");
		else if (CIELab == rs_osds[i].osd_bk_color_type)
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "CIELab");
		else if (HSV == rs_osds[i].osd_bk_color_type)
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "HSV");

		osd_options[i].TextOption->BackgroundColor->Color->__sizeColorspaceRange = 1;
		osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange =
			(struct tt__ColorspaceRange *)soap_malloc(soap, sizeof(struct tt__ColorspaceRange));
		osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X =
			(struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
		osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y =
			(struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
		osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z =
			(struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
		osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Colorspace =
			(char *)soap_malloc(soap, sizeof(char) * 10);
		if (YCbCr == rs_osds[i].osd_bk_color_type) {
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "YCbCr");
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Min = 0.;
		} else if (CIELuv == rs_osds[i].osd_bk_color_type) {
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "CIELuv");
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Min = 0.;
		} else if (CIELab == rs_osds[i].osd_bk_color_type) {
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "CIELab");
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Min = 0.;
		} else if (HSV == rs_osds[i].osd_bk_color_type) {
			strcpy(osd_options[i].TextOption->BackgroundColor->Color->ColorList->Colorspace, "HSV");
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->X->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Y->Min = 0.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Max = 255.;
			osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->Z->Min = 0.;
		}
		osd_options[i].TextOption->BackgroundColor->Color->ColorspaceRange->__anyAttribute = NULL;
		osd_options[i].TextOption->BackgroundColor->Color->__anyAttribute = NULL;
		osd_options[i].TextOption->BackgroundColor->Transparent =
			(struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
		osd_options[i].TextOption->BackgroundColor->Transparent->Max = 16;
		osd_options[i].TextOption->BackgroundColor->Transparent->Min = 0;
		osd_options[i].TextOption->BackgroundColor->Extension = NULL;
		osd_options[i].TextOption->BackgroundColor->__anyAttribute = NULL;
		osd_options[i].TextOption->Extension = NULL;
		osd_options[i].TextOption->__anyAttribute = NULL;
		osd_options[i].ImageOption = NULL;
		osd_options[i].Extension = NULL;
		osd_options[i].__anyAttribute = NULL;

	}

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetOSD(struct soap *soap,
	struct _trt__SetOSD *trt__SetOSD,
	struct _trt__SetOSDResponse *trt__SetOSDResponse)
{
/*
		struct _trt__SetOSD
		{
			struct tt__OSDConfiguration * OSD;
			int __size;
			char *__any;
		};
*/
	uint8 i = 0;
	uint8 is_token_exist = 0;
	uint8 osd_items = 0;
	int ret = 0;
	osd_configuration_t *rs_osds = NULL;
	/*
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
		soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	*/
	soap->header->wsse__Security = NULL;
	rs_osds = (osd_configuration_t *)soap_malloc(soap, sizeof(osd_configuration_t) * MAX_OSD_NUM);
	ret = rsOnvifSysInfoGetOSDConfigs(rs_osds);
	ASSERT(!ret);

	osd_items = 0;
	while (osd_items < MAX_OSD_NUM &&
			rs_osds[osd_items].token[0] != '\0')
		osd_items++;

	for (i = 0; i < osd_items; i++) {
		if (!strcmp(trt__SetOSD->OSD->token, rs_osds[i].token)) {
			is_token_exist = 1;
			break;
		}
	}
	/*if token does not exist*/
	if (!is_token_exist) {
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}


	if (trt__SetOSD->OSD->Type != 0) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:OnlySupportTextOSD");
		return SOAP_FAULT;
	}
	rs_osds[i].osd_type = 0;/*text 0 image 1 others 2+*/

	if (!strcmp("Custom", trt__SetOSD->OSD->Position->Type) !=  0) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:OnlySupportCustomOSDPosition");
		return SOAP_FAULT;
	}

	rs_osds[i].osd_start_x = trt__SetOSD->OSD->Position->Pos->x[0];
	rs_osds[i].osd_end_x  = trt__SetOSD->OSD->Position->Pos->x[1];
	rs_osds[i].osd_start_y = trt__SetOSD->OSD->Position->Pos->y[0];
	rs_osds[i].osd_end_y  = trt__SetOSD->OSD->Position->Pos->y[1];

	if (trt__SetOSD->OSD->TextString->PlainText != NULL)
		strcpy(rs_osds[i].osd_string_buf, trt__SetOSD->OSD->TextString->PlainText);
	else
		memset(rs_osds[i].osd_string_buf, 0, sizeof(char) * OSD_STRING_LENGTH);

	rs_osds[i].osd_color_x = trt__SetOSD->OSD->TextString->FontColor->Color->X;
	rs_osds[i].osd_color_y = trt__SetOSD->OSD->TextString->FontColor->Color->Y;
	rs_osds[i].osd_color_z = trt__SetOSD->OSD->TextString->FontColor->Color->Z;
	if (!strcmp("YCbCr", trt__SetOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[i].osd_color_type = YCbCr;
	else if (!strcmp("CIELuv", trt__SetOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[i].osd_color_type = CIELuv;
	else if (!strcmp("CIELab", trt__SetOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[i].osd_color_type = CIELab;
	else if (!strcmp("HSV", trt__SetOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[i].osd_color_type = HSV;
	rs_osds[i].osd_transparent = *(trt__SetOSD->OSD->TextString->FontColor->Transparent);


	rs_osds[i].osd_bk_color_x = trt__SetOSD->OSD->TextString->BackgroundColor->Color->X;
	rs_osds[i].osd_bk_color_y = trt__SetOSD->OSD->TextString->BackgroundColor->Color->Y;
	rs_osds[i].osd_bk_color_z = trt__SetOSD->OSD->TextString->BackgroundColor->Color->Z;
	if (!strcmp("YCbCr", trt__SetOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[i].osd_bk_color_type = YCbCr;
	else if (!strcmp("CIELuv", trt__SetOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[i].osd_bk_color_type = CIELuv;
	else if (!strcmp("CIELab", trt__SetOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[i].osd_bk_color_type = CIELab;
	else if (!strcmp("HSV", trt__SetOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[i].osd_bk_color_type = HSV;
	rs_osds[i].osd_bk_transparent = *(trt__SetOSD->OSD->TextString->BackgroundColor->Transparent);

	rs_osds[i].osd_font_size = *(trt__SetOSD->OSD->TextString->FontSize);

	ret = rsOnvifSysInfoSetOSDConfigs(rs_osds);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__CreateOSD(struct soap *soap,
	struct _trt__CreateOSD *trt__CreateOSD,
	struct _trt__CreateOSDResponse *trt__CreateOSDResponse)
{
	uint8 i = 0;
	uint8 is_token_exist = 0;
	uint8 osd_items = 0;
	int ret = 0;
	osd_configuration_t *rs_osds = NULL;
	/*
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	*/
	soap->header->wsse__Security = NULL;
	rs_osds = (osd_configuration_t *)soap_malloc(soap, sizeof(osd_configuration_t) * MAX_OSD_NUM);
	ret = rsOnvifSysInfoGetOSDConfigs(rs_osds);
	ASSERT(!ret);

	osd_items = 0;
	while (osd_items < MAX_OSD_NUM &&
			rs_osds[osd_items].token[0] != '\0')
		osd_items++;

	if (osd_items == MAX_OSD_NUM) {
		onvif_fault(soap, RECEIVER, "ter:Action", "ter:MaxOSDs");
		return SOAP_FAULT;
	}

	for (i = 0; i < osd_items; i++) {
		if (!strcmp(trt__CreateOSD->OSD->token, rs_osds[i].token)) {
			is_token_exist = 1;
			break;
		}
	}
	/*if token  exist*/
	if (is_token_exist) {
		onvif_fault(soap, "ter:InvalidArgVal", "ter:TokenExisted");
		return SOAP_FAULT;
	}


	if (trt__CreateOSD->OSD->Type != 0) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:OnlySupportTextOSD");
		return SOAP_FAULT;
	}
	rs_osds[osd_items+1].osd_type = 0;/*text 0 image 1 others 2+*/

	if (!strcmp("Custom", trt__CreateOSD->OSD->Position->Type) != 0) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:OnlySupportCustomOSDPosition");
		return SOAP_FAULT;
	}

	rs_osds[osd_items+1].osd_start_x = trt__CreateOSD->OSD->Position->Pos->x[0];
	rs_osds[osd_items+1].osd_end_x  = trt__CreateOSD->OSD->Position->Pos->x[1];
	rs_osds[osd_items+1].osd_start_y = trt__CreateOSD->OSD->Position->Pos->y[0];
	rs_osds[osd_items+1].osd_end_y  = trt__CreateOSD->OSD->Position->Pos->y[1];

	if (trt__CreateOSD->OSD->TextString->PlainText != NULL)
		strcpy(rs_osds[osd_items+1].osd_string_buf, trt__CreateOSD->OSD->TextString->PlainText);
	else
		memset(rs_osds[osd_items+1].osd_string_buf, 0, sizeof(char) * OSD_STRING_LENGTH);

	rs_osds[osd_items+1].osd_color_x = trt__CreateOSD->OSD->TextString->FontColor->Color->X;
	rs_osds[osd_items+1].osd_color_y = trt__CreateOSD->OSD->TextString->FontColor->Color->Y;
	rs_osds[osd_items+1].osd_color_z = trt__CreateOSD->OSD->TextString->FontColor->Color->Z;
	if (!strcmp("YCbCr", trt__CreateOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_color_type = YCbCr;
	else if (!strcmp("CIELuv", trt__CreateOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_color_type = CIELuv;
	else if (!strcmp("CIELab", trt__CreateOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_color_type = CIELab;
	else if (!strcmp("HSV", trt__CreateOSD->OSD->TextString->FontColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_color_type = HSV;
	rs_osds[osd_items+1].osd_transparent = *(trt__CreateOSD->OSD->TextString->FontColor->Transparent);


	rs_osds[osd_items+1].osd_bk_color_x = trt__CreateOSD->OSD->TextString->BackgroundColor->Color->X;
	rs_osds[osd_items+1].osd_bk_color_y = trt__CreateOSD->OSD->TextString->BackgroundColor->Color->Y;
	rs_osds[osd_items+1].osd_bk_color_z = trt__CreateOSD->OSD->TextString->BackgroundColor->Color->Z;
	if (!strcmp("YCbCr", trt__CreateOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_bk_color_type = YCbCr;
	else if (!strcmp("CIELuv", trt__CreateOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_bk_color_type = CIELuv;
	else if (!strcmp("CIELab", trt__CreateOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_bk_color_type = CIELab;
	else if (!strcmp("HSV", trt__CreateOSD->OSD->TextString->BackgroundColor->Color->Colorspace))
		rs_osds[osd_items+1].osd_bk_color_type = HSV;
	rs_osds[osd_items+1].osd_bk_transparent = *(trt__CreateOSD->OSD->TextString->BackgroundColor->Transparent);

	rs_osds[osd_items+1].osd_font_size = *(trt__CreateOSD->OSD->TextString->FontSize);

	ret = rsOnvifSysInfoSetOSDConfigs(rs_osds);


	trt__CreateOSDResponse->OSDToken = (char *)soap_malloc(soap, sizeof(char) * OSD_TOKEN_LENGTH);
	strcpy(trt__CreateOSDResponse->OSDToken, rs_osds[osd_items+1].token);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__DeleteOSD(struct soap *soap,
	struct _trt__DeleteOSD *trt__DeleteOSD,
	struct _trt__DeleteOSDResponse *trt__DeleteOSDResponse)
{
	uint8 is_token_exist = 0;
	uint8 i = 0, index = 0;
	uint8 osd_items = 0;
	int ret = 0;
	osd_configuration_t *rs_osds = NULL;
	/*
	if (!rsOnvifUsrAuthentication(soap, tt__UserLevel__User)) {
		if (soap->header)
			soap->header->wsse__Security = NULL;
		onvif_not_authorized(soap,
			"ter:NotAuthorized",
			"The action requested requires authorization \
			and the sender is not authorized");
		return SOAP_FAULT;
	}
	*/
	soap->header->wsse__Security = NULL;
	rs_osds = (osd_configuration_t *)soap_malloc(soap, sizeof(osd_configuration_t) * MAX_OSD_NUM);
	ret = rsOnvifSysInfoGetOSDConfigs(rs_osds);
	ASSERT(!ret);

	osd_items = 0;
	while (osd_items < MAX_OSD_NUM &&
			rs_osds[osd_items].token[0] != '\0')
		osd_items++;

	for (i = 0; i < osd_items; i++) {
		if (!strcmp(trt__DeleteOSD->OSDToken, rs_osds[i].token)) {
			is_token_exist = 1;
			break;
		}
	}


	if (!is_token_exist) {
		onvif_fault(soap, "ter:InvalidArgVal", "ter:NoConfig");
		return SOAP_FAULT;
	}


	for (index = i; index < MAX_OSD_NUM - 1; index++) {
		memcpy(&rs_osds[index], &rs_osds[index + 1], sizeof(osd_configuration_t));
	}
	memset(&rs_osds[MAX_OSD_NUM-1], 0, sizeof(osd_configuration_t));

	ret = rsOnvifSysInfoSetOSDConfigs(rs_osds);
	ASSERT(!ret);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceModes(struct soap *soap,
	struct _trt__GetVideoSourceModes *trt__GetVideoSourceModes,
	struct _trt__GetVideoSourceModesResponse *trt__GetVideoSourceModesResponse)
{
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoSourceMode(struct soap *soap,
	struct _trt__SetVideoSourceMode *trt__SetVideoSourceMode,
	struct _trt__SetVideoSourceModeResponse *trt__SetVideoSourceModeResponse)
{
	return SOAP_OK;
}
