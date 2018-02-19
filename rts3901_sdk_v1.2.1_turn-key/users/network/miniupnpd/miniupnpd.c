/* $Id: miniupnpd.c,v 1.211 2015/08/26 08:04:46 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2015 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <syslog.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/param.h>

#if defined(sun)
#include <kstat.h>
#elif !defined(__linux__)
/* for BSD's sysctl */
#include <sys/sysctl.h>
#endif
#include <rtsnm.h>

#include "config.h"
#include "miniupnp.h"
#include "macros.h"
#include "upnpglobalvars.h"
#include "upnphttp.h"
#include "upnpdescgen.h"
#include "miniupnpdpath.h"
#include "getifaddr.h"
#include "upnpsoap.h"
#include "options.h"
#include "minissdp.h"
#include "miniupnpdtypes.h"
#include "daemonize.h"
#include "upnpevents.h"
#include "asyncsendto.h"
#include "commonrdr.h"
#include "upnputils.h"
#ifdef USE_IFACEWATCHER
#include "ifacewatcher.h"
#endif

#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG	"/var/nm/miniupnpd.conf"
#endif

#define MAC_LENGTH 6

/* variables used by signals */
static volatile sig_atomic_t quitting;
volatile sig_atomic_t should_send_public_address_change_notif = 0;
static volatile sig_atomic_t network_type = 0;

/* OpenAndConfHTTPSocket() :
 * setup the socket used to handle incoming HTTP connections. */
static int
#ifdef ENABLE_IPV6
OpenAndConfHTTPSocket(unsigned short *port, int ipv6)
#else
OpenAndConfHTTPSocket(unsigned short *port)
#endif
{
	int s;
	int i = 1;
#ifdef ENABLE_IPV6
	struct sockaddr_in6 listenname6;
	struct sockaddr_in listenname4;
#else
	struct sockaddr_in listenname;
#endif
	socklen_t listenname_len;

	s = socket(
#ifdef ENABLE_IPV6
		   ipv6 ? PF_INET6 : PF_INET,
#else
		   PF_INET,
#endif
		   SOCK_STREAM, 0);
#ifdef ENABLE_IPV6
	if (s < 0 && ipv6 && errno == EAFNOSUPPORT) {
		/* the system doesn't support IPV6 */
		syslog(LOG_WARNING, "socket(PF_INET6, ...) failed with EAFNOSUPPORT, disabling IPv6");
		SETFLAG(IPV6DISABLEDMASK);
		ipv6 = 0;
		s = socket(PF_INET, SOCK_STREAM, 0);
	}
#endif
	if (s < 0) {
		syslog(LOG_ERR, "socket(http): %m");
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) {
		syslog(LOG_WARNING, "setsockopt(http, SO_REUSEADDR): %m");
	}
#if 0
	/* enable this to force IPV6 only for IPV6 socket.
	 * see http://www.ietf.org/rfc/rfc3493.txt section 5.3 */
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &i, sizeof(i)) < 0) {
		syslog(LOG_WARNING, "setsockopt(http, IPV6_V6ONLY): %m");
	}
#endif

	if (!set_non_blocking(s)) {
		syslog(LOG_WARNING, "set_non_blocking(http): %m");
	}

#ifdef ENABLE_IPV6
	if (ipv6) {
		memset(&listenname6, 0, sizeof(struct sockaddr_in6));
		listenname6.sin6_family = AF_INET6;
		listenname6.sin6_port = htons(*port);
		listenname6.sin6_addr = ipv6_bind_addr;
		listenname_len =  sizeof(struct sockaddr_in6);
	} else {
		memset(&listenname4, 0, sizeof(struct sockaddr_in));
		listenname4.sin_family = AF_INET;
		listenname4.sin_port = htons(*port);
		listenname4.sin_addr.s_addr = htonl(INADDR_ANY);
		listenname_len =  sizeof(struct sockaddr_in);
	}
#else
	memset(&listenname, 0, sizeof(struct sockaddr_in));
	listenname.sin_family = AF_INET;
	listenname.sin_port = htons(*port);
	listenname.sin_addr.s_addr = htonl(INADDR_ANY);
	listenname_len =  sizeof(struct sockaddr_in);
#endif

#if defined(SO_BINDTODEVICE) && !defined(MULTIPLE_EXTERNAL_IP)
	/* One and only one LAN interface */
	if (lan_addrs.lh_first != NULL && lan_addrs.lh_first->list.le_next == NULL
	   && strlen(lan_addrs.lh_first->ifname) > 0) {
		if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE,
			      lan_addrs.lh_first->ifname,
			      strlen(lan_addrs.lh_first->ifname)) < 0)
			syslog(LOG_WARNING, "setsockopt(http, SO_BINDTODEVICE, %s): %m",
				lan_addrs.lh_first->ifname);
	}
#endif /* defined(SO_BINDTODEVICE) && !defined(MULTIPLE_EXTERNAL_IP) */

#ifdef ENABLE_IPV6
	if (bind(s,
		ipv6 ? (struct sockaddr *)&listenname6 : (struct sockaddr *)&listenname4,
		listenname_len) < 0) {
#else
	if (bind(s, (struct sockaddr *)&listenname, listenname_len) < 0) {
#endif
		syslog(LOG_ERR, "bind(http): %m");
		close(s);
		return -1;
	}

	if (listen(s, 5) < 0) {
		syslog(LOG_ERR, "listen(http): %m");
		close(s);
		return -1;
	}

	if (*port == 0) {
#ifdef ENABLE_IPV6
		if (ipv6) {
			struct sockaddr_in6 sockinfo;
			socklen_t len = sizeof(struct sockaddr_in6);
			if (getsockname(s, (struct sockaddr *)&sockinfo, &len) < 0) {
				syslog(LOG_ERR, "getsockname(): %m");
			} else {
				*port = ntohs(sockinfo.sin6_port);
			}
		} else {
#endif /* ENABLE_IPV6 */
			struct sockaddr_in sockinfo;
			socklen_t len = sizeof(struct sockaddr_in);
			if (getsockname(s, (struct sockaddr *)&sockinfo, &len) < 0) {
				syslog(LOG_ERR, "getsockname(): %m");
			} else {
				*port = ntohs(sockinfo.sin_port);
			}
#ifdef ENABLE_IPV6
		}
#endif /* ENABLE_IPV6 */
	}
	return s;
}

