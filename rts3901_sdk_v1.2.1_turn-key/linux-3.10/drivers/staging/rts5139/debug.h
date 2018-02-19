/* Driver for Realtek RTS51xx USB card reader
 * Header file
 *
 * Copyright(c) 2009 Realtek Semiconductor Corp. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 *   wwang (wei_wang@realsil.com.cn)
 *   No. 450, Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#ifndef __RTS51X_DEBUG_H
#define __RTS51X_DEBUG_H

#include <linux/kernel.h>

#define RTS51X_TIP "rts51x: "

#ifdef CONFIG_RTS5139_DEBUG
#define DBG	1
#else
#define DBG	0
#endif

#if DBG
#define DEBUGP(x...) printk( KERN_DEBUG RTS51X_TIP x )
#define DEBUGPN(x...) printk( KERN_DEBUG x )
#define DEBUGPX(x...) printk( x )
#define DEBUGDO(x) x
#else
#define DEBUGP(x...)
#define DEBUGPN(x...)
#define DEBUGPX(x...)
#define DEBUGDO(x)
#endif

#define RTS51X_DEBUGP(x) DEBUGP x
#define RTS51X_DEBUGPN(x) DEBUGPN x
#define RTS51X_DEBUGDO(x) DEBUGDO(x)

#endif   // __RTS51X_DEBUG_H

