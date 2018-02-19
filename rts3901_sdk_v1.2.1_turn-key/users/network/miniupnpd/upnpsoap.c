/* $Id: upnpsoap.c,v 1.138 2015/09/22 09:58:30 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2015 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "macros.h"
#include "config.h"
#include "upnpglobalvars.h"
#include "upnphttp.h"
#include "upnpsoap.h"
#include "upnpreplyparse.h"
#include "getifaddr.h"
#include "getifstats.h"
#include "getconnstatus.h"
#include "upnpurns.h"

static void
BuildSendAndCloseSoapResp(struct upnphttp *h,
			  const char *body, int bodylen)
{
	static const char beforebody[] =
		"<?xml version=\"1.0\"?>\r\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>";

	static const char afterbody[] =
		"</s:Body>"
		"</s:Envelope>\r\n";

	int r = BuildHeader_upnphttp(h, 200, "OK",  sizeof(beforebody) - 1
				     + sizeof(afterbody) - 1 + bodylen);

	if (r >= 0) {
		memcpy(h->res_buf + h->res_buflen,
			beforebody, sizeof(beforebody) - 1);
		h->res_buflen += sizeof(beforebody) - 1;

		memcpy(h->res_buf + h->res_buflen,
			body, bodylen);
		h->res_buflen += bodylen;

		memcpy(h->res_buf + h->res_buflen,
			afterbody, sizeof(afterbody) - 1);
		h->res_buflen += sizeof(afterbody) - 1;
	} else {
		BuildResp2_upnphttp(h, 500,
			"Internal Server Error", NULL, 0);
	}

	SendRespAndClose_upnphttp(h);
}

#ifdef ENABLE_6FC_SERVICE
#ifndef ENABLE_IPV6
#error "ENABLE_6FC_SERVICE needs ENABLE_IPV6"
#endif
/* WANIPv6FirewallControl actions */
static void
GetFirewallStatus(struct upnphttp *h, const char *action, const char *ns)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<FirewallEnabled>%d</FirewallEnabled>"
		"<InboundPinholeAllowed>%d</InboundPinholeAllowed>"
		"</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = snprintf(body, sizeof(body), resp,
		/*"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1",*/
		action, ns,
		GETFLAG(IPV6FCFWDISABLEDMASK) ? 0 : 1,
		GETFLAG(IPV6FCINBOUNDDISALLOWEDMASK) ? 0 : 1,
		action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static int
CheckStatus(struct upnphttp *h)
{
	if (GETFLAG(IPV6FCFWDISABLEDMASK)) {
		SoapError(h, 702, "FirewallDisabled");
		return 0;
	} else if (GETFLAG(IPV6FCINBOUNDDISALLOWEDMASK)) {
		SoapError(h, 703, "InboundPinholeNotAllowed");
		return 0;
	} else
		return 1;
}

