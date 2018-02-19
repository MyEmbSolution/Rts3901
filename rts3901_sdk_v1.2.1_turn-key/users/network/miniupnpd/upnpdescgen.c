/* $Id: upnpdescgen.c,v 1.79 2015/09/22 10:07:13 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2015 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#ifdef ENABLE_EVENTS
#include "getifaddr.h"
#endif
#include "upnpdescgen.h"
#include "miniupnpdpath.h"
#include "upnpglobalvars.h"
#include "upnpdescstrings.h"
#include "upnpurns.h"
#include "getconnstatus.h"


/* Event magical values codes */
#define CONNECTIONSTATUS_MAGICALVALUE (249)
#define FIREWALLENABLED_MAGICALVALUE (250)
#define INBOUNDPINHOLEALLOWED_MAGICALVALUE (251)
#define SYSTEMUPDATEID_MAGICALVALUE (252)
#define PORTMAPPINGNUMBEROFENTRIES_MAGICALVALUE (253)
#define EXTERNALIPADDRESS_MAGICALVALUE (254)
#define DEFAULTCONNECTIONSERVICE_MAGICALVALUE (255)


static const char * const upnptypes[] = {
	"string",
	"boolean",
	"ui2",
	"ui4",
	"bin.base64"
};

static const char * const upnpdefaultvalues[] = {
	0,
	"IP_Routed"/*"Unconfigured"*/, /* 1 default value for ConnectionType */
	"3600", /* 2 default value for PortMappingLeaseDuration */
};

static const char * const upnpallowedvalues[] = {
	0,		/* 0 */
	"DSL",	/* 1 */
	"POTS",
	"Cable",
	"Ethernet",
	0,
	"Up",	/* 6 */
	"Down",
	"Initializing",
	"Unavailable",
	0,
	"TCP",	/* 11 */
	"UDP",
	0,
	"Unconfigured",	/* 14 */
	"IP_Routed",
	"IP_Bridged",
	0,
	"Unconfigured",	/* 18 */
	"Connecting",
	"Connected",
	"PendingDisconnect",
	"Disconnecting",
	"Disconnected",
	0,
	"ERROR_NONE",	/* 25 */
/* Optionals values :
 * ERROR_COMMAND_ABORTED
 * ERROR_NOT_ENABLED_FOR_INTERNET
 * ERROR_USER_DISCONNECT
 * ERROR_ISP_DISCONNECT
 * ERROR_IDLE_DISCONNECT
 * ERROR_FORCED_DISCONNECT
 * ERROR_NO_CARRIER
 * ERROR_IP_CONFIGURATION
 * ERROR_UNKNOWN */
	0,
	"",		/* 27 */
	0
};

static const int upnpallowedranges[] = {
	0,
	/* 1 PortMappingLeaseDuration */
	0,
	604800,
	/* 3 InternalPort */
	1,
	65535,
    /* 5 LeaseTime */
	1,
	86400,
	/* 7 OutboundPinholeTimeout */
	100,
	200,
};

static const char *magicargname[] = {
	0,
	"StartPort",
	"EndPort",
	"RemoteHost",
	"RemotePort",
	"InternalClient",
	"InternalPort",
	"IsWorking"
};

static const char xmlver[] =
	"<?xml version=\"1.0\"?>\r\n";
static const char root_service[] =
	"scpd xmlns=\"urn:schemas-upnp-org:service-1-0\"";
static const char root_device[] =
	"root xmlns=\"urn:schemas-upnp-org:device-1-0\"";

/* root Description of the UPnP Device
 * fixed to match UPnP_DigitalSecurityCamera 1.0.pdf
 * presentationURL is only "recommended" but the router doesn't appears
 * in "Network connections" in Windows XP if it is not present. */
