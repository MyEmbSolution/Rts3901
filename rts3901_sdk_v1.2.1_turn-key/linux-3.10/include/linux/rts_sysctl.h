/* Header for Realtek IP Camera SoC
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
 *   Wei WANG <wei_wang@realsil.com.cn>
 *   No. 128, West Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#ifndef __RTS_SYSCTL_H
#define __RTS_SYSCTL_H

enum force_reset_model {
	FORCE_RESET_VIDEO,
	FORCE_RESET_H264,
	FORCE_RESET_JPG,
	FORCE_RESET_MIPI,
	FORCE_RESET_MCU,
	FORCE_RESET_MCU_PREPARE,
	FORCE_RESET_MCU_DONE,
	FORCE_RESET_ISP,
	FORCE_RESET_CIPHER,
	FORCE_RESET_SD,
	FORCE_RESET_I2S,
	FORCE_RESET_I2C,
	FORCE_RESET_U2DEV,
	FORCE_RESET_U2HOST,
	FORCE_RESET_UART,
	FORCE_RESET_ETHERNET,
	FORCE_RESET_ETHERNET_CLR
};

struct rts_soc_hw_id {
	u16 hw_ver;
#define HW_ID_VER_RTS3901	0x01
	u8 hw_rev;
#define HW_ID_REV_A		0x00
#define HW_ID_REV_B		0x01
#define HW_ID_REV_C		0x02
	u8 isp_ver;
};

extern struct rts_soc_hw_id RTS_SOC_HW_ID;

extern void rts_sys_force_reset(int model);
extern int rts_pll3_ssc_config(u32 range, u32 period);
extern int rts_pll4_ssc_config(u32 range, u32 period);
extern int rts_dma_copy(dma_addr_t dst_addr, dma_addr_t src_addr, size_t n);

#endif
