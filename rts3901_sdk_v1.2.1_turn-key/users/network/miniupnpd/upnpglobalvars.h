/* $Id: upnpglobalvars.h,v 1.41 2014/05/22 07:51:08 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2014 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef UPNPGLOBALVARS_H_INCLUDED
#define UPNPGLOBALVARS_H_INCLUDED

#include <time.h>
#include "upnppermissions.h"
#include "miniupnpdtypes.h"
#include "config.h"

/* name of the network interface used to acces internet */
extern const char *ext_if_name;

/* file to store all leases */
#ifdef ENABLE_LEASEFILE
extern const char *lease_file;
#endif

/* forced ip address to use for this interface
 * when NULL, getifaddr() is used */
extern const char *use_ext_ip_addr;

/* parameters to return to upnp client when asked */
extern unsigned long downstream_bitrate;
extern unsigned long upstream_bitrate;

/* statup time */
extern time_t startup_time;

extern unsigned long int min_lifetime;
extern unsigned long int max_lifetime;

/* runtime boolean flags */
extern int runtime_flags;
#define LOGPACKETSMASK		0x0001
#define SYSUPTIMEMASK		0x0002
#define CHECKCLIENTIPMASK	0x0008
#define SECUREMODEMASK		0x0010

#define ENABLEUPNPMASK		0x0020

#ifdef ENABLE_IPV6
#define IPV6DISABLEDMASK	0x0080
#endif
#ifdef ENABLE_6FC_SERVICE
#define IPV6FCFWDISABLEDMASK		0x0100
#define IPV6FCINBOUNDDISALLOWEDMASK	0x0200
#endif

#define SETFLAG(mask)	(runtime_flags |= mask)
#define GETFLAG(mask)	(runtime_flags & mask)
#define CLEARFLAG(mask)	(runtime_flags &= ~mask)

extern const char *pidfilename;

extern char uuidvalue_ipcam[];	/* uuid of root device (IGD) */
extern char uuidvalue_wan[];	/* uuid of WAN Device */
extern char uuidvalue_wcd[];	/* uuid of WAN Connection Device */

#define SERIALNUMBER_MAX_LEN (10)
extern char serialnumber[];

#define MODELNUMBER_MAX_LEN (48)
extern char modelnumber[];

#define PRESENTATIONURL_MAX_LEN (64)
extern char presentationurl[];

#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
#define FRIENDLY_NAME_MAX_LEN (64)
extern char friendly_name[];
#endif

/* UPnP permission rules : */
extern struct upnpperm *upnppermlist;
extern unsigned int num_upnpperm;

/* For automatic removal of expired rules (with LeaseDuration) */
extern unsigned int nextruletoclean_timestamp;

/* lan addresses to listen to SSDP traffic */
extern struct lan_addr_list lan_addrs;

#ifdef ENABLE_IPV6
/* ipv6 address used for HTTP */
extern char ipv6_addr_for_http_with_brackets[64];

/* address used to bind local services */
extern struct in6_addr ipv6_bind_addr;

#endif

extern const char *minissdpdsocketpath;

/* BOOTID.UPNP.ORG and CONFIGID.UPNP.ORG */
extern unsigned int upnp_bootid;
extern unsigned int upnp_configid;

#endif