static struct upnphttp *
ProcessIncomingHTTP(int shttpl, const char *protocol)
{
	int shttp;
	socklen_t clientnamelen;
#ifdef ENABLE_IPV6
	struct sockaddr_storage clientname;
	clientnamelen = sizeof(struct sockaddr_storage);
#else
	struct sockaddr_in clientname;
	clientnamelen = sizeof(struct sockaddr_in);
#endif
	shttp = accept(shttpl, (struct sockaddr *)&clientname, &clientnamelen);
	if (shttp < 0) {
		/* ignore EAGAIN, EWOULDBLOCK, EINTR, we just try again later */
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			syslog(LOG_ERR, "accept(http): %m");
	} else {
		struct upnphttp *tmp = 0;
		char addr_str[64];

		sockaddr_to_string((struct sockaddr *)&clientname, addr_str, sizeof(addr_str));
		syslog(LOG_INFO, "%s connection from %s", protocol, addr_str);
		if (get_lan_for_peer((struct sockaddr *)&clientname) == NULL) {
			/* The peer is not a LAN ! */
			syslog(LOG_WARNING,
			       "%s peer %s is not from a LAN, closing the connection",
			       protocol, addr_str);
			close(shttp);
		} else {
			/* Create a new upnphttp object and add it to
			 * the active upnphttp object list */
			tmp = New_upnphttp(shttp);
			if (tmp) {
#ifdef ENABLE_IPV6
				if (clientname.ss_family == AF_INET) {
					tmp->clientaddr = ((struct sockaddr_in *)&clientname)->sin_addr;
				} else if (clientname.ss_family == AF_INET6) {
					struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&clientname;
					if (IN6_IS_ADDR_V4MAPPED(&addr->sin6_addr)) {
						memcpy(&tmp->clientaddr,
						       &addr->sin6_addr.s6_addr[12],
						       4);
					} else {
						tmp->ipv6 = 1;
						memcpy(&tmp->clientaddr_v6,
						       &addr->sin6_addr,
						       sizeof(struct in6_addr));
					}
				}
#else
				tmp->clientaddr = clientname.sin_addr;
#endif
				return tmp;
			} else {
				syslog(LOG_ERR, "New_upnphttp() failed");
				close(shttp);
			}
		}
	}
	return NULL;
}

/* Handler for the SIGTERM signal (kill)
 * SIGINT is also handled */
static void
sigterm(int sig)
{
	UNUSED(sig);
	/*int save_errno = errno; */
	/*signal(sig, SIG_IGN);*/	/* Ignore this signal while we are quitting */
	/* Note : isn't it useless ? */

#if 0
	/* calling syslog() is forbidden in signal handler according to
	 * signal(3) */
	syslog(LOG_NOTICE, "received signal %d, good-bye", sig);
#endif

	quitting = 1;
	/*errno = save_errno;*/
}

/* Handler for the SIGUSR1 signal indicating public IP address change. */
static void
sigusr1(int sig)
{
	UNUSED(sig);
#if 0
	/* calling syslog() is forbidden in signal handler according to
	 * signal(3) */
	syslog(LOG_INFO, "received signal %d, public ip address change", sig);
#endif

	should_send_public_address_change_notif = 1;
}

/* record the startup time, for returning uptime */
static void
set_startup_time(int sysuptime)
{
	startup_time = time(NULL);
#ifdef USE_TIME_AS_BOOTID
	if (startup_time > 60*60*24 && upnp_bootid == 1) {
		/* We know we are not January the 1st 1970 */
		upnp_bootid = (unsigned int)startup_time;
		/* from UDA v1.1 :
		 * A convenient mechanism is to set this field value to the time
		 * that the device sends its initial announcement, expressed as
		 * seconds elapsed since midnight January 1, 1970; */
	}
#endif /* USE_TIME_AS_BOOTID */
	if (sysuptime) {
		/* use system uptime instead of daemon uptime */
#if defined(__linux__)
		char buff[64];
		int uptime = 0, fd;
		fd = open("/proc/uptime", O_RDONLY);
		if (fd < 0) {
			syslog(LOG_ERR, "open(\"/proc/uptime\" : %m");
		} else {
			memset(buff, 0, sizeof(buff));
			if (read(fd, buff, sizeof(buff) - 1) < 0) {
				syslog(LOG_ERR, "read(\"/proc/uptime\" : %m");
			} else {
				uptime = atoi(buff);
				syslog(LOG_INFO, "system uptime is %d seconds", uptime);
			}
			close(fd);
			startup_time -= uptime;
		}
#elif defined(SOLARIS_KSTATS)
		kstat_ctl_t *kc;
		kc = kstat_open();
		if (kc != NULL) {
			kstat_t *ksp;
			ksp = kstat_lookup(kc, "unix", 0, "system_misc");
			if (ksp && (kstat_read(kc, ksp, NULL) != -1)) {
				void *ptr = kstat_data_lookup(ksp, "boot_time");
				if (ptr)
					memcpy(&startup_time, ptr, sizeof(startup_time));
				else
					syslog(LOG_ERR, "cannot find boot_time kstat");
			} else {
				syslog(LOG_ERR, "cannot open kstats for unix/0/system_misc: %m");
			}
			kstat_close(kc);
		}
#else
		struct timeval boottime;
		size_t size = sizeof(boottime);
		int name[2] = { CTL_KERN, KERN_BOOTTIME };
		if (sysctl(name, 2, &boottime, &size, NULL, 0) < 0) {
			syslog(LOG_ERR, "sysctl(\"kern.boottime\") failed");
		} else {
			startup_time = boottime.tv_sec;
		}
#endif
	}
}

/**
structure containing variables used during "main loop"
 * that are filled during the init
 */
struct runtime_vars {
	/* LAN IP addresses for SSDP traffic and HTTP */
	/* moved to global vars */
	int port;	/* HTTP Port */
#ifdef ENABLE_HTTPS
	int https_port;	/* HTTPS Port */
#endif
	int notify_interval;	/* seconds between SSDP announces */
	/* unused rules cleaning related variables : */
	int clean_ruleset_threshold;	/* threshold for removing unused rules */
	int clean_ruleset_interval;		/* (minimum) interval between checks */
};

/**
parselanaddr()
 * parse address with mask
 * ex: 192.168.1.1/24 or 192.168.1.1/255.255.255.0
 * When MULTIPLE_EXTERNAL_IP is enabled, the ip address of the
 * external interface associated with the lan subnet follows.
 * ex : 192.168.1.1/24 81.21.41.11
 *
 * Can also use the interface name (ie eth0)
 *
 * return value :
 *    0 : ok
 *   -1 : error
 */
