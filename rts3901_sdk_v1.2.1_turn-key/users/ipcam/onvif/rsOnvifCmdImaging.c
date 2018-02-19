#include <octopus/rts_cam.h>
#include "soapH.h"
#include "rsOnvifTypes.h"
#include "rsOnvifMsg.h"
#include "rsOnvifDevCtrl.h"

/*logs ctrl*/
#if 0
#define RS_DBG write_log
#else
#define RS_DBG
#endif

/******************************************************************************\
 **
 * Server-Side Operations *
 **
\******************************************************************************/

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetServiceCapabilities(struct soap *soap,
	struct _timg__GetServiceCapabilities *timg__GetServiceCapabilities,
	struct _timg__GetServiceCapabilitiesResponse *timg__GetServiceCapabilitiesResponse)
{
	soap->header->wsse__Security = NULL;

	timg__GetServiceCapabilitiesResponse->Capabilities =
		(struct timg__Capabilities *)soap_malloc(soap,
				sizeof(struct timg__Capabilities));

	timg__GetServiceCapabilitiesResponse->Capabilities->ImageStabilization =
		(enum xsd__boolean *)soap_malloc(soap,
				sizeof(enum xsd__boolean));
	timg__GetServiceCapabilitiesResponse->Capabilities->ImageStabilization[0] =
		xsd__boolean__false_;
	timg__GetServiceCapabilitiesResponse->Capabilities->__size = 0;
	timg__GetServiceCapabilitiesResponse->Capabilities->__any = NULL;
	timg__GetServiceCapabilitiesResponse->Capabilities->__anyAttribute = NULL;

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetImagingSettings(struct soap *soap,
	struct _timg__GetImagingSettings *timg__GetImagingSettings,
	struct _timg__GetImagingSettingsResponse *timg__GetImagingSettingsResponse)
{
	int i = 0, flag = 0;
	int j = 0;
	int ret = 0, fd = 0;
	int token_exist = 0;
	int imaging_exist = 0;
	media_config_t *rs_configs = NULL;
	int vsc_count = 0;
	int is_vstoken_exist = 0;

	struct _timg__GetImagingSettings *p_request = timg__GetImagingSettings;
	struct _timg__GetImagingSettingsResponse *p_response =
		timg__GetImagingSettingsResponse;

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

	vsc_count = 0;
	while (vsc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[vsc_count].source_token[0] != '\0')
		vsc_count++;

	for (j = 0; j < vsc_count; j++) {
		if (!strcmp(rs_configs->vsc[i].source_token,
					p_request->VideoSourceToken)) {
			is_vstoken_exist = 1;
			break;
		}
	}

	if (!is_vstoken_exist) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:NoSource");
		return SOAP_FAULT;
	}