/* Check the security policy right */
static int
PinholeVerification(struct upnphttp *h, char *int_ip, unsigned short int_port)
{
	int n;
	char senderAddr[INET6_ADDRSTRLEN] = "";
	struct addrinfo hints, *ai, *p;
	struct in6_addr result_ip;

	/* Pinhole InternalClient address must correspond to the action sender */
	syslog(LOG_INFO,
		"Checking internal IP@ and port (Security policy purpose)");

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;

	/* if ip not valid assume hostname and convert */
	if (inet_pton(AF_INET6, int_ip, &result_ip) <= 0) {
		n = getaddrinfo(int_ip, NULL, &hints, &ai);
		if (!n && ai->ai_family == AF_INET6) {
			for (p = ai; p; p = p->ai_next) {
				inet_ntop(AF_INET6, (struct in6_addr *) p,
					int_ip, sizeof(struct in6_addr));
				result_ip = *((struct in6_addr *) p);
				/* TODO : deal with more than one ip per hostname */
				break;
			}
		} else {
			syslog(LOG_ERR,
				"Failed to convert hostname '%s' to ip address", int_ip);
			SoapError(h, 402, "Invalid Args");
			return -1;
		}
	freeaddrinfo(p);
	}

	if (inet_ntop(AF_INET6, &(h->clientaddr_v6),
		senderAddr, INET6_ADDRSTRLEN) == NULL) {
		syslog(LOG_ERR, "inet_ntop: %m");
	}
#ifdef DEBUG
	printf("\tPinholeVerification:\n\t\tCompare sender @: %s\n\t\t  to intClient @: %s\n",
		senderAddr, int_ip);
#endif
	if (strcmp(senderAddr, int_ip) != 0)
		if (h->clientaddr_v6.s6_addr != result_ip.s6_addr) {
			syslog(LOG_INFO,
				"Client %s tried to access pinhole for internal %s and is not authorized to do it",
				senderAddr, int_ip);
			SoapError(h, 606, "Action not authorized");
			return 0;
		}

	/* Pinhole InternalPort must be greater than or equal to 1024 */
	if (int_port < 1024) {
		syslog(LOG_INFO,
			"Client %s tried to access pinhole with port < 1024 and is not authorized to do it",
			senderAddr);
		SoapError(h, 606, "Action not authorized");
		return 0;
	}
	return 1;
}