static const struct XMLElt rootDesc[] = {
/* 0 */
	{root_device, INITHELPER(1, 2)},
	{"specVersion", INITHELPER(3, 2)},
#if defined(HAS_DUMMY_SERVICE) || defined(ENABLE_DP_SERVICE)
	{"device", INITHELPER(5, 13)},
#else
	{"device", INITHELPER(5, 12)},
#endif
	{"/major", UPNP_VERSION_MAJOR_STR},
	{"/minor", UPNP_VERSION_MINOR_STR},
	/* 5 */
	{"/deviceType", DEVICE_TYPE_IGD},
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
	{"/friendlyName", friendly_name/*ROOTDEV_FRIENDLYNAME*/},	/* required */
#else
	{"/friendlyName", ROOTDEV_FRIENDLYNAME},	/* required */
#endif
	{"/manufacturer", ROOTDEV_MANUFACTURER},	/* required */
	/* 8 */
	{"/manufacturerURL", ROOTDEV_MANUFACTURERURL},	/* optional */
	{"/modelDescription", ROOTDEV_MODELDESCRIPTION}, /* recommended */
	{"/modelName", ROOTDEV_MODELNAME},	/* required */
	{"/modelNumber", modelnumber},
	{"/modelURL", ROOTDEV_MODELURL},
	{"/serialNumber", serialnumber},
	{"/UDN", uuidvalue_ipcam},	/* required */
	/* see if /UPC is needed. */
#ifdef ENABLE_6FC_SERVICE
#define SERVICES_OFFSET 63
#else
#define SERVICES_OFFSET 58
#endif
#if defined(HAS_DUMMY_SERVICE) || defined(ENABLE_DP_SERVICE)
	/* here we dening Services for the root device :
	 * DUMMY and DeviceProtection */
#define NSERVICES1 0
#ifdef HAS_DUMMY_SERVICE
#define NSERVICES2 1
#else
#define NSERVICES2 0
#endif
#ifdef ENABLE_DP_SERVICE
#define NSERVICES3 1
#else
#define NSERVICES3 0
#endif
#define NSERVICES (NSERVICES1+NSERVICES2+NSERVICES3)
	{"serviceList", INITHELPER(SERVICES_OFFSET, NSERVICES)},
	{"deviceList", INITHELPER(18, 1)},
	{"/presentationURL", presentationurl},	/* recommended */
#else
	{"serviceList", INITHELPER(18, 1)},
	{"/presentationURL", presentationurl},	/* recommended */
	{0, 0},
#endif
/* 18 */
	{"service", INITHELPER(19, 5)},
	{"/serviceType", "urn:schemas-upnp-org:service:EmbeddedNetDeviceControl:1"},
	{"/serviceId", "urn:upnp-org:serviceId:EmbeddedNetDeviceControl"},
	{"/controlURL", "/"},
	{"/eventSubURL", "/"},
	{"/SCPDURL", "/"},
	{0, 0}
};

static const struct argument AddPortMappingArgs[] = {
	{0, 0}
};

static const struct argument GetExternalIPAddressArgs[] = {
	{0, 0}
};

static const struct argument DeletePortMappingArgs[] = {
	{0, 0}
};

static const struct argument SetConnectionTypeArgs[] = {
	{0, 0}
};

static const struct argument GetConnectionTypeInfoArgs[] = {
	{0, 0}
};

static const struct argument GetStatusInfoArgs[] = {
	{2, 2},
	{2, 4},
	{2, 3},
	{0, 0}
};

static const struct argument GetNATRSIPStatusArgs[] = {
	{0, 0}
};

static const struct argument GetGenericPortMappingEntryArgs[] = {
	{0, 0}
};

static const struct argument GetSpecificPortMappingEntryArgs[] = {
	{0, 0}
};

static const struct action WANIPCnActions[] = {
	{0, 0}
};
/* R=Required, O=Optional */

/* ignore "warning: missing initializer" */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

