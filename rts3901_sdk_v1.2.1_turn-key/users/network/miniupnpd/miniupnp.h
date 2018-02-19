/* $Id: miniupnpdtypes.h,v 1.5 2012/09/27 15:47:15 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2012 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#ifndef MINIUPNP_H_INCLUDED
#define MINIUPNP_H_INCLUDED

#include "config.h"
#include <netinet/in.h>
#include <net/if.h>
#include <sys/queue.h>

struct network_cfg {
	struct in_addr ipaddr;
	struct in_addr netmask;
};

#endif