static void
AddPinhole(struct upnphttp *h, const char *action)
{
	int r;
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<UniqueID>%d</UniqueID>"
		"</u:%sResponse>";
	char body[512];
	int bodylen;
	struct NameValueParserData data;
	char *rem_host, *rem_port, *int_ip, *int_port, *protocol, *leaseTime;
	int uid = 0;
	unsigned short iport, rport;
	int ltime;
	long proto;
	char rem_ip[INET6_ADDRSTRLEN];

	if (CheckStatus(h) == 0)
		return;

	ParseNameValue(h->req_buf + h->req_contentoff,
		h->req_contentlen, &data);
	rem_host = GetValueFromNameValueList(&data,
		"RemoteHost");
	rem_port = GetValueFromNameValueList(&data,
		"RemotePort");
	int_ip = GetValueFromNameValueList(&data,
		"InternalClient");
	int_port = GetValueFromNameValueList(&data,
		"InternalPort");
	protocol = GetValueFromNameValueList(&data,
		"Protocol");
	leaseTime = GetValueFromNameValueList(&data,
		"LeaseTime");

	rport = (unsigned short)(rem_port ? atoi(rem_port) : 0);
	iport = (unsigned short)(int_port ? atoi(int_port) : 0);
	ltime = leaseTime ? atoi(leaseTime) : -1;
	errno = 0;
	proto = protocol ? strtol(protocol, NULL, 0) : -1;
	if (errno != 0 || proto > 65535 || proto < 0) {
		SoapError(h, 402, "Invalid Args");
		goto clear_and_exit;
	}
	if (iport == 0) {
		SoapError(h, 706,
			"InternalPortWilcardingNotAllowed");
		goto clear_and_exit;
	}

	/* In particular, [IGD2] RECOMMENDS that unauthenticated and
	 * unauthorized control points are only allowed to invoke
	 * this action with:
	 * - InternalPort value greater than or equal to 1024,
	 * - InternalClient value equals to the control point's IP address.
	 * It is REQUIRED that InternalClient cannot be one of IPv6
	 * addresses used by the gateway. */
	if (!int_ip || 0 == strlen(int_ip) || 0 == strcmp(int_ip, "*")) {
		SoapError(h, 708,
			"WildCardNotPermittedInSrcIP");
		goto clear_and_exit;
	}
	/* I guess it is useless to convert int_ip to literal ipv6 address */
	/* rem_host should be converted to literal ipv6 : */
	if (rem_host && (rem_host[0] != '\0')) {
		struct addrinfo *ai, *p;
		struct addrinfo hints;
		int err;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET6;
		/*hints.ai_flags = */
		/* hints.ai_protocol = proto; */
		err = getaddrinfo(rem_host, rem_port, &hints, &ai);
		if (err == 0) {
			/* take the 1st IPv6 address */
			for (p = ai; p; p = p->ai_next) {
				if (p->ai_family == AF_INET6) {
					inet_ntop(AF_INET6,
						  &(((struct sockaddr_in6 *)p->ai_addr)->sin6_addr),
						  rem_ip, sizeof(rem_ip));
					syslog(LOG_INFO,
						"resolved '%s' to '%s'", rem_host, rem_ip);
					rem_host = rem_ip;
					break;
				}
			}
			freeaddrinfo(ai);
		} else {
			syslog(LOG_WARNING,
				"AddPinhole : getaddrinfo(%s) : %s",
			       rem_host, gai_strerror(err));
#if 0
			SoapError(h, 402, "Invalid Args");
			goto clear_and_exit;
#endif
		}
	}

	if (proto == 65535) {
		SoapError(h, 707,
			"ProtocolWilcardingNotAllowed");
		goto clear_and_exit;
	}
	if (proto != IPPROTO_UDP && proto != IPPROTO_TCP
#ifdef IPPROTO_UDPITE
	   && atoi(protocol) != IPPROTO_UDPLITE
#endif
	  ) {
		SoapError(h, 705, "ProtocolNotSupported");
		goto clear_and_exit;
	}
	if (ltime < 1 || ltime > 86400) {
		syslog(LOG_WARNING,
			"%s: LeaseTime=%d not supported, (ip=%s)",
		       action, ltime, int_ip);
		SoapError(h, 402, "Invalid Args");
		goto clear_and_exit;
	}

	if (PinholeVerification(h, int_ip, iport) <= 0)
		goto clear_and_exit;

	syslog(LOG_INFO,
		"%s: (inbound) from [%s]:%hu to [%s]:%hu with proto %ld during %d sec",
		action, rem_host?rem_host:"any",
		rport, int_ip, iport,
		proto, ltime);

	/* In cases where the RemoteHost, RemotePort, InternalPort,
	 * InternalClient and Protocol are the same than an existing pinhole,
	 * but LeaseTime is different, the device MUST extend the existing
	 * pinhole's lease time and return the UniqueID of the existing pinhole. */
	r = upnp_add_inboundpinhole(rem_host, rport, int_ip, iport, proto,
						"IGD2 pinhole", ltime, &uid);

	switch (r) {
	case 1:	        /* success */
			bodylen = snprintf(body, sizeof(body),
					   resp, action,
					   "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1",
					   uid, action);
			BuildSendAndCloseSoapResp(h, body, bodylen);
			break;
	case -1:	/* not permitted */
			SoapError(h, 701, "PinholeSpaceExhausted");
			break;
	default:
			SoapError(h, 501, "ActionFailed");
			break;
	}
	/* 606 Action not authorized
	 * 701 PinholeSpaceExhausted
	 * 702 FirewallDisabled
	 * 703 InboundPinholeNotAllowed
	 * 705 ProtocolNotSupported
	 * 706 InternalPortWildcardingNotAllowed
	 * 707 ProtocolWildcardingNotAllowed
	 * 708 WildCardNotPermittedInSrcIP */
clear_and_exit:
	ClearNameValueList(&data);
}