static const struct stateVar WANIPCnVars[] = {
/* 0 */
	{"ConnectionType", 0, 1, 14, 15}, /* required */
	{"PossibleConnectionTypes", 0|0x80, 0, 0, 15},
	 /* Required
	  * Allowed values : Unconfigured / IP_Routed / IP_Bridged */
	{"ConnectionStatus", 0|0x80, 0/*1*/, 18,
	 CONNECTIONSTATUS_MAGICALVALUE }, /* required */
	 /* Allowed Values : Unconfigured / Connecting(opt) / Connected
	  *                  PendingDisconnect(opt) / Disconnecting (opt)
	  *                  Disconnected */
	{"Uptime", 3, 0},	/* Required */
	{"LastConnectionError", 0, 0, 25},	/* required : */
	 /* Allowed Values : ERROR_NONE(req) / ERROR_COMMAND_ABORTED(opt)
	  *                  ERROR_NOT_ENABLED_FOR_INTERNET(opt)
	  *                  ERROR_USER_DISCONNECT(opt)
	  *                  ERROR_ISP_DISCONNECT(opt)
	  *                  ERROR_IDLE_DISCONNECT(opt)
	  *                  ERROR_FORCED_DISCONNECT(opt)
	  *                  ERROR_NO_CARRIER(opt)
	  *                  ERROR_IP_CONFIGURATION(opt)
	  *                  ERROR_UNKNOWN(opt) */
	{"RSIPAvailable", 1, 0}, /* required */
	{"NATEnabled", 1, 0},    /* required */
	{"ExternalIPAddress", 0|0x80, 0, 0,
	 EXTERNALIPADDRESS_MAGICALVALUE}, /* required. Default : empty string */
	{"PortMappingNumberOfEntries", 2|0x80, 0, 0,
	 PORTMAPPINGNUMBEROFENTRIES_MAGICALVALUE}, /* required >= 0 */
	{"PortMappingEnabled", 1, 0}, /* Required */
	/* 10 */
	{"PortMappingLeaseDuration", 3, 2, 1}, /* required */
	/* TODO : for IGD v2 :
	 * <stateVariable sendEvents="no">
	 *   <name>PortMappingLeaseDuration</name>
	 *   <dataType>ui4</dataType>
	 *   <defaultValue>Vendor-defined</defaultValue>
	 *   <allowedValueRange>
	 *      <minimum>0</minimum>
	 *      <maximum>604800</maximum>
	 *   </allowedValueRange>
	 * </stateVariable> */
	{"RemoteHost", 0, 0},   /* required. Default : empty string */
	{"ExternalPort", 2, 0}, /* required */
	{"InternalPort", 2, 0, 3}, /* required */
	{"PortMappingProtocol", 0, 0, 11}, /* required allowedValues: TCP/UDP */
	{"InternalClient", 0, 0}, /* required */
	{"PortMappingDescription", 0, 0}, /* required default: empty string */
	{0, 0}
};

static const struct serviceDesc scpdWANIPCn = {
	WANIPCnActions, WANIPCnVars };

/* WANCfg.xml */
/* See UPnP_IGD_WANCommonInterfaceConfig 1.0.pdf */

static const struct argument GetCommonLinkPropertiesArgs[] = {
	{2, 0},
	{2, 1},
	{2, 2},
	{2, 3},
	{0, 0}
};

static const struct argument GetTotalBytesSentArgs[] = {
	{2, 4},
	{0, 0}
};

static const struct argument GetTotalBytesReceivedArgs[] = {
	{2, 5},
	{0, 0}
};

static const struct argument GetTotalPacketsSentArgs[] = {
	{2, 6},
	{0, 0}
};

static const struct argument GetTotalPacketsReceivedArgs[] = {
	{2, 7},
	{0, 0}
};

static const struct action WANCfgActions[] = {
	{"GetCommonLinkProperties", GetCommonLinkPropertiesArgs}, /* Required */
	{"GetTotalBytesSent", GetTotalBytesSentArgs},             /* optional */
	{"GetTotalBytesReceived", GetTotalBytesReceivedArgs},     /* optional */
	{"GetTotalPacketsSent", GetTotalPacketsSentArgs},         /* optional */
	{"GetTotalPacketsReceived", GetTotalPacketsReceivedArgs}, /* optional */
	{0, 0}
};