static int
parselanaddr(struct lan_addr_s *lan_addr, const char *str)
{
	const char *p;
	int n;
	char tmp[16];

	memset(lan_addr, 0, sizeof(struct lan_addr_s));
	p = str;
	while (*p && *p != '/' && !isspace(*p))
		p++;
	n = p - str;
	if (!isdigit(str[0]) && n < (int)sizeof(lan_addr->ifname)) {
		/* not starting with a digit : suppose it is an interface name */
		memcpy(lan_addr->ifname, str, n);
		lan_addr->ifname[n] = '\0';
		if (getifaddr(lan_addr->ifname, lan_addr->str, sizeof(lan_addr->str),
			     &lan_addr->addr, &lan_addr->mask) < 0) {
#ifdef ENABLE_IPV6
			fprintf(stderr, "interface \"%s\" has no IPv4 address\n", str);
			lan_addr->str[0] = '\0';
			lan_addr->addr.s_addr = htonl(0x00000000u);
			lan_addr->mask.s_addr = htonl(0xffffffffu);
#else /* ENABLE_IPV6 */
			goto parselan_error;
#endif /* ENABLE_IPV6 */
		}
		/*printf("%s => %s\n", lan_addr->ifname, lan_addr->str);*/
	} else {
		if (n > 15)
			goto parselan_error;
		memcpy(lan_addr->str, str, n);
		lan_addr->str[n] = '\0';
		if (!inet_aton(lan_addr->str, &lan_addr->addr))
			goto parselan_error;
	}
	if (*p == '/') {
		const char *q = ++p;
		while (*p && isdigit(*p))
			p++;
		if (*p == '.') {
			/* parse mask in /255.255.255.0 format */
			while (*p && (*p == '.' || isdigit(*p)))
				p++;
			n = p - q;
			if (n > 15)
				goto parselan_error;
			memcpy(tmp, q, n);
			tmp[n] = '\0';
			if (!inet_aton(tmp, &lan_addr->mask))
				goto parselan_error;
		} else {
			/* it is a /24 format */
			int nbits = atoi(q);
			if (nbits > 32 || nbits < 0)
				goto parselan_error;
			lan_addr->mask.s_addr = htonl(nbits ? (0xffffffffu << (32 - nbits)) : 0);
		}
	} else if (lan_addr->mask.s_addr == 0) {
		/* by default, networks are /24 */
		lan_addr->mask.s_addr = htonl(0xffffff00u);
	}
#ifdef ENABLE_IPV6
	if (lan_addr->ifname[0] != '\0') {
		lan_addr->index = if_nametoindex(lan_addr->ifname);
		if (lan_addr->index == 0)
			fprintf(stderr, "Cannot get index for network interface %s",
				lan_addr->ifname);
	} else {
		fprintf(stderr,
			"Error: please specify LAN network interface by name instead of IPv4 address : %s\n",
			str);
		return -1;
	}
#endif
	return 0;
parselan_error:
	fprintf(stderr, "Error parsing address/mask (or interface name) : %s\n",
		str);
	return -1;
}

/* fill uuidvalue_wan and uuidvalue_wcd based on uuidvalue_ipcam */
void complete_uuidvalues(void)
{
	size_t len;
	len = strlen(uuidvalue_ipcam);
	memcpy(uuidvalue_wan, uuidvalue_ipcam, len+1);
	switch (uuidvalue_wan[len-1]) {
	case '9':
		uuidvalue_wan[len-1] = 'a';
		break;
	case 'f':
		uuidvalue_wan[len-1] = '0';
		break;
	default:
		uuidvalue_wan[len-1]++;
	}
	memcpy(uuidvalue_wcd, uuidvalue_wan, len+1);
	switch (uuidvalue_wcd[len-1]) {
	case '9':
		uuidvalue_wcd[len-1] = 'a';
		break;
	case 'f':
		uuidvalue_wcd[len-1] = '0';
		break;
	default:
		uuidvalue_wcd[len-1]++;
	}
}

void GetDevUUID(char *uuid, int length)
{
	int ret;
	unsigned char mac_addr[MAC_LENGTH] = {0};

	ret = rts_nm_get_info(MSG_ETH_MAC, mac_addr, MAC_LENGTH);
	if (ret < 0) {
		syslog(LOG_ERR, "Failed to get MAC ADDR");
	}

	sprintf(uuid,
		"20140201-2ed2-d7e4-10bf-%02X%02X%02X%02X%02X%02X",
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);
}

void GetDevSerialNum(char *serialnum, int length)
{
	int ret;
	unsigned int mac_addr[MAC_LENGTH] = {0};

	ret = rts_nm_get_info(MSG_ETH_MAC, mac_addr, MAC_LENGTH);
	if (ret < 0) {
		syslog(LOG_ERR, "Failed to get MAC ADDR");
	}

	sprintf(serialnum,
		"%02X%02X%02X%02X%02X%02X",
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4]);
}

int GetNetworkInfo(char *ifname, void *msg_buf)
{
	int ret = 0;
	struct network_cfg *network_cfg_s = (struct network_cfg *) msg_buf;

	switch (network_type) {
	case 1: {
		struct nm_lan lan_cfg_s;
		while (1) {
			ret = rts_nm_get_info(MSG_LAN_INFO,
				&lan_cfg_s,
				sizeof(lan_cfg_s));
			if (ret < 0) {
				/*syslog(LOG_ERR, "network is not ready yet\n");*/
				sleep(1);
			} else {
				network_cfg_s->ipaddr.s_addr = lan_cfg_s.ipaddr.s_addr;
				network_cfg_s->netmask.s_addr = lan_cfg_s.netmask.s_addr;
				break;
			}
		}
		break;
	}
	default: {
		struct nm_wan wan_cfg_s;
		while (1) {
			ret = rts_nm_get_info(MSG_WAN_INFO,
				&wan_cfg_s,
				sizeof(wan_cfg_s));
			if (ret < 0) {
				/*syslog(LOG_ERR, "network is not ready yet\n");*/
				sleep(1);
			} else {
				network_cfg_s->ipaddr.s_addr = wan_cfg_s.ipaddr.s_addr;
				network_cfg_s->netmask.s_addr = wan_cfg_s.netmask.s_addr;
				break;
			}
		}
		break;
	}
	}
	return ret;
}

/**
init phase :
 * 1) read configuration file
 * 2) read command line arguments
 * 3) daemonize
 * 4) open syslog
 * 5) check and write pid file
 * 6) set startup time stamp
 * 7) compute presentation URL
 * 8) set signal handlers
 * 9) init random generator (srandom())
 * 10) init redirection engine
 * 11) reload mapping from leasefile
 */