static void
UpdatePinhole(struct upnphttp *h, const char *action)
{
	static const char resp[] =
		"<u:UpdatePinholeResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\">"
		"</u:UpdatePinholeResponse>";
	struct NameValueParserData data;
	const char *uid_str, *leaseTime;
	char iaddr[INET6_ADDRSTRLEN];
	unsigned short iport;
	int ltime;
	int uid;
	int n;

	if (CheckStatus(h) == 0)
		return;

	ParseNameValue(h->req_buf + h->req_contentoff,
		h->req_contentlen, &data);
	uid_str = GetValueFromNameValueList(&data,
		"UniqueID");
	leaseTime = GetValueFromNameValueList(&data,
		"NewLeaseTime");
	uid = uid_str ? atoi(uid_str) : -1;
	ltime = leaseTime ? atoi(leaseTime) : -1;
	ClearNameValueList(&data);

	if (uid < 0 || uid > 65535 || ltime <= 0 || ltime > 86400) {
		SoapError(h, 402, "Invalid Args");
		return;
	}

	/* Check that client is not updating an pinhole
	 * it doesn't have access to, because of its public access */
	n = upnp_get_pinhole_info(uid, NULL, 0, NULL,
				  iaddr, sizeof(iaddr), &iport,
				  NULL, /* proto */
				  NULL, 0, /* desc, desclen */
				  NULL, NULL);
	if (n >= 0) {
		if (PinholeVerification(h, iaddr, iport) <= 0)
			return;
	} else if (n == -2) {
		SoapError(h, 704, "NoSuchEntry");
		return;
	} else {
		SoapError(h, 501, "ActionFailed");
		return;
	}

	syslog(LOG_INFO,
		"%s: (inbound) updating lease duration to %d for pinhole with ID: %d",
		action, ltime, uid);

	n = upnp_update_inboundpinhole(uid, ltime);
	if (n == -1)
		SoapError(h, 704, "NoSuchEntry");
	else if (n < 0)
		SoapError(h, 501, "ActionFailed");
	else
		BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
}

static void
GetOutboundPinholeTimeout(struct upnphttp *h, const char *action)
{
	int r;

	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<OutboundPinholeTimeout>%d</OutboundPinholeTimeout>"
		"</u:%sResponse>";

	char body[512];
	int bodylen;
	struct NameValueParserData data;
	char *int_ip, *int_port, *rem_host, *rem_port, *protocol;
	int opt = 0, proto = 0;
	unsigned short iport, rport;

	if (GETFLAG(IPV6FCFWDISABLEDMASK)) {
		SoapError(h, 702, "FirewallDisabled");
		return;
	}

	ParseNameValue(h->req_buf + h->req_contentoff,
		h->req_contentlen, &data);
	int_ip = GetValueFromNameValueList(&data,
		"InternalClient");
	int_port = GetValueFromNameValueList(&data,
		"InternalPort");
	rem_host = GetValueFromNameValueList(&data,
		"RemoteHost");
	rem_port = GetValueFromNameValueList(&data,
		"RemotePort");
	protocol = GetValueFromNameValueList(&data,
		"Protocol");

	rport = (unsigned short)atoi(rem_port);
	iport = (unsigned short)atoi(int_port);
	proto = atoi(protocol);

	syslog(LOG_INFO,
		"%s: retrieving timeout for outbound pinhole from [%s]:%hu to [%s]:%hu protocol %s",
		action, int_ip, iport,
		rem_host, rport, protocol);

	/* TODO */
	/*upnp_check_outbound_pinhole(proto, &opt);*/
	r = -1;

	switch (r) {
	case 1:	/* success */
			bodylen = snprintf(body, sizeof(body), resp,
					   action,
					   "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1",
					   opt, action);
			BuildSendAndCloseSoapResp(h, body, bodylen);
			break;
	case -5:	/* Protocol not supported */
			SoapError(h, 705, "ProtocolNotSupported");
			break;
	default:
			SoapError(h, 501, "ActionFailed");
	}
	ClearNameValueList(&data);
}