/* See UPnP_IGD_WANCommonInterfaceConfig 1.0.pdf */
static const struct stateVar WANCfgVars[] = {
	{"WANAccessType", 0, 0, 1},
	/* Allowed Values : DSL / POTS / Cable / Ethernet
	 * Default value : empty string */
	{"Layer1UpstreamMaxBitRate", 3, 0},
	{"Layer1DownstreamMaxBitRate", 3, 0},
	{"PhysicalLinkStatus", 0|0x80, 0, 6, 6},
	/*  allowed values :
	 *   Up / Down / Initializing (optional) / Unavailable (optionnal)
	 *  no Default value
	 *  Evented */
	{"TotalBytesSent", 3, 0},	   /* Optional */
	{"TotalBytesReceived", 3, 0},  /* Optional */
	{"TotalPacketsSent", 3, 0},    /* Optional */
	{"TotalPacketsReceived", 3, 0},/* Optional */
	/*{"MaximumActiveConnections", 2, 0},
	  * allowed Range value // OPTIONAL
	  * {"WANAccessProvider", 0, 0},
	  * Optional */
	{0, 0}
};

static const struct serviceDesc scpdWANCfg = {
	WANCfgActions, WANCfgVars };
#ifdef ENABLE_DP_SERVICE
/* UPnP-gw-DeviceProtection-v1-Service.pdf */
static const struct action DPActions[] = {
	{"SendSetupMessage", 0},
	{"GetSupportedProtocols", 0},
	{"GetAssignedRoles", 0},
	{0, 0}
};

static const struct stateVar DPVars[] = {
	{"SetupReady", 1|0x80},
	{"SupportedProtocols", 0},
	{"A_ARG_TYPE_ACL", 0},
	{"A_ARG_TYPE_IdentityList", 0},
	{"A_ARG_TYPE_Identity", 0},
	{"A_ARG_TYPE_Base64", 4},
	{"A_ARG_TYPE_String", 0},
	{0, 0}
};

static const struct serviceDesc scpdDP = {
	DPActions, DPVars };
#endif

/* strcat_str()
 * concatenate the string and use realloc to increase the
 * memory buffer if needed. */
static char *
strcat_str(char *str, int *len, int *tmplen, const char *s2)
{
	int s2len;
	int newlen;
	char *p;

	s2len = (int)strlen(s2);
	if (*tmplen <= (*len + s2len)) {
		if (s2len < 256)
			newlen = *tmplen + 256;
		else
			newlen = *tmplen + s2len + 1;
		p = (char *)realloc(str, newlen);
		if (p == NULL) /* handle a failure of realloc() */
			return str;
		str = p;
		*tmplen = newlen;
	}
	/*strcpy(str + *len, s2); */
	memcpy(str + *len, s2, s2len + 1);
	*len += s2len;
	return str;
}

/* strcat_char() :
 * concatenate a character and use realloc to increase the
 * size of the memory buffer if needed */
static char *
strcat_char(char *str, int *len, int *tmplen, char c)
{
	char *p;

	if (*tmplen <= (*len + 1)) {
		*tmplen += 256;
		p = (char *)realloc(str, *tmplen);
		/* handle a failure of realloc() */
		if (p == NULL) {
			*tmplen -= 256;
			return str;
		}
		str = p;
	}
	str[*len] = c;
	(*len)++;
	return str;
}

/* strcat_int()
 * concatenate the string representation of the integer.
 * call strcat_char() */
static char *
strcat_int(char *str, int *len, int *tmplen, int i)
{
	char buf[16];
	int j;

	if (i < 0) {
		str = strcat_char(str, len, tmplen, '-');
		i = -i;
	} else if (i == 0) {
		/* special case for 0 */
		str = strcat_char(str, len, tmplen, '0');
		return str;
	}
	j = 0;
	while (i && j < (int)sizeof(buf)) {
		buf[j++] = '0' + (i % 10);
		i = i / 10;
	}
	while (j > 0) {
		str = strcat_char(str, len, tmplen, buf[--j]);
	}
	return str;
}

/* iterative subroutine using a small stack
 * This way, the progam stack usage is kept low */