static int
init(int argc, char **argv, struct runtime_vars *v)
{
	int i;
	int pid;
	int debug_flag = 0;
	int openlog_option;
	struct sigaction sa;
	/*const char * logfilename = 0;*/
	const char *presurl = 0;
#ifndef DISABLE_CONFIG_FILE
	int options_flag = 0;
	const char *optionsfile = DEFAULT_CONFIG;
#endif /* DISABLE_CONFIG_FILE */
	struct lan_addr_s *lan_addr;
	struct lan_addr_s *lan_addr2;
	struct lan_addr_s *lan_addr3;
	struct network_cfg *network_cfg_s;
	char addr_string[40] = {0};
	char mask_string[16] = {0};

	/* only print usage if -h is used */
	for (i = 1; i < argc; i++) {
		if (0 == strcmp(argv[i], "-h"))
			goto print_usage;
	}

#ifndef DISABLE_CONFIG_FILE
	/* first check if "-f" option is used */
	for (i = 2; i < argc; i++) {
		if (0 == strcmp(argv[i-1], "-f")) {
			optionsfile = argv[i];
			options_flag = 1;
			break;
		}
	}
#endif /* DISABLE_CONFIG_FILE */

	/* set initial values */
	SETFLAG(ENABLEUPNPMASK);	/* UPnP is enabled by default */
#ifdef ENABLE_IPV6
	ipv6_bind_addr = in6addr_any;
#endif /* ENABLE_IPV6 */

	LIST_INIT(&lan_addrs);
	v->port = -1;
#ifdef ENABLE_HTTPS
	v->https_port = -1;
#endif
	v->notify_interval = 30;	/* seconds between SSDP announces */
	v->clean_ruleset_threshold = 20;
	v->clean_ruleset_interval = 0;	/* interval between ruleset check. 0=disabled */
#ifndef DISABLE_CONFIG_FILE
	/* read options file first since
	 * command line arguments have final say */
	if (readoptionsfile(optionsfile) < 0) {
		/* only error if file exists or using -f */
		if (access(optionsfile, F_OK) == 0 || options_flag)
			fprintf(stderr, "Error reading configuration file %s\n", optionsfile);
	} else {
		for (i = 0; i < (int)num_options; i++) {
			switch (ary_options[i].id) {
			case UPNPEXT_IFNAME:
				ext_if_name = ary_options[i].value;
				break;
			case UPNPEXT_IP:
				use_ext_ip_addr = ary_options[i].value;
				break;
			case UPNPLISTENING_IP:
				lan_addr = (struct lan_addr_s *) malloc(sizeof(struct lan_addr_s));
				if (lan_addr == NULL) {
					fprintf(stderr, "malloc(sizeof(struct lan_addr_s)): %m");
					break;
				}
				if (parselanaddr(lan_addr, ary_options[i].value) != 0) {
					fprintf(stderr, "can't parse \"%s\" as a valid "
#ifndef ENABLE_IPV6
						"LAN address or "
#endif
						"interface name\n", ary_options[i].value);
					free(lan_addr);
					break;
				}
				LIST_INSERT_HEAD(&lan_addrs, lan_addr, list);
				break;
#ifdef ENABLE_IPV6
			case UPNPIPV6_LISTENING_IP:
				if (inet_pton(AF_INET6, ary_options[i].value, &ipv6_bind_addr) < 1) {
					fprintf(stderr, "can't parse \"%s\" as valid IPv6 listening address", ary_options[i].value);
				}
				break;
#endif /* ENABLE_IPV6 */
			case UPNPPORT:
				v->port = atoi(ary_options[i].value);
				break;
#ifdef ENABLE_HTTPS
			case UPNPHTTPSPORT:
				v->https_port = atoi(ary_options[i].value);
				break;
#endif
			case UPNPBITRATE_UP:
				upstream_bitrate = strtoul(ary_options[i].value, 0, 0);
				break;
			case UPNPBITRATE_DOWN:
				downstream_bitrate = strtoul(ary_options[i].value, 0, 0);
				break;
			case UPNPPRESENTATIONURL:
				presurl = ary_options[i].value;
				break;
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
			case UPNPFRIENDLY_NAME:
				strncpy(friendly_name, ary_options[i].value, FRIENDLY_NAME_MAX_LEN);
				friendly_name[FRIENDLY_NAME_MAX_LEN-1] = '\0';
				break;
#endif	/* ENABLE_MANUFACTURER_INFO_CONFIGURATION */
			case UPNPNOTIFY_INTERVAL:
				v->notify_interval = atoi(ary_options[i].value);
				break;
			case UPNPSYSTEM_UPTIME:
				if (strcmp(ary_options[i].value, "yes") == 0)
					SETFLAG(SYSUPTIMEMASK);	/*sysuptime = 1;*/
				break;
			case UPNPUUID:
				strncpy(uuidvalue_ipcam+5, ary_options[i].value,
					strlen(uuidvalue_ipcam+5) + 1);
				complete_uuidvalues();
				break;
			case UPNPSERIAL:
				strncpy(serialnumber, ary_options[i].value, SERIALNUMBER_MAX_LEN);
				serialnumber[SERIALNUMBER_MAX_LEN-1] = '\0';
				break;
			case UPNPMODEL_NUMBER:
				strncpy(modelnumber, ary_options[i].value, MODELNUMBER_MAX_LEN);
				modelnumber[MODELNUMBER_MAX_LEN-1] = '\0';
				break;
			case UPNPCLEANTHRESHOLD:
				v->clean_ruleset_threshold = atoi(ary_options[i].value);
				break;
			case UPNPCLEANINTERVAL:
				v->clean_ruleset_interval = atoi(ary_options[i].value);
				break;
			case UPNPSECUREMODE:
				if (strcmp(ary_options[i].value, "yes") == 0)
					SETFLAG(SECUREMODEMASK);
				break;
			case UPNPMINISSDPDSOCKET:
				minissdpdsocketpath = ary_options[i].value;
				break;
			case UPNPENABLE:
				break;
			default:
				fprintf(stderr, "Unknown option in file %s\n",
					optionsfile);
			}
		}
	}
#endif /* DISABLE_CONFIG_FILE */

	/* command line arguments processing */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
		} else
		switch (argv[i][1]) {
		case 'b':
			if (i + 1 < argc)
				upnp_bootid = (unsigned int)strtoul(argv[++i], NULL, 10);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'o':
			if (i + 1 < argc)
				use_ext_ip_addr = argv[++i];
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 't':
			if (i + 1 < argc)
				v->notify_interval = atoi(argv[++i]);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'r':
			if (i + 1 < argc)
				v->clean_ruleset_interval = atoi(argv[++i]);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'u':
			if (i + 1 < argc) {
				strncpy(uuidvalue_ipcam+5, argv[++i], strlen(uuidvalue_ipcam+5) + 1);
				complete_uuidvalues();
			} else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
		case 'z':
			if (i + 1 < argc)
				strncpy(friendly_name, argv[++i], FRIENDLY_NAME_MAX_LEN);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			friendly_name[FRIENDLY_NAME_MAX_LEN-1] = '\0';
			break;
#endif	/* ENABLE_MANUFACTURER_INFO_CONFIGURATION */
		case 's':
			if (i + 1 < argc)
				strncpy(serialnumber, argv[++i], SERIALNUMBER_MAX_LEN);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			serialnumber[SERIALNUMBER_MAX_LEN-1] = '\0';
			break;
		case 'm':
			if (i + 1 < argc)
				strncpy(modelnumber, argv[++i], MODELNUMBER_MAX_LEN);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			modelnumber[MODELNUMBER_MAX_LEN-1] = '\0';
			break;
		case 'U':
			/*sysuptime = 1;*/
			SETFLAG(SYSUPTIMEMASK);
			break;
		case 'S':
			SETFLAG(SECUREMODEMASK);
			break;
		case 'i':
			if (i + 1 < argc)
				ext_if_name = argv[++i];
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'N':
			if (i + 1 < argc) {
				if (strcmp(argv[++i], "LAN") == 0)
					network_type = 1;
				else
					network_type = 0;
			} else {
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			}
			break;
		case 'p':
			if (i + 1 < argc)
				v->port = atoi(argv[++i]);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
#ifdef ENABLE_HTTPS
		case 'H':
			if (i + 1 < argc)
				v->https_port = atoi(argv[++i]);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
#endif	/* ENABLE_HTTPS */
		case 'P':
			if (i + 1 < argc)
				pidfilename = argv[++i];
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'd':
			debug_flag = 1;
			break;
		case 'w':
			if (i + 1 < argc)
				presurl = argv[++i];
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'B':
			if (i + 2 < argc) {
				downstream_bitrate = strtoul(argv[++i], 0, 0);
				upstream_bitrate = strtoul(argv[++i], 0, 0);
			} else {
				fprintf(stderr, "Option -%c takes two arguments.\n", argv[i][1]);
			}
			break;
		case 'a':
			if (i + 1 < argc) {
				i++;
				lan_addr = (struct lan_addr_s *) malloc(sizeof(struct lan_addr_s));
				if (lan_addr == NULL) {
					fprintf(stderr, "malloc(sizeof(struct lan_addr_s)): %m");
					break;
				}
				if (parselanaddr(lan_addr, argv[i]) != 0) {
					fprintf(stderr, "can't parse \"%s\" as a valid "
#ifndef ENABLE_IPV6
						"LAN address or "
#endif	/* #ifndef ENABLE_IPV6 */
						"interface name\n", argv[i]);
					free(lan_addr);
					break;
				}
				/* check if we already have this address */
				for (lan_addr2 = lan_addrs.lh_first; lan_addr2 != NULL;
					lan_addr2 = lan_addr2->list.le_next) {
					if (0 == strncmp(lan_addr2->str, lan_addr->str, 15))
						break;
				}
				if (lan_addr2 == NULL)
					LIST_INSERT_HEAD(&lan_addrs, lan_addr, list);
			} else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'A':
			if (i + 1 < argc) {
				void *tmp;
				tmp = realloc(upnppermlist, sizeof(struct upnpperm) * (num_upnpperm+1));
				if (tmp == NULL) {
					fprintf(stderr, "memory allocation error for permission\n");
				} else {
					upnppermlist = tmp;
					if (read_permission_line(upnppermlist + num_upnpperm, argv[++i]) >= 0) {
						num_upnpperm++;
					} else {
						fprintf(stderr, "Permission rule parsing error :\n%s\n", argv[i]);
					}
				}
			} else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'f':
			i++;	/* discarding, the config file is already read */
			break;
		default:
			fprintf(stdout, "Unknown option: %s\n", argv[i]);
		}
	}

	if (debug_flag) {
		pid = getpid();
	} else {
#ifdef USE_DAEMON
		if (daemon(0, 0) < 0) {
			perror("daemon()");
		}
		pid = getpid();
#else
		pid = daemonize();
#endif
	}

	openlog_option = LOG_PID|LOG_CONS;
	if (debug_flag) {
		openlog_option |= LOG_PERROR;	/* also log on stderr */
	}
	openlog("miniupnpd", openlog_option, LOG_MINIUPNPD);
	if (!debug_flag) {
		/* speed things up and ignore LOG_INFO and LOG_DEBUG */
		setlogmask(LOG_UPTO(LOG_NOTICE));
	}

	if (checkforrunning(pidfilename) < 0) {
		syslog(LOG_ERR, "MiniUPnPd is already running. EXITING");
		return 1;
	}
	if (writepidfile(pidfilename, pid) < 0)
		pidfilename = NULL;

	/* customer for rs */
	GetDevUUID(uuidvalue_ipcam, strlen(uuidvalue_ipcam) + 1);
	strcpy(serialnumber, uuidvalue_ipcam+28);
	network_cfg_s = (struct network_cfg *) malloc(sizeof(struct network_cfg));
	if (!network_cfg_s) {
		fprintf(stderr, "malloc(sizeof(struct network_cfg)): %m");
		return 1;
	}
	GetNetworkInfo(ext_if_name, network_cfg_s);

	lan_addr3 = (struct lan_addr_s *) malloc(sizeof(struct lan_addr_s));
	if (lan_addr3 == NULL) {
		fprintf(stderr, "malloc(sizeof(struct lan_addr_s)): %m");
		return 1;
	}

	strcpy(addr_string, inet_ntoa(network_cfg_s->ipaddr));
	strcpy(mask_string, inet_ntoa(network_cfg_s->netmask));
	addr_string[strlen(addr_string)] = '/';
	strcat(addr_string,  mask_string);

	if (parselanaddr(lan_addr3, addr_string) != 0) {
		fprintf(stderr, "can't parse \"%s\" as a valid "
			 "interface name\n", argv[i]);
			free(lan_addr3);
			free(network_cfg_s);
			return 1;
	}