static void
DeletePinhole(struct upnphttp *h, const char *action)
{
	int n;

	static const char resp[] =
		"<u:DeletePinholeResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\">"
		"</u:DeletePinholeResponse>";

	struct NameValueParserData data;
	const char *uid_str;
	char iaddr[INET6_ADDRSTRLEN];
	int proto;
	unsigned short iport;
	unsigned int leasetime;
	int uid;

	if (CheckStatus(h) == 0)
		return;

	ParseNameValue(h->req_buf + h->req_contentoff,
		h->req_contentlen, &data);
	uid_str = GetValueFromNameValueList(&data,
		"UniqueID");
	uid = uid_str ? atoi(uid_str) : -1;
	ClearNameValueList(&data);

	if (uid < 0 || uid > 65535) {
		SoapError(h, 402, "Invalid Args");
		return;
	}

	/* Check that client is not deleting an pinhole
	 * it doesn't have access to, because of its public access */
	n = upnp_get_pinhole_info(uid, NULL, 0, NULL,
				  iaddr, sizeof(iaddr), &iport,
				  &proto,
				  NULL, 0, /* desc, desclen */
				  &leasetime, NULL);
	if (n >= 0) {
		if (PinholeVerification(h, iaddr, iport) <= 0)
			return;
	} else if (n == -2) {
		SoapError(h, 704, "NoSuchEntry");
		return;
	} else {
		SoapError(h, 501, "ActionFailed");
		return;
	}

	n = upnp_delete_inboundpinhole(uid);
	if (n < 0) {
		syslog(LOG_INFO,
			"%s: (inbound) failed to remove pinhole with ID: %d",
			action, uid);
		SoapError(h, 501, "ActionFailed");
		return;
	}
	syslog(LOG_INFO,
		"%s: (inbound) pinhole with ID %d successfully removed",
		action, uid);
	BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
}

static void
CheckPinholeWorking(struct upnphttp *h, const char *action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<IsWorking>%d</IsWorking>"
		"</u:%sResponse>";
	char body[512];
	int bodylen;
	int r;
	struct NameValueParserData data;
	const char *uid_str;
	int uid;
	char iaddr[INET6_ADDRSTRLEN];
	unsigned short iport;
	unsigned int packets;

	if (CheckStatus(h) == 0)
		return;

	ParseNameValue(h->req_buf + h->req_contentoff,
		h->req_contentlen, &data);
	uid_str = GetValueFromNameValueList(&data,
		"UniqueID");
	uid = uid_str ? atoi(uid_str) : -1;
	ClearNameValueList(&data);

	if (uid < 0 || uid > 65535) {
		SoapError(h, 402, "Invalid Args");
		return;
	}

	/* Check that client is not checking a pinhole
	 * it doesn't have access to, because of its public access */
	r = upnp_get_pinhole_info(uid,
				  NULL, 0, NULL,
				  iaddr, sizeof(iaddr), &iport,
				  NULL, /* proto */
				  NULL, 0, /* desc, desclen */
				  NULL, &packets);
	if (r >= 0) {
		if (PinholeVerification(h, iaddr, iport) <= 0)
			return;
		if (packets == 0) {
			SoapError(h, 709, "NoPacketSent");
			return;
		}
		bodylen = snprintf(body, sizeof(body), resp,
						action,
						"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1",
						1, action);
		BuildSendAndCloseSoapResp(h, body, bodylen);
	} else if (r == -2)
		SoapError(h, 704, "NoSuchEntry");
	else
		SoapError(h, 501, "ActionFailed");
}

