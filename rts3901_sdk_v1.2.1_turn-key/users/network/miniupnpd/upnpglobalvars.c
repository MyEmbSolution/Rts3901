/* $Id: upnpglobalvars.c,v 1.39 2014/12/10 09:49:22 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2014 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <sys/types.h>
#include <netinet/in.h>

#include "config.h"
#include "upnpglobalvars.h"
#include "upnpdescstrings.h"

/* network interface for internet */
const char *ext_if_name = "br0";

/* forced ip address to use for this interface
 * when NULL, getifaddr() is used */
const char *use_ext_ip_addr = 0;

unsigned long downstream_bitrate = 0;
unsigned long upstream_bitrate = 0;

/* startup time */
time_t startup_time = 0;

int runtime_flags = 0;

const char *pidfilename = "/var/run/miniupnpd.pid";

char uuidvalue_ipcam[] = "uuid:00000000-0000-0000-0000-000000000000";
char uuidvalue_wan[] = "uuid:00000000-0000-0000-0000-000000000000";
char uuidvalue_wcd[] = "uuid:00000000-0000-0000-0000-000000000000";
char serialnumber[SERIALNUMBER_MAX_LEN] = "00000000";

char modelnumber[MODELNUMBER_MAX_LEN] = "v0.8";

/* presentation url :
 * http://nnn.nnn.nnn.nnn:ppppp/  => max 30 bytes including terminating 0 */
char presentationurl[PRESENTATIONURL_MAX_LEN];

#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
/* friendly name for root devices in XML description */
char friendly_name[FRIENDLY_NAME_MAX_LEN] = "RTS3901-000000";
#endif

/* UPnP permission rules : */
struct upnpperm *upnppermlist = 0;
unsigned int num_upnpperm = 0;

/* For automatic removal of expired rules (with LeaseDuration) */
unsigned int nextruletoclean_timestamp = 0;

struct lan_addr_list lan_addrs;

#ifdef ENABLE_IPV6
/* ipv6 address used for HTTP */
char ipv6_addr_for_http_with_brackets[64];

/* address used to bind local services */
struct in6_addr ipv6_bind_addr;
#endif

/* Path of the Unix socket used to communicate with MiniSSDPd */
const char *minissdpdsocketpath = "/var/run/minissdpd.sock";

/* BOOTID.UPNP.ORG and CONFIGID.UPNP.ORG */
/* See UPnP Device Architecture v1.1 section 1.2 Advertisement :
 * The field value of the BOOTID.UPNP.ORG header field MUST be increased
 * each time a device (re)joins the network and sends an initial announce
 * (a "reboot" in UPnP terms), or adds a UPnP-enabled interface.
 * Unless the device explicitly announces a change in the BOOTID.UPNP.ORG
 * field value using an SSDP message, as long as the device remains
 * continuously available in the network, the same BOOTID.UPNP.ORG field
 * value MUST be used in all repeat announcements, search responses,
 * update messages and eventually bye-bye messages. */
unsigned int upnp_bootid = 1;      /* BOOTID.UPNP.ORG */
/* The field value of the CONFIGID.UPNP.ORG header field identifies the
 * current set of device and service descriptions; control points can
 * parse this header field to detect whether they need to send new
 * description query messages. */
/* UPnP 1.1 devices MAY freely assign configid numbers from 0 to
 * 16777215 (2^24-1). Higher numbers are reserved for future use, and
 * can be assigned by the Technical Committee. The configuration of a
 * root device consists of the following information: the DDD of the
 * root device and all its embedded devices, and the SCPDs of all the
 * contained services. If any part of the configuration changes, the
 * CONFIGID.UPNP.ORG field value MUST be changed.
 * DDD = Device Description Document
 * SCPD = Service Control Protocol Description */
unsigned int upnp_configid = 1337; /* CONFIGID.UPNP.ORG */