/* check if we already have this address */
	for (lan_addr2 = lan_addrs.lh_first; lan_addr2 != NULL;
		lan_addr2 = lan_addr2->list.le_next) {
		if (0 == strncmp(lan_addr2->str, lan_addr3->str, 15))
			break;
	}
	if (lan_addr2 == NULL)
		LIST_INSERT_HEAD(&lan_addrs, lan_addr3, list);

	if (!ext_if_name || !lan_addrs.lh_first) {
		/* bad configuration */
		goto print_usage;
	}

	set_startup_time(GETFLAG(SYSUPTIMEMASK));

	/* presentation url */
	if (presurl) {
		strncpy(presentationurl, presurl, PRESENTATIONURL_MAX_LEN);
		presentationurl[PRESENTATIONURL_MAX_LEN-1] = '\0';
	} else {
		snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
			 "http://%s:80", lan_addrs.lh_first->str);
	}

	/* set signal handler */
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sigterm;

	if (sigaction(SIGTERM, &sa, NULL) < 0) {
		syslog(LOG_ERR, "Failed to set %s handler. EXITING", "SIGTERM");
		return 1;
	}
	if (sigaction(SIGINT, &sa, NULL) < 0) {
		syslog(LOG_ERR, "Failed to set %s handler. EXITING", "SIGINT");
		return 1;
	}
	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) < 0) {
		syslog(LOG_ERR, "Failed to ignore SIGPIPE signals");
	}
	sa.sa_handler = sigusr1;
	if (sigaction(SIGUSR1, &sa, NULL) < 0) {
		syslog(LOG_NOTICE, "Failed to set %s handler", "SIGUSR1");
	}

	/* initialize random number generator */
	srandom((unsigned int)time(NULL));