	if (rs_configs->vec[0].vec_path[0] != '\0') {
		fd = rts_cam_open(rs_configs->vec[0].vec_path);
	} else {
		onvif_fault(soap,
				RECEIVER,
				"ter:NoCamDevice",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (fd < 0) {
		onvif_fault(soap,
				RECEIVER,
				"ter:NoCamDevice",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (rts_cam_lock(fd, 1000)) {
		rts_cam_close(fd);
		onvif_fault(soap,
				RECEIVER,
				"ter:LockCamDev",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	struct attr_video *p_img_settings = NULL;
	ret = rts_cam_get_video_attr(fd, &p_img_settings);

	if (ret) {
		rts_cam_unlock(fd);
		rts_cam_close(fd);
		onvif_fault(soap,
				RECEIVER,
				"ter:LockCamDev",
				"ter:GetImageSettingError");
		return SOAP_FAULT;
	}

/*
	for (i = 0; i < oSysInfo->nprofile; i++) {
		if (strcmp(timg__GetImagingSettings->VideoSourceToken,
		oSysInfo->Oprofile[i].AVSC.Vsourcetoken) == 0)
		{
			token_exist = EXIST;
			break;
		}
	}

	if (!token_exist) {
		onvif_fault(soap,"ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	for (i = 0; i < oSysInfo->nprofile; i++) {
		if (strcmp(timg__GetImagingSettings->VideoSourceToken,
		oSysInfo->imaging_conf[i].Isourcetoken) == 0) {
			imaging_exist = EXIST;
			break;
		}
	}
	if (!imaging_exist) {
		onvif_fault(soap, "ter:ActionNotSupported", "ter:NoImagingForSource");
		return SOAP_FAULT;
	}
*/

	p_response->ImagingSettings =
		(struct tt__ImagingSettings20 *)soap_malloc(soap,
			sizeof(struct tt__ImagingSettings20));

	if (p_img_settings->brightness.supported) {
		p_response->ImagingSettings->Brightness =
			(float *)soap_malloc(soap, sizeof(float));
		*p_response->ImagingSettings->Brightness =
			(float)p_img_settings->brightness.current;
	} else {
		p_response->ImagingSettings->Brightness = NULL;
	}

	if (p_img_settings->saturation.supported) {
		p_response->ImagingSettings->ColorSaturation =
			(float *)soap_malloc(soap, sizeof(float));
		*p_response->ImagingSettings->ColorSaturation =
			(float)p_img_settings->saturation.current;
	} else {
		p_response->ImagingSettings->ColorSaturation = NULL;
	}

	if (p_img_settings->contrast.supported) {
		p_response->ImagingSettings->Contrast =
			(float *)soap_malloc(soap, sizeof(float));
		*p_response->ImagingSettings->Contrast =
			(float)p_img_settings->contrast.current;
	} else {
		p_response->ImagingSettings->Contrast = NULL;
	}

	if (p_img_settings->sharpness.supported) {
		p_response->ImagingSettings->Sharpness =
			(float *)soap_malloc(soap, sizeof(float));
		*p_response->ImagingSettings->Sharpness =
			(float)p_img_settings->sharpness.current;
	} else {
		p_response->ImagingSettings->Sharpness = NULL;
	}

	p_response->ImagingSettings->BacklightCompensation = NULL;
	p_response->ImagingSettings->Exposure = NULL;
	p_response->ImagingSettings->Focus = NULL;
	p_response->ImagingSettings->WideDynamicRange = NULL;
	p_response->ImagingSettings->IrCutFilter = NULL;
	p_response->ImagingSettings->WhiteBalance = NULL;
	p_response->ImagingSettings->Extension = NULL;

	rts_cam_free_attr(p_img_settings);

	rts_cam_unlock(fd);
	rts_cam_close(fd);
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__SetImagingSettings(struct soap *soap,
	struct _timg__SetImagingSettings *timg__SetImagingSettings,
	struct _timg__SetImagingSettingsResponse *timg__SetImagingSettingsResponse)
{
	struct _timg__SetImagingSettings *p_request = timg__SetImagingSettings;
	int ret = 0, fd = 0;
	int vsc_count = 0;
	int i = 0;
	int j = 0;
	media_config_t *rs_configs = NULL;
	int is_vstoken_exist = 0;

/*
	for (i = 0; i < oSysInfo->nprofile; i++) {
		if (strcmp(timg__SetImagingSettings->VideoSourceToken, oSysInfo->Oprofile[i].AVSC.Vsourcetoken) == 0) {
			token_exist = EXIST;
			imaging_conf_tmp.position = i;
			break;
		}
	}

	if (!token_exist) {
		onvif_fault(soap,"ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}

	for (i = 0; i < oSysInfo->nprofile; i++) {
		if (strcmp(timg__SetImagingSettings->VideoSourceToken, oSysInfo->imaging_conf[i].Isourcetoken) == 0) {
			imaging_exist = EXIST;
			break;
		}
	}
	if (!imaging_exist) {
		onvif_fault(soap,"ter:ActionNotSupported", "ter:NoImagingForSource");
		return SOAP_FAULT;
	}
	strcpy(imaging_conf_tmp.imaging_conf_in.Isourcetoken_t, timg__SetImagingSettings->VideoSourceToken);

*/
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

	vsc_count = 0;
	while (vsc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[vsc_count].source_token[0] != '\0')
		vsc_count++;

	for (j = 0; j < vsc_count; j++) {
		if (!strcmp(rs_configs->vsc[i].source_token,
					p_request->VideoSourceToken)) {
			is_vstoken_exist = 1;
			break;
		}
	}

	if (!is_vstoken_exist) {
		onvif_fault(soap,
				SENDER,
				"ter:InvalidArgVal",
				"ter:NoSource");
		return SOAP_FAULT;
	}

	if (rs_configs->vec[0].vec_path[0] != '\0') {
		fd = rts_cam_open(rs_configs->vec[0].vec_path);
	} else {
		onvif_fault(soap,
				RECEIVER,
				"ter:NoCamDevice",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (fd < 0) {
		onvif_fault(soap,
				RECEIVER,
				"ter:NoCamDevice",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (rts_cam_lock(fd, 1000)) {
		rts_cam_close(fd);
		onvif_fault(soap,
				RECEIVER,
				"ter:LockCamDev",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}
	struct attr_video *p_img_settings = NULL;
	ret = rts_cam_get_video_attr(fd, &p_img_settings);

	if (ret) {
		rts_cam_unlock(fd);
		rts_cam_close(fd);
		onvif_fault(soap,
				RECEIVER,
				"ter:LockCamDev",
				"ter:GetImageSettingError");
		return SOAP_FAULT;
	}

	if (timg__SetImagingSettings->ImagingSettings->Brightness != NULL) {
		if (p_img_settings->brightness.supported) {
			if ((float)*timg__SetImagingSettings->ImagingSettings->Brightness >= p_img_settings->brightness.minimum &&
				(float)*timg__SetImagingSettings->ImagingSettings->Brightness <= p_img_settings->brightness.maximum) {
				p_img_settings->brightness.current =
					(float)*timg__SetImagingSettings->ImagingSettings->Brightness;
			} else {
				onvif_fault(soap,
						SENDER,
						"ter:InvalidArgVal",
						"ter:SettingsInvalid");
				return SOAP_FAULT;
			}
		}
	}

	if (timg__SetImagingSettings->ImagingSettings->ColorSaturation != NULL) {
		if (p_img_settings->saturation.supported) {
			if ((float)*timg__SetImagingSettings->ImagingSettings->ColorSaturation >= p_img_settings->saturation.minimum &&
				(float)*timg__SetImagingSettings->ImagingSettings->ColorSaturation <= p_img_settings->saturation.maximum) {
				p_img_settings->saturation.current =
					(float)*timg__SetImagingSettings->ImagingSettings->ColorSaturation;
			} else {
				onvif_fault(soap,
						SENDER,
						"ter:InvalidArgVal",
						"ter:SettingsInvalid");
				return SOAP_FAULT;
			}
		}
	}

	if (timg__SetImagingSettings->ImagingSettings->Contrast != NULL) {
		if (p_img_settings->contrast.supported) {
			if ((float)*timg__SetImagingSettings->ImagingSettings->Contrast >= p_img_settings->contrast.minimum &&
				(float)*timg__SetImagingSettings->ImagingSettings->Contrast <= p_img_settings->contrast.maximum) {
				p_img_settings->contrast.current =
					(float)*timg__SetImagingSettings->ImagingSettings->Contrast;
			} else {
				onvif_fault(soap,
						SENDER,
						"ter:InvalidArgVal",
						"ter:SettingsInvalid");
				return SOAP_FAULT;
			}
		}
	}

	if (timg__SetImagingSettings->ImagingSettings->Sharpness != NULL) {
		if (p_img_settings->sharpness.supported) {
			if ((float)*timg__SetImagingSettings->ImagingSettings->Sharpness >= p_img_settings->sharpness.minimum &&
				(float)*timg__SetImagingSettings->ImagingSettings->Sharpness <= p_img_settings->sharpness.maximum) {
				p_img_settings->sharpness.current =
					(float)*timg__SetImagingSettings->ImagingSettings->Sharpness;
			} else {
				onvif_fault(soap,
						SENDER,
						"ter:InvalidArgVal",
						"ter:SettingsInvalid");
				return SOAP_FAULT;
			}
		}
	}

	ret = rts_cam_set_video_attr(fd, p_img_settings);
	rts_cam_free_attr(p_img_settings);

	rts_cam_unlock(fd);
	rts_cam_close(fd);
	if (ret) {
		onvif_fault(soap,
				RECEIVER,
				"ter:FailToSetImage",
				"ter:NoImagingForSource");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetOptions(struct soap *soap,
	struct _timg__GetOptions *timg__GetOptions,
	struct _timg__GetOptionsResponse *timg__GetOptionsResponse)
{

	struct _timg__GetOptions *p_request = timg__GetOptions;
	int ret = 0, fd = 0;
	int vsc_count = 0;
	int i = 0;
	int j = 0;
	media_config_t *rs_configs = NULL;
	int is_vstoken_exist = 0;

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

	vsc_count = 0;
	while (vsc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[vsc_count].source_token[0] != '\0')
		vsc_count++;

	for (j = 0; j < vsc_count; j++) {
		if (!strcmp(rs_configs->vsc[i].source_token,
					p_request->VideoSourceToken)) {
			is_vstoken_exist = 1;
			break;
		}
	}

	if (!is_vstoken_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoSource");
		return SOAP_FAULT;
	}

	if (rs_configs->vec[0].vec_path[0] != '\0') {
		fd = rts_cam_open(rs_configs->vec[0].vec_path);
	} else {
		onvif_fault(soap,
				RECEIVER,
				"ter:NoCamDevice",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (fd < 0) {
		onvif_fault(soap,
				RECEIVER,
				"ter:NoCamDevice",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}

	if (rts_cam_lock(fd, 1000)) {
		rts_cam_close(fd);
		onvif_fault(soap,
				RECEIVER,
				"ter:LockCamDev",
				"ter:InvalidArgValue");
		return SOAP_FAULT;
	}
	struct attr_video *p_img_settings = NULL;
	ret = rts_cam_get_video_attr(fd, &p_img_settings);

	if (ret) {
		rts_cam_unlock(fd);
		rts_cam_close(fd);
		onvif_fault(soap,
				RECEIVER,
				"ter:LockCamDev",
				"ter:GetImageSettingError");
		return SOAP_FAULT;
	}
	/*
	SysInfo *oSysInfo = GetSysInfo();
	int i = 0, token_exist;
	int imaging_exist = 0;
	int index = 0;
	for (i = 0; i < oSysInfo->nprofile; i++) {
		if (strcmp(timg__GetOptions->VideoSourceToken, oSysInfo->Oprofile[i].AVSC.Vsourcetoken) == 0) {
			token_exist = EXIST;
			index = i;
			break;
		}
	}
	if (!token_exist) {
		onvif_fault(soap,"ter:InvalidArgVal", "ter:NoProfile");
		return SOAP_FAULT;
	}
	for (i = 0; i < oSysInfo->nprofile; i++) {
		if (strcmp(timg__GetOptions->VideoSourceToken, oSysInfo->imaging_conf[i].Isourcetoken) == 0) {
			imaging_exist = EXIST;
			break;
		}
	}
	if (!imaging_exist) {
		onvif_fault(soap,"ter:ActionNotSupported", "ter:NoImagingForSource");
		return SOAP_FAULT;
	}
	*/

	timg__GetOptionsResponse->ImagingOptions =
		(struct tt__ImagingOptions20 *)soap_malloc(soap,
				sizeof(struct tt__ImagingOptions20));

	if (p_img_settings->brightness.supported) {
		timg__GetOptionsResponse->ImagingOptions->Brightness =
			(struct tt__FloatRange *)soap_malloc(soap,
					sizeof(struct tt__FloatRange));
		timg__GetOptionsResponse->ImagingOptions->Brightness->Min =
			p_img_settings->brightness.minimum;
		timg__GetOptionsResponse->ImagingOptions->Brightness->Max =
			p_img_settings->brightness.maximum;
	} else {
		timg__GetOptionsResponse->ImagingOptions->Brightness = NULL;
	}

	if (p_img_settings->saturation.supported) {
		timg__GetOptionsResponse->ImagingOptions->ColorSaturation =
			(struct tt__FloatRange *)soap_malloc(soap,
					sizeof(struct tt__FloatRange));
		timg__GetOptionsResponse->ImagingOptions->ColorSaturation->Min =
			p_img_settings->saturation.minimum;
		timg__GetOptionsResponse->ImagingOptions->ColorSaturation->Max =
			p_img_settings->saturation.maximum;
	} else {
		timg__GetOptionsResponse->ImagingOptions->ColorSaturation = NULL;
	}

	if (p_img_settings->contrast.supported) {
		timg__GetOptionsResponse->ImagingOptions->Contrast =
			(struct tt__FloatRange *)soap_malloc(soap,
					sizeof(struct tt__FloatRange));
		timg__GetOptionsResponse->ImagingOptions->Contrast->Min =
			p_img_settings->contrast.minimum;
		timg__GetOptionsResponse->ImagingOptions->Contrast->Max =
			p_img_settings->contrast.maximum;
	} else {
		timg__GetOptionsResponse->ImagingOptions->Contrast = NULL;
	}

	if (p_img_settings->sharpness.supported) {
		timg__GetOptionsResponse->ImagingOptions->Sharpness =
			(struct tt__FloatRange *)soap_malloc(soap,
					sizeof(struct tt__FloatRange));
		timg__GetOptionsResponse->ImagingOptions->Sharpness->Min =
			p_img_settings->sharpness.minimum;
		timg__GetOptionsResponse->ImagingOptions->Sharpness->Max =
			p_img_settings->sharpness.maximum;
	} else {
		timg__GetOptionsResponse->ImagingOptions->Sharpness = NULL;
	}

	timg__GetOptionsResponse->ImagingOptions->__sizeIrCutFilterModes = 0;
	timg__GetOptionsResponse->ImagingOptions->IrCutFilterModes = NULL;
	timg__GetOptionsResponse->ImagingOptions->BacklightCompensation = NULL;
	timg__GetOptionsResponse->ImagingOptions->Exposure = NULL;
	timg__GetOptionsResponse->ImagingOptions->Focus = NULL;
	timg__GetOptionsResponse->ImagingOptions->WideDynamicRange = NULL;
	timg__GetOptionsResponse->ImagingOptions->WhiteBalance = NULL;
	timg__GetOptionsResponse->ImagingOptions->Extension = NULL;

	rts_cam_free_attr(p_img_settings);

	rts_cam_unlock(fd);
	rts_cam_close(fd);
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__Move(struct soap *soap,
	struct _timg__Move *timg__Move,
	struct _timg__MoveResponse *timg__MoveResponse)
{
	/*move to focus*/
	onvif_fault(soap,
			RECEIVER,
			"ter:ActionNotSupported",
			"ter:NoImagingForSource");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__Stop(struct soap *soap,
	struct _timg__Stop *timg__Stop,
	struct _timg__StopResponse *timg__StopResponse)
{
	/*stop focus*/
	onvif_fault(soap,
			RECEIVER,
			"ter:ActionNotSupported",
			"ter:NoImagingForSource");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetStatus(struct soap *soap,
	struct _timg__GetStatus *timg__GetStatus,
	struct _timg__GetStatusResponse *timg__GetStatusResponse)
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

	onvif_fault(soap,
			RECEIVER,
			"ter:ActionNotSupported",
			"ter:NoImagingForSource");
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetMoveOptions(struct soap *soap,
	struct _timg__GetMoveOptions *timg__GetMoveOptions,
	struct _timg__GetMoveOptionsResponse *timg__GetMoveOptionsResponse)
{
	struct _timg__GetMoveOptions *p_request = timg__GetMoveOptions;
	int ret = 0, fd = 0;
	int vsc_count = 0;
	int i = 0;
	int j = 0;
	media_config_t *rs_configs = NULL;
	int is_vstoken_exist = 0;

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

	vsc_count = 0;
	while (vsc_count < MAX_MEDIA_ASC_CONFIG &&
			rs_configs->asc[vsc_count].source_token[0] != '\0')
		vsc_count++;

	for (j = 0; j < vsc_count; j++) {
		if (!strcmp(rs_configs->vsc[i].source_token,
					p_request->VideoSourceToken)) {
			is_vstoken_exist = 1;
			break;
		}
	}

	if (!is_vstoken_exist) {
		onvif_fault(soap, SENDER, "ter:InvalidArgVal", "ter:NoSource");
		return SOAP_FAULT;
	}

	timg__GetMoveOptionsResponse->MoveOptions =
		(struct tt__MoveOptions20 *)soap_malloc(soap,
				sizeof(struct tt__MoveOptions20));

	timg__GetMoveOptionsResponse->MoveOptions->Absolute = NULL;
	timg__GetMoveOptionsResponse->MoveOptions->Relative = NULL;
	timg__GetMoveOptionsResponse->MoveOptions->Continuous = NULL;

	return SOAP_OK;
}


/*This Func is special for rs to tune isp*/
SOAP_FMAC5 int SOAP_FMAC6 __timg__rsIQTuning(struct soap *soap,
	struct _timg__rsIQTuning *timg__rsIQTuning,
	struct _timg__rsIQTuningResponse *timg__rsIQTuningResponse)
{
	onvif_fault(soap,
			RECEIVER,
			"ter:ActionNotSupported",
			"ter:NoImagingForSource");
	return SOAP_FAULT;
}
