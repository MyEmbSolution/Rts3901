/* $Id: miniupnpdpath.h,v 1.9 2012/09/27 15:47:15 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2011 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef MINIUPNPDPATH_H_INCLUDED
#define MINIUPNPDPATH_H_INCLUDED

#include "config.h"

/* Paths and other URLs in the miniupnpd http server */

#define ROOTDESC_PATH		"/rootDesc.xml"

#ifdef HAS_DUMMY_SERVICE
#define DUMMY_PATH			"/dummy.xml"
#endif

#define WANCFG_PATH			"/WANCfg.xml"
#define WANCFG_CONTROLURL	"/ctl/CmnIfCfg"
#define WANCFG_EVENTURL		"/evt/CmnIfCfg"

#define WANIPC_PATH			"/WANIPCn.xml"
#define WANIPC_CONTROLURL	"/ctl/IPConn"
#define WANIPC_EVENTURL		"/evt/IPConn"

#ifdef ENABLE_DP_SERVICE
/* For DeviceProtection introduced in IGD v2 */
#define DP_PATH				"/DP.xml"
#define DP_CONTROLURL		"/ctl/DP"
#define DP_EVENTURL			"/evt/DP"
#endif

#endif