#ifdef ENABLE_LEASEFILE
	/*remove(lease_file);*/
	syslog(LOG_INFO, "Reloading rules from lease file");
	reload_from_lease_file();
#endif

	return 0;
print_usage:
	fprintf(stderr, "Usage:\n\t"
		"%s "
#ifndef DISABLE_CONFIG_FILE
			"[-f config_file] "
#endif
			"[-i ext_ifname] [-o ext_ip]\n"
			"\t\t[-a listening_ip]"
#ifdef ENABLE_HTTPS
			" [-H https_port]"
#endif
			" [-p port] [-d]"
			" [-U] [-S]"
			"\n"
			/*"[-l logfile] " not functionnal */
			"\t\t[-u uuid] [-s serial] [-m model_number]\n"
			"\t\t[-t notify_interval] [-P pid_filename] "
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
			"[-z fiendly_name]"
#endif
			"[-N network type]"
			"\n\t\t[-B down up] [-w url] [-r clean_ruleset_interval]\n"
			"\t\t[-A \"permission rule\"] [-b BOOTID]\n"
		"\nNotes:\n\tThere can be one or several listening_ips.\n"
		"\tNotify interval is in seconds. Default is 30 seconds.\n"
			"\tDefault pid file is '%s'.\n"
			"\tDefault config file is '%s'.\n"
			"\tWith -d miniupnpd will run as a standard program.\n"
			"\t-U causes miniupnpd to report system uptime instead "
			"of daemon uptime.\n"
			"\t-B sets bitrates reported by daemon in bits per second.\n"
			"\t-b sets the value of BOOTID.UPNP.ORG SSDP header\n"
			"\t-h prints this help and quits.\n"
		"", argv[0], pidfilename, DEFAULT_CONFIG);
	return 1;
}