static char *
genXML(char *str, int *len, int *tmplen,
		   const struct XMLElt *p)
{
	unsigned short i, j;
	unsigned long k;
	int top;
	const char *eltname, *s;
	char c;
	struct {
		unsigned short i;
		unsigned short j;
		const char *eltname;
	} pile[16]; /* stack */
	top = -1;
	i = 0;	/* current node */
	j = 1;	/* i + number of nodes*/
	for (;;) {
		eltname = p[i].eltname;
		if (!eltname)
			return str;
		if (eltname[0] == '/') {
			if (p[i].data && p[i].data[0]) {
				/*printf("<%s>%s<%s>\n", eltname+1, p[i].data, eltname); */
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname+1);
				str = strcat_char(str, len, tmplen, '>');
				str = strcat_str(str, len, tmplen, p[i].data);
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname);
				str = strcat_char(str, len, tmplen, '>');
			} for (;;) {
				if (top < 0)
					return str;
				i = ++(pile[top].i);
				j = pile[top].j;
				/*printf("  pile[%d]\t%d %d\n", top, i, j); */
				if (i == j) {
					/*printf("</%s>\n", pile[top].eltname); */
					str = strcat_char(str, len, tmplen, '<');
					str = strcat_char(str, len, tmplen, '/');
					s = pile[top].eltname;
					for (c = *s; c > ' '; c = *(++s))
						str = strcat_char(str, len, tmplen, c);
					str = strcat_char(str, len, tmplen, '>');
					top--;
				} else
					break;
			}
		} else {
			/*printf("<%s>\n", eltname); */
			str = strcat_char(str, len, tmplen, '<');
			str = strcat_str(str, len, tmplen, eltname);
			str = strcat_char(str, len, tmplen, '>');
			k = (unsigned long)p[i].data;
			i = k & 0xffff;
			j = i + (k >> 16);
			top++;
			/*printf(" +pile[%d]\t%d %d\n", top, i, j); */
			pile[top].i = i;
			pile[top].j = j;
			pile[top].eltname = eltname;
		}
	}
}

/* genRootDesc() :
 * - Generate the root description of the UPnP device.
 * - the len argument is used to return the length of
 *   the returned string.
 * - tmp_uuid argument is used to build the uuid string */
char *
genRootDesc(int *len)
{
	char *str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if (str == NULL)
		return NULL;
	*len = strlen(xmlver);
	/*strcpy(str, xmlver); */
	memcpy(str, xmlver, *len + 1);
	str = genXML(str, len, &tmplen, rootDesc);
	str[*len] = '\0';
	return str;
}

/* genServiceDesc() :
 * Generate service description with allowed methods and
 * related variables. */