static void
GetPinholePackets(struct upnphttp *h, const char *action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<PinholePackets>%u</PinholePackets>"
		"</u:%sResponse>";
	char body[512];
	int bodylen;
	struct NameValueParserData data;
	const char *uid_str;
	int n;
	char iaddr[INET6_ADDRSTRLEN];
	unsigned short iport;
	unsigned int packets = 0;
	int uid;
	int proto;
	unsigned int leasetime;

	if (CheckStatus(h) == 0)
		return;

	ParseNameValue(h->req_buf + h->req_contentoff,
		h->req_contentlen, &data);
	uid_str = GetValueFromNameValueList(&data,
		"UniqueID");
	uid = uid_str ? atoi(uid_str) : -1;
	ClearNameValueList(&data);

	if (uid < 0 || uid > 65535) {
		SoapError(h, 402, "Invalid Args");
		return;
	}

	/* Check that client is not getting infos of a pinhole
	 * it doesn't have access to, because of its public access */
	n = upnp_get_pinhole_info(uid, NULL, 0, NULL,
				  iaddr, sizeof(iaddr), &iport,
				  &proto,
				  NULL, 0, /* desc, desclen */
				  &leasetime, &packets);
	if (n >= 0) {
		if (PinholeVerification(h, iaddr, iport) <= 0)
			return;
	}

	bodylen = snprintf(body, sizeof(body), resp,
			action,
			"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1",
			packets, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}
#endif

#ifdef ENABLE_DP_SERVICE
static void
SendSetupMessage(struct upnphttp *h, const char *action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewOutMessage>%s</NewOutMessage>"
		"</u:%sResponse>";
	char body[1024];
	int bodylen;
	struct NameValueParserData data;
	const char *ProtocolType;	/* string */
	const char *InMessage;		/* base64 */
	const char *OutMessage = "";	/* base64 */

	ParseNameValue(h->req_buf + h->req_contentoff,
		h->req_contentlen, &data);
	ProtocolType = GetValueFromNameValueList(&data,
		"NewProtocolType");	/* string */
	InMessage = GetValueFromNameValueList(&data,
		"NewInMessage");	/* base64 */

	if (ProtocolType == NULL || InMessage == NULL) {
		ClearNameValueList(&data);
		SoapError(h, 402, "Invalid Args");
		return;
	}
	/*if(strcmp(ProtocolType, "DeviceProtection:1") != 0)*/
	if (strcmp(ProtocolType, "WPS") != 0) {
		ClearNameValueList(&data);
		SoapError(h, 600, "Argument Value Invalid"); /* 703 ? */
		return;
	}
	/* TODO : put here code for WPS */

	bodylen = snprintf(body, sizeof(body), resp,
			   action,
			   "urn:schemas-upnp-org:service:DeviceProtection:1",
			   OutMessage, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
	ClearNameValueList(&data);
}

static void
GetSupportedProtocols(struct upnphttp *h, const char *action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewProtocolList>%s</NewProtocolList>"
		"</u:%sResponse>";
	char body[1024];
	int bodylen;
	const char *ProtocolList =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<SupportedProtocols xmlns=\"urn:schemas-upnp-org:gw:DeviceProtection\""
		" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
		" xsi:schemaLocation=\"urn:schemas-upnp-org:gw:DeviceProtection"
		" http://www.upnp.org/schemas/gw/DeviceProtection-v1.xsd\">"
		"<Introduction><Name>WPS</Name></Introduction>"
		"<Login><Name>PKCS5</Name></Login>"
		"</SupportedProtocols>";

	bodylen = snprintf(body, sizeof(body), resp,
			   action,
			   "urn:schemas-upnp-org:service:DeviceProtection:1",
			   ProtocolList, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetAssignedRoles(struct upnphttp *h, const char *action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewRoleList>%s</NewRoleList>"
		"</u:%sResponse>";
	char body[1024];
	int bodylen;
	const char *RoleList = "Public"; /* list of roles separated by spaces */

#ifdef ENABLE_HTTPS
	if (h->ssl != NULL) {
		/* we should get the Roles of the session (based on client certificate) */
		X509 *peercert;
		peercert = SSL_get_peer_certificate(h->ssl);
		if (peercert != NULL) {
			RoleList = "Admin Basic";
			X509_free(peercert);
		}
	}
#endif

	bodylen = snprintf(body, sizeof(body), resp,
			   action, "urn:schemas-upnp-org:service:DeviceProtection:1",
			   RoleList, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}
#endif

/* Windows XP as client send the following requests :
 * GetConnectionTypeInfo
 * GetNATRSIPStatus
 * ? GetTotalBytesSent - WANCommonInterfaceConfig
 * ? GetTotalBytesReceived - idem
 * ? GetTotalPacketsSent - idem
 * ? GetTotalPacketsReceived - idem
 * GetCommonLinkProperties - idem
 * GetStatusInfo - WANIPConnection
 * GetExternalIPAddress
 * QueryStateVariable / ConnectionStatus!
 */
static const struct
{
	const char *methodName;
	void (*methodImpl)(struct upnphttp *, const char *, const char *);
}
soapMethods[] = {
	{ 0, 0 }
};

void
ExecuteSoapAction(struct upnphttp *h, const char *action, int n)
{
	char *p;
	char *p2;
	int i, len, methodlen;
	char namespace[256];

	/* SoapAction example :
	 * urn:schemas-upnp-org:service:WANIPConnection:1#GetStatusInfo */
	p = strchr(action, '#');
	if (p && (p - action) < n) {
		for (i = 0; i < ((int)sizeof(namespace) - 1) && (action + i) < p; i++)
			namespace[i] = action[i];
		namespace[i] = '\0';
		p++;
		p2 = strchr(p, '"');
		if (p2 && (p2 - action) <= n)
			methodlen = p2 - p;
		else
			methodlen = n - (p - action);
		/*syslog(LOG_DEBUG, "SoapMethod: %.*s %d %d %p %p %d",
		       methodlen, p, methodlen, n, action, p, (int)(p - action));*/
		for (i = 0; soapMethods[i].methodName; i++) {
			len = strlen(soapMethods[i].methodName);
			if ((len == methodlen) && memcmp(p, soapMethods[i].methodName, len) == 0) {
#ifdef DEBUG
				syslog(LOG_DEBUG, "Remote Call of SoapMethod '%s'",
				       soapMethods[i].methodName);
#endif /* DEBUG */
				soapMethods[i].methodImpl(h, soapMethods[i].methodName,
					namespace);
				return;
			}
		}
		syslog(LOG_NOTICE,
			"SoapMethod: Unknown: %.*s", methodlen, p);
	} else {
		syslog(LOG_NOTICE,
			"cannot parse SoapAction");
	}

	SoapError(h, 401, "Invalid Action");
}

/* Standard Errors:
 *
 * errorCode errorDescription Description
 * --------	---------------- -----------
 * 401		Invalid Action	No action by that name at this service.
 * 402		Invalid Args	Could be any of the following: not enough in args,
 *							too many in args, no in arg by that name,
 *							one or more in args are of the wrong data type.
 * 403		Out of Sync	Out of synchronization.
 * 501		Action Failed	May be returned in current state of service
 *							prevents invoking that action.
 * 600-699	TBD			Common action errors. Defined by UPnP Forum
 *							Technical Committee.
 * 700-799	TBD			Action-specific errors for standard actions.
 *							Defined by UPnP Forum working committee.
 * 800-899	TBD			Action-specific errors for non-standard actions.
 *							Defined by UPnP vendor.
*/
void
SoapError(struct upnphttp *h, int errCode, const char *errDesc)
{
	static const char resp[] =
		"<s:Envelope "
		"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>"
		"<s:Fault>"
		"<faultcode>s:Client</faultcode>"
		"<faultstring>UPnPError</faultstring>"
		"<detail>"
		"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
		"<errorCode>%d</errorCode>"
		"<errorDescription>%s</errorDescription>"
		"</UPnPError>"
		"</detail>"
		"</s:Fault>"
		"</s:Body>"
		"</s:Envelope>";

	char body[2048];
	int bodylen;

	syslog(LOG_INFO,
		"Returning UPnPError %d: %s", errCode, errDesc);
	bodylen = snprintf(body, sizeof(body),
		resp, errCode, errDesc);
	BuildResp2_upnphttp(h, 500,
		"Internal Server Error", body, bodylen);
	SendRespAndClose_upnphttp(h);
}