/* === main === */
/* process HTTP or SSDP requests */
int
miniupnpd(int argc, char **argv)
{
	int i;
	int shttpl = -1;	/* socket for HTTP */
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
	int shttpl_v4 = -1;	/* socket for HTTP (ipv4 only) */
#endif
#ifdef ENABLE_HTTPS
	int shttpsl = -1;	/* socket for HTTPS */
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
	int shttpsl_v4 = -1;	/* socket for HTTPS (ipv4 only) */
#endif
#endif /* ENABLE_HTTPS */
	int sudp = -1;		/* IP v4 socket for receiving SSDP */
#ifdef ENABLE_IPV6
	int sudpv6 = -1;	/* IP v6 socket for receiving SSDP */
#endif
#ifdef USE_IFACEWATCHER
	int sifacewatcher = -1;
#endif

	int *snotify = NULL;
	int addr_count;
	LIST_HEAD(httplisthead, upnphttp) upnphttphead;
	struct upnphttp *e = 0;
	struct upnphttp *next;
	fd_set readset;	/* for select() */
	fd_set writeset;
	struct timeval timeout, timeofday, lasttimeofday = {0, 0};
	int max_fd = -1;
	struct runtime_vars v;
	struct lan_addr_s *lan_addr;

	if (init(argc, argv, &v) != 0)
		return 1;
#ifdef ENABLE_HTTPS
	if (init_ssl() < 0)
		return 1;
#endif /* ENABLE_HTTPS */
	/* count lan addrs */
	addr_count = 0;
	for (lan_addr = lan_addrs.lh_first; lan_addr != NULL; lan_addr = lan_addr->list.le_next)
		addr_count++;
	if (addr_count > 0) {
#ifndef ENABLE_IPV6
		snotify = calloc(addr_count, sizeof(int));
#else
		/* one for IPv4, one for IPv6 */
		snotify = calloc(addr_count * 2, sizeof(int));
#endif
	}
	LIST_INIT(&upnphttphead);

	if (!GETFLAG(ENABLEUPNPMASK)) {
		syslog(LOG_ERR, "Why did you run me anyway?");
		return 0;
	}

	syslog(LOG_INFO, "version " MINIUPNPD_VERSION " starting%s%sext if %s BOOTID=%u",
	       " ",
	       GETFLAG(ENABLEUPNPMASK) ? "UPnP-IPCAM " : "",
	       ext_if_name, upnp_bootid);

	if (GETFLAG(ENABLEUPNPMASK)) {
		unsigned short listen_port;
		listen_port = (v.port > 0) ? v.port : 0;
		/* open socket for HTTP connections. Listen on the 1st LAN address */
#ifdef ENABLE_IPV6
		shttpl = OpenAndConfHTTPSocket(&listen_port, 1);
#else /* ENABLE_IPV6 */
		shttpl = OpenAndConfHTTPSocket(&listen_port);
#endif /* ENABLE_IPV6 */
		if (shttpl < 0) {
			syslog(LOG_ERR, "Failed to open socket for HTTP. EXITING");
			return 1;
		}
		v.port = listen_port;
		syslog(LOG_NOTICE, "HTTP listening on port %d", v.port);
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if (!GETFLAG(IPV6DISABLEDMASK)) {
			shttpl_v4 =  OpenAndConfHTTPSocket(&listen_port, 0);
			if (shttpl_v4 < 0) {
				syslog(LOG_ERR, "Failed to open socket for HTTP on port %hu (IPv4). EXITING", v.port);
				return 1;
			}
		}
#endif /* V6SOCKETS_ARE_V6ONLY */
#ifdef ENABLE_HTTPS
		/* https */
		listen_port = (v.https_port > 0) ? v.https_port : 0;
#ifdef ENABLE_IPV6
		shttpsl = OpenAndConfHTTPSocket(&listen_port, 1);
#else /* ENABLE_IPV6 */
		shttpsl = OpenAndConfHTTPSocket(&listen_port);
#endif /* ENABLE_IPV6 */
		if (shttpl < 0) {
			syslog(LOG_ERR, "Failed to open socket for HTTPS. EXITING");
			return 1;
		}
		v.https_port = listen_port;
		syslog(LOG_NOTICE, "HTTPS listening on port %d", v.https_port);
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		shttpsl_v4 =  OpenAndConfHTTPSocket(&listen_port, 0);
		if (shttpsl_v4 < 0) {
			syslog(LOG_ERR, "Failed to open socket for HTTPS on port %hu (IPv4). EXITING", v.https_port);
			return 1;
		}
#endif /* V6SOCKETS_ARE_V6ONLY */
#endif /* ENABLE_HTTPS */
#ifdef ENABLE_IPV6
		if (find_ipv6_addr(NULL, ipv6_addr_for_http_with_brackets,
			sizeof(ipv6_addr_for_http_with_brackets)) > 0) {
			syslog(LOG_NOTICE, "HTTP IPv6 address given to control points : %s",
			       ipv6_addr_for_http_with_brackets);
		} else {
			memcpy(ipv6_addr_for_http_with_brackets, "[::1]", 6);
			syslog(LOG_WARNING, "no HTTP IPv6 address, disabling IPv6");
			SETFLAG(IPV6DISABLEDMASK);
		}
#endif

		/* open socket for SSDP connections */
		sudp = OpenAndConfSSDPReceiveSocket(0);
		if (sudp < 0) {
			syslog(LOG_NOTICE,
				"Failed to open socket for receiving SSDP. Trying to use MiniSSDPd");
			if (SubmitServicesToMiniSSDPD(lan_addrs.lh_first->str, v.port) < 0) {
				syslog(LOG_ERR, "Failed to connect to MiniSSDPd. EXITING");
				return 1;
			}
		}
#ifdef ENABLE_IPV6
		if (!GETFLAG(IPV6DISABLEDMASK)) {
			sudpv6 = OpenAndConfSSDPReceiveSocket(1);
			if (sudpv6 < 0) {
				syslog(LOG_WARNING,
					"Failed to open socket for receiving SSDP (IP v6).");
			}
		}
#endif

		/* open socket for sending notifications */
		if (OpenAndConfSSDPNotifySockets(snotify) < 0) {
			syslog(LOG_ERR, "Failed to open sockets for sending SSDP notify "
				"messages. EXITING");
			return 1;
		}

#ifdef USE_IFACEWATCHER
		/* open socket for kernel notifications about new network interfaces */
		if (sudp >= 0) {
			sifacewatcher = OpenAndConfInterfaceWatchSocket();
			if (sifacewatcher < 0) {
				syslog(LOG_ERR,
					"Failed to open socket for receiving network interface notifications");
			}
		}
#endif
	}

	/* main loop */
	while (!quitting) {
		/* Correct startup_time if it was set with a RTC close to 0 */
		if ((startup_time < 60*60*24) && (time(NULL) > 60*60*24)) {
			set_startup_time(GETFLAG(SYSUPTIMEMASK));
		}
		/* send public address change notifications if needed */
		if (should_send_public_address_change_notif) {
			syslog(LOG_INFO,
				"should send external iface address change notification(s)");
#ifdef ENABLE_EVENTS
			if (GETFLAG(ENABLEUPNPMASK)) {
				upnp_event_var_change_notify(EWanIPC);
			}
#endif
			should_send_public_address_change_notif = 0;
			goto shutdown;
		}
		/* Check if we need to send SSDP NOTIFY messages and do it if
		 * needed */
		if (gettimeofday(&timeofday, 0) < 0) {
			syslog(LOG_ERR, "gettimeofday(): %m");
			timeout.tv_sec = v.notify_interval;
			timeout.tv_usec = 0;
		} else {
			/* the comparaison is not very precise but who cares ? */
			if (timeofday.tv_sec >= (lasttimeofday.tv_sec + v.notify_interval)) {
				if (GETFLAG(ENABLEUPNPMASK))
					SendSSDPNotifies2(snotify,
						  (unsigned short)v.port,
#ifdef ENABLE_HTTPS
						      (unsigned short)v.https_port,
#endif
						  v.notify_interval << 1);
				memcpy(&lasttimeofday, &timeofday, sizeof(struct timeval));
				timeout.tv_sec = v.notify_interval;
				timeout.tv_usec = 0;
			} else {
				timeout.tv_sec = lasttimeofday.tv_sec + v.notify_interval
						 - timeofday.tv_sec;
				if (timeofday.tv_usec > lasttimeofday.tv_usec) {
					timeout.tv_usec = 1000000 + lasttimeofday.tv_usec
							  - timeofday.tv_usec;
					timeout.tv_sec--;
				} else {
					timeout.tv_usec = lasttimeofday.tv_usec - timeofday.tv_usec;
				}
			}
		}

		/* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
		FD_ZERO(&readset);
		FD_ZERO(&writeset);

		if (sudp >= 0) {
			FD_SET(sudp, &readset);
			max_fd = MAX(max_fd, sudp);
#ifdef USE_IFACEWATCHER
			if (sifacewatcher >= 0) {
				FD_SET(sifacewatcher, &readset);
				max_fd = MAX(max_fd, sifacewatcher);
			}
#endif
		}
		if (shttpl >= 0) {
			FD_SET(shttpl, &readset);
			max_fd = MAX(max_fd, shttpl);
		}
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if (shttpl_v4 >= 0) {
			FD_SET(shttpl_v4, &readset);
			max_fd = MAX(max_fd, shttpl_v4);
		}
#endif
#ifdef ENABLE_HTTPS
		if (shttpsl >= 0) {
			FD_SET(shttpsl, &readset);
			max_fd = MAX(max_fd, shttpsl);
		}
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if (shttpsl_v4 >= 0) {
			FD_SET(shttpsl_v4, &readset);
			max_fd = MAX(max_fd, shttpsl_v4);
		}
#endif
#endif /* ENABLE_HTTPS */
#ifdef ENABLE_IPV6
		if (sudpv6 >= 0) {
			FD_SET(sudpv6, &readset);
			max_fd = MAX(max_fd, sudpv6);
		}
#endif

		i = 0;	/* active HTTP connections count */
		for (e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next) {
			if (e->socket >= 0) {
				if (e->state <= EWaitingForHttpContent)
					FD_SET(e->socket, &readset);
				else if (e->state == ESendingAndClosing)
					FD_SET(e->socket, &writeset);
				else
					continue;
				max_fd = MAX(max_fd, e->socket);
				i++;
			}
		}
		/* for debug */
#ifdef DEBUG
		if (i > 1) {
			syslog(LOG_DEBUG, "%d active incoming HTTP connections", i);
		}
#endif

#ifdef ENABLE_EVENTS
		upnpevents_selectfds(&readset, &writeset, &max_fd);
#endif

		/* queued "sendto" */
		{
			struct timeval next_send;
			i = get_next_scheduled_send(&next_send);
			if (i > 0) {
#ifdef DEBUG
				syslog(LOG_DEBUG, "%d queued sendto", i);
#endif
				i = get_sendto_fds(&writeset, &max_fd, &timeofday);
				if (timeofday.tv_sec > next_send.tv_sec ||
				   (timeofday.tv_sec == next_send.tv_sec &&
				   timeofday.tv_usec >= next_send.tv_usec)) {
					if (i > 0) {
						timeout.tv_sec = 0;
						timeout.tv_usec = 0;
					}
				} else {
					struct timeval tmp_timeout;
					tmp_timeout.tv_sec = (next_send.tv_sec - timeofday.tv_sec);
					tmp_timeout.tv_usec = (next_send.tv_usec - timeofday.tv_usec);
					if (tmp_timeout.tv_usec < 0) {
						tmp_timeout.tv_usec += 1000000;
						tmp_timeout.tv_sec--;
					}
					if (timeout.tv_sec > tmp_timeout.tv_sec
					   || (timeout.tv_sec == tmp_timeout.tv_sec &&
					   timeout.tv_usec > tmp_timeout.tv_usec)) {
						timeout.tv_sec = tmp_timeout.tv_sec;
						timeout.tv_usec = tmp_timeout.tv_usec;
					}
				}
			}
		}

		if (select(max_fd+1, &readset, &writeset, 0, &timeout) < 0) {
			if (quitting)
				goto shutdown;
			if (errno == EINTR)
				continue; /* interrupted by a signal, start again */
			syslog(LOG_ERR, "select(all): %m");
			syslog(LOG_ERR, "Failed to select open sockets. EXITING");
			return 1;	/* very serious cause of error */
		}
		i = try_sendto(&writeset);
		/*if (i < 0) {
			syslog(LOG_ERR, "try_sendto failed to send %d packets", -i);
		}*/
#ifdef ENABLE_EVENTS
		upnpevents_processfds(&readset, &writeset);
#endif
		/* process SSDP packets */
		if (sudp >= 0 && FD_ISSET(sudp, &readset)) {
			/*syslog(LOG_INFO, "Received UDP Packet");*/
#ifdef ENABLE_HTTPS
			ProcessSSDPRequest(sudp, (unsigned short)v.port, (unsigned short)v.https_port);
#else
			ProcessSSDPRequest(sudp, (unsigned short)v.port);
#endif
		}
#ifdef ENABLE_IPV6
		if (sudpv6 >= 0 && FD_ISSET(sudpv6, &readset)) {
			syslog(LOG_INFO, "Received UDP Packet (IPv6)");
#ifdef ENABLE_HTTPS
			ProcessSSDPRequest(sudpv6, (unsigned short)v.port, (unsigned short)v.https_port);
#else
			ProcessSSDPRequest(sudpv6, (unsigned short)v.port);
#endif
		}
#endif
#ifdef USE_IFACEWATCHER
		/* process kernel notifications */
		if (sifacewatcher >= 0 && FD_ISSET(sifacewatcher, &readset))
			ProcessInterfaceWatchNotify(sifacewatcher);
#endif

		/* process active HTTP connections */
		/* LIST_FOREACH macro is not available under linux */
		for (e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next) {
			if (e->socket >= 0) {
				if (FD_ISSET(e->socket, &readset) ||
				   FD_ISSET(e->socket, &writeset)) {
					Process_upnphttp(e);
				}
			}
		}
		/* process incoming HTTP connections */
		if (shttpl >= 0 && FD_ISSET(shttpl, &readset)) {
			struct upnphttp *tmp;
			tmp = ProcessIncomingHTTP(shttpl, "HTTP");
			if (tmp) {
				LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
			}
		}
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if (shttpl_v4 >= 0 && FD_ISSET(shttpl_v4, &readset)) {
			struct upnphttp *tmp;
			tmp = ProcessIncomingHTTP(shttpl_v4, "HTTP");
			if (tmp) {
				LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
			}
		}
#endif
#ifdef ENABLE_HTTPS
		if (shttpsl >= 0 && FD_ISSET(shttpsl, &readset)) {
			struct upnphttp *tmp;
			tmp = ProcessIncomingHTTP(shttpsl, "HTTPS");
			if (tmp) {
				InitSSL_upnphttp(tmp);
				LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
			}
		}
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if (shttpsl_v4 >= 0 && FD_ISSET(shttpsl_v4, &readset)) {
			struct upnphttp *tmp;
			tmp = ProcessIncomingHTTP(shttpsl_v4, "HTTPS");
			if (tmp) {
				InitSSL_upnphttp(tmp);
				LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
			}
		}
#endif
#endif /* ENABLE_HTTPS */
		/* delete finished HTTP connections */
		for (e = upnphttphead.lh_first; e != NULL; ) {
			next = e->entries.le_next;
			if (e->state >= EToDelete) {
				LIST_REMOVE(e, entries);
				Delete_upnphttp(e);
			}
			e = next;
		}
	}	/* end of main loop */

shutdown:
	syslog(LOG_NOTICE, "shutting down MiniUPnPd");

	/* send good-bye */
	if (GETFLAG(ENABLEUPNPMASK)) {
#ifndef ENABLE_IPV6
		if (SendSSDPGoodbye(snotify, addr_count) < 0)
#else
		if (SendSSDPGoodbye(snotify, addr_count * 2) < 0)
#endif
		{
			syslog(LOG_ERR, "Failed to broadcast good-bye notifications");
		}
	}
	/* try to send pending packets */
	finalize_sendto();
	/* close out open sockets */
	while (upnphttphead.lh_first != NULL) {
		e = upnphttphead.lh_first;
		LIST_REMOVE(e, entries);
		Delete_upnphttp(e);
	}

	if (sudp >= 0)
		close(sudp);
	if (shttpl >= 0)
		close(shttpl);
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
	if (shttpl_v4 >= 0)
		close(shttpl_v4);
#endif
#ifdef ENABLE_IPV6
	if (sudpv6 >= 0)
		close(sudpv6);
#endif
#ifdef USE_IFACEWATCHER
	if (sifacewatcher >= 0)
		close(sifacewatcher);
#endif

	if (GETFLAG(ENABLEUPNPMASK)) {
#ifndef ENABLE_IPV6
		for (i = 0; i < addr_count; i++)
#else
		for (i = 0; i < addr_count * 2; i++)
#endif
			close(snotify[i]);
	}

	/* remove pidfile */
	if (pidfilename && (unlink(pidfilename) < 0))
		syslog(LOG_ERR, "Failed to remove pidfile %s: %m", pidfilename);

	/* delete lists */
	while (lan_addrs.lh_first != NULL) {
		lan_addr = lan_addrs.lh_first;
		LIST_REMOVE(lan_addrs.lh_first, list);
		free(lan_addr);
	}

#ifdef ENABLE_HTTPS
	free_ssl();
#endif
	free(snotify);
	closelog();
#ifndef DISABLE_CONFIG_FILE
	freeoptions();
#endif

	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;
	do {
		ret = miniupnpd(argc, argv);
		if (ret)
			return 1;
	} while (!(quitting ||should_send_public_address_change_notif));

	return 0;
}