static char *
genServiceDesc(int *len, const struct serviceDesc *s)
{
	int i, j;
	const struct action *acts;
	const struct stateVar *vars;
	const struct argument *args;
	const char *p;
	char *str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if (str == NULL)
		return NULL;
	/*strcpy(str, xmlver); */
	*len = strlen(xmlver);
	memcpy(str, xmlver, *len + 1);

	acts = s->actionList;
	vars = s->serviceStateTable;

	str = strcat_char(str, len, &tmplen, '<');
	str = strcat_str(str, len, &tmplen, root_service);
	str = strcat_char(str, len, &tmplen, '>');

	str = strcat_str(str, len, &tmplen,
		"<specVersion><major>" UPNP_VERSION_MAJOR_STR "</major>"
		"<minor>" UPNP_VERSION_MINOR_STR "</minor></specVersion>");

	i = 0;
	str = strcat_str(str, len, &tmplen, "<actionList>");
	while (acts[i].name) {
		str = strcat_str(str, len, &tmplen, "<action><name>");
		str = strcat_str(str, len, &tmplen, acts[i].name);
		str = strcat_str(str, len, &tmplen, "</name>");
		/* argument List */
		args = acts[i].args;
		if (args) {
			str = strcat_str(str, len, &tmplen, "<argumentList>");
			j = 0;
			while (args[j].dir) {
				str = strcat_str(str, len, &tmplen, "<argument><name>");
				if ((args[j].dir & 0x80) == 0) {
					str = strcat_str(str, len, &tmplen, "New");
				}
				p = vars[args[j].relatedVar].name;
				if (args[j].dir & 0x7c) {
					/* use magic values ... */
					str = strcat_str(str, len, &tmplen,
						magicargname[(args[j].dir & 0x7c) >> 2]);
				} else if (0 == memcmp(p, "PortMapping", 11)
				   && 0 != memcmp(p + 11, "Description", 11)) {
					if (0 == memcmp(p + 11, "NumberOfEntries", 15)) {
						/* PortMappingNumberOfEntries */
						str = strcat_str(str, len, &tmplen, "PortMappingIndex");
					} else {
						/* PortMappingEnabled
						 * PortMappingLeaseDuration
						 * PortMappingProtocol */
						str = strcat_str(str, len, &tmplen, p + 11);
					}
				} else {
					str = strcat_str(str, len, &tmplen, p);
				}
				str = strcat_str(str, len, &tmplen, "</name><direction>");
				str = strcat_str(str, len, &tmplen, (args[j].dir&0x03) == 1?"in":"out");
				str = strcat_str(str, len, &tmplen,
						"</direction><relatedStateVariable>");
				str = strcat_str(str, len, &tmplen, p);
				str = strcat_str(str, len, &tmplen,
						"</relatedStateVariable></argument>");
				j++;
			}
			str = strcat_str(str, len, &tmplen, "</argumentList>");
		}
		str = strcat_str(str, len, &tmplen, "</action>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</actionList><serviceStateTable>");
	i = 0;
	while (vars[i].name) {
		str = strcat_str(str, len, &tmplen,
				"<stateVariable sendEvents=\"");
#ifdef ENABLE_EVENTS
		str = strcat_str(str, len, &tmplen, (vars[i].itype & 0x80)?"yes":"no");
#else
		/* for the moment always send no. Wait for SUBSCRIBE implementation
		 * before setting it to yes */
		str = strcat_str(str, len, &tmplen, "no");
#endif
		str = strcat_str(str, len, &tmplen, "\"><name>");
		str = strcat_str(str, len, &tmplen, vars[i].name);
		str = strcat_str(str, len, &tmplen, "</name><dataType>");
		str = strcat_str(str, len, &tmplen, upnptypes[vars[i].itype & 0x0f]);
		str = strcat_str(str, len, &tmplen, "</dataType>");
		if (vars[i].iallowedlist) {
			if ((vars[i].itype & 0x0f) == 0) {
			  /* string */
				str = strcat_str(str, len, &tmplen, "<allowedValueList>");
				for (j = vars[i].iallowedlist; upnpallowedvalues[j]; j++) {
					str = strcat_str(str, len, &tmplen, "<allowedValue>");
					str = strcat_str(str, len, &tmplen, upnpallowedvalues[j]);
					str = strcat_str(str, len, &tmplen, "</allowedValue>");
				}
				str = strcat_str(str, len, &tmplen, "</allowedValueList>");
			} else {
				/* ui2 and ui4 */
				str = strcat_str(str, len, &tmplen, "<allowedValueRange><minimum>");
				str = strcat_int(str, len, &tmplen, upnpallowedranges[vars[i].iallowedlist]);
				str = strcat_str(str, len, &tmplen, "</minimum><maximum>");
				str = strcat_int(str, len, &tmplen, upnpallowedranges[vars[i].iallowedlist+1]);
				str = strcat_str(str, len, &tmplen, "</maximum></allowedValueRange>");
			}
		}
		/*if(vars[i].defaultValue) */
		if (vars[i].idefault) {
			str = strcat_str(str, len, &tmplen, "<defaultValue>");
			/*str = strcat_str(str, len, &tmplen, vars[i].defaultValue); */
			str = strcat_str(str, len, &tmplen, upnpdefaultvalues[vars[i].idefault]);
			str = strcat_str(str, len, &tmplen, "</defaultValue>");
		}
			str = strcat_str(str, len, &tmplen, "</stateVariable>");
			/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
			i++;
	}
	str = strcat_str(str, len, &tmplen, "</serviceStateTable></scpd>");
	str[*len] = '\0';
	return str;
}

/* genWANIPCn() :
 * Generate the WANIPConnection xml description */
char *
genWANIPCn(int *len)
{
	return genServiceDesc(len, &scpdWANIPCn);
}

/* genWANCfg() :
 * Generate the WANInterfaceConfig xml description. */
char *
genWANCfg(int *len)
{
	return genServiceDesc(len, &scpdWANCfg);
}
#ifdef ENABLE_DP_SERVICE
char *
genDP(int *len)
{
	return genServiceDesc(len, &scpdDP);
}
#endif

#ifdef ENABLE_EVENTS
static char *
genEventVars(int *len, const struct serviceDesc *s)
{
	char tmp[16];
	const struct stateVar *v;
	char *str;
	int tmplen;
	tmplen = 512;
	str = (char *)malloc(tmplen);
	if (str == NULL)
		return NULL;
	*len = 0;
	v = s->serviceStateTable;
	str = strcat_str(str, len, &tmplen,
		"<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">");
	while (v->name) {
		if (v->itype & 0x80) {
			str = strcat_str(str, len, &tmplen, "<e:property><");
			str = strcat_str(str, len, &tmplen, v->name);
			str = strcat_str(str, len, &tmplen, ">");
			/*printf("<e:property><%s>", v->name);*/
			switch (v->ieventvalue) {
			case 0:
				break;
			case CONNECTIONSTATUS_MAGICALVALUE:
				/* or get_wan_connection_status_str(ext_if_name) */
				str = strcat_str(str, len, &tmplen,
				   upnpallowedvalues[18 + get_wan_connection_status(ext_if_name)]);
				break;
			case PORTMAPPINGNUMBEROFENTRIES_MAGICALVALUE:
				break;
			case EXTERNALIPADDRESS_MAGICALVALUE:
				/* External ip address magical value */
				if (use_ext_ip_addr)
					str = strcat_str(str, len, &tmplen, use_ext_ip_addr);
				else {
					char ext_ip_addr[INET_ADDRSTRLEN];
					if (getifaddr(ext_if_name, ext_ip_addr,
						INET_ADDRSTRLEN, NULL, NULL) < 0) {
						str = strcat_str(str, len, &tmplen, "0.0.0.0");
					} else {
						str = strcat_str(str, len, &tmplen, ext_ip_addr);
					}
				}
				break;
			case DEFAULTCONNECTIONSERVICE_MAGICALVALUE:
				/* DefaultConnectionService magical value */
				str = strcat_str(str, len, &tmplen, uuidvalue_wcd);
				str = strcat_str(str, len, &tmplen,
					":WANConnectionDevice:1,urn:upnp-org:serviceId:WANIPConn1");
				break;
			default:
				str = strcat_str(str, len, &tmplen, upnpallowedvalues[v->ieventvalue]);
			}
			str = strcat_str(str, len, &tmplen, "</");
			str = strcat_str(str, len, &tmplen, v->name);
			str = strcat_str(str, len, &tmplen, "></e:property>");
			/*printf("</%s></e:property>\n", v->name);*/
		}
		v++;
	}
	str = strcat_str(str, len, &tmplen, "</e:propertyset>");
#if 0
	printf("</e:propertyset>\n");
	printf("\n");
	printf("%d\n", tmplen);
#endif
	str[*len] = '\0';
	return str;
}

char *
getVarsWANIPCn(int *l)
{
	return genEventVars(l,
			&scpdWANIPCn);
}

char *
getVarsWANCfg(int *l)
{
	return genEventVars(l,
			    &scpdWANCfg);
}
#ifdef ENABLE_DP_SERVICE
char *
getVarsDP(int *l)
{
	return genEventVars(l,
			    &scpdDP);
}
#endif

#endif /* ENABLE_EVENTS */
