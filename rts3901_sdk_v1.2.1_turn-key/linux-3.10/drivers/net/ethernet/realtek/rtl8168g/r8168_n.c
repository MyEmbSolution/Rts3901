/*
################################################################################
#
# r8168 is the Linux device driver released for Realtek Gigabit Ethernet
# controllers with PCI-Express interface.
#
# Copyright(c) 2014 Realtek Semiconductor Corp. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.
#
# Author:
# Realtek NIC software team <nicfae@realtek.com>
# No. 2, Innovation Road II, Hsinchu Science Park, Hsinchu 300, Taiwan
#
################################################################################
*/

/************************************************************************************
 *  This product is covered by one or more of the following patents:
 *  US6,570,884, US6,115,776, and US6,327,625.
 ***********************************************************************************/

/*
 * This driver is modified from r8169.c in Linux kernel 2.6.18
 */

#include <linux/module.h>
#include <linux/version.h>
//#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>
#include <linux/platform_device.h>
//#include <linux/pci-aspm.h>
#include <linux/dma-mapping.h>
#include <linux/moduleparam.h>
#include <linux/rts_efuse.h>
#include <linux/rts_sysctl.h>
#include <linux/clk.h>
#include <linux/r8168_platform.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include "r8168.h"
#include "r8168_asf.h"
#include "rtl_eeprom.h"
#include "rtltool.h"

static int eee_enable = 0 ;
module_param(eee_enable, int, S_IRUGO);

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static const int max_interrupt_work = 20;

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC. */
static const int multicast_filter_limit = 32;

#define _R(NAME,MAC,RCR,MASK, JumFrameSz) \
    { .name = NAME, .mcfg = MAC, .RCR_Cfg = RCR, .RxConfigMask = MASK, .jumbo_frame_sz = JumFrameSz }

static const struct {
    const char *name;
    u8 mcfg;
    u32 RCR_Cfg;
    u32 RxConfigMask;   /* Clears the bits supported by this chip */
    u32 jumbo_frame_sz;
} rtl_chip_info[] = {
    _R("RTL8168B/8111B",
    CFG_METHOD_1,
    (Reserved2_data << Reserved2_shift) | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_4k),

    _R("RTL8168B/8111B",
    CFG_METHOD_2,
    (Reserved2_data << Reserved2_shift) | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_4k),

    _R("RTL8168B/8111B",
    CFG_METHOD_3,
    (Reserved2_data << Reserved2_shift) | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_4k),

    _R("RTL8168C/8111C",
    CFG_METHOD_4, RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_6k),

    _R("RTL8168C/8111C",
    CFG_METHOD_5,
    RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_6k),

    _R("RTL8168C/8111C",
    CFG_METHOD_6,
    RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_6k),

    _R("RTL8168CP/8111CP",
    CFG_METHOD_7,
    RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_6k),

    _R("RTL8168CP/8111CP",
    CFG_METHOD_8,
    RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_6k),

    _R("RTL8168D/8111D",
    CFG_METHOD_9,
    RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168D/8111D",
    CFG_METHOD_10,
    RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168DP/8111DP",
    CFG_METHOD_11,
    RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168DP/8111DP",
    CFG_METHOD_12,
    RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168DP/8111DP",
    CFG_METHOD_13,
    RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168E/8111E",
    CFG_METHOD_14,
    RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168E/8111E",
    CFG_METHOD_15,
    RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168E-VL/8111E-VL",
    CFG_METHOD_16,
    RxEarly_off_V1 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e0080,
    Jumbo_Frame_9k),

    _R("RTL8168E-VL/8111E-VL",
    CFG_METHOD_17,
    RxEarly_off_V1 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168F/8111F",
    CFG_METHOD_18,
    RxEarly_off_V1 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168F/8111F",
    CFG_METHOD_19,
    RxEarly_off_V1 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8411",
    CFG_METHOD_20,
    (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168G/8111G",
    CFG_METHOD_21,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168G/8111G",
    CFG_METHOD_22,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168EP/8111EP",
    CFG_METHOD_23,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168GU/8111GU",
    CFG_METHOD_24,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168GU/8111GU",
    CFG_METHOD_25,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("8411B",
    CFG_METHOD_26,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168EP/8111EP",
    CFG_METHOD_27,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("RTL8168EP/8111EP",
    CFG_METHOD_28,
    RxEarly_off_V2 | (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    Jumbo_Frame_9k),

    _R("Unknown",
    CFG_METHOD_DEFAULT,
    (RX_DMA_BURST << RxCfgDMAShift),
    0xff7e1880,
    RX_BUF_SIZE)
};
#undef _R

static int rx_copybreak = 200;
static int timer_count = 0x2600;

static struct {
    u32 msg_enable;
} debug = { -1 };

static unsigned short speed = SPEED_100;
static int duplex = DUPLEX_FULL;
static int autoneg = AUTONEG_ENABLE;
#ifdef CONFIG_ASPM
static int aspm = 1;
#else
static int aspm = 0;
#endif
#ifdef ENABLE_S5WOL
static int s5wol = 1;
#else
static int s5wol = 0;
#endif

MODULE_AUTHOR("Realtek and the Linux r8168 crew <netdev@vger.kernel.org>");
MODULE_DESCRIPTION("RealTek RTL-8168 Gigabit Ethernet driver");

module_param(speed, ushort, 0);
MODULE_PARM_DESC(speed, "force phy operation. Deprecated by ethtool (8).");

module_param(duplex, int, 0);
MODULE_PARM_DESC(duplex, "force phy operation. Deprecated by ethtool (8).");

module_param(autoneg, int, 0);
MODULE_PARM_DESC(autoneg, "force phy operation. Deprecated by ethtool (8).");

module_param(aspm, int, 0);
MODULE_PARM_DESC(aspm, "Enable ASPM.");

module_param(s5wol, int, 0);
MODULE_PARM_DESC(s5wol, "Enable Shutdown Wake On Lan.");

module_param(rx_copybreak, int, 0);
MODULE_PARM_DESC(rx_copybreak, "Copy breakpoint for copy-only-tiny-frames");

module_param(timer_count, int, 0);
MODULE_PARM_DESC(timer_count, "Timer Interrupt Interval.");

module_param_named(debug, debug.msg_enable, int, 0);
MODULE_PARM_DESC(debug, "Debug verbosity level (0=none, ..., 16=all)");


MODULE_LICENSE("GPL");

MODULE_VERSION(RTL8168_VERSION);

static void rtl8168_sleep_rx_enable(struct net_device *dev);

static void rtl8168_link_timer(unsigned long __opaque);
static void rtl8168_tx_clear(struct rtl8168_private *tp);
static void rtl8168_rx_clear(struct rtl8168_private *tp);

static int rtl8168_open(struct net_device *dev);
static int rtl8168_start_xmit(struct sk_buff *skb, struct net_device *dev);

static irqreturn_t rtl8168_interrupt(int irq, void *dev_instance);

static void rtl8168_rx_desc_offset0_init(struct rtl8168_private *, int);
static int rtl8168_init_ring(struct net_device *dev);
static void rtl8168_hw_start(struct net_device *dev);
static int rtl8168_close(struct net_device *dev);
static void rtl8168_set_rx_mode(struct net_device *dev);
static void rtl8168_tx_timeout(struct net_device *dev);
static struct net_device_stats *rtl8168_get_stats(struct net_device *dev);
static int rtl8168_rx_interrupt(struct net_device *, struct rtl8168_private *, void __iomem *, u32 budget);
static int rtl8168_change_mtu(struct net_device *dev, int new_mtu);
static void rtl8168_down(struct net_device *dev);

static int rtl8168_set_mac_address(struct net_device *dev, void *p);
void rtl8168_rar_set(struct rtl8168_private *tp, uint8_t *addr);
static void rtl8168_desc_addr_fill(struct rtl8168_private *);
static void rtl8168_tx_desc_init(struct rtl8168_private *tp);
static void rtl8168_rx_desc_init(struct rtl8168_private *tp);

static void rtl8168_hw_reset(struct net_device *dev);

static void rtl8168_phy_power_up (struct net_device *dev);
static void rtl8168_phy_power_down (struct net_device *dev);
static int rtl8168_set_speed(struct net_device *dev, u8 autoneg,  u16 speed, u8 duplex);

#ifdef CONFIG_R8168_NAPI
static int rtl8168_poll(napi_ptr napi, napi_budget budget);
#endif



static inline void eth_copy_and_sum (struct sk_buff *dest,
                                     const unsigned char *src,
                                     int len, int base)
{
    memcpy (dest->data, src, len);
}

static inline u16 map_phy_ocp_addr(u16 PageNum, u8 RegNum)
{
    u16 OcpPageNum = 0;
    u8 OcpRegNum = 0;
    u16 OcpPhyAddress = 0;

    if( PageNum == 0 ) {
        OcpPageNum = OCP_STD_PHY_BASE_PAGE + ( RegNum / 8 );
        OcpRegNum = 0x10 + ( RegNum % 8 );
    } else {
        OcpPageNum = PageNum;
        OcpRegNum = RegNum;
    }

    OcpPageNum <<= 4;

    if( OcpRegNum < 16 ) {
        OcpPhyAddress = 0;
    } else {
        OcpRegNum -= 16;
        OcpRegNum <<= 1;

        OcpPhyAddress = OcpPageNum + OcpRegNum;
    }


    return OcpPhyAddress;
}

static void mdio_real_write(struct rtl8168_private *tp,
                            u32 RegAddr,
                            u32 value)
{
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

    if (RegAddr == 0x1F) {
        tp->cur_page = value;
    }

    if (tp->mcfg == CFG_METHOD_21) {
        u32 data32;
        u16 ocp_addr;

        if (RegAddr == 0x1F) {
            return;
        }
        ocp_addr = map_phy_ocp_addr(tp->cur_page, RegAddr);

        WARN_ON_ONCE(ocp_addr % 2);
        data32 = ocp_addr/2;
        data32 <<= OCPR_Addr_Reg_shift;
        data32 |= OCPR_Write | value;

        RTL_W32(PHYOCP, data32);
        for (i = 0; i < 100; i++) {
            udelay(1);

            if (!(RTL_R32(PHYOCP) & OCPR_Flag))
                break;
        }
    } else {
        RTL_W32(PHYAR, PHYAR_Write |
                (RegAddr & PHYAR_Reg_Mask) << PHYAR_Reg_shift |
                (value & PHYAR_Data_Mask));

        for (i = 0; i < 10; i++) {
            udelay(100);

            /* Check if the RTL8168 has completed writing to the specified MII register */
            if (!(RTL_R32(PHYAR) & PHYAR_Flag)) {
                udelay(20);
                break;
            }
        }
    }
}

void mdio_write(struct rtl8168_private *tp,
                u32 RegAddr,
                u32 value)
{
    if (tp->rtk_enable_diag) return;

    mdio_real_write(tp, RegAddr, value);
}

void mdio_prot_write(struct rtl8168_private *tp,
                     u32 RegAddr,
                     u32 value)
{
    mdio_real_write(tp, RegAddr, value);
}

u32 mdio_read(struct rtl8168_private *tp,
              u32 RegAddr)
{
    void __iomem *ioaddr = tp->mmio_addr;
    int i, value = 0;

	if (tp->mcfg == CFG_METHOD_21) {
        u32 data32;
        u16 ocp_addr;

        ocp_addr = map_phy_ocp_addr(tp->cur_page, RegAddr);

        WARN_ON_ONCE(ocp_addr % 2);

        data32 = ocp_addr/2;
        data32 <<= OCPR_Addr_Reg_shift;

        RTL_W32(PHYOCP, data32);
        for (i = 0; i < 100; i++) {
            udelay(1);

            if (RTL_R32(PHYOCP) & OCPR_Flag)
                break;
        }
        value = RTL_R32(PHYOCP) & OCPDR_Data_Mask;
    } else {
        RTL_W32(PHYAR, PHYAR_Read | (RegAddr & PHYAR_Reg_Mask) << PHYAR_Reg_shift);

        for (i = 0; i < 10; i++) {
            udelay(100);

            /* Check if the RTL8168 has completed retrieving data from the specified MII register */
            if (RTL_R32(PHYAR) & PHYAR_Flag) {
                value = RTL_R32(PHYAR) & PHYAR_Data_Mask;
                udelay(20);
                break;
            }
        }
    }

    return value;
}

static void mac_ocp_write(struct rtl8168_private *tp, u16 reg_addr, u16 value)
{
    void __iomem *ioaddr = tp->mmio_addr;
    u32 data32;

    WARN_ON_ONCE(reg_addr % 2);

    data32 = reg_addr/2;
    data32 <<= OCPR_Addr_Reg_shift;
    data32 += value;
    data32 |= OCPR_Write;

    RTL_W32(MACOCP, data32);
}

static u16 mac_ocp_read(struct rtl8168_private *tp, u16 reg_addr)
{
    void __iomem *ioaddr = tp->mmio_addr;
    u32 data32;
    u16 data16 = 0;

    WARN_ON_ONCE(reg_addr % 2);

    data32 = reg_addr/2;
    data32 <<= OCPR_Addr_Reg_shift;

    RTL_W32(MACOCP, data32);
    data16 = (u16)RTL_R32(MACOCP);

    return data16;
}

u32 OCP_read(struct rtl8168_private *tp, u8 mask, u16 Reg)
{
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

	RTL_W32(OCPAR, ((u32)mask&0xF)<<12 | (Reg&0xFFF));
	for (i = 0; i < 20; i++) {
		udelay(100);
		if (RTL_R32(OCPAR) & OCPAR_Flag)
			break;
	}
	return RTL_R32(OCPDR);
}

void OCP_write(struct rtl8168_private *tp, u8 mask, u16 Reg, u32 data)
{
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

	RTL_W32(OCPDR, data);
 	RTL_W32(OCPAR, OCPAR_Flag | ((u32)mask&0xF)<<12 | (Reg&0xFFF));
  	for (i = 0; i < 20; i++) {
 		udelay(100);
		if ((RTL_R32(OCPAR)&OCPAR_Flag) == 0)
			break;
	}
}

void OOB_mutex_lock(struct rtl8168_private *tp)
{
    u8 reg_16, reg_a0;
    u32 wait_cnt_0, wait_Cnt_1;
    u16 ocp_reg_mutex_ib;
    u16 ocp_reg_mutex_oob;
    u16 ocp_reg_mutex_prio;

	ocp_reg_mutex_oob = 0x110;
	ocp_reg_mutex_ib = 0x114;
	ocp_reg_mutex_prio = 0x11C;


    OCP_write(tp, 0x1, ocp_reg_mutex_ib, BIT_0);
    reg_16 = OCP_read(tp, 0xF, ocp_reg_mutex_oob);
    wait_cnt_0 = 0;
    while(reg_16) {
        reg_a0 = OCP_read(tp, 0xF, ocp_reg_mutex_prio);
        if(reg_a0) {
            OCP_write(tp, 0x1, ocp_reg_mutex_ib, 0x00);
            reg_a0 = OCP_read(tp, 0xF, ocp_reg_mutex_prio);
            wait_Cnt_1 = 0;
            while(reg_a0) {
                reg_a0 = OCP_read(tp, 0xF, ocp_reg_mutex_prio);

                wait_Cnt_1++;

                if(wait_Cnt_1 > 2000)
                    break;
            };
            OCP_write(tp, 0x1, ocp_reg_mutex_ib, BIT_0);

        }
        reg_16 = OCP_read(tp, 0xF, ocp_reg_mutex_oob);

        wait_cnt_0++;

        if(wait_cnt_0 > 2000)
            break;
    };
}

void OOB_mutex_unlock(struct rtl8168_private *tp)
{
    u16 ocp_reg_mutex_ib;
    u16 ocp_reg_mutex_oob;
    u16 ocp_reg_mutex_prio;

	ocp_reg_mutex_oob = 0x110;
	ocp_reg_mutex_ib = 0x114;
	ocp_reg_mutex_prio = 0x11C;

    OCP_write(tp, 0x1, ocp_reg_mutex_prio, BIT_0);
    OCP_write(tp, 0x1, ocp_reg_mutex_ib, 0x00);
}

void OOB_notify(struct rtl8168_private *tp, u8 cmd)
{
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

    RTL_W8(ERIDR, cmd);
    RTL_W32(ERIAR, 0x800010E8);
    mdelay(2);
    for (i = 0; i < 5; i++) {
        udelay(100);
        if (!(RTL_R32(ERIAR) & ERIAR_Flag))
            break;
    }

    OCP_write(tp, 0x1, 0x30, 0x00000001);
}

static int rtl8168_check_dash(struct rtl8168_private *tp)
{
    
	u32 reg;

	if (tp->mcfg == CFG_METHOD_13)
		reg = 0xb8;
	else
		reg = 0x10;

	if (OCP_read(tp, 0xF, reg) & 0x00008000)
		return 1;
	else
		return 0;
}

void Dash2DisableTx(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (!tp->DASH) return;

    if( HW_DASH_SUPPORT_TYPE_2( tp ) ) {
        u16 WaitCnt;
        u8 TmpUchar;

        //Disable oob Tx
        RTL_W8(IBCR2, RTL_R8(IBCR2) & ~( BIT_0 ));
        WaitCnt = 0;

        //wait oob tx disable
        do {
            TmpUchar = RTL_R8(IBISR0);

            if( TmpUchar & ISRIMR_DASH_TYPE2_TX_DISABLE_IDLE ) {
                break;
            }

            udelay( 50 );
            WaitCnt++;
        } while(WaitCnt < 2000);

        //Clear ISRIMR_DASH_TYPE2_TX_DISABLE_IDLE
        RTL_W8(IBISR0, RTL_R8(IBISR0) | ISRIMR_DASH_TYPE2_TX_DISABLE_IDLE);
    }
}

void Dash2EnableTx(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (!tp->DASH) return;

    if( HW_DASH_SUPPORT_TYPE_2( tp ) )
        RTL_W8(IBCR2, RTL_R8(IBCR2) | BIT_0);
}

void Dash2DisableRx(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (!tp->DASH) return;

    if( HW_DASH_SUPPORT_TYPE_2( tp ) )
        RTL_W8(IBCR0, RTL_R8(IBCR0) & ~( BIT_0 ));
}

void Dash2EnableRx(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (!tp->DASH) return;

    if( HW_DASH_SUPPORT_TYPE_2( tp ) )
        RTL_W8(IBCR0, RTL_R8(IBCR0) | BIT_0);
}


void rtl8168_ephy_write(void __iomem *ioaddr, int RegAddr, int value)
{
    int i;

    RTL_W32(EPHYAR,
            EPHYAR_Write |
            (RegAddr & EPHYAR_Reg_Mask) << EPHYAR_Reg_shift |
            (value & EPHYAR_Data_Mask));

    for (i = 0; i < 10; i++) {
        udelay(100);

        /* Check if the RTL8168 has completed EPHY write */
        if (!(RTL_R32(EPHYAR) & EPHYAR_Flag))
            break;
    }

    udelay(20);
}

u16 rtl8168_ephy_read(void __iomem *ioaddr, int RegAddr)
{
    int i;
    u16 value = 0xffff;

    RTL_W32(EPHYAR,
            EPHYAR_Read | (RegAddr & EPHYAR_Reg_Mask) << EPHYAR_Reg_shift);

    for (i = 0; i < 10; i++) {
        udelay(100);

        /* Check if the RTL8168 has completed EPHY read */
        if (RTL_R32(EPHYAR) & EPHYAR_Flag) {
            value = (u16) (RTL_R32(EPHYAR) & EPHYAR_Data_Mask);
            break;
        }
    }

    udelay(20);

    return value;
}


u32 rtl8168_eri_read(void __iomem *ioaddr, int addr, int len, int type)
{
    int i, val_shift, shift = 0;
    u32 value1 = 0, value2 = 0, mask;

    if (len > 4 || len <= 0)
        return -1;

    while (len > 0) {
        val_shift = addr % ERIAR_Addr_Align;
        addr = addr & ~0x3;

        RTL_W32(ERIAR,
                ERIAR_Read |
                type << ERIAR_Type_shift |
                ERIAR_ByteEn << ERIAR_ByteEn_shift |
                addr);

        for (i = 0; i < 10; i++) {
            udelay(100);

            /* Check if the RTL8168 has completed ERI read */
            if (RTL_R32(ERIAR) & ERIAR_Flag)
                break;
        }

        if (len == 1)       mask = (0xFF << (val_shift * 8)) & 0xFFFFFFFF;
        else if (len == 2)  mask = (0xFFFF << (val_shift * 8)) & 0xFFFFFFFF;
        else if (len == 3)  mask = (0xFFFFFF << (val_shift * 8)) & 0xFFFFFFFF;
        else            mask = (0xFFFFFFFF << (val_shift * 8)) & 0xFFFFFFFF;

        value1 = RTL_R32(ERIDR) & mask;
        value2 |= (value1 >> val_shift * 8) << shift * 8;

        if (len <= 4 - val_shift) {
            len = 0;
        } else {
            len -= (4 - val_shift);
            shift = 4 - val_shift;
            addr += 4;
        }
    }

    udelay(20);

    return value2;
}

int rtl8168_eri_write(void __iomem *ioaddr, int addr, int len, u32 value, int type)
{

    int i, val_shift, shift = 0;
    u32 value1 = 0, mask;

    if (len > 4 || len <= 0)
        return -1;

    while (len > 0) {
        val_shift = addr % ERIAR_Addr_Align;
        addr = addr & ~0x3;

        if (len == 1)       mask = (0xFF << (val_shift * 8)) & 0xFFFFFFFF;
        else if (len == 2)  mask = (0xFFFF << (val_shift * 8)) & 0xFFFFFFFF;
        else if (len == 3)  mask = (0xFFFFFF << (val_shift * 8)) & 0xFFFFFFFF;
        else            mask = (0xFFFFFFFF << (val_shift * 8)) & 0xFFFFFFFF;

        value1 = rtl8168_eri_read(ioaddr, addr, 4, type) & ~mask;
        value1 |= ((value << val_shift * 8) >> shift * 8);

        RTL_W32(ERIDR, value1);
        RTL_W32(ERIAR,
                ERIAR_Write |
                type << ERIAR_Type_shift |
                ERIAR_ByteEn << ERIAR_ByteEn_shift |
                addr);

        for (i = 0; i < 10; i++) {
            udelay(100);

            /* Check if the RTL8168 has completed ERI write */
            if (!(RTL_R32(ERIAR) & ERIAR_Flag))
                break;
        }

        if (len <= 4 - val_shift) {
            len = 0;
        } else {
            len -= (4 - val_shift);
            shift = 4 - val_shift;
            addr += 4;
        }
    }

    udelay(20);

    return 0;
}

static void
rtl8168_enable_rxdvgate(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

    if(tp->mcfg == CFG_METHOD_21) {
        RTL_W8(0xF2, RTL_R8(0xF2) | BIT_3);
        mdelay(2);
    }
}

static void
rtl8168_disable_rxdvgate(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

    if(tp->mcfg == CFG_METHOD_21) {
        RTL_W8(0xF2, RTL_R8(0xF2) & ~BIT_3);
        mdelay(2);
    }
}

static void
rtl8168_wait_txrx_fifo_empty(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

    if(tp->mcfg == CFG_METHOD_21) {
        for (i = 0; i < 10; i++) {
            udelay(100);
            if (RTL_R32(TxConfig) & BIT_11)
                break;
        }

        for (i = 0; i < 10; i++) {
            udelay(100);
            if ((RTL_R8(MCUCmd_reg) & (Txfifo_empty | Rxfifo_empty)) == (Txfifo_empty | Rxfifo_empty))
                break;

        }
    }
}

#ifdef ENABLE_DASH_SUPPORT

inline void
rtl8168_enable_dash2_interrupt(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    if (!tp->DASH) return;

    if( HW_DASH_SUPPORT_TYPE_2( tp ) )
        RTL_W8(IBIMR0, ( ISRIMR_DASH_TYPE2_ROK | ISRIMR_DASH_TYPE2_TOK | ISRIMR_DASH_TYPE2_TDU | ISRIMR_DASH_TYPE2_RDU | ISRIMR_DASH_TYPE2_RX_DISABLE_IDLE ));
}

static inline void
rtl8168_disable_dash2_interrupt(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    if (!tp->DASH) return;

    if( HW_DASH_SUPPORT_TYPE_2( tp ) )
        RTL_W8(IBIMR0, 0);
}
#endif

static inline void
rtl8168_enable_hw_interrupt(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    RTL_W16(IntrMask, tp->intr_mask);

#ifdef ENABLE_DASH_SUPPORT
    if (tp->DASH)
        rtl8168_enable_dash2_interrupt(tp, ioaddr);
#endif
}

static inline void
rtl8168_disable_hw_interrupt(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    RTL_W16(IntrMask, 0x0000);

#ifdef ENABLE_DASH_SUPPORT
    if (tp->DASH)
        rtl8168_disable_dash2_interrupt(tp, ioaddr);
#endif
}


static inline void
rtl8168_switch_to_hw_interrupt(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    RTL_W32(TimeIntr, 0x0000);

    rtl8168_enable_hw_interrupt(tp, ioaddr);
}

static inline void
rtl8168_switch_to_timer_interrupt(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    if (tp->use_timer_interrrupt) {
        RTL_W32(TCTR, timer_count);
        RTL_W32(TimeIntr, timer_count);
        RTL_W16(IntrMask, tp->timer_intr_mask);

#ifdef ENABLE_DASH_SUPPORT
        if (tp->DASH)
            rtl8168_enable_dash2_interrupt(tp, ioaddr);
#endif
    } else {
        rtl8168_switch_to_hw_interrupt(tp, ioaddr);
    }
}

static void
rtl8168_irq_mask_and_ack(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    rtl8168_disable_hw_interrupt(tp, ioaddr);
    RTL_W16(IntrStatus, RTL_R16(IntrStatus));

#ifdef ENABLE_DASH_SUPPORT
    if ( tp->DASH ) {
        if( HW_DASH_SUPPORT_TYPE_2( tp ) ) {
            RTL_W8(IBISR0, RTL_R16(IBISR0));
        }
    }
#endif
}

static void
rtl8168_nic_reset(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

    RTL_W32(RxConfig, (RX_DMA_BURST << RxCfgDMAShift));

    rtl8168_enable_rxdvgate(dev);

    rtl8168_wait_txrx_fifo_empty(dev);

	mdelay(10);
	
    /* Soft reset the chip. */
    RTL_W8(ChipCmd, CmdReset);

    /* Check that the chip has finished the reset. */
    for (i = 100; i > 0; i--) {
        udelay(100);
        if ((RTL_R8(ChipCmd) & CmdReset) == 0)
            break;
    }
}

static void
rtl8168_hw_reset(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

    /* Disable interrupts */
    rtl8168_irq_mask_and_ack(tp, ioaddr);

    rtl8168_nic_reset(dev);
}

static void rtl8168_mac_loopback_test(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;
    struct net_device *dev = tp->dev;
    struct sk_buff *skb, *rx_skb;
    dma_addr_t mapping;
    struct TxDesc *txd;
    struct RxDesc *rxd;
    void *tmpAddr;
    u32 len, rx_len, rx_cmd;
    u16 type;
    u8 pattern;
    int i;

    if (tp->DASH)
        return;

    pattern = 0x5A;
    len = 60;
    type = htons(ETH_P_IP);
    txd = tp->TxDescArray;
    rxd = tp->RxDescArray;
    rx_skb = tp->Rx_skbuff[0];
    RTL_W32(TxConfig, (RTL_R32(TxConfig) & ~0x00060000) | 0x00020000);

    do {
	skb = alloc_skb(len + RTK_RX_ALIGN + NET_SKB_PAD, GFP_ATOMIC);
        if (unlikely(!skb))
            dev_printk(KERN_NOTICE, &tp->platform_dev->dev, "-ENOMEM;\n");
    } while (unlikely(skb == NULL));
    skb_reserve(skb, RTK_RX_ALIGN);

    memcpy(skb_put(skb, dev->addr_len), dev->dev_addr, dev->addr_len);
    memcpy(skb_put(skb, dev->addr_len), dev->dev_addr, dev->addr_len);
    memcpy(skb_put(skb, sizeof(type)), &type, sizeof(type));
    tmpAddr = skb_put(skb, len - 14);

    mapping = dma_map_single(&tp->platform_dev->dev, skb->data, len, DMA_TO_DEVICE);
    dma_sync_single_for_cpu(&tp->platform_dev->dev, le64_to_cpu(mapping),
                                len, DMA_TO_DEVICE);
    txd->addr = cpu_to_le64(mapping);
    txd->opts2 = 0;
    while (1) {
        memset(tmpAddr, pattern++, len - 14);
        dma_sync_single_for_device(&tp->platform_dev->dev,
                                       le64_to_cpu(mapping),
                                       len, DMA_TO_DEVICE);
        txd->opts1 = cpu_to_le32(DescOwn | FirstFrag | LastFrag | len);

        RTL_W32(RxConfig, RTL_R32(RxConfig)  | AcceptMyPhys);

        smp_wmb();
        RTL_W8(TxPoll, NPQ);    /* set polling bit */

        for (i = 0; i < 50; i++) {
            udelay(200);
            rx_cmd = le32_to_cpu(rxd->opts1);
            if ((rx_cmd & DescOwn) == 0)
                break;
        }

        RTL_W32(RxConfig, RTL_R32(RxConfig) & ~(AcceptErr | AcceptRunt | AcceptBroadcast | AcceptMulticast | AcceptMyPhys |  AcceptAllPhys));

        rx_len = rx_cmd & 0x3FFF;
        rx_len -= 4;
        rxd->opts1 = cpu_to_le32(DescOwn | tp->rx_buf_sz);

        dma_sync_single_for_cpu(&tp->platform_dev->dev, le64_to_cpu(mapping), len, DMA_TO_DEVICE);

        if (rx_len == len) {
            dma_sync_single_for_cpu(&tp->platform_dev->dev, le64_to_cpu(rxd->addr), tp->rx_buf_sz, DMA_FROM_DEVICE);
            i = memcmp(skb->data, rx_skb->data, rx_len);
            dma_sync_single_for_device(&tp->platform_dev->dev, le64_to_cpu(rxd->addr), tp->rx_buf_sz, DMA_FROM_DEVICE);
            if (i == 0) {
//              dev_printk(KERN_INFO, &tp->platform_dev->dev, "loopback test finished\n",rx_len,len);
                break;
            }
        }

        rtl8168_hw_reset(dev);
        rtl8168_disable_rxdvgate(dev);
        RTL_W8(ChipCmd, CmdTxEnb | CmdRxEnb);
    }
    tp->dirty_tx++;
    tp->dirty_rx++;
    tp->cur_tx++;
    tp->cur_rx++;
    dma_unmap_single(&tp->platform_dev->dev, le64_to_cpu(mapping),
                     len, DMA_TO_DEVICE);
    RTL_W32(TxConfig, RTL_R32(TxConfig) & ~0x00060000);
    dev_kfree_skb_any(skb);
    RTL_W16(IntrStatus, 0xFFBF);
}

static unsigned int
rtl8168_xmii_reset_pending(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned int retval;
    unsigned long flags;

    spin_lock_irqsave(&tp->phy_lock, flags);
    mdio_write(tp, 0x1f, 0x0000);
    retval = mdio_read(tp, MII_BMCR) & BMCR_RESET;
    spin_unlock_irqrestore(&tp->phy_lock, flags);

    return retval;
}

static unsigned int
rtl8168_xmii_link_ok(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    unsigned int retval;

    retval = (RTL_R8(PHYstatus) & LinkStatus) ? 1 : 0;

    return retval;
}

static void
rtl8168_xmii_reset_enable(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    int i, val = 0;
    unsigned long flags;

    spin_lock_irqsave(&tp->phy_lock, flags);
    mdio_write(tp, 0x1f, 0x0000);
    mdio_write(tp, MII_BMCR, mdio_read(tp, MII_BMCR) | BMCR_RESET);
    spin_unlock_irqrestore(&tp->phy_lock, flags);

    for (i = 0; i < 2500; i++) {
        spin_lock_irqsave(&tp->phy_lock, flags);
        val = mdio_read(tp, MII_BMSR) & BMCR_RESET;
        spin_unlock_irqrestore(&tp->phy_lock, flags);

        if (!val)
            return;

        mdelay(1);
    }
}

void rtl8168_init_ring_indexes(struct rtl8168_private *tp)
{
    tp->dirty_tx = 0;
    tp->dirty_rx = 0;
    tp->cur_tx = 0;
    tp->cur_rx = 0;
}


#ifdef ENABLE_DASH_SUPPORT
static void
NICChkTypeEnableDashInterrupt(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (tp->DASH) {
        //
        // even disconnected, enable 3 dash interrupt mask bits for in-band/out-band communication
        //
        if( HW_DASH_SUPPORT_TYPE_2( tp ) ) {
            rtl8168_enable_dash2_interrupt(tp, ioaddr);
            RTL_W16(IntrMask, (ISRIMR_DASH_INTR_EN | ISRIMR_DASH_INTR_CMAC_RESET));
        } else {
            RTL_W16(IntrMask, (ISRIMR_DP_DASH_OK | ISRIMR_DP_HOST_OK | ISRIMR_DP_REQSYS_OK));
        }
    }
}
#endif

static void
rtl8168_check_link_status(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    int link_status_on;
    unsigned long flags;

    link_status_on = tp->link_ok(dev);

    if (netif_carrier_ok(dev) != link_status_on) {
        if (link_status_on) {
            if (tp->mcfg == CFG_METHOD_18) {
                if (RTL_R8(PHYstatus) & _1000bpsF) {
                    rtl8168_eri_write(ioaddr, 0x1bc, 4, 0x00000011, ERIAR_ExGMAC);
                    rtl8168_eri_write(ioaddr, 0x1dc, 4, 0x00000005, ERIAR_ExGMAC);
                } else {
                    rtl8168_eri_write(ioaddr, 0x1bc, 4, 0x0000001f, ERIAR_ExGMAC);
                    rtl8168_eri_write(ioaddr, 0x1dc, 4, 0x0000003f, ERIAR_ExGMAC);
                }
            } else if ((tp->mcfg == CFG_METHOD_21) && netif_running(dev)) {
                if (RTL_R8(PHYstatus)&FullDup)
                    RTL_W32(TxConfig, (RTL_R32(TxConfig) | (BIT_24 | BIT_25)) & ~BIT_19);
                else
                    RTL_W32(TxConfig, (RTL_R32(TxConfig) | BIT_25) & ~(BIT_19 | BIT_24));
            }

            if (tp->mcfg == CFG_METHOD_21) {
                /*half mode*/
                if (!(RTL_R8(PHYstatus)&FullDup)) {
                    spin_lock_irqsave(&tp->phy_lock, flags);
                    mdio_write(tp, 0x1F, 0x0000);
                    mdio_write(tp, MII_ADVERTISE, mdio_read(tp, MII_ADVERTISE)&~(ADVERTISE_PAUSE_CAP|ADVERTISE_PAUSE_ASYM));
                    spin_unlock_irqrestore(&tp->phy_lock, flags);
                }
            }

            rtl8168_hw_start(dev);

            netif_carrier_on(dev);

            if (netif_msg_ifup(tp))
                printk(KERN_INFO PFX "%s: link up\n", dev->name);
        } else {
            if (netif_msg_ifdown(tp))
                printk(KERN_INFO PFX "%s: link down\n", dev->name);
            netif_carrier_off(dev);

            netif_stop_queue(dev);

            rtl8168_hw_reset(dev);

            rtl8168_tx_clear(tp);

            rtl8168_init_ring_indexes(tp);

            rtl8168_set_speed(dev, tp->autoneg, tp->speed, tp->duplex);


#ifdef ENABLE_DASH_SUPPORT
            if (tp->DASH) {
                NICChkTypeEnableDashInterrupt(tp);
            }
#endif
        }
    }


}

static void
rtl8168_link_option(int idx,
                    u8 *aut,
                    u16 *spd,
                    u8 *dup)
{
    if ((*spd != SPEED_1000) && (*spd != SPEED_100) && (*spd != SPEED_10))
        *spd = SPEED_100;

    if ((*dup != DUPLEX_FULL) && (*dup != DUPLEX_HALF))
        *dup = DUPLEX_FULL;

    if ((*aut != AUTONEG_ENABLE) && (*aut != AUTONEG_DISABLE))
        *aut = AUTONEG_ENABLE;
}

static void
rtl8168_wait_ll_share_fifo_ready(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

    for (i = 0; i < 10; i++) {
        udelay(100);
        if (RTL_R16(0xD2) & BIT_9)
            break;
    }
}


#define WAKE_ANY (WAKE_PHY | WAKE_MAGIC | WAKE_UCAST | WAKE_BCAST | WAKE_MCAST)

static void
rtl8168_get_hw_wol(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u8 options;
    u32 csi_tmp;
    unsigned long flags;


    spin_lock_irqsave(&tp->lock, flags);

    tp->wol_opts = 0;
    options = RTL_R8(Config1);
    if (!(options & PMEnable))
        goto out_unlock;

    options = RTL_R8(Config3);
    if (options & LinkUp)
        tp->wol_opts |= WAKE_PHY;

    
    csi_tmp = rtl8168_eri_read(ioaddr, 0xDE, 1, ERIAR_ExGMAC);
    if (csi_tmp & BIT_0)
        tp->wol_opts |= WAKE_MAGIC;
      
    options = RTL_R8(Config5);
    if (options & UWF)
        tp->wol_opts |= WAKE_UCAST;
    if (options & BWF)
        tp->wol_opts |= WAKE_BCAST;
    if (options & MWF)
        tp->wol_opts |= WAKE_MCAST;

out_unlock:
    tp->wol_enabled = (tp->wol_opts) ? WOL_ENABLED : WOL_DISABLED;

    spin_unlock_irqrestore(&tp->lock, flags);
}

static void
rtl8168_set_hw_wol(struct net_device *dev, u32 wolopts)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    int i,tmp;
    u32 csi_tmp;
    static struct {
        u32 opt;
        u16 reg;
        u8  mask;
    } cfg[] = {
        { WAKE_ANY,   Config1, PMEnable },
        { WAKE_PHY,   Config3, LinkUp },
        { WAKE_UCAST, Config5, UWF },
        { WAKE_BCAST, Config5, BWF },
        { WAKE_MCAST, Config5, MWF },
        { WAKE_ANY,   Config5, LanWake },
        { WAKE_MAGIC, Config3, MagicPacket },
    };

    RTL_W8(Cfg9346, Cfg9346_Unlock);


    tmp = ARRAY_SIZE(cfg) - 1;
	csi_tmp = rtl8168_eri_read(ioaddr, 0xDE, 1, ERIAR_ExGMAC);
	if (wolopts & WAKE_MAGIC)
		csi_tmp |= BIT_0;
 	else
		csi_tmp &= ~BIT_0;
	rtl8168_eri_write(ioaddr, 0xDE, 1, csi_tmp, ERIAR_ExGMAC);
        

    for (i = 0; i < tmp; i++) {
        u8 options = RTL_R8(cfg[i].reg) & ~cfg[i].mask;
        if (wolopts & cfg[i].opt)
            options |= cfg[i].mask;
        RTL_W8(cfg[i].reg, options);
    }

    RTL_W8(Cfg9346, Cfg9346_Lock);
}

static void
rtl8168_powerdown_pll(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

    if (tp->wol_enabled == WOL_ENABLED || tp->DASH) {
        int auto_nego;
        int giga_ctrl;
        u16 val;
        unsigned long flags;

        rtl8168_set_hw_wol(dev, tp->wol_opts);

        if (tp->mcfg == CFG_METHOD_21) {
            RTL_W8(Cfg9346, Cfg9346_Unlock);
            RTL_W8(Config2, RTL_R8(Config2) | PMSTS_En);
            RTL_W8(Cfg9346, Cfg9346_Lock);
        }

        spin_lock_irqsave(&tp->phy_lock, flags);
        mdio_write(tp, 0x1F, 0x0000);
        auto_nego = mdio_read(tp, MII_ADVERTISE);
        auto_nego &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL
                       | ADVERTISE_100HALF | ADVERTISE_100FULL);

        val = mdio_read(tp, MII_LPA);

#ifdef CONFIG_DOWN_SPEED_100
        auto_nego |= (ADVERTISE_100FULL | ADVERTISE_100HALF | ADVERTISE_10HALF | ADVERTISE_10FULL);
#else
        if (val & (LPA_10HALF | LPA_10FULL))
            auto_nego |= (ADVERTISE_10HALF | ADVERTISE_10FULL);
        else
            auto_nego |= (ADVERTISE_100FULL | ADVERTISE_100HALF | ADVERTISE_10HALF | ADVERTISE_10FULL);
#endif

        if (tp->DASH)
            auto_nego |= (ADVERTISE_100FULL | ADVERTISE_100HALF | ADVERTISE_10HALF | ADVERTISE_10FULL);

        if (((tp->mcfg == CFG_METHOD_7) || (tp->mcfg == CFG_METHOD_8)) && (RTL_R16(CPlusCmd) & ASF))
            auto_nego |= (ADVERTISE_100FULL | ADVERTISE_100HALF | ADVERTISE_10HALF | ADVERTISE_10FULL);

        giga_ctrl = mdio_read(tp, MII_CTRL1000) & ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
        mdio_write(tp, MII_ADVERTISE, auto_nego);
        mdio_write(tp, MII_CTRL1000, giga_ctrl);
        mdio_write(tp, MII_BMCR, BMCR_RESET | BMCR_ANENABLE | BMCR_ANRESTART);
        spin_unlock_irqrestore(&tp->phy_lock, flags);

        RTL_W32(RxConfig, RTL_R32(RxConfig) | AcceptBroadcast | AcceptMulticast | AcceptMyPhys);

        return;
    }

    if (tp->DASH)
        return;

    if (((tp->mcfg == CFG_METHOD_7) || (tp->mcfg == CFG_METHOD_8)) && (RTL_R16(CPlusCmd) & ASF))
        return;

    rtl8168_phy_power_down(dev);
    RTL_W8(PMCH, RTL_R8(PMCH) & ~BIT_7);
}

static void rtl8168_powerup_pll(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

	RTL_W8(PMCH, RTL_R8(PMCH) | BIT_7 | BIT_6);
    rtl8168_phy_power_up (dev);
}

static void
rtl8168_get_wol(struct net_device *dev,
                struct ethtool_wolinfo *wol)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u8 options;
    unsigned long flags;

    wol->wolopts = 0;

    if (tp->mcfg == CFG_METHOD_DEFAULT) {
        wol->supported = 0;
        return;
    } else {
        wol->supported = WAKE_ANY;
    }

    spin_lock_irqsave(&tp->lock, flags);

    options = RTL_R8(Config1);
    if (!(options & PMEnable))
        goto out_unlock;

    wol->wolopts = tp->wol_opts;

out_unlock:
    spin_unlock_irqrestore(&tp->lock, flags);
}

static int
rtl8168_set_wol(struct net_device *dev,
                struct ethtool_wolinfo *wol)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned long flags;

    if (tp->mcfg == CFG_METHOD_DEFAULT)
        return -EOPNOTSUPP;

    spin_lock_irqsave(&tp->lock, flags);

    tp->wol_opts = wol->wolopts;

    tp->wol_enabled = (tp->wol_opts) ? WOL_ENABLED : WOL_DISABLED;

    spin_unlock_irqrestore(&tp->lock, flags);

    return 0;
}

static void
rtl8168_get_drvinfo(struct net_device *dev,
                    struct ethtool_drvinfo *info)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    strcpy(info->driver, MODULENAME);
    strcpy(info->version, RTL8168_VERSION);
    strcpy(info->bus_info, "platform");
    info->regdump_len = R8168_REGS_SIZE;
    info->eedump_len = tp->eeprom_len;
}

static int
rtl8168_get_regs_len(struct net_device *dev)
{
    return R8168_REGS_SIZE;
}

static int
rtl8168_set_speed_xmii(struct net_device *dev,
                       u8 autoneg,
                       u16 speed,
                       u8 duplex)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    int auto_nego = 0;
    int giga_ctrl = 0;
    int bmcr_true_force = 0;
    unsigned long flags;

    if ((speed != SPEED_1000) &&
        (speed != SPEED_100) &&
        (speed != SPEED_10)) {
        speed = SPEED_100;
        duplex = DUPLEX_FULL;
    }

    auto_nego = mdio_read(tp, MII_ADVERTISE);
    auto_nego &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | ADVERTISE_100FULL | ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);

    giga_ctrl = mdio_read(tp, MII_CTRL1000);
    giga_ctrl &= ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);

    if ((autoneg == AUTONEG_ENABLE) || (speed == SPEED_1000)) {
        /*n-way force*/
        if ((speed == SPEED_10) && (duplex == DUPLEX_HALF)) {
            auto_nego |= ADVERTISE_10HALF;
        } else if ((speed == SPEED_10) && (duplex == DUPLEX_FULL)) {
            auto_nego |= ADVERTISE_10HALF |
                         ADVERTISE_10FULL;
        } else if ((speed == SPEED_100) && (duplex == DUPLEX_HALF)) {
            auto_nego |= ADVERTISE_100HALF |
                         ADVERTISE_10HALF |
                         ADVERTISE_10FULL;
        } else if ((speed == SPEED_100) && (duplex == DUPLEX_FULL)) {
            auto_nego |= ADVERTISE_100HALF |
                         ADVERTISE_100FULL |
                         ADVERTISE_10HALF |
                         ADVERTISE_10FULL;
        } else if (speed == SPEED_1000) {
            giga_ctrl |= ADVERTISE_1000HALF |
                         ADVERTISE_1000FULL;

            auto_nego |= ADVERTISE_100HALF |
                         ADVERTISE_100FULL |
                         ADVERTISE_10HALF |
                         ADVERTISE_10FULL;
        }

        //flow contorol
        if (dev->mtu <= ETH_DATA_LEN)
            auto_nego |= ADVERTISE_PAUSE_CAP|ADVERTISE_PAUSE_ASYM;

        tp->phy_auto_nego_reg = auto_nego;
        tp->phy_1000_ctrl_reg = giga_ctrl;

        spin_lock_irqsave(&tp->phy_lock, flags);
        mdio_write(tp, 0x1f, 0x0000);
        mdio_write(tp, MII_ADVERTISE, auto_nego);
        mdio_write(tp, MII_CTRL1000, giga_ctrl);
        mdio_write(tp, MII_BMCR, BMCR_RESET | BMCR_ANENABLE | BMCR_ANRESTART);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        mdelay(20);
    } else {
        /*true force*/
#ifndef BMCR_SPEED100
#define BMCR_SPEED100   0x0040
#endif

#ifndef BMCR_SPEED10
#define BMCR_SPEED10    0x0000
#endif
        if ((speed == SPEED_10) && (duplex == DUPLEX_HALF)) {
            bmcr_true_force = BMCR_SPEED10;
        } else if ((speed == SPEED_10) && (duplex == DUPLEX_FULL)) {
            bmcr_true_force = BMCR_SPEED10 | BMCR_FULLDPLX;
        } else if ((speed == SPEED_100) && (duplex == DUPLEX_HALF)) {
            bmcr_true_force = BMCR_SPEED100;
        } else if ((speed == SPEED_100) && (duplex == DUPLEX_FULL)) {
            bmcr_true_force = BMCR_SPEED100 | BMCR_FULLDPLX;
        }

        spin_lock_irqsave(&tp->phy_lock, flags);
        mdio_write(tp, 0x1f, 0x0000);
        mdio_write(tp, MII_BMCR, bmcr_true_force);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
    }

    tp->autoneg = autoneg;
    tp->speed = speed;
    tp->duplex = duplex;

    return 0;
}

static int
rtl8168_set_speed(struct net_device *dev,
                  u8 autoneg,
                  u16 speed,
                  u8 duplex)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    int ret;

    ret = tp->set_speed(dev, autoneg, speed, duplex);

    return ret;
}

static int
rtl8168_set_settings(struct net_device *dev,
                     struct ethtool_cmd *cmd)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    int ret;
    unsigned long flags;

    spin_lock_irqsave(&tp->lock, flags);
    ret = rtl8168_set_speed(dev, cmd->autoneg, cmd->speed, cmd->duplex);
    spin_unlock_irqrestore(&tp->lock, flags);

    return ret;
}

#ifdef CONFIG_R8168_VLAN

static inline u32
rtl8168_tx_vlan_tag(struct rtl8168_private *tp,
                    struct sk_buff *skb)
{
    u32 tag;

    tag = (vlan_tx_tag_present(skb)) ?
          TxVlanTag | swab16(vlan_tx_tag_get(skb)) : 0x00;
    return tag;
}

static int
rtl8168_rx_vlan_skb(struct rtl8168_private *tp,
                    struct RxDesc *desc,
                    struct sk_buff *skb)
{
    u32 opts2 = le32_to_cpu(desc->opts2);
    int ret = -1;


    if (opts2 & RxVlanTag)
        __vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), swab16(opts2 & 0xffff));

    desc->opts2 = 0;
    return ret;
}

#else /* !CONFIG_R8168_VLAN */

static inline u32
rtl8168_tx_vlan_tag(struct rtl8168_private *tp,
                    struct sk_buff *skb)
{
    return 0;
}

static int
rtl8168_rx_vlan_skb(struct rtl8168_private *tp,
                    struct RxDesc *desc,
                    struct sk_buff *skb)
{
    return -1;
}

#endif


static netdev_features_t rtl8168_fix_features(struct net_device *dev,
        netdev_features_t features)

{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned long flags;

    spin_lock_irqsave(&tp->lock, flags);
    if (dev->mtu > ETH_DATA_LEN) {
        features &= ~NETIF_F_ALL_TSO;
        features &= ~NETIF_F_ALL_CSUM;
    }
    spin_unlock_irqrestore(&tp->lock, flags);

    return features;
}

static int rtl8168_hw_set_features(struct net_device *dev, u32 features)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

    if (features & NETIF_F_RXCSUM)
        tp->cp_cmd |= RxChkSum;
    else
        tp->cp_cmd &= ~RxChkSum;

    if (dev->features & NETIF_F_HW_VLAN_RX)
        tp->cp_cmd |= RxVlan;
    else
        tp->cp_cmd &= ~RxVlan;

    RTL_W16(CPlusCmd, tp->cp_cmd);
    RTL_R16(CPlusCmd);

    return 0;
}


static int rtl8168_set_features(struct net_device *dev,
                                netdev_features_t features)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned long flags;

    spin_lock_irqsave(&tp->lock, flags);

    rtl8168_hw_set_features(dev, features);

    spin_unlock_irqrestore(&tp->lock, flags);

    return 0;
}


static void rtl8168_gset_xmii(struct net_device *dev,
                              struct ethtool_cmd *cmd)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u8 status;
    unsigned long flags;

    cmd->supported = SUPPORTED_10baseT_Half |
                     SUPPORTED_10baseT_Full |
                     SUPPORTED_100baseT_Half |
                     SUPPORTED_100baseT_Full |
                     SUPPORTED_1000baseT_Full |
                     SUPPORTED_Autoneg |
                     SUPPORTED_TP;

    spin_lock_irqsave(&tp->phy_lock, flags);
    cmd->autoneg = (mdio_read(tp, MII_BMCR) & BMCR_ANENABLE) ? 1 : 0;
    spin_unlock_irqrestore(&tp->phy_lock, flags);
    cmd->advertising = ADVERTISED_TP | ADVERTISED_Autoneg;

    if (tp->phy_auto_nego_reg & ADVERTISE_10HALF)
        cmd->advertising |= ADVERTISED_10baseT_Half;
    if (tp->phy_auto_nego_reg & ADVERTISE_10FULL)
        cmd->advertising |= ADVERTISED_10baseT_Full;
    if (tp->phy_auto_nego_reg & ADVERTISE_100HALF)
        cmd->advertising |= ADVERTISED_100baseT_Half;
    if (tp->phy_auto_nego_reg & ADVERTISE_100FULL)
        cmd->advertising |= ADVERTISED_100baseT_Full;
    if (tp->phy_1000_ctrl_reg & ADVERTISE_1000FULL)
        cmd->advertising |= ADVERTISED_1000baseT_Full;

    status = RTL_R8(PHYstatus);

    if (status & _1000bpsF)
        cmd->speed = SPEED_1000;
    else if (status & _100bps)
        cmd->speed = SPEED_100;
    else if (status & _10bps)
        cmd->speed = SPEED_10;

    if (status & TxFlowCtrl)
        cmd->advertising |= ADVERTISED_Asym_Pause;

    if (status & RxFlowCtrl)
        cmd->advertising |= ADVERTISED_Pause;

    cmd->duplex = ((status & _1000bpsF) || (status & FullDup)) ?
                  DUPLEX_FULL : DUPLEX_HALF;


}

static int
rtl8168_get_settings(struct net_device *dev,
                     struct ethtool_cmd *cmd)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    tp->get_settings(dev, cmd);

    return 0;
}

static void rtl8168_get_regs(struct net_device *dev, struct ethtool_regs *regs,
                             void *p)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned int i;
    u8 *data = p;
    unsigned long flags;

    if (regs->len > R8168_REGS_SIZE)
        regs->len = R8168_REGS_SIZE;

    spin_lock_irqsave(&tp->lock, flags);
    for (i = 0; i < regs->len; i++)
        data[i] = readb(tp->mmio_addr + i);
    spin_unlock_irqrestore(&tp->lock, flags);
}

static u32
rtl8168_get_msglevel(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    return tp->msg_enable;
}

static void
rtl8168_set_msglevel(struct net_device *dev,
                     u32 value)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    tp->msg_enable = value;
}

static const char rtl8168_gstrings[][ETH_GSTRING_LEN] = {
    "tx_packets",
    "rx_packets",
    "tx_errors",
    "rx_errors",
    "rx_missed",
    "align_errors",
    "tx_single_collisions",
    "tx_multi_collisions",
    "unicast",
    "broadcast",
    "multicast",
    "tx_aborted",
    "tx_underrun",
};

struct rtl8168_counters {
    u64 tx_packets;
    u64 rx_packets;
    u64 tx_errors;
    u32 rx_errors;
    u16 rx_missed;
    u16 align_errors;
    u32 tx_one_collision;
    u32 tx_multi_collision;
    u64 rx_unicast;
    u64 rx_broadcast;
    u32 rx_multicast;
    u16 tx_aborted;
    u16 tx_underun;
};


static int rtl8168_get_sset_count(struct net_device *dev, int sset)
{
    switch (sset) {
    case ETH_SS_STATS:
        return ARRAY_SIZE(rtl8168_gstrings);
    default:
        return -EOPNOTSUPP;
    }
}

static void
rtl8168_get_ethtool_stats(struct net_device *dev,
                          struct ethtool_stats *stats,
                          u64 *data)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    struct rtl8168_counters *counters;
    dma_addr_t paddr;
    u32 cmd;

    ASSERT_RTNL();

    counters = tp->tally_vaddr;
    paddr = tp->tally_paddr;
    if (!counters)
        return;

    RTL_W32(CounterAddrHigh, (u64)paddr >> 32);
    cmd = (u64)paddr & DMA_BIT_MASK(32);
    RTL_W32(CounterAddrLow, cmd);
    RTL_W32(CounterAddrLow, cmd | CounterDump);

    while (RTL_R32(CounterAddrLow) & CounterDump) {
        if (msleep_interruptible(1))
            break;
    }

    data[0] = le64_to_cpu(counters->tx_packets);
    data[1] = le64_to_cpu(counters->rx_packets);
    data[2] = le64_to_cpu(counters->tx_errors);
    data[3] = le32_to_cpu(counters->rx_errors);
    data[4] = le16_to_cpu(counters->rx_missed);
    data[5] = le16_to_cpu(counters->align_errors);
    data[6] = le32_to_cpu(counters->tx_one_collision);
    data[7] = le32_to_cpu(counters->tx_multi_collision);
    data[8] = le64_to_cpu(counters->rx_unicast);
    data[9] = le64_to_cpu(counters->rx_broadcast);
    data[10] = le32_to_cpu(counters->rx_multicast);
    data[11] = le16_to_cpu(counters->tx_aborted);
    data[12] = le16_to_cpu(counters->tx_underun);
}

static void
rtl8168_get_strings(struct net_device *dev,
                    u32 stringset,
                    u8 *data)
{
    switch (stringset) {
    case ETH_SS_STATS:
        memcpy(data, *rtl8168_gstrings, sizeof(rtl8168_gstrings));
        break;
    }
}


#undef ethtool_op_get_link
#define ethtool_op_get_link _kc_ethtool_op_get_link
static u32 _kc_ethtool_op_get_link(struct net_device *dev)
{
    return netif_carrier_ok(dev) ? 1 : 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
#undef ethtool_op_get_sg
#define ethtool_op_get_sg _kc_ethtool_op_get_sg
u32 _kc_ethtool_op_get_sg(struct net_device *dev)
{
#ifdef NETIF_F_SG
    return (dev->features & NETIF_F_SG) != 0;
#else
    return 0;
#endif
}

#undef ethtool_op_set_sg
#define ethtool_op_set_sg _kc_ethtool_op_set_sg
int _kc_ethtool_op_set_sg(struct net_device *dev, u32 data)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    if (tp->mcfg == CFG_METHOD_DEFAULT)
        return -EOPNOTSUPP;

#ifdef NETIF_F_SG
    if (data)
        dev->features |= NETIF_F_SG;
    else
        dev->features &= ~NETIF_F_SG;
#endif

    return 0;
}
#endif

static const struct ethtool_ops rtl8168_ethtool_ops = {
    .get_drvinfo        = rtl8168_get_drvinfo,
    .get_regs_len       = rtl8168_get_regs_len,
    .get_link			= ethtool_op_get_link,
    .get_settings       = rtl8168_get_settings,
    .set_settings       = rtl8168_set_settings,
    .get_msglevel       = rtl8168_get_msglevel,
    .set_msglevel       = rtl8168_set_msglevel,
    .get_regs       	= rtl8168_get_regs,
    .get_wol        	= rtl8168_get_wol,
    .set_wol        	= rtl8168_set_wol,
    .get_strings        = rtl8168_get_strings,
    .get_sset_count     = rtl8168_get_sset_count,
    .get_ethtool_stats  = rtl8168_get_ethtool_stats,
    .get_ts_info        = ethtool_op_get_ts_info,

};


static int rtl8168_enable_EEE(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;
    int ret;
    u16 data;
    unsigned long flags;

    ret = 0;
    switch (tp->mcfg) {
    case CFG_METHOD_18:
        spin_lock_irqsave(&tp->phy_lock, flags);
        data = rtl8168_eri_read(ioaddr,0x1B0 ,4,ERIAR_ExGMAC);
        data |= BIT_1 | BIT_0;
        rtl8168_eri_write(ioaddr, 0x1B0, 4, data, ERIAR_ExGMAC);
        mdio_write(tp, 0x1F, 0x0007);
        mdio_write(tp, 0x1e, 0x0020);
        data = mdio_read(tp, 0x15);
        data |= BIT_8;
        mdio_write(tp, 0x15, data);
        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B85);
        data = mdio_read(tp, 0x06);
        data |= BIT_13;
        mdio_write(tp, 0x06, data);
        mdio_write(tp, 0x1F, 0x0000);
        mdio_write(tp, 0x0D, 0x0007);
        mdio_write(tp, 0x0E, 0x003C);
        mdio_write(tp, 0x0D, 0x4007);
        mdio_write(tp, 0x0E, 0x0006);
        mdio_write(tp, 0x0D, 0x0000);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;

    case CFG_METHOD_21:
        spin_lock_irqsave(&tp->phy_lock, flags);
        data = rtl8168_eri_read(ioaddr, 0x1B0, 4, ERIAR_ExGMAC);
        data |= BIT_1 | BIT_0;
        rtl8168_eri_write(ioaddr, 0x1B0, 4, data, ERIAR_ExGMAC);
        mdio_write(tp, 0x1F, 0x0A43);
        data = mdio_read(tp, 0x11);
        mdio_write(tp, 0x11, data | BIT_4);
        mdio_write(tp, 0x1F, 0x0A5D);
        mdio_write(tp, 0x10, 0x0006);
        mdio_write(tp, 0x1F, 0x0000);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;

    default:
//      dev_printk(KERN_DEBUG, &tp->platform_dev->dev, "Not Support EEE\n");
        ret = -EOPNOTSUPP;
    }

    return ret;
}

static int rtl8168_disable_EEE(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;
    int ret;
    u16 data;
    unsigned long flags;

    ret = 0;
    switch (tp->mcfg) {
	case CFG_METHOD_18:
        spin_lock_irqsave(&tp->phy_lock, flags);
        data = rtl8168_eri_read(ioaddr,0x1B0 ,4,ERIAR_ExGMAC);
        data &= ~(BIT_1 | BIT_0);
        rtl8168_eri_write(ioaddr, 0x1B0, 4, data, ERIAR_ExGMAC);
        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B85);
        data = mdio_read(tp, 0x06);
        data &= ~BIT_13;
        mdio_write(tp, 0x06, data);
        mdio_write(tp, 0x1F, 0x0007);
        mdio_write(tp, 0x1e, 0x0020);
        data = mdio_read(tp, 0x15);
        data &= ~BIT_8;
        mdio_write(tp, 0x15, data);
        mdio_write(tp, 0x1F, 0x0000);
        mdio_write(tp, 0x0D, 0x0007);
        mdio_write(tp, 0x0E, 0x003C);
        mdio_write(tp, 0x0D, 0x4007);
        mdio_write(tp, 0x0E, 0x0000);
        mdio_write(tp, 0x0D, 0x0000);
        mdio_write(tp, 0x1F, 0x0000);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;

    case CFG_METHOD_21:
        spin_lock_irqsave(&tp->phy_lock, flags);
        data = rtl8168_eri_read(ioaddr, 0x1B0, 4, ERIAR_ExGMAC);
        data &= ~(BIT_1 | BIT_0);
        rtl8168_eri_write(ioaddr, 0x1B0, 4, data, ERIAR_ExGMAC);
        mdio_write(tp, 0x1F, 0x0A43);
        data = mdio_read(tp, 0x11);
        mdio_write(tp, 0x11, data & ~BIT_4);
        mdio_write(tp, 0x1F, 0x0A5D);
        mdio_write(tp, 0x10, 0x0000);
        mdio_write(tp, 0x1F, 0x0000);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;

    default:
//      dev_printk(KERN_DEBUG, &tp->platform_dev->dev, "Not Support EEE\n");
        ret = -EOPNOTSUPP;
        break;
    }

    /*Advanced EEE*/
    switch (tp->mcfg) {
    case CFG_METHOD_25:
        rtl8168_eri_write(ioaddr, 0x1EA, 1, 0x00, ERIAR_ExGMAC);

        spin_lock_irqsave(&tp->phy_lock, flags);
        mdio_write(tp, 0x1F, 0x0A42);
        data = mdio_read(tp, 0x16);
        data &= ~(BIT_1);
        mdio_write(tp, 0x16, data);
        mdio_write(tp, 0x1F, 0x0000);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;
    case CFG_METHOD_26:
        data = mac_ocp_read(tp, 0xE052);
        data &= ~(BIT_0);
        mac_ocp_write(tp, 0xE052, data);

        spin_lock_irqsave(&tp->phy_lock, flags);
        mdio_write(tp, 0x1F, 0x0A42);
        data = mdio_read(tp, 0x16);
        data &= ~(BIT_1);
        mdio_write(tp, 0x16, data);
        mdio_write(tp, 0x1F, 0x0000);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;
    case CFG_METHOD_27:
    case CFG_METHOD_28:
        data = mac_ocp_read(tp, 0xE052);
        data &= ~(BIT_0);
        mac_ocp_write(tp, 0xE052, data);
        break;
    }

    return ret;
}

static void rtl8168_get_mac_version(struct rtl8168_private *tp, void __iomem *ioaddr)
{
    u32 reg,val32;
    u32 ICVerID;

    val32 = RTL_R32(TxConfig)  ;
    reg = val32 & 0x7c800000;
    ICVerID = val32 & 0x00700000;

    switch (reg) {
    case 0x30000000:
        tp->mcfg = CFG_METHOD_1;
        tp->efuse = EFUSE_NOT_SUPPORT;
        break;
    case 0x38000000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_2;
        } else if (ICVerID == 0x00500000) {
            tp->mcfg = CFG_METHOD_3;
        } else {
            tp->mcfg = CFG_METHOD_3;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_NOT_SUPPORT;
        break;
    case 0x3C000000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_4;
        } else if (ICVerID == 0x00200000) {
            tp->mcfg = CFG_METHOD_5;
        } else if (ICVerID == 0x00400000) {
            tp->mcfg = CFG_METHOD_6;
        } else {
            tp->mcfg = CFG_METHOD_6;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_NOT_SUPPORT;
        break;
    case 0x3C800000:
        if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_7;
        } else if (ICVerID == 0x00300000) {
            tp->mcfg = CFG_METHOD_8;
        } else {
            tp->mcfg = CFG_METHOD_8;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_NOT_SUPPORT;
        break;
    case 0x28000000:
        if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_9;
        } else if (ICVerID == 0x00300000) {
            tp->mcfg = CFG_METHOD_10;
        } else {
            tp->mcfg = CFG_METHOD_10;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x28800000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_11;
        } else if (ICVerID == 0x00200000) {
            tp->mcfg = CFG_METHOD_12;
            RTL_W32(0xD0, RTL_R32(0xD0) | 0x00020000);
        } else if (ICVerID == 0x00300000) {
            tp->mcfg = CFG_METHOD_13;
        } else {
            tp->mcfg = CFG_METHOD_13;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x2C000000:
        if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_14;
        } else if (ICVerID == 0x00200000) {
            tp->mcfg = CFG_METHOD_15;
        } else {
            tp->mcfg = CFG_METHOD_15;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x2C800000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_16;
        } else if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_17;
        } else {
            tp->mcfg = CFG_METHOD_17;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x48000000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_18;
        } else if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_19;
        } else {
            tp->mcfg = CFG_METHOD_19;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x48800000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_20;
        } else {
            tp->mcfg = CFG_METHOD_20;
            tp->HwIcVerUnknown = TRUE;
        }

        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x4C000000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_21;
        } else if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_22;
        } else {
            tp->mcfg = CFG_METHOD_22;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x50000000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_23;
        } else if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_27;
        } else if (ICVerID == 0x00200000) {
            tp->mcfg = CFG_METHOD_28;
        } else {
            tp->mcfg = CFG_METHOD_28;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x50800000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_24;
        } else if (ICVerID == 0x00100000) {
            tp->mcfg = CFG_METHOD_25;
        } else {
            tp->mcfg = CFG_METHOD_25;
            tp->HwIcVerUnknown = TRUE;
        }
        tp->efuse = EFUSE_SUPPORT;
        break;
    case 0x5C800000:
        if (ICVerID == 0x00000000) {
            tp->mcfg = CFG_METHOD_26;
        } else {
            tp->mcfg = CFG_METHOD_26;
            tp->HwIcVerUnknown = TRUE;
        }

        tp->efuse = EFUSE_SUPPORT;
        break;
    default:
        printk("unknown chip version (%x)\n",reg);
        tp->mcfg = CFG_METHOD_DEFAULT;
        tp->HwIcVerUnknown = TRUE;
        tp->efuse = EFUSE_NOT_SUPPORT;
        break;
    }
}

static void
rtl8168_print_mac_version(struct rtl8168_private *tp)
{
    int i;
    for (i = ARRAY_SIZE(rtl_chip_info) - 1; i >= 0; i--) {
        if (tp->mcfg == rtl_chip_info[i].mcfg) {
            dprintk("Realtek PCIe GBE Family Controller mcfg = %04d\n",
                    rtl_chip_info[i].mcfg);
            return;
        }
    }

    dprintk("mac_version == Unknown\n");
}

static void
rtl8168_tally_counter_addr_fill(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (!tp->tally_paddr)
        return;

    RTL_W32(CounterAddrHigh, (u64)tp->tally_paddr >> 32);
    RTL_W32(CounterAddrLow, (u64)tp->tally_paddr & (DMA_BIT_MASK(32)));
}

static void
rtl8168_tally_counter_clear(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (!tp->tally_paddr)
        return;

    RTL_W32(CounterAddrHigh, (u64)tp->tally_paddr >> 32);
    RTL_W32(CounterAddrLow, (u64)tp->tally_paddr & (DMA_BIT_MASK(32) | BIT_0));
}

static void
rtl8168_exit_oob(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u16 data16;

    rtl8168_nic_reset(dev);

	if(tp->mcfg == CFG_METHOD_21) {
        RTL_W8(MCUCmd_reg, RTL_R8(MCUCmd_reg) & ~Now_is_oob);

        data16 = mac_ocp_read(tp, 0xE8DE) & ~BIT_14;
        mac_ocp_write(tp, 0xE8DE, data16);
        rtl8168_wait_ll_share_fifo_ready(dev);

        data16 = mac_ocp_read(tp, 0xE8DE) | BIT_15;
        mac_ocp_write(tp, 0xE8DE, data16);

        rtl8168_wait_ll_share_fifo_ready(dev); 
    }
}

static void
rtl8168_hw_mac_mcu_config(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned int i;


    if (tp->NotWrMcuPatchCode == TRUE) return;

    if (tp->mcfg == CFG_METHOD_21) {
        u16 rtl8111g_phy_value[]= {
            0xE008, 0xE01B, 0xE01D, 0xE01F, 0xE022,
            0xE025, 0xE031, 0xE04D, 0x49D2, 0xF10D,
            0x766C, 0x49E2, 0xF00A, 0x1EC0, 0x8EE1,
            0xC60A, 0x77C0, 0x4870, 0x9FC0, 0x1EA0,
            0xC707, 0x8EE1, 0x9D6C, 0xC603, 0xBE00,
            0xB416, 0x0076, 0xE86C, 0xC602, 0xBE00,
            0xA000, 0xC602, 0xBE00, 0x0000, 0x1B76,
            0xC202, 0xBA00, 0x059C, 0x1B76, 0xC602,
            0xBE00, 0x065A, 0x74E6, 0x1B78, 0x46DC,
            0x1300, 0xF005, 0x74F8, 0x48C3, 0x48C4,
            0x8CF8, 0x64E7, 0xC302, 0xBB00, 0x06A0,
            0x74E4, 0x49C5, 0xF106, 0x49C6, 0xF107,
            0x48C8, 0x48C9, 0xE011, 0x48C9, 0x4848,
            0xE00E, 0x4848, 0x49C7, 0xF00A, 0x48C9,
            0xC60D, 0x1D1F, 0x8DC2, 0x1D00, 0x8DC3,
            0x1D11, 0x8DC0, 0xE002, 0x4849, 0x94E5,
            0xC602, 0xBE00, 0x01F0, 0xE434, 0x49D9,
            0xF01B, 0xC31E, 0x7464, 0x49C4, 0xF114,
            0xC31B, 0x6460, 0x14FA, 0xFA02, 0xE00F,
            0xC317, 0x7460, 0x49C0, 0xF10B, 0xC311,
            0x7462, 0x48C1, 0x9C62, 0x4841, 0x9C62,
            0xC30A, 0x1C04, 0x8C60, 0xE004, 0x1C15,
            0xC305, 0x8C60, 0xC602, 0xBE00, 0x0384,
            0xE434, 0xE030, 0xE61C, 0xE906
        };
        mac_ocp_write(tp, 0xFC28, 0x0000);
        mac_ocp_write(tp, 0xFC2A, 0x0000);
        mac_ocp_write(tp, 0xFC2C, 0x0000);
        mac_ocp_write(tp, 0xFC2E, 0x0000);
        mac_ocp_write(tp, 0xFC30, 0x0000);
        mac_ocp_write(tp, 0xFC32, 0x0000);
        mac_ocp_write(tp, 0xFC34, 0x0000);
        mac_ocp_write(tp, 0xFC36, 0x0000);
        mdelay(3);
        mac_ocp_write(tp, 0xFC26, 0x0000);
        for (i = 0; i < ARRAY_SIZE(rtl8111g_phy_value); i++)
            mac_ocp_write(tp, 0xF800+i*2, rtl8111g_phy_value[i]);
        mac_ocp_write(tp, 0xFC26, 0x8000);
        mac_ocp_write(tp, 0xFC28, 0x0075);
        mac_ocp_write(tp, 0xFC2E, 0x059B);
        mac_ocp_write(tp, 0xFC30, 0x0659);
        mac_ocp_write(tp, 0xFC32, 0x0000);
        mac_ocp_write(tp, 0xFC34, 0x0000);
        mac_ocp_write(tp, 0xFC36, 0x0000);
    } 
}

static void
rtl8168_hw_init(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

	RTL_W8(Cfg9346, Cfg9346_Unlock);
	RTL_W8(Config5, RTL_R8(Config5) & ~BIT_0);
	RTL_W8(Config2, RTL_R8(Config2) & ~BIT_7);
	RTL_W8(Cfg9346, Cfg9346_Lock);
	RTL_W8(0xF1, RTL_R8(0xF1) & ~BIT_7);

	if(tp->mcfg == CFG_METHOD_18)
		 RTL_W8(0xF2, (RTL_R8(0xF2) % ~(BIT_2 | BIT_1 | BIT_0)));   


    switch (tp->mcfg) {
    case CFG_METHOD_18:
        if (aspm) {
            RTL_W8(0x6E, RTL_R8(0x6E) | BIT_6);
            rtl8168_eri_write(ioaddr, 0x1AE, 2, 0x0403, ERIAR_ExGMAC);
        }
        break;
    case CFG_METHOD_21:
        if (aspm) {
            if (RTL_R8(Config5) & BIT_3) {
                RTL_W8(0x6E, RTL_R8(0x6E) | BIT_6);
                rtl8168_eri_write(ioaddr, 0x1AE, 2, 0x0403, ERIAR_ExGMAC);
            }
        }
        break;
    }

	if(tp->mcfg == CFG_METHOD_21) {
		rtl8168_eri_write(ioaddr, 0x174, 2, 0x0000, ERIAR_ExGMAC);
        mac_ocp_write(tp, 0xE428, 0x0010);
	}

    rtl8168_hw_mac_mcu_config(dev);
}

static void
rtl8168_hw_ephy_config(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u16 ephy_data;


    
    if (tp->mcfg == CFG_METHOD_18) 
	{
        ephy_data = rtl8168_ephy_read(ioaddr, 0x06);
        ephy_data |= BIT_5;
        ephy_data &= ~(BIT_7 | BIT_6);
        rtl8168_ephy_write(ioaddr, 0x06, ephy_data);

        ephy_data = rtl8168_ephy_read(ioaddr, 0x08);
        ephy_data |= BIT_1;
        ephy_data &= ~BIT_0;
        rtl8168_ephy_write(ioaddr, 0x08, ephy_data);

        ephy_data = rtl8168_ephy_read(ioaddr, 0x09);
        ephy_data |= BIT_7;
        rtl8168_ephy_write(ioaddr, 0x09, ephy_data);

        ephy_data = rtl8168_ephy_read(ioaddr, 0x19);
        ephy_data |= (BIT_2 | BIT_5 | BIT_9);
        rtl8168_ephy_write(ioaddr, 0x19, ephy_data);
    } else if (tp->mcfg == CFG_METHOD_21) {

        ephy_data = rtl8168_ephy_read(ioaddr, 0x00);
        ephy_data &= ~(BIT_3);
        rtl8168_ephy_write(ioaddr, 0x00, ephy_data);
        ephy_data = rtl8168_ephy_read(ioaddr, 0x0C);
        ephy_data &= ~(BIT_13 | BIT_12 | BIT_11 | BIT_10 | BIT_9 | BIT_8 | BIT_7 | BIT_6 | BIT_5 | BIT_4);
        ephy_data |= (BIT_5 | BIT_11);
        rtl8168_ephy_write(ioaddr, 0x0C, ephy_data);

        ephy_data = rtl8168_ephy_read(ioaddr, 0x1E);
        ephy_data |= (BIT_0);
        rtl8168_ephy_write(ioaddr, 0x1E, ephy_data);

        ephy_data = rtl8168_ephy_read(ioaddr, 0x19);
        ephy_data &= ~(BIT_15);
        rtl8168_ephy_write(ioaddr, 0x19, ephy_data);
    } 
}

static int
rtl8168_check_hw_phy_mcu_code_ver(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    int ram_code_ver_match = 0;
    u16 sw_ram_code_ver = 0xFFFF;
    u16 hw_ram_code_ver = 0;

    switch (tp->mcfg) {
    case CFG_METHOD_18:
		sw_ram_code_ver = NIC_RAMCODE_VERSION_CFG_METHOD_18;
        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B30);
        hw_ram_code_ver = mdio_read(tp, 0x06);
        mdio_write(tp, 0x1F, 0x0000);
        break;
    case CFG_METHOD_21:
		sw_ram_code_ver = NIC_RAMCODE_VERSION_CFG_METHOD_21;
        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0x801E);
        hw_ram_code_ver = mdio_read(tp, 0x14);
        mdio_write(tp, 0x1F, 0x0000);
        break;
    }

    if( hw_ram_code_ver == sw_ram_code_ver) {
        ram_code_ver_match = 1;
        tp->HwHasWrRamCodeToMicroP = TRUE;
    }

    return ram_code_ver_match;
}

static void
rtl8168_write_hw_phy_mcu_code_ver(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    //u16 sw_ram_code_ver = 0xFFFF;

    switch (tp->mcfg) {
    case CFG_METHOD_18:
        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B30);
        mdio_write(tp, 0x06, NIC_RAMCODE_VERSION_CFG_METHOD_18);
        mdio_write(tp, 0x1F, 0x0000);
        break;
    case CFG_METHOD_21:
        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0x801E);
        mdio_write(tp, 0x14, NIC_RAMCODE_VERSION_CFG_METHOD_21);
        mdio_write(tp, 0x1F, 0x0000);
        break;
    }
}

static void
rtl8168_init_hw_phy_mcu(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned int gphy_val,i;
    //u32 csi_tmp;

    if (tp->NotWrRamCodeToMicroP == TRUE) return;
    if (rtl8168_check_hw_phy_mcu_code_ver(dev)) return;

    if (tp->mcfg == CFG_METHOD_18) {
        mdio_write(tp,0x1f, 0x0000);
        mdio_write(tp,0x00, 0x1800);
        gphy_val = mdio_read(tp, 0x15);
        gphy_val &= ~(BIT_12);
        mdio_write(tp,0x15, gphy_val);
        mdio_write(tp,0x00, 0x4800);
        mdio_write(tp,0x1f, 0x0007);
        mdio_write(tp,0x1e, 0x002f);
        for (i = 0; i < 1000; i++) {
            udelay(100);
            gphy_val = mdio_read(tp, 0x1c);
            if (gphy_val & 0x0080)
                break;
        }
        mdio_write(tp,0x1f, 0x0000);
        mdio_write(tp,0x00, 0x1800);
        mdio_write(tp,0x1f, 0x0007);
        mdio_write(tp,0x1e, 0x0023);
        for (i = 0; i < 200; i++) {
            udelay(100);
            gphy_val = mdio_read(tp, 0x18);
            if (!(gphy_val & 0x0001))
                break;
        }
        mdio_write(tp, 0x1f, 0x0005);
        mdio_write(tp, 0x05, 0xfff6);
        mdio_write(tp, 0x06, 0x0080);
        mdio_write(tp, 0x1f, 0x0007);
        mdio_write(tp, 0x1e, 0x0023);
        mdio_write(tp, 0x16, 0x0306);
        mdio_write(tp, 0x16, 0x0307);
        mdio_write(tp, 0x15, 0x0194);
        mdio_write(tp, 0x19, 0x407D);
        mdio_write(tp, 0x15, 0x0098);
        mdio_write(tp, 0x19, 0x7c0b);
        mdio_write(tp, 0x15, 0x0099);
        mdio_write(tp, 0x19, 0x6c0b);
        mdio_write(tp, 0x15, 0x00eb);
        mdio_write(tp, 0x19, 0x6c0b);
        mdio_write(tp, 0x15, 0x00f8);
        mdio_write(tp, 0x19, 0x6f0b);
        mdio_write(tp, 0x15, 0x00fe);
        mdio_write(tp, 0x19, 0x6f0f);
        mdio_write(tp, 0x15, 0x00db);
        mdio_write(tp, 0x19, 0x6f09);
        mdio_write(tp, 0x15, 0x00dc);
        mdio_write(tp, 0x19, 0xaefd);
        mdio_write(tp, 0x15, 0x00dd);
        mdio_write(tp, 0x19, 0x6f0b);
        mdio_write(tp, 0x15, 0x00de);
        mdio_write(tp, 0x19, 0xc60b);
        mdio_write(tp, 0x15, 0x00df);
        mdio_write(tp, 0x19, 0x00fa);
        mdio_write(tp, 0x15, 0x00e0);
        mdio_write(tp, 0x19, 0x30e1);
        mdio_write(tp, 0x15, 0x020c);
        mdio_write(tp, 0x19, 0x3224);
        mdio_write(tp, 0x15, 0x020e);
        mdio_write(tp, 0x19, 0x9813);
        mdio_write(tp, 0x15, 0x020f);
        mdio_write(tp, 0x19, 0x7801);
        mdio_write(tp, 0x15, 0x0210);
        mdio_write(tp, 0x19, 0x930f);
        mdio_write(tp, 0x15, 0x0211);
        mdio_write(tp, 0x19, 0x9206);
        mdio_write(tp, 0x15, 0x0212);
        mdio_write(tp, 0x19, 0x4002);
        mdio_write(tp, 0x15, 0x0213);
        mdio_write(tp, 0x19, 0x7800);
        mdio_write(tp, 0x15, 0x0214);
        mdio_write(tp, 0x19, 0x588f);
        mdio_write(tp, 0x15, 0x0215);
        mdio_write(tp, 0x19, 0x5520);
        mdio_write(tp, 0x15, 0x0216);
        mdio_write(tp, 0x19, 0x3224);
        mdio_write(tp, 0x15, 0x0217);
        mdio_write(tp, 0x19, 0x4002);
        mdio_write(tp, 0x15, 0x0218);
        mdio_write(tp, 0x19, 0x7800);
        mdio_write(tp, 0x15, 0x0219);
        mdio_write(tp, 0x19, 0x588d);
        mdio_write(tp, 0x15, 0x021a);
        mdio_write(tp, 0x19, 0x5540);
        mdio_write(tp, 0x15, 0x021b);
        mdio_write(tp, 0x19, 0x9e03);
        mdio_write(tp, 0x15, 0x021c);
        mdio_write(tp, 0x19, 0x7c40);
        mdio_write(tp, 0x15, 0x021d);
        mdio_write(tp, 0x19, 0x6840);
        mdio_write(tp, 0x15, 0x021e);
        mdio_write(tp, 0x19, 0x3224);
        mdio_write(tp, 0x15, 0x021f);
        mdio_write(tp, 0x19, 0x4002);
        mdio_write(tp, 0x15, 0x0220);
        mdio_write(tp, 0x19, 0x3224);
        mdio_write(tp, 0x15, 0x0221);
        mdio_write(tp, 0x19, 0x9e03);
        mdio_write(tp, 0x15, 0x0222);
        mdio_write(tp, 0x19, 0x7c40);
        mdio_write(tp, 0x15, 0x0223);
        mdio_write(tp, 0x19, 0x6840);
        mdio_write(tp, 0x15, 0x0224);
        mdio_write(tp, 0x19, 0x7800);
        mdio_write(tp, 0x15, 0x0225);
        mdio_write(tp, 0x19, 0x3231);
        mdio_write(tp, 0x15, 0x0000);
        mdio_write(tp, 0x16, 0x0306);
        mdio_write(tp, 0x16, 0x0300);
        mdio_write(tp, 0x1f, 0x0000);
        mdio_write(tp, 0x1f, 0x0005);
        mdio_write(tp, 0x05, 0xfff6);
        mdio_write(tp, 0x06, 0x0080);
        mdio_write(tp, 0x05, 0x8000);
        mdio_write(tp, 0x06, 0x0280);
        mdio_write(tp, 0x06, 0x48f7);
        mdio_write(tp, 0x06, 0x00e0);
        mdio_write(tp, 0x06, 0xfff7);
        mdio_write(tp, 0x06, 0xa080);
        mdio_write(tp, 0x06, 0x02ae);
        mdio_write(tp, 0x06, 0xf602);
        mdio_write(tp, 0x06, 0x0118);
        mdio_write(tp, 0x06, 0x0201);
        mdio_write(tp, 0x06, 0x2502);
        mdio_write(tp, 0x06, 0x8090);
        mdio_write(tp, 0x06, 0x0201);
        mdio_write(tp, 0x06, 0x4202);
        mdio_write(tp, 0x06, 0x015c);
        mdio_write(tp, 0x06, 0x0280);
        mdio_write(tp, 0x06, 0xad02);
        mdio_write(tp, 0x06, 0x80ca);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x88e1);
        mdio_write(tp, 0x06, 0x8b89);
        mdio_write(tp, 0x06, 0x1e01);
        mdio_write(tp, 0x06, 0xe18b);
        mdio_write(tp, 0x06, 0x8a1e);
        mdio_write(tp, 0x06, 0x01e1);
        mdio_write(tp, 0x06, 0x8b8b);
        mdio_write(tp, 0x06, 0x1e01);
        mdio_write(tp, 0x06, 0xe18b);
        mdio_write(tp, 0x06, 0x8c1e);
        mdio_write(tp, 0x06, 0x01e1);
        mdio_write(tp, 0x06, 0x8b8d);
        mdio_write(tp, 0x06, 0x1e01);
        mdio_write(tp, 0x06, 0xe18b);
        mdio_write(tp, 0x06, 0x8e1e);
        mdio_write(tp, 0x06, 0x01a0);
        mdio_write(tp, 0x06, 0x00c7);
        mdio_write(tp, 0x06, 0xaebb);
        mdio_write(tp, 0x06, 0xd484);
        mdio_write(tp, 0x06, 0x3ce4);
        mdio_write(tp, 0x06, 0x8b92);
        mdio_write(tp, 0x06, 0xe58b);
        mdio_write(tp, 0x06, 0x93ee);
        mdio_write(tp, 0x06, 0x8ac8);
        mdio_write(tp, 0x06, 0x03ee);
        mdio_write(tp, 0x06, 0x8aca);
        mdio_write(tp, 0x06, 0x60ee);
        mdio_write(tp, 0x06, 0x8ac0);
        mdio_write(tp, 0x06, 0x00ee);
        mdio_write(tp, 0x06, 0x8ac1);
        mdio_write(tp, 0x06, 0x00ee);
        mdio_write(tp, 0x06, 0x8abe);
        mdio_write(tp, 0x06, 0x07ee);
        mdio_write(tp, 0x06, 0x8abf);
        mdio_write(tp, 0x06, 0x73ee);
        mdio_write(tp, 0x06, 0x8a95);
        mdio_write(tp, 0x06, 0x02bf);
        mdio_write(tp, 0x06, 0x8b88);
        mdio_write(tp, 0x06, 0xec00);
        mdio_write(tp, 0x06, 0x19a9);
        mdio_write(tp, 0x06, 0x8b90);
        mdio_write(tp, 0x06, 0xf9ee);
        mdio_write(tp, 0x06, 0xfff6);
        mdio_write(tp, 0x06, 0x00ee);
        mdio_write(tp, 0x06, 0xfff7);
        mdio_write(tp, 0x06, 0xfed1);
        mdio_write(tp, 0x06, 0x00bf);
        mdio_write(tp, 0x06, 0x85a4);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7dd1);
        mdio_write(tp, 0x06, 0x01bf);
        mdio_write(tp, 0x06, 0x85a7);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7d04);
        mdio_write(tp, 0x06, 0xf8e0);
        mdio_write(tp, 0x06, 0x8b8a);
        mdio_write(tp, 0x06, 0xad20);
        mdio_write(tp, 0x06, 0x14ee);
        mdio_write(tp, 0x06, 0x8b8a);
        mdio_write(tp, 0x06, 0x0002);
        mdio_write(tp, 0x06, 0x204b);
        mdio_write(tp, 0x06, 0xe0e4);
        mdio_write(tp, 0x06, 0x26e1);
        mdio_write(tp, 0x06, 0xe427);
        mdio_write(tp, 0x06, 0xeee4);
        mdio_write(tp, 0x06, 0x2623);
        mdio_write(tp, 0x06, 0xe5e4);
        mdio_write(tp, 0x06, 0x27fc);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x8dad);
        mdio_write(tp, 0x06, 0x2014);
        mdio_write(tp, 0x06, 0xee8b);
        mdio_write(tp, 0x06, 0x8d00);
        mdio_write(tp, 0x06, 0xe08a);
        mdio_write(tp, 0x06, 0x5a78);
        mdio_write(tp, 0x06, 0x039e);
        mdio_write(tp, 0x06, 0x0902);
        mdio_write(tp, 0x06, 0x05e8);
        mdio_write(tp, 0x06, 0x0281);
        mdio_write(tp, 0x06, 0x4f02);
        mdio_write(tp, 0x06, 0x326c);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8e0);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0xad20);
        mdio_write(tp, 0x06, 0x1df6);
        mdio_write(tp, 0x06, 0x20e4);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0x022f);
        mdio_write(tp, 0x06, 0x0902);
        mdio_write(tp, 0x06, 0x2ab0);
        mdio_write(tp, 0x06, 0x0285);
        mdio_write(tp, 0x06, 0x1602);
        mdio_write(tp, 0x06, 0x03ba);
        mdio_write(tp, 0x06, 0x0284);
        mdio_write(tp, 0x06, 0xe502);
        mdio_write(tp, 0x06, 0x2df1);
        mdio_write(tp, 0x06, 0x0283);
        mdio_write(tp, 0x06, 0x8302);
        mdio_write(tp, 0x06, 0x0475);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x8ead);
        mdio_write(tp, 0x06, 0x210b);
        mdio_write(tp, 0x06, 0xf621);
        mdio_write(tp, 0x06, 0xe48b);
        mdio_write(tp, 0x06, 0x8e02);
        mdio_write(tp, 0x06, 0x83f8);
        mdio_write(tp, 0x06, 0x021c);
        mdio_write(tp, 0x06, 0x99e0);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0xad22);
        mdio_write(tp, 0x06, 0x08f6);
        mdio_write(tp, 0x06, 0x22e4);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0x0235);
        mdio_write(tp, 0x06, 0x63e0);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0xad23);
        mdio_write(tp, 0x06, 0x08f6);
        mdio_write(tp, 0x06, 0x23e4);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0x0231);
        mdio_write(tp, 0x06, 0x57e0);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0xad24);
        mdio_write(tp, 0x06, 0x05f6);
        mdio_write(tp, 0x06, 0x24e4);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x8ead);
        mdio_write(tp, 0x06, 0x2505);
        mdio_write(tp, 0x06, 0xf625);
        mdio_write(tp, 0x06, 0xe48b);
        mdio_write(tp, 0x06, 0x8ee0);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0xad26);
        mdio_write(tp, 0x06, 0x08f6);
        mdio_write(tp, 0x06, 0x26e4);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0x022d);
        mdio_write(tp, 0x06, 0x1ce0);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0xad27);
        mdio_write(tp, 0x06, 0x05f6);
        mdio_write(tp, 0x06, 0x27e4);
        mdio_write(tp, 0x06, 0x8b8e);
        mdio_write(tp, 0x06, 0x0203);
        mdio_write(tp, 0x06, 0x80fc);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xf9e0);
        mdio_write(tp, 0x06, 0x8b81);
        mdio_write(tp, 0x06, 0xac26);
        mdio_write(tp, 0x06, 0x1ae0);
        mdio_write(tp, 0x06, 0x8b81);
        mdio_write(tp, 0x06, 0xac21);
        mdio_write(tp, 0x06, 0x14e0);
        mdio_write(tp, 0x06, 0x8b85);
        mdio_write(tp, 0x06, 0xac20);
        mdio_write(tp, 0x06, 0x0ee0);
        mdio_write(tp, 0x06, 0x8b85);
        mdio_write(tp, 0x06, 0xac23);
        mdio_write(tp, 0x06, 0x08e0);
        mdio_write(tp, 0x06, 0x8b87);
        mdio_write(tp, 0x06, 0xac24);
        mdio_write(tp, 0x06, 0x02ae);
        mdio_write(tp, 0x06, 0x3802);
        mdio_write(tp, 0x06, 0x1ac2);
        mdio_write(tp, 0x06, 0xeee4);
        mdio_write(tp, 0x06, 0x1c04);
        mdio_write(tp, 0x06, 0xeee4);
        mdio_write(tp, 0x06, 0x1d04);
        mdio_write(tp, 0x06, 0xe2e0);
        mdio_write(tp, 0x06, 0x7ce3);
        mdio_write(tp, 0x06, 0xe07d);
        mdio_write(tp, 0x06, 0xe0e0);
        mdio_write(tp, 0x06, 0x38e1);
        mdio_write(tp, 0x06, 0xe039);
        mdio_write(tp, 0x06, 0xad2e);
        mdio_write(tp, 0x06, 0x1bad);
        mdio_write(tp, 0x06, 0x390d);
        mdio_write(tp, 0x06, 0xd101);
        mdio_write(tp, 0x06, 0xbf22);
        mdio_write(tp, 0x06, 0x7a02);
        mdio_write(tp, 0x06, 0x387d);
        mdio_write(tp, 0x06, 0x0281);
        mdio_write(tp, 0x06, 0xacae);
        mdio_write(tp, 0x06, 0x0bac);
        mdio_write(tp, 0x06, 0x3802);
        mdio_write(tp, 0x06, 0xae06);
        mdio_write(tp, 0x06, 0x0281);
        mdio_write(tp, 0x06, 0xe902);
        mdio_write(tp, 0x06, 0x822e);
        mdio_write(tp, 0x06, 0x021a);
        mdio_write(tp, 0x06, 0xd3fd);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8e1);
        mdio_write(tp, 0x06, 0x8af4);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x81ad);
        mdio_write(tp, 0x06, 0x2602);
        mdio_write(tp, 0x06, 0xf728);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x81ad);
        mdio_write(tp, 0x06, 0x2105);
        mdio_write(tp, 0x06, 0x0222);
        mdio_write(tp, 0x06, 0x8ef7);
        mdio_write(tp, 0x06, 0x29e0);
        mdio_write(tp, 0x06, 0x8b85);
        mdio_write(tp, 0x06, 0xad20);
        mdio_write(tp, 0x06, 0x0502);
        mdio_write(tp, 0x06, 0x14b8);
        mdio_write(tp, 0x06, 0xf72a);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x85ad);
        mdio_write(tp, 0x06, 0x2305);
        mdio_write(tp, 0x06, 0x0212);
        mdio_write(tp, 0x06, 0xf4f7);
        mdio_write(tp, 0x06, 0x2be0);
        mdio_write(tp, 0x06, 0x8b87);
        mdio_write(tp, 0x06, 0xad24);
        mdio_write(tp, 0x06, 0x0502);
        mdio_write(tp, 0x06, 0x8284);
        mdio_write(tp, 0x06, 0xf72c);
        mdio_write(tp, 0x06, 0xe58a);
        mdio_write(tp, 0x06, 0xf4fc);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x81ad);
        mdio_write(tp, 0x06, 0x2600);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x81ad);
        mdio_write(tp, 0x06, 0x2109);
        mdio_write(tp, 0x06, 0xe08a);
        mdio_write(tp, 0x06, 0xf4ac);
        mdio_write(tp, 0x06, 0x2003);
        mdio_write(tp, 0x06, 0x0222);
        mdio_write(tp, 0x06, 0x7de0);
        mdio_write(tp, 0x06, 0x8b85);
        mdio_write(tp, 0x06, 0xad20);
        mdio_write(tp, 0x06, 0x09e0);
        mdio_write(tp, 0x06, 0x8af4);
        mdio_write(tp, 0x06, 0xac21);
        mdio_write(tp, 0x06, 0x0302);
        mdio_write(tp, 0x06, 0x1408);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x85ad);
        mdio_write(tp, 0x06, 0x2309);
        mdio_write(tp, 0x06, 0xe08a);
        mdio_write(tp, 0x06, 0xf4ac);
        mdio_write(tp, 0x06, 0x2203);
        mdio_write(tp, 0x06, 0x0213);
        mdio_write(tp, 0x06, 0x07e0);
        mdio_write(tp, 0x06, 0x8b87);
        mdio_write(tp, 0x06, 0xad24);
        mdio_write(tp, 0x06, 0x09e0);
        mdio_write(tp, 0x06, 0x8af4);
        mdio_write(tp, 0x06, 0xac23);
        mdio_write(tp, 0x06, 0x0302);
        mdio_write(tp, 0x06, 0x8289);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8e1);
        mdio_write(tp, 0x06, 0x8af4);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x81ad);
        mdio_write(tp, 0x06, 0x2602);
        mdio_write(tp, 0x06, 0xf628);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x81ad);
        mdio_write(tp, 0x06, 0x210a);
        mdio_write(tp, 0x06, 0xe083);
        mdio_write(tp, 0x06, 0xecf6);
        mdio_write(tp, 0x06, 0x27a0);
        mdio_write(tp, 0x06, 0x0502);
        mdio_write(tp, 0x06, 0xf629);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x85ad);
        mdio_write(tp, 0x06, 0x2008);
        mdio_write(tp, 0x06, 0xe08a);
        mdio_write(tp, 0x06, 0xe8ad);
        mdio_write(tp, 0x06, 0x2102);
        mdio_write(tp, 0x06, 0xf62a);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x85ad);
        mdio_write(tp, 0x06, 0x2308);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x20a0);
        mdio_write(tp, 0x06, 0x0302);
        mdio_write(tp, 0x06, 0xf62b);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x87ad);
        mdio_write(tp, 0x06, 0x2408);
        mdio_write(tp, 0x06, 0xe08a);
        mdio_write(tp, 0x06, 0xc2a0);
        mdio_write(tp, 0x06, 0x0302);
        mdio_write(tp, 0x06, 0xf62c);
        mdio_write(tp, 0x06, 0xe58a);
        mdio_write(tp, 0x06, 0xf4a1);
        mdio_write(tp, 0x06, 0x0008);
        mdio_write(tp, 0x06, 0xd100);
        mdio_write(tp, 0x06, 0xbf22);
        mdio_write(tp, 0x06, 0x7a02);
        mdio_write(tp, 0x06, 0x387d);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xee8a);
        mdio_write(tp, 0x06, 0xc200);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x87ad);
        mdio_write(tp, 0x06, 0x241e);
        mdio_write(tp, 0x06, 0xe08a);
        mdio_write(tp, 0x06, 0xc2a0);
        mdio_write(tp, 0x06, 0x0005);
        mdio_write(tp, 0x06, 0x0282);
        mdio_write(tp, 0x06, 0xb0ae);
        mdio_write(tp, 0x06, 0xf5a0);
        mdio_write(tp, 0x06, 0x0105);
        mdio_write(tp, 0x06, 0x0282);
        mdio_write(tp, 0x06, 0xc0ae);
        mdio_write(tp, 0x06, 0x0ba0);
        mdio_write(tp, 0x06, 0x0205);
        mdio_write(tp, 0x06, 0x0282);
        mdio_write(tp, 0x06, 0xcaae);
        mdio_write(tp, 0x06, 0x03a0);
        mdio_write(tp, 0x06, 0x0300);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8fa);
        mdio_write(tp, 0x06, 0xef69);
        mdio_write(tp, 0x06, 0x0282);
        mdio_write(tp, 0x06, 0xe1ee);
        mdio_write(tp, 0x06, 0x8ac2);
        mdio_write(tp, 0x06, 0x01ef);
        mdio_write(tp, 0x06, 0x96fe);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8ee);
        mdio_write(tp, 0x06, 0x8ac9);
        mdio_write(tp, 0x06, 0x0002);
        mdio_write(tp, 0x06, 0x8317);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8e0);
        mdio_write(tp, 0x06, 0x8ac8);
        mdio_write(tp, 0x06, 0xe18a);
        mdio_write(tp, 0x06, 0xc91f);
        mdio_write(tp, 0x06, 0x019e);
        mdio_write(tp, 0x06, 0x0611);
        mdio_write(tp, 0x06, 0xe58a);
        mdio_write(tp, 0x06, 0xc9ae);
        mdio_write(tp, 0x06, 0x04ee);
        mdio_write(tp, 0x06, 0x8ac2);
        mdio_write(tp, 0x06, 0x01fc);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xf9fa);
        mdio_write(tp, 0x06, 0xef69);
        mdio_write(tp, 0x06, 0xfbbf);
        mdio_write(tp, 0x06, 0x8ac4);
        mdio_write(tp, 0x06, 0xef79);
        mdio_write(tp, 0x06, 0xd200);
        mdio_write(tp, 0x06, 0xd400);
        mdio_write(tp, 0x06, 0x221e);
        mdio_write(tp, 0x06, 0x02bf);
        mdio_write(tp, 0x06, 0x3024);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7dbf);
        mdio_write(tp, 0x06, 0x13ff);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x500d);
        mdio_write(tp, 0x06, 0x4559);
        mdio_write(tp, 0x06, 0x1fef);
        mdio_write(tp, 0x06, 0x97dd);
        mdio_write(tp, 0x06, 0xd308);
        mdio_write(tp, 0x06, 0x1a93);
        mdio_write(tp, 0x06, 0xdd12);
        mdio_write(tp, 0x06, 0x17a2);
        mdio_write(tp, 0x06, 0x04de);
        mdio_write(tp, 0x06, 0xffef);
        mdio_write(tp, 0x06, 0x96fe);
        mdio_write(tp, 0x06, 0xfdfc);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xf9fa);
        mdio_write(tp, 0x06, 0xef69);
        mdio_write(tp, 0x06, 0xfbee);
        mdio_write(tp, 0x06, 0x8ac2);
        mdio_write(tp, 0x06, 0x03d5);
        mdio_write(tp, 0x06, 0x0080);
        mdio_write(tp, 0x06, 0xbf8a);
        mdio_write(tp, 0x06, 0xc4ef);
        mdio_write(tp, 0x06, 0x79ef);
        mdio_write(tp, 0x06, 0x45bf);
        mdio_write(tp, 0x06, 0x3024);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7dbf);
        mdio_write(tp, 0x06, 0x13ff);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x50ad);
        mdio_write(tp, 0x06, 0x2702);
        mdio_write(tp, 0x06, 0x78ff);
        mdio_write(tp, 0x06, 0xe18a);
        mdio_write(tp, 0x06, 0xca1b);
        mdio_write(tp, 0x06, 0x01aa);
        mdio_write(tp, 0x06, 0x2eef);
        mdio_write(tp, 0x06, 0x97d9);
        mdio_write(tp, 0x06, 0x7900);
        mdio_write(tp, 0x06, 0x9e2b);
        mdio_write(tp, 0x06, 0x81dd);
        mdio_write(tp, 0x06, 0xbf85);
        mdio_write(tp, 0x06, 0xad02);
        mdio_write(tp, 0x06, 0x387d);
        mdio_write(tp, 0x06, 0xd101);
        mdio_write(tp, 0x06, 0xef02);
        mdio_write(tp, 0x06, 0x100c);
        mdio_write(tp, 0x06, 0x11b0);
        mdio_write(tp, 0x06, 0xfc0d);
        mdio_write(tp, 0x06, 0x11bf);
        mdio_write(tp, 0x06, 0x85aa);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7dd1);
        mdio_write(tp, 0x06, 0x00bf);
        mdio_write(tp, 0x06, 0x85aa);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7dee);
        mdio_write(tp, 0x06, 0x8ac2);
        mdio_write(tp, 0x06, 0x02ae);
        mdio_write(tp, 0x06, 0x0413);
        mdio_write(tp, 0x06, 0xa38b);
        mdio_write(tp, 0x06, 0xb4d3);
        mdio_write(tp, 0x06, 0x8012);
        mdio_write(tp, 0x06, 0x17a2);
        mdio_write(tp, 0x06, 0x04ad);
        mdio_write(tp, 0x06, 0xffef);
        mdio_write(tp, 0x06, 0x96fe);
        mdio_write(tp, 0x06, 0xfdfc);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xf9e0);
        mdio_write(tp, 0x06, 0x8b85);
        mdio_write(tp, 0x06, 0xad25);
        mdio_write(tp, 0x06, 0x48e0);
        mdio_write(tp, 0x06, 0x8a96);
        mdio_write(tp, 0x06, 0xe18a);
        mdio_write(tp, 0x06, 0x977c);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x9e35);
        mdio_write(tp, 0x06, 0xee8a);
        mdio_write(tp, 0x06, 0x9600);
        mdio_write(tp, 0x06, 0xee8a);
        mdio_write(tp, 0x06, 0x9700);
        mdio_write(tp, 0x06, 0xe08a);
        mdio_write(tp, 0x06, 0xbee1);
        mdio_write(tp, 0x06, 0x8abf);
        mdio_write(tp, 0x06, 0xe28a);
        mdio_write(tp, 0x06, 0xc0e3);
        mdio_write(tp, 0x06, 0x8ac1);
        mdio_write(tp, 0x06, 0x0237);
        mdio_write(tp, 0x06, 0x74ad);
        mdio_write(tp, 0x06, 0x2012);
        mdio_write(tp, 0x06, 0xee8a);
        mdio_write(tp, 0x06, 0x9603);
        mdio_write(tp, 0x06, 0xee8a);
        mdio_write(tp, 0x06, 0x97b7);
        mdio_write(tp, 0x06, 0xee8a);
        mdio_write(tp, 0x06, 0xc000);
        mdio_write(tp, 0x06, 0xee8a);
        mdio_write(tp, 0x06, 0xc100);
        mdio_write(tp, 0x06, 0xae11);
        mdio_write(tp, 0x06, 0x15e6);
        mdio_write(tp, 0x06, 0x8ac0);
        mdio_write(tp, 0x06, 0xe78a);
        mdio_write(tp, 0x06, 0xc1ae);
        mdio_write(tp, 0x06, 0x08ee);
        mdio_write(tp, 0x06, 0x8ac0);
        mdio_write(tp, 0x06, 0x00ee);
        mdio_write(tp, 0x06, 0x8ac1);
        mdio_write(tp, 0x06, 0x00fd);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xae20);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x06, 0xf8fa);
        mdio_write(tp, 0x06, 0xef69);
        mdio_write(tp, 0x06, 0xe0e0);
        mdio_write(tp, 0x06, 0x00e1);
        mdio_write(tp, 0x06, 0xe001);
        mdio_write(tp, 0x06, 0xad27);
        mdio_write(tp, 0x06, 0x32e0);
        mdio_write(tp, 0x06, 0x8b40);
        mdio_write(tp, 0x06, 0xf720);
        mdio_write(tp, 0x06, 0xe48b);
        mdio_write(tp, 0x06, 0x40bf);
        mdio_write(tp, 0x06, 0x3230);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x50ad);
        mdio_write(tp, 0x06, 0x2821);
        mdio_write(tp, 0x06, 0xe0e0);
        mdio_write(tp, 0x06, 0x20e1);
        mdio_write(tp, 0x06, 0xe021);
        mdio_write(tp, 0x06, 0xad20);
        mdio_write(tp, 0x06, 0x18e0);
        mdio_write(tp, 0x06, 0x8b40);
        mdio_write(tp, 0x06, 0xf620);
        mdio_write(tp, 0x06, 0xe48b);
        mdio_write(tp, 0x06, 0x40ee);
        mdio_write(tp, 0x06, 0x8b3b);
        mdio_write(tp, 0x06, 0xffe0);
        mdio_write(tp, 0x06, 0x8a8a);
        mdio_write(tp, 0x06, 0xe18a);
        mdio_write(tp, 0x06, 0x8be4);
        mdio_write(tp, 0x06, 0xe000);
        mdio_write(tp, 0x06, 0xe5e0);
        mdio_write(tp, 0x06, 0x01ef);
        mdio_write(tp, 0x06, 0x96fe);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8f9);
        mdio_write(tp, 0x06, 0xface);
        mdio_write(tp, 0x06, 0xfaef);
        mdio_write(tp, 0x06, 0x69fa);
        mdio_write(tp, 0x06, 0xd401);
        mdio_write(tp, 0x06, 0x55b4);
        mdio_write(tp, 0x06, 0xfebf);
        mdio_write(tp, 0x06, 0x1c1e);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x50ac);
        mdio_write(tp, 0x06, 0x280b);
        mdio_write(tp, 0x06, 0xbf1c);
        mdio_write(tp, 0x06, 0x1b02);
        mdio_write(tp, 0x06, 0x3850);
        mdio_write(tp, 0x06, 0xac28);
        mdio_write(tp, 0x06, 0x49ae);
        mdio_write(tp, 0x06, 0x64bf);
        mdio_write(tp, 0x06, 0x1c1b);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x50ac);
        mdio_write(tp, 0x06, 0x285b);
        mdio_write(tp, 0x06, 0xd000);
        mdio_write(tp, 0x06, 0x0284);
        mdio_write(tp, 0x06, 0xcaac);
        mdio_write(tp, 0x06, 0x2105);
        mdio_write(tp, 0x06, 0xac22);
        mdio_write(tp, 0x06, 0x02ae);
        mdio_write(tp, 0x06, 0x4ebf);
        mdio_write(tp, 0x06, 0xe0c4);
        mdio_write(tp, 0x06, 0xbe85);
        mdio_write(tp, 0x06, 0xf6d2);
        mdio_write(tp, 0x06, 0x04d8);
        mdio_write(tp, 0x06, 0x19d9);
        mdio_write(tp, 0x06, 0x1907);
        mdio_write(tp, 0x06, 0xdc19);
        mdio_write(tp, 0x06, 0xdd19);
        mdio_write(tp, 0x06, 0x0789);
        mdio_write(tp, 0x06, 0x89ef);
        mdio_write(tp, 0x06, 0x645e);
        mdio_write(tp, 0x06, 0x07ff);
        mdio_write(tp, 0x06, 0x0d65);
        mdio_write(tp, 0x06, 0x5cf8);
        mdio_write(tp, 0x06, 0x001e);
        mdio_write(tp, 0x06, 0x46dc);
        mdio_write(tp, 0x06, 0x19dd);
        mdio_write(tp, 0x06, 0x19b2);
        mdio_write(tp, 0x06, 0xe2d4);
        mdio_write(tp, 0x06, 0x0001);
        mdio_write(tp, 0x06, 0xbf1c);
        mdio_write(tp, 0x06, 0x1b02);
        mdio_write(tp, 0x06, 0x387d);
        mdio_write(tp, 0x06, 0xae1d);
        mdio_write(tp, 0x06, 0xbee0);
        mdio_write(tp, 0x06, 0xc4bf);
        mdio_write(tp, 0x06, 0x85f6);
        mdio_write(tp, 0x06, 0xd204);
        mdio_write(tp, 0x06, 0xd819);
        mdio_write(tp, 0x06, 0xd919);
        mdio_write(tp, 0x06, 0x07dc);
        mdio_write(tp, 0x06, 0x19dd);
        mdio_write(tp, 0x06, 0x1907);
        mdio_write(tp, 0x06, 0xb2f4);
        mdio_write(tp, 0x06, 0xd400);
        mdio_write(tp, 0x06, 0x00bf);
        mdio_write(tp, 0x06, 0x1c1b);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7dfe);
        mdio_write(tp, 0x06, 0xef96);
        mdio_write(tp, 0x06, 0xfec6);
        mdio_write(tp, 0x06, 0xfefd);
        mdio_write(tp, 0x06, 0xfc05);
        mdio_write(tp, 0x06, 0xf9e2);
        mdio_write(tp, 0x06, 0xe0ea);
        mdio_write(tp, 0x06, 0xe3e0);
        mdio_write(tp, 0x06, 0xeb5a);
        mdio_write(tp, 0x06, 0x070c);
        mdio_write(tp, 0x06, 0x031e);
        mdio_write(tp, 0x06, 0x20e6);
        mdio_write(tp, 0x06, 0xe0ea);
        mdio_write(tp, 0x06, 0xe7e0);
        mdio_write(tp, 0x06, 0xebe0);
        mdio_write(tp, 0x06, 0xe0fc);
        mdio_write(tp, 0x06, 0xe1e0);
        mdio_write(tp, 0x06, 0xfdfd);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xfaef);
        mdio_write(tp, 0x06, 0x69e0);
        mdio_write(tp, 0x06, 0x8b80);
        mdio_write(tp, 0x06, 0xad27);
        mdio_write(tp, 0x06, 0x22bf);
        mdio_write(tp, 0x06, 0x4616);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x50e0);
        mdio_write(tp, 0x06, 0x8b44);
        mdio_write(tp, 0x06, 0x1f01);
        mdio_write(tp, 0x06, 0x9e15);
        mdio_write(tp, 0x06, 0xe58b);
        mdio_write(tp, 0x06, 0x44ad);
        mdio_write(tp, 0x06, 0x2907);
        mdio_write(tp, 0x06, 0xac28);
        mdio_write(tp, 0x06, 0x04d1);
        mdio_write(tp, 0x06, 0x01ae);
        mdio_write(tp, 0x06, 0x02d1);
        mdio_write(tp, 0x06, 0x00bf);
        mdio_write(tp, 0x06, 0x85b0);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7def);
        mdio_write(tp, 0x06, 0x96fe);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8e0);
        mdio_write(tp, 0x06, 0x8b85);
        mdio_write(tp, 0x06, 0xad26);
        mdio_write(tp, 0x06, 0x30e0);
        mdio_write(tp, 0x06, 0xe036);
        mdio_write(tp, 0x06, 0xe1e0);
        mdio_write(tp, 0x06, 0x37e1);
        mdio_write(tp, 0x06, 0x8b3f);
        mdio_write(tp, 0x06, 0x1f10);
        mdio_write(tp, 0x06, 0x9e23);
        mdio_write(tp, 0x06, 0xe48b);
        mdio_write(tp, 0x06, 0x3fac);
        mdio_write(tp, 0x06, 0x200b);
        mdio_write(tp, 0x06, 0xac21);
        mdio_write(tp, 0x06, 0x0dac);
        mdio_write(tp, 0x06, 0x250f);
        mdio_write(tp, 0x06, 0xac27);
        mdio_write(tp, 0x06, 0x11ae);
        mdio_write(tp, 0x06, 0x1202);
        mdio_write(tp, 0x06, 0x2c47);
        mdio_write(tp, 0x06, 0xae0d);
        mdio_write(tp, 0x06, 0x0285);
        mdio_write(tp, 0x06, 0x4fae);
        mdio_write(tp, 0x06, 0x0802);
        mdio_write(tp, 0x06, 0x2c69);
        mdio_write(tp, 0x06, 0xae03);
        mdio_write(tp, 0x06, 0x022c);
        mdio_write(tp, 0x06, 0x7cfc);
        mdio_write(tp, 0x06, 0x04f8);
        mdio_write(tp, 0x06, 0xfaef);
        mdio_write(tp, 0x06, 0x6902);
        mdio_write(tp, 0x06, 0x856c);
        mdio_write(tp, 0x06, 0xe0e0);
        mdio_write(tp, 0x06, 0x14e1);
        mdio_write(tp, 0x06, 0xe015);
        mdio_write(tp, 0x06, 0xad26);
        mdio_write(tp, 0x06, 0x08d1);
        mdio_write(tp, 0x06, 0x1ebf);
        mdio_write(tp, 0x06, 0x2cd9);
        mdio_write(tp, 0x06, 0x0238);
        mdio_write(tp, 0x06, 0x7def);
        mdio_write(tp, 0x06, 0x96fe);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0xf8e0);
        mdio_write(tp, 0x06, 0x8b85);
        mdio_write(tp, 0x06, 0xad27);
        mdio_write(tp, 0x06, 0x2fd0);
        mdio_write(tp, 0x06, 0x0b02);
        mdio_write(tp, 0x06, 0x3682);
        mdio_write(tp, 0x06, 0x5882);
        mdio_write(tp, 0x06, 0x7882);
        mdio_write(tp, 0x06, 0x9f24);
        mdio_write(tp, 0x06, 0xe08b);
        mdio_write(tp, 0x06, 0x32e1);
        mdio_write(tp, 0x06, 0x8b33);
        mdio_write(tp, 0x06, 0x1f10);
        mdio_write(tp, 0x06, 0x9e1a);
        mdio_write(tp, 0x06, 0x10e4);
        mdio_write(tp, 0x06, 0x8b32);
        mdio_write(tp, 0x06, 0xe0e0);
        mdio_write(tp, 0x06, 0x28e1);
        mdio_write(tp, 0x06, 0xe029);
        mdio_write(tp, 0x06, 0xf72c);
        mdio_write(tp, 0x06, 0xe4e0);
        mdio_write(tp, 0x06, 0x28e5);
        mdio_write(tp, 0x06, 0xe029);
        mdio_write(tp, 0x06, 0xf62c);
        mdio_write(tp, 0x06, 0xe4e0);
        mdio_write(tp, 0x06, 0x28e5);
        mdio_write(tp, 0x06, 0xe029);
        mdio_write(tp, 0x06, 0xfc04);
        mdio_write(tp, 0x06, 0x00e1);
        mdio_write(tp, 0x06, 0x4077);
        mdio_write(tp, 0x06, 0xe140);
        mdio_write(tp, 0x06, 0x52e0);
        mdio_write(tp, 0x06, 0xeed9);
        mdio_write(tp, 0x06, 0xe04c);
        mdio_write(tp, 0x06, 0xbbe0);
        mdio_write(tp, 0x06, 0x2a00);
        mdio_write(tp, 0x05, 0xe142);
        gphy_val = mdio_read(tp, 0x06);
        gphy_val |= BIT_0;
        mdio_write(tp,0x06, gphy_val);
        mdio_write(tp, 0x05, 0xe140);
        gphy_val = mdio_read(tp, 0x06);
        gphy_val |= BIT_0;
        mdio_write(tp,0x06, gphy_val);
        mdio_write(tp, 0x1f, 0x0000);
        mdio_write(tp,0x1f, 0x0005);
        for (i = 0; i < 200; i++) {
            udelay(100);
            gphy_val = mdio_read(tp, 0x00);
            if (gphy_val & BIT_7)
                break;
        }
        mdio_write(tp, 0x1f, 0x0007);
        mdio_write(tp, 0x1e, 0x0023);
        gphy_val = mdio_read(tp, 0x17);
        gphy_val |= BIT_1;
        mdio_write(tp, 0x17, gphy_val);
        mdio_write(tp, 0x1f, 0x0000);

        mdio_write(tp, 0x1F, 0x0003);
        mdio_write(tp, 0x09, 0xA20F);
        mdio_write(tp, 0x1F, 0x0000);
        mdio_write(tp, 0x1f, 0x0003);
        mdio_write(tp, 0x01, 0x328A);
        mdio_write(tp, 0x1f, 0x0000);

        mdio_write(tp, 0x1f, 0x0000);
        mdio_write(tp, 0x00, 0x9200);
    }  else if (tp->mcfg == CFG_METHOD_21) {
        mdio_write(tp, 0x1f, 0x0B82);
        gphy_val = mdio_read(tp, 0x10);
        gphy_val |= BIT_4;
        mdio_write(tp, 0x10, gphy_val);
        mdio_write(tp, 0x1f, 0x0B80);
        for (i = 0; i < 10; i++) {
            if (mdio_read(tp, 0x10) & 0x0040)
                break;
            mdelay(10);
        }
        mdio_write(tp, 0x1f, 0x0A43);
        mdio_write(tp, 0x13, 0x8146);
        mdio_write(tp, 0x14, 0x2300);
        mdio_write(tp, 0x13, 0xB820);
        mdio_write(tp, 0x14, 0x0210);
        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0xB820);
        mdio_write(tp, 0x14, 0x0290);
        mdio_write(tp, 0x13, 0xA012);
        mdio_write(tp, 0x14, 0x0000);
        mdio_write(tp, 0x13, 0xA014);
        mdio_write(tp, 0x14, 0x2c04);
        mdio_write(tp, 0x14, 0x2c1b);
        mdio_write(tp, 0x14, 0x2c65);
        mdio_write(tp, 0x14, 0x2d06);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x4092);
        mdio_write(tp, 0x14, 0xba04);
        mdio_write(tp, 0x14, 0x3084);
        mdio_write(tp, 0x14, 0x1cf6);
        mdio_write(tp, 0x14, 0x1ccf);
        mdio_write(tp, 0x14, 0x1cda);
        mdio_write(tp, 0x14, 0xaeff);
        mdio_write(tp, 0x14, 0xaf02);
        mdio_write(tp, 0x14, 0x8f02);
        mdio_write(tp, 0x14, 0x8eff);
        mdio_write(tp, 0x14, 0xce01);
        mdio_write(tp, 0x14, 0xe070);
        mdio_write(tp, 0x14, 0x0f00);
        mdio_write(tp, 0x14, 0xaf01);
        mdio_write(tp, 0x14, 0x8f01);
        mdio_write(tp, 0x14, 0xd712);
        mdio_write(tp, 0x14, 0x5fe8);
        mdio_write(tp, 0x14, 0xaf02);
        mdio_write(tp, 0x14, 0x8f02);
        mdio_write(tp, 0x14, 0x8e01);
        mdio_write(tp, 0x14, 0x1ce4);
        mdio_write(tp, 0x14, 0x27f2);
        mdio_write(tp, 0x14, 0xd05a);
        mdio_write(tp, 0x14, 0xd19a);
        mdio_write(tp, 0x14, 0xd709);
        mdio_write(tp, 0x14, 0x608f);
        mdio_write(tp, 0x14, 0xd06b);
        mdio_write(tp, 0x14, 0xd18a);
        mdio_write(tp, 0x14, 0x2c25);
        mdio_write(tp, 0x14, 0xd0be);
        mdio_write(tp, 0x14, 0xd188);
        mdio_write(tp, 0x14, 0x2c25);
        mdio_write(tp, 0x14, 0xd708);
        mdio_write(tp, 0x14, 0x4072);
        mdio_write(tp, 0x14, 0xc104);
        mdio_write(tp, 0x14, 0x2c37);
        mdio_write(tp, 0x14, 0x4076);
        mdio_write(tp, 0x14, 0xc110);
        mdio_write(tp, 0x14, 0x2c37);
        mdio_write(tp, 0x14, 0x4071);
        mdio_write(tp, 0x14, 0xc102);
        mdio_write(tp, 0x14, 0x2c37);
        mdio_write(tp, 0x14, 0x4070);
        mdio_write(tp, 0x14, 0xc101);
        mdio_write(tp, 0x14, 0x2c37);
        mdio_write(tp, 0x14, 0x175b);
        mdio_write(tp, 0x14, 0xd709);
        mdio_write(tp, 0x14, 0x3390);
        mdio_write(tp, 0x14, 0x5c32);
        mdio_write(tp, 0x14, 0x2c47);
        mdio_write(tp, 0x14, 0x175b);
        mdio_write(tp, 0x14, 0xd708);
        mdio_write(tp, 0x14, 0x6193);
        mdio_write(tp, 0x14, 0xd709);
        mdio_write(tp, 0x14, 0x5f9d);
        mdio_write(tp, 0x14, 0x408b);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x6042);
        mdio_write(tp, 0x14, 0xb401);
        mdio_write(tp, 0x14, 0x175b);
        mdio_write(tp, 0x14, 0xd708);
        mdio_write(tp, 0x14, 0x6073);
        mdio_write(tp, 0x14, 0x5fbc);
        mdio_write(tp, 0x14, 0x2c46);
        mdio_write(tp, 0x14, 0x26ed);
        mdio_write(tp, 0x14, 0xb280);
        mdio_write(tp, 0x14, 0xa841);
        mdio_write(tp, 0x14, 0x9420);
        mdio_write(tp, 0x14, 0x8710);
        mdio_write(tp, 0x14, 0xd709);
        mdio_write(tp, 0x14, 0x42ec);
        mdio_write(tp, 0x14, 0x606d);
        mdio_write(tp, 0x14, 0xd207);
        mdio_write(tp, 0x14, 0x2c50);
        mdio_write(tp, 0x14, 0xd203);
        mdio_write(tp, 0x14, 0x33ff);
        mdio_write(tp, 0x14, 0x563b);
        mdio_write(tp, 0x14, 0x3275);
        mdio_write(tp, 0x14, 0x7c57);
        mdio_write(tp, 0x14, 0xb240);
        mdio_write(tp, 0x14, 0xb402);
        mdio_write(tp, 0x14, 0x263b);
        mdio_write(tp, 0x14, 0x6096);
        mdio_write(tp, 0x14, 0xb240);
        mdio_write(tp, 0x14, 0xb406);
        mdio_write(tp, 0x14, 0x263b);
        mdio_write(tp, 0x14, 0x31d7);
        mdio_write(tp, 0x14, 0x7c60);
        mdio_write(tp, 0x14, 0xb240);
        mdio_write(tp, 0x14, 0xb40e);
        mdio_write(tp, 0x14, 0x263b);
        mdio_write(tp, 0x14, 0xb410);
        mdio_write(tp, 0x14, 0x8802);
        mdio_write(tp, 0x14, 0xb240);
        mdio_write(tp, 0x14, 0x940e);
        mdio_write(tp, 0x14, 0x263b);
        mdio_write(tp, 0x14, 0xba04);
        mdio_write(tp, 0x14, 0x1ccf);
        mdio_write(tp, 0x14, 0xa902);
        mdio_write(tp, 0x14, 0xd711);
        mdio_write(tp, 0x14, 0x4045);
        mdio_write(tp, 0x14, 0xa980);
        mdio_write(tp, 0x14, 0x3003);
        mdio_write(tp, 0x14, 0x59b1);
        mdio_write(tp, 0x14, 0xa540);
        mdio_write(tp, 0x14, 0xa601);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4043);
        mdio_write(tp, 0x14, 0xa910);
        mdio_write(tp, 0x14, 0xd711);
        mdio_write(tp, 0x14, 0x60a0);
        mdio_write(tp, 0x14, 0xca33);
        mdio_write(tp, 0x14, 0xcb33);
        mdio_write(tp, 0x14, 0xa941);
        mdio_write(tp, 0x14, 0x2c7b);
        mdio_write(tp, 0x14, 0xcaff);
        mdio_write(tp, 0x14, 0xcbff);
        mdio_write(tp, 0x14, 0xa921);
        mdio_write(tp, 0x14, 0xce02);
        mdio_write(tp, 0x14, 0xe070);
        mdio_write(tp, 0x14, 0x0f10);
        mdio_write(tp, 0x14, 0xaf01);
        mdio_write(tp, 0x14, 0x8f01);
        mdio_write(tp, 0x14, 0x1766);
        mdio_write(tp, 0x14, 0x8e02);
        mdio_write(tp, 0x14, 0x1787);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x609c);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fa4);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0x1ce2);
        mdio_write(tp, 0x14, 0xce04);
        mdio_write(tp, 0x14, 0xe070);
        mdio_write(tp, 0x14, 0x0f20);
        mdio_write(tp, 0x14, 0xaf01);
        mdio_write(tp, 0x14, 0x8f01);
        mdio_write(tp, 0x14, 0x1766);
        mdio_write(tp, 0x14, 0x8e04);
        mdio_write(tp, 0x14, 0x6044);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0xa520);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4043);
        mdio_write(tp, 0x14, 0x2cba);
        mdio_write(tp, 0x14, 0xe00f);
        mdio_write(tp, 0x14, 0x0501);
        mdio_write(tp, 0x14, 0x1ce8);
        mdio_write(tp, 0x14, 0xb801);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x4060);
        mdio_write(tp, 0x14, 0x7fc4);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0x1cee);
        mdio_write(tp, 0x14, 0xe00f);
        mdio_write(tp, 0x14, 0x0502);
        mdio_write(tp, 0x14, 0x1ce8);
        mdio_write(tp, 0x14, 0xb802);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x4061);
        mdio_write(tp, 0x14, 0x7fc4);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0x1cee);
        mdio_write(tp, 0x14, 0xe00f);
        mdio_write(tp, 0x14, 0x0504);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x6099);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fa4);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0xc17f);
        mdio_write(tp, 0x14, 0xc200);
        mdio_write(tp, 0x14, 0xc43f);
        mdio_write(tp, 0x14, 0xcc03);
        mdio_write(tp, 0x14, 0xa701);
        mdio_write(tp, 0x14, 0xa510);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4018);
        mdio_write(tp, 0x14, 0x9910);
        mdio_write(tp, 0x14, 0x8510);
        mdio_write(tp, 0x14, 0x2860);
        mdio_write(tp, 0x14, 0xe00f);
        mdio_write(tp, 0x14, 0x0504);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x6099);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fa4);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0xa608);
        mdio_write(tp, 0x14, 0xc17d);
        mdio_write(tp, 0x14, 0xc200);
        mdio_write(tp, 0x14, 0xc43f);
        mdio_write(tp, 0x14, 0xcc03);
        mdio_write(tp, 0x14, 0xa701);
        mdio_write(tp, 0x14, 0xa510);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4018);
        mdio_write(tp, 0x14, 0x9910);
        mdio_write(tp, 0x14, 0x8510);
        mdio_write(tp, 0x14, 0x2926);
        mdio_write(tp, 0x14, 0x1792);
        mdio_write(tp, 0x14, 0x27db);
        mdio_write(tp, 0x14, 0xc000);
        mdio_write(tp, 0x14, 0xc100);
        mdio_write(tp, 0x14, 0xc200);
        mdio_write(tp, 0x14, 0xc300);
        mdio_write(tp, 0x14, 0xc400);
        mdio_write(tp, 0x14, 0xc500);
        mdio_write(tp, 0x14, 0xc600);
        mdio_write(tp, 0x14, 0xc7c1);
        mdio_write(tp, 0x14, 0xc800);
        mdio_write(tp, 0x14, 0xcc00);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x14, 0xca0f);
        mdio_write(tp, 0x14, 0xcbff);
        mdio_write(tp, 0x14, 0xa901);
        mdio_write(tp, 0x14, 0x8902);
        mdio_write(tp, 0x14, 0xc900);
        mdio_write(tp, 0x14, 0xca00);
        mdio_write(tp, 0x14, 0xcb00);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x14, 0xb804);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x6044);
        mdio_write(tp, 0x14, 0x9804);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x6099);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fa4);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x14, 0xa510);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x6098);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fa4);
        mdio_write(tp, 0x14, 0x2ccd);
        mdio_write(tp, 0x14, 0x8510);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x14, 0xd711);
        mdio_write(tp, 0x14, 0x3003);
        mdio_write(tp, 0x14, 0x1cfa);
        mdio_write(tp, 0x14, 0x2d04);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x60be);
        mdio_write(tp, 0x14, 0xe060);
        mdio_write(tp, 0x14, 0x0920);
        mdio_write(tp, 0x14, 0x1ccf);
        mdio_write(tp, 0x14, 0x2c82);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x3063);
        mdio_write(tp, 0x14, 0x1948);
        mdio_write(tp, 0x14, 0x288a);
        mdio_write(tp, 0x14, 0x1ccf);
        mdio_write(tp, 0x14, 0x29bd);
        mdio_write(tp, 0x14, 0xa802);
        mdio_write(tp, 0x14, 0xa303);
        mdio_write(tp, 0x14, 0x843f);
        mdio_write(tp, 0x14, 0x81ff);
        mdio_write(tp, 0x14, 0x8208);
        mdio_write(tp, 0x14, 0xa201);
        mdio_write(tp, 0x14, 0xc001);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x30a0);
        mdio_write(tp, 0x14, 0x0d15);
        mdio_write(tp, 0x14, 0x30a0);
        mdio_write(tp, 0x14, 0x3d0c);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7f4c);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0xe003);
        mdio_write(tp, 0x14, 0x0202);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x6090);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fac);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0xa20c);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x6091);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fac);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0x820e);
        mdio_write(tp, 0x14, 0xa3e0);
        mdio_write(tp, 0x14, 0xa520);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x609d);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fac);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0x8520);
        mdio_write(tp, 0x14, 0x6703);
        mdio_write(tp, 0x14, 0x2d2d);
        mdio_write(tp, 0x14, 0xa13e);
        mdio_write(tp, 0x14, 0xc001);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4000);
        mdio_write(tp, 0x14, 0x6046);
        mdio_write(tp, 0x14, 0x2d06);
        mdio_write(tp, 0x14, 0xa43f);
        mdio_write(tp, 0x14, 0xa101);
        mdio_write(tp, 0x14, 0xc020);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x3121);
        mdio_write(tp, 0x14, 0x0d3e);
        mdio_write(tp, 0x14, 0x30c0);
        mdio_write(tp, 0x14, 0x3d06);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7f4c);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0xa540);
        mdio_write(tp, 0x14, 0xc001);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4001);
        mdio_write(tp, 0x14, 0xe00f);
        mdio_write(tp, 0x14, 0x0501);
        mdio_write(tp, 0x14, 0x1da5);
        mdio_write(tp, 0x14, 0xc1c4);
        mdio_write(tp, 0x14, 0xa268);
        mdio_write(tp, 0x14, 0xa303);
        mdio_write(tp, 0x14, 0x8420);
        mdio_write(tp, 0x14, 0xe00f);
        mdio_write(tp, 0x14, 0x0502);
        mdio_write(tp, 0x14, 0x1da5);
        mdio_write(tp, 0x14, 0xc002);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4000);
        mdio_write(tp, 0x14, 0x8208);
        mdio_write(tp, 0x14, 0x8410);
        mdio_write(tp, 0x14, 0xa121);
        mdio_write(tp, 0x14, 0xc002);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4000);
        mdio_write(tp, 0x14, 0x8120);
        mdio_write(tp, 0x14, 0x8180);
        mdio_write(tp, 0x14, 0x1d90);
        mdio_write(tp, 0x14, 0xa180);
        mdio_write(tp, 0x14, 0xa13a);
        mdio_write(tp, 0x14, 0x8240);
        mdio_write(tp, 0x14, 0xa430);
        mdio_write(tp, 0x14, 0xc010);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x30e1);
        mdio_write(tp, 0x14, 0x0abc);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7f8c);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0xa480);
        mdio_write(tp, 0x14, 0xa230);
        mdio_write(tp, 0x14, 0xa303);
        mdio_write(tp, 0x14, 0xc001);
        mdio_write(tp, 0x14, 0xd70c);
        mdio_write(tp, 0x14, 0x4124);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x6120);
        mdio_write(tp, 0x14, 0xd711);
        mdio_write(tp, 0x14, 0x3128);
        mdio_write(tp, 0x14, 0x3d6f);
        mdio_write(tp, 0x14, 0x2d69);
        mdio_write(tp, 0x14, 0xa801);
        mdio_write(tp, 0x14, 0x2d65);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4000);
        mdio_write(tp, 0x14, 0xe018);
        mdio_write(tp, 0x14, 0x0208);
        mdio_write(tp, 0x14, 0xa1f8);
        mdio_write(tp, 0x14, 0x8480);
        mdio_write(tp, 0x14, 0xc004);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4000);
        mdio_write(tp, 0x14, 0x6046);
        mdio_write(tp, 0x14, 0x2d06);
        mdio_write(tp, 0x14, 0xa43f);
        mdio_write(tp, 0x14, 0xa105);
        mdio_write(tp, 0x14, 0x8228);
        mdio_write(tp, 0x14, 0xc004);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4000);
        mdio_write(tp, 0x14, 0x81bc);
        mdio_write(tp, 0x14, 0xa220);
        mdio_write(tp, 0x14, 0x1d90);
        mdio_write(tp, 0x14, 0x8220);
        mdio_write(tp, 0x14, 0xa1bc);
        mdio_write(tp, 0x14, 0xc040);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x30e1);
        mdio_write(tp, 0x14, 0x0abc);
        mdio_write(tp, 0x14, 0x30e1);
        mdio_write(tp, 0x14, 0x3d06);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7f4c);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0xa802);
        mdio_write(tp, 0x14, 0xd70c);
        mdio_write(tp, 0x14, 0x4244);
        mdio_write(tp, 0x14, 0xa301);
        mdio_write(tp, 0x14, 0xc004);
        mdio_write(tp, 0x14, 0xd711);
        mdio_write(tp, 0x14, 0x3128);
        mdio_write(tp, 0x14, 0x3d9e);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x5f80);
        mdio_write(tp, 0x14, 0xd711);
        mdio_write(tp, 0x14, 0x3109);
        mdio_write(tp, 0x14, 0x3da0);
        mdio_write(tp, 0x14, 0x2da4);
        mdio_write(tp, 0x14, 0xa801);
        mdio_write(tp, 0x14, 0x2d93);
        mdio_write(tp, 0x14, 0xa802);
        mdio_write(tp, 0x14, 0xc004);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x4000);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x14, 0xa510);
        mdio_write(tp, 0x14, 0xd710);
        mdio_write(tp, 0x14, 0x609a);
        mdio_write(tp, 0x14, 0xd71e);
        mdio_write(tp, 0x14, 0x7fac);
        mdio_write(tp, 0x14, 0x2ab6);
        mdio_write(tp, 0x14, 0x8510);
        mdio_write(tp, 0x14, 0x0800);
        mdio_write(tp, 0x13, 0xA01A);
        mdio_write(tp, 0x14, 0x0000);
        mdio_write(tp, 0x13, 0xA006);
        mdio_write(tp, 0x14, 0x0ad6);
        mdio_write(tp, 0x13, 0xA004);
        mdio_write(tp, 0x14, 0x07f5);
        mdio_write(tp, 0x13, 0xA002);
        mdio_write(tp, 0x14, 0x06cc);
        mdio_write(tp, 0x13, 0xA000);
        mdio_write(tp, 0x14, 0xf7db);
        mdio_write(tp, 0x13, 0xB820);
        mdio_write(tp, 0x14, 0x0210);
        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0x83a0);
        mdio_write(tp, 0x14, 0xaf83);
        mdio_write(tp, 0x14, 0xacaf);
        mdio_write(tp, 0x14, 0x83b8);
        mdio_write(tp, 0x14, 0xaf83);
        mdio_write(tp, 0x14, 0xcdaf);
        mdio_write(tp, 0x14, 0x83d3);
        mdio_write(tp, 0x14, 0x0204);
        mdio_write(tp, 0x14, 0x9a02);
        mdio_write(tp, 0x14, 0x09a9);
        mdio_write(tp, 0x14, 0x0284);
        mdio_write(tp, 0x14, 0x61af);
        mdio_write(tp, 0x14, 0x02fc);
        mdio_write(tp, 0x14, 0xad20);
        mdio_write(tp, 0x14, 0x0302);
        mdio_write(tp, 0x14, 0x867c);
        mdio_write(tp, 0x14, 0xad21);
        mdio_write(tp, 0x14, 0x0302);
        mdio_write(tp, 0x14, 0x85c9);
        mdio_write(tp, 0x14, 0xad22);
        mdio_write(tp, 0x14, 0x0302);
        mdio_write(tp, 0x14, 0x1bc0);
        mdio_write(tp, 0x14, 0xaf17);
        mdio_write(tp, 0x14, 0xe302);
        mdio_write(tp, 0x14, 0x8703);
        mdio_write(tp, 0x14, 0xaf18);
        mdio_write(tp, 0x14, 0x6201);
        mdio_write(tp, 0x14, 0x06e0);
        mdio_write(tp, 0x14, 0x8148);
        mdio_write(tp, 0x14, 0xaf3c);
        mdio_write(tp, 0x14, 0x69f8);
        mdio_write(tp, 0x14, 0xf9fa);
        mdio_write(tp, 0x14, 0xef69);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0x10f7);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0x131f);
        mdio_write(tp, 0x14, 0xd104);
        mdio_write(tp, 0x14, 0xbf87);
        mdio_write(tp, 0x14, 0xf302);
        mdio_write(tp, 0x14, 0x4259);
        mdio_write(tp, 0x14, 0x0287);
        mdio_write(tp, 0x14, 0x88bf);
        mdio_write(tp, 0x14, 0x87cf);
        mdio_write(tp, 0x14, 0xd7b8);
        mdio_write(tp, 0x14, 0x22d0);
        mdio_write(tp, 0x14, 0x0c02);
        mdio_write(tp, 0x14, 0x4252);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0xcda0);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0xce8b);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0xd1f5);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0xd2a9);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0xd30a);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0xf010);
        mdio_write(tp, 0x14, 0xee80);
        mdio_write(tp, 0x14, 0xf38f);
        mdio_write(tp, 0x14, 0xee81);
        mdio_write(tp, 0x14, 0x011e);
        mdio_write(tp, 0x14, 0xee81);
        mdio_write(tp, 0x14, 0x0b4a);
        mdio_write(tp, 0x14, 0xee81);
        mdio_write(tp, 0x14, 0x0c7c);
        mdio_write(tp, 0x14, 0xee81);
        mdio_write(tp, 0x14, 0x127f);
        mdio_write(tp, 0x14, 0xd100);
        mdio_write(tp, 0x14, 0x0210);
        mdio_write(tp, 0x14, 0xb5ee);
        mdio_write(tp, 0x14, 0x8088);
        mdio_write(tp, 0x14, 0xa4ee);
        mdio_write(tp, 0x14, 0x8089);
        mdio_write(tp, 0x14, 0x44ee);
        mdio_write(tp, 0x14, 0x809a);
        mdio_write(tp, 0x14, 0xa4ee);
        mdio_write(tp, 0x14, 0x809b);
        mdio_write(tp, 0x14, 0x44ee);
        mdio_write(tp, 0x14, 0x809c);
        mdio_write(tp, 0x14, 0xa7ee);
        mdio_write(tp, 0x14, 0x80a5);
        mdio_write(tp, 0x14, 0xa7d2);
        mdio_write(tp, 0x14, 0x0002);
        mdio_write(tp, 0x14, 0x0e66);
        mdio_write(tp, 0x14, 0x0285);
        mdio_write(tp, 0x14, 0xc0ee);
        mdio_write(tp, 0x14, 0x87fc);
        mdio_write(tp, 0x14, 0x00e0);
        mdio_write(tp, 0x14, 0x8245);
        mdio_write(tp, 0x14, 0xf622);
        mdio_write(tp, 0x14, 0xe482);
        mdio_write(tp, 0x14, 0x45ef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xfdfc);
        mdio_write(tp, 0x14, 0x0402);
        mdio_write(tp, 0x14, 0x847a);
        mdio_write(tp, 0x14, 0x0284);
        mdio_write(tp, 0x14, 0xb302);
        mdio_write(tp, 0x14, 0x0cab);
        mdio_write(tp, 0x14, 0x020c);
        mdio_write(tp, 0x14, 0xc402);
        mdio_write(tp, 0x14, 0x0cef);
        mdio_write(tp, 0x14, 0x020d);
        mdio_write(tp, 0x14, 0x0802);
        mdio_write(tp, 0x14, 0x0d33);
        mdio_write(tp, 0x14, 0x020c);
        mdio_write(tp, 0x14, 0x3d04);
        mdio_write(tp, 0x14, 0xf8fa);
        mdio_write(tp, 0x14, 0xef69);
        mdio_write(tp, 0x14, 0xe182);
        mdio_write(tp, 0x14, 0x2fac);
        mdio_write(tp, 0x14, 0x291a);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x24ac);
        mdio_write(tp, 0x14, 0x2102);
        mdio_write(tp, 0x14, 0xae22);
        mdio_write(tp, 0x14, 0x0210);
        mdio_write(tp, 0x14, 0x57f6);
        mdio_write(tp, 0x14, 0x21e4);
        mdio_write(tp, 0x14, 0x8224);
        mdio_write(tp, 0x14, 0xd101);
        mdio_write(tp, 0x14, 0xbf44);
        mdio_write(tp, 0x14, 0xd202);
        mdio_write(tp, 0x14, 0x4259);
        mdio_write(tp, 0x14, 0xae10);
        mdio_write(tp, 0x14, 0x0212);
        mdio_write(tp, 0x14, 0x4cf6);
        mdio_write(tp, 0x14, 0x29e5);
        mdio_write(tp, 0x14, 0x822f);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x24f6);
        mdio_write(tp, 0x14, 0x21e4);
        mdio_write(tp, 0x14, 0x8224);
        mdio_write(tp, 0x14, 0xef96);
        mdio_write(tp, 0x14, 0xfefc);
        mdio_write(tp, 0x14, 0x04f8);
        mdio_write(tp, 0x14, 0xe182);
        mdio_write(tp, 0x14, 0x2fac);
        mdio_write(tp, 0x14, 0x2a18);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x24ac);
        mdio_write(tp, 0x14, 0x2202);
        mdio_write(tp, 0x14, 0xae26);
        mdio_write(tp, 0x14, 0x0284);
        mdio_write(tp, 0x14, 0xf802);
        mdio_write(tp, 0x14, 0x8565);
        mdio_write(tp, 0x14, 0xd101);
        mdio_write(tp, 0x14, 0xbf44);
        mdio_write(tp, 0x14, 0xd502);
        mdio_write(tp, 0x14, 0x4259);
        mdio_write(tp, 0x14, 0xae0e);
        mdio_write(tp, 0x14, 0x0284);
        mdio_write(tp, 0x14, 0xea02);
        mdio_write(tp, 0x14, 0x85a9);
        mdio_write(tp, 0x14, 0xe182);
        mdio_write(tp, 0x14, 0x2ff6);
        mdio_write(tp, 0x14, 0x2ae5);
        mdio_write(tp, 0x14, 0x822f);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x24f6);
        mdio_write(tp, 0x14, 0x22e4);
        mdio_write(tp, 0x14, 0x8224);
        mdio_write(tp, 0x14, 0xfc04);
        mdio_write(tp, 0x14, 0xf9e2);
        mdio_write(tp, 0x14, 0x8011);
        mdio_write(tp, 0x14, 0xad31);
        mdio_write(tp, 0x14, 0x05d2);
        mdio_write(tp, 0x14, 0x0002);
        mdio_write(tp, 0x14, 0x0e66);
        mdio_write(tp, 0x14, 0xfd04);
        mdio_write(tp, 0x14, 0xf8f9);
        mdio_write(tp, 0x14, 0xfaef);
        mdio_write(tp, 0x14, 0x69e0);
        mdio_write(tp, 0x14, 0x8011);
        mdio_write(tp, 0x14, 0xad21);
        mdio_write(tp, 0x14, 0x5cbf);
        mdio_write(tp, 0x14, 0x43be);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x97ac);
        mdio_write(tp, 0x14, 0x281b);
        mdio_write(tp, 0x14, 0xbf43);
        mdio_write(tp, 0x14, 0xc102);
        mdio_write(tp, 0x14, 0x4297);
        mdio_write(tp, 0x14, 0xac28);
        mdio_write(tp, 0x14, 0x12bf);
        mdio_write(tp, 0x14, 0x43c7);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x97ac);
        mdio_write(tp, 0x14, 0x2804);
        mdio_write(tp, 0x14, 0xd300);
        mdio_write(tp, 0x14, 0xae07);
        mdio_write(tp, 0x14, 0xd306);
        mdio_write(tp, 0x14, 0xaf85);
        mdio_write(tp, 0x14, 0x56d3);
        mdio_write(tp, 0x14, 0x03e0);
        mdio_write(tp, 0x14, 0x8011);
        mdio_write(tp, 0x14, 0xad26);
        mdio_write(tp, 0x14, 0x25bf);
        mdio_write(tp, 0x14, 0x4559);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x97e2);
        mdio_write(tp, 0x14, 0x8073);
        mdio_write(tp, 0x14, 0x0d21);
        mdio_write(tp, 0x14, 0xf637);
        mdio_write(tp, 0x14, 0x0d11);
        mdio_write(tp, 0x14, 0xf62f);
        mdio_write(tp, 0x14, 0x1b21);
        mdio_write(tp, 0x14, 0xaa02);
        mdio_write(tp, 0x14, 0xae10);
        mdio_write(tp, 0x14, 0xe280);
        mdio_write(tp, 0x14, 0x740d);
        mdio_write(tp, 0x14, 0x21f6);
        mdio_write(tp, 0x14, 0x371b);
        mdio_write(tp, 0x14, 0x21aa);
        mdio_write(tp, 0x14, 0x0313);
        mdio_write(tp, 0x14, 0xae02);
        mdio_write(tp, 0x14, 0x2b02);
        mdio_write(tp, 0x14, 0x020e);
        mdio_write(tp, 0x14, 0x5102);
        mdio_write(tp, 0x14, 0x0e66);
        mdio_write(tp, 0x14, 0x020f);
        mdio_write(tp, 0x14, 0xa3ef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xfdfc);
        mdio_write(tp, 0x14, 0x04f8);
        mdio_write(tp, 0x14, 0xf9fa);
        mdio_write(tp, 0x14, 0xef69);
        mdio_write(tp, 0x14, 0xe080);
        mdio_write(tp, 0x14, 0x12ad);
        mdio_write(tp, 0x14, 0x2733);
        mdio_write(tp, 0x14, 0xbf43);
        mdio_write(tp, 0x14, 0xbe02);
        mdio_write(tp, 0x14, 0x4297);
        mdio_write(tp, 0x14, 0xac28);
        mdio_write(tp, 0x14, 0x09bf);
        mdio_write(tp, 0x14, 0x43c1);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x97ad);
        mdio_write(tp, 0x14, 0x2821);
        mdio_write(tp, 0x14, 0xbf45);
        mdio_write(tp, 0x14, 0x5902);
        mdio_write(tp, 0x14, 0x4297);
        mdio_write(tp, 0x14, 0xe387);
        mdio_write(tp, 0x14, 0xffd2);
        mdio_write(tp, 0x14, 0x001b);
        mdio_write(tp, 0x14, 0x45ac);
        mdio_write(tp, 0x14, 0x2711);
        mdio_write(tp, 0x14, 0xe187);
        mdio_write(tp, 0x14, 0xfebf);
        mdio_write(tp, 0x14, 0x87e4);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x590d);
        mdio_write(tp, 0x14, 0x11bf);
        mdio_write(tp, 0x14, 0x87e7);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59ef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xfdfc);
        mdio_write(tp, 0x14, 0x04f8);
        mdio_write(tp, 0x14, 0xfaef);
        mdio_write(tp, 0x14, 0x69d1);
        mdio_write(tp, 0x14, 0x00bf);
        mdio_write(tp, 0x14, 0x87e4);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59bf);
        mdio_write(tp, 0x14, 0x87e7);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59ef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xfc04);
        mdio_write(tp, 0x14, 0xee87);
        mdio_write(tp, 0x14, 0xff46);
        mdio_write(tp, 0x14, 0xee87);
        mdio_write(tp, 0x14, 0xfe01);
        mdio_write(tp, 0x14, 0x04f8);
        mdio_write(tp, 0x14, 0xfaef);
        mdio_write(tp, 0x14, 0x69e0);
        mdio_write(tp, 0x14, 0x8241);
        mdio_write(tp, 0x14, 0xa000);
        mdio_write(tp, 0x14, 0x0502);
        mdio_write(tp, 0x14, 0x85eb);
        mdio_write(tp, 0x14, 0xae0e);
        mdio_write(tp, 0x14, 0xa001);
        mdio_write(tp, 0x14, 0x0502);
        mdio_write(tp, 0x14, 0x1a5a);
        mdio_write(tp, 0x14, 0xae06);
        mdio_write(tp, 0x14, 0xa002);
        mdio_write(tp, 0x14, 0x0302);
        mdio_write(tp, 0x14, 0x1ae6);
        mdio_write(tp, 0x14, 0xef96);
        mdio_write(tp, 0x14, 0xfefc);
        mdio_write(tp, 0x14, 0x04f8);
        mdio_write(tp, 0x14, 0xf9fa);
        mdio_write(tp, 0x14, 0xef69);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x29f6);
        mdio_write(tp, 0x14, 0x21e4);
        mdio_write(tp, 0x14, 0x8229);
        mdio_write(tp, 0x14, 0xe080);
        mdio_write(tp, 0x14, 0x10ac);
        mdio_write(tp, 0x14, 0x2202);
        mdio_write(tp, 0x14, 0xae76);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x27f7);
        mdio_write(tp, 0x14, 0x21e4);
        mdio_write(tp, 0x14, 0x8227);
        mdio_write(tp, 0x14, 0xbf43);
        mdio_write(tp, 0x14, 0x1302);
        mdio_write(tp, 0x14, 0x4297);
        mdio_write(tp, 0x14, 0xef21);
        mdio_write(tp, 0x14, 0xbf43);
        mdio_write(tp, 0x14, 0x1602);
        mdio_write(tp, 0x14, 0x4297);
        mdio_write(tp, 0x14, 0x0c11);
        mdio_write(tp, 0x14, 0x1e21);
        mdio_write(tp, 0x14, 0xbf43);
        mdio_write(tp, 0x14, 0x1902);
        mdio_write(tp, 0x14, 0x4297);
        mdio_write(tp, 0x14, 0x0c12);
        mdio_write(tp, 0x14, 0x1e21);
        mdio_write(tp, 0x14, 0xe682);
        mdio_write(tp, 0x14, 0x43a2);
        mdio_write(tp, 0x14, 0x000a);
        mdio_write(tp, 0x14, 0xe182);
        mdio_write(tp, 0x14, 0x27f6);
        mdio_write(tp, 0x14, 0x29e5);
        mdio_write(tp, 0x14, 0x8227);
        mdio_write(tp, 0x14, 0xae42);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x44f7);
        mdio_write(tp, 0x14, 0x21e4);
        mdio_write(tp, 0x14, 0x8244);
        mdio_write(tp, 0x14, 0x0246);
        mdio_write(tp, 0x14, 0xaebf);
        mdio_write(tp, 0x14, 0x4325);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x97ef);
        mdio_write(tp, 0x14, 0x21bf);
        mdio_write(tp, 0x14, 0x431c);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x970c);
        mdio_write(tp, 0x14, 0x121e);
        mdio_write(tp, 0x14, 0x21bf);
        mdio_write(tp, 0x14, 0x431f);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x970c);
        mdio_write(tp, 0x14, 0x131e);
        mdio_write(tp, 0x14, 0x21bf);
        mdio_write(tp, 0x14, 0x4328);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x970c);
        mdio_write(tp, 0x14, 0x141e);
        mdio_write(tp, 0x14, 0x21bf);
        mdio_write(tp, 0x14, 0x44b1);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x970c);
        mdio_write(tp, 0x14, 0x161e);
        mdio_write(tp, 0x14, 0x21e6);
        mdio_write(tp, 0x14, 0x8242);
        mdio_write(tp, 0x14, 0xee82);
        mdio_write(tp, 0x14, 0x4101);
        mdio_write(tp, 0x14, 0xef96);
        mdio_write(tp, 0x14, 0xfefd);
        mdio_write(tp, 0x14, 0xfc04);
        mdio_write(tp, 0x14, 0xf8fa);
        mdio_write(tp, 0x14, 0xef69);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x46a0);
        mdio_write(tp, 0x14, 0x0005);
        mdio_write(tp, 0x14, 0x0286);
        mdio_write(tp, 0x14, 0x96ae);
        mdio_write(tp, 0x14, 0x06a0);
        mdio_write(tp, 0x14, 0x0103);
        mdio_write(tp, 0x14, 0x0219);
        mdio_write(tp, 0x14, 0x19ef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xfc04);
        mdio_write(tp, 0x14, 0xf8fa);
        mdio_write(tp, 0x14, 0xef69);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x29f6);
        mdio_write(tp, 0x14, 0x20e4);
        mdio_write(tp, 0x14, 0x8229);
        mdio_write(tp, 0x14, 0xe080);
        mdio_write(tp, 0x14, 0x10ac);
        mdio_write(tp, 0x14, 0x2102);
        mdio_write(tp, 0x14, 0xae54);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x27f7);
        mdio_write(tp, 0x14, 0x20e4);
        mdio_write(tp, 0x14, 0x8227);
        mdio_write(tp, 0x14, 0xbf42);
        mdio_write(tp, 0x14, 0xe602);
        mdio_write(tp, 0x14, 0x4297);
        mdio_write(tp, 0x14, 0xac28);
        mdio_write(tp, 0x14, 0x22bf);
        mdio_write(tp, 0x14, 0x430d);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x97e5);
        mdio_write(tp, 0x14, 0x8247);
        mdio_write(tp, 0x14, 0xac28);
        mdio_write(tp, 0x14, 0x20d1);
        mdio_write(tp, 0x14, 0x03bf);
        mdio_write(tp, 0x14, 0x4307);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59ee);
        mdio_write(tp, 0x14, 0x8246);
        mdio_write(tp, 0x14, 0x00e1);
        mdio_write(tp, 0x14, 0x8227);
        mdio_write(tp, 0x14, 0xf628);
        mdio_write(tp, 0x14, 0xe582);
        mdio_write(tp, 0x14, 0x27ae);
        mdio_write(tp, 0x14, 0x21d1);
        mdio_write(tp, 0x14, 0x04bf);
        mdio_write(tp, 0x14, 0x4307);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59ae);
        mdio_write(tp, 0x14, 0x08d1);
        mdio_write(tp, 0x14, 0x05bf);
        mdio_write(tp, 0x14, 0x4307);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59e0);
        mdio_write(tp, 0x14, 0x8244);
        mdio_write(tp, 0x14, 0xf720);
        mdio_write(tp, 0x14, 0xe482);
        mdio_write(tp, 0x14, 0x4402);
        mdio_write(tp, 0x14, 0x46ae);
        mdio_write(tp, 0x14, 0xee82);
        mdio_write(tp, 0x14, 0x4601);
        mdio_write(tp, 0x14, 0xef96);
        mdio_write(tp, 0x14, 0xfefc);
        mdio_write(tp, 0x14, 0x04f8);
        mdio_write(tp, 0x14, 0xfaef);
        mdio_write(tp, 0x14, 0x69e0);
        mdio_write(tp, 0x14, 0x8013);
        mdio_write(tp, 0x14, 0xad24);
        mdio_write(tp, 0x14, 0x1cbf);
        mdio_write(tp, 0x14, 0x87f0);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x97ad);
        mdio_write(tp, 0x14, 0x2813);
        mdio_write(tp, 0x14, 0xe087);
        mdio_write(tp, 0x14, 0xfca0);
        mdio_write(tp, 0x14, 0x0005);
        mdio_write(tp, 0x14, 0x0287);
        mdio_write(tp, 0x14, 0x36ae);
        mdio_write(tp, 0x14, 0x10a0);
        mdio_write(tp, 0x14, 0x0105);
        mdio_write(tp, 0x14, 0x0287);
        mdio_write(tp, 0x14, 0x48ae);
        mdio_write(tp, 0x14, 0x08e0);
        mdio_write(tp, 0x14, 0x8230);
        mdio_write(tp, 0x14, 0xf626);
        mdio_write(tp, 0x14, 0xe482);
        mdio_write(tp, 0x14, 0x30ef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xfc04);
        mdio_write(tp, 0x14, 0xf8e0);
        mdio_write(tp, 0x14, 0x8245);
        mdio_write(tp, 0x14, 0xf722);
        mdio_write(tp, 0x14, 0xe482);
        mdio_write(tp, 0x14, 0x4502);
        mdio_write(tp, 0x14, 0x46ae);
        mdio_write(tp, 0x14, 0xee87);
        mdio_write(tp, 0x14, 0xfc01);
        mdio_write(tp, 0x14, 0xfc04);
        mdio_write(tp, 0x14, 0xf8fa);
        mdio_write(tp, 0x14, 0xef69);
        mdio_write(tp, 0x14, 0xfb02);
        mdio_write(tp, 0x14, 0x46d3);
        mdio_write(tp, 0x14, 0xad50);
        mdio_write(tp, 0x14, 0x2fbf);
        mdio_write(tp, 0x14, 0x87ed);
        mdio_write(tp, 0x14, 0xd101);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59bf);
        mdio_write(tp, 0x14, 0x87ed);
        mdio_write(tp, 0x14, 0xd100);
        mdio_write(tp, 0x14, 0x0242);
        mdio_write(tp, 0x14, 0x59e0);
        mdio_write(tp, 0x14, 0x8245);
        mdio_write(tp, 0x14, 0xf622);
        mdio_write(tp, 0x14, 0xe482);
        mdio_write(tp, 0x14, 0x4502);
        mdio_write(tp, 0x14, 0x46ae);
        mdio_write(tp, 0x14, 0xd100);
        mdio_write(tp, 0x14, 0xbf87);
        mdio_write(tp, 0x14, 0xf002);
        mdio_write(tp, 0x14, 0x4259);
        mdio_write(tp, 0x14, 0xee87);
        mdio_write(tp, 0x14, 0xfc00);
        mdio_write(tp, 0x14, 0xe082);
        mdio_write(tp, 0x14, 0x30f6);
        mdio_write(tp, 0x14, 0x26e4);
        mdio_write(tp, 0x14, 0x8230);
        mdio_write(tp, 0x14, 0xffef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xfc04);
        mdio_write(tp, 0x14, 0xf8f9);
        mdio_write(tp, 0x14, 0xface);
        mdio_write(tp, 0x14, 0xfaef);
        mdio_write(tp, 0x14, 0x69fb);
        mdio_write(tp, 0x14, 0xbf87);
        mdio_write(tp, 0x14, 0xb3d7);
        mdio_write(tp, 0x14, 0x001c);
        mdio_write(tp, 0x14, 0xd819);
        mdio_write(tp, 0x14, 0xd919);
        mdio_write(tp, 0x14, 0xda19);
        mdio_write(tp, 0x14, 0xdb19);
        mdio_write(tp, 0x14, 0x07ef);
        mdio_write(tp, 0x14, 0x9502);
        mdio_write(tp, 0x14, 0x4259);
        mdio_write(tp, 0x14, 0x073f);
        mdio_write(tp, 0x14, 0x0004);
        mdio_write(tp, 0x14, 0x9fec);
        mdio_write(tp, 0x14, 0xffef);
        mdio_write(tp, 0x14, 0x96fe);
        mdio_write(tp, 0x14, 0xc6fe);
        mdio_write(tp, 0x14, 0xfdfc);
        mdio_write(tp, 0x14, 0x0400);
        mdio_write(tp, 0x14, 0x0145);
        mdio_write(tp, 0x14, 0x7d00);
        mdio_write(tp, 0x14, 0x0345);
        mdio_write(tp, 0x14, 0x5c00);
        mdio_write(tp, 0x14, 0x0143);
        mdio_write(tp, 0x14, 0x4f00);
        mdio_write(tp, 0x14, 0x0387);
        mdio_write(tp, 0x14, 0xdb00);
        mdio_write(tp, 0x14, 0x0987);
        mdio_write(tp, 0x14, 0xde00);
        mdio_write(tp, 0x14, 0x0987);
        mdio_write(tp, 0x14, 0xe100);
        mdio_write(tp, 0x14, 0x0087);
        mdio_write(tp, 0x14, 0xeaa4);
        mdio_write(tp, 0x14, 0x00b8);
        mdio_write(tp, 0x14, 0x20c4);
        mdio_write(tp, 0x14, 0x1600);
        mdio_write(tp, 0x14, 0x000f);
        mdio_write(tp, 0x14, 0xf800);
        mdio_write(tp, 0x14, 0x7098);
        mdio_write(tp, 0x14, 0xa58a);
        mdio_write(tp, 0x14, 0xb6a8);
        mdio_write(tp, 0x14, 0x3e50);
        mdio_write(tp, 0x14, 0xa83e);
        mdio_write(tp, 0x14, 0x33bc);
        mdio_write(tp, 0x14, 0xc622);
        mdio_write(tp, 0x14, 0xbcc6);
        mdio_write(tp, 0x14, 0xaaa4);
        mdio_write(tp, 0x14, 0x42ff);
        mdio_write(tp, 0x14, 0xc408);
        mdio_write(tp, 0x14, 0x00c4);
        mdio_write(tp, 0x14, 0x16a8);
        mdio_write(tp, 0x14, 0xbcc0);
        mdio_write(tp, 0x13, 0xb818);
        mdio_write(tp, 0x14, 0x02f3);
        mdio_write(tp, 0x13, 0xb81a);
        mdio_write(tp, 0x14, 0x17d1);
        mdio_write(tp, 0x13, 0xb81c);
        mdio_write(tp, 0x14, 0x185a);
        mdio_write(tp, 0x13, 0xb81e);
        mdio_write(tp, 0x14, 0x3c66);
        mdio_write(tp, 0x13, 0xb820);
        mdio_write(tp, 0x14, 0x021f);
        mdio_write(tp, 0x13, 0xc416);
        mdio_write(tp, 0x14, 0x0500);
        mdio_write(tp, 0x13, 0xb82e);
        mdio_write(tp, 0x14, 0xfffc);
        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0x0000);
        mdio_write(tp, 0x14, 0x0000);
        mdio_write(tp, 0x1f, 0x0B82);
        gphy_val = mdio_read(tp, 0x10);
        gphy_val &= ~(BIT_9);
        mdio_write(tp, 0x10, gphy_val);
        mdio_write(tp, 0x1f, 0x0A43);
        mdio_write(tp, 0x13, 0x8146);
        mdio_write(tp, 0x14, 0x0000);
        mdio_write(tp, 0x1f, 0x0B82);
        gphy_val = mdio_read(tp, 0x10);
        gphy_val &= ~(BIT_4);
        mdio_write(tp, 0x10, gphy_val);
    } 

    rtl8168_write_hw_phy_mcu_code_ver(dev);

    mdio_write(tp,0x1F, 0x0000);

    tp->HwHasWrRamCodeToMicroP = TRUE;
}

static void
rtl8168_hw_phy_config(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned int gphy_val;
    unsigned long flags;

    tp->phy_reset_enable(dev);

    spin_lock_irqsave(&tp->phy_lock, flags);

    rtl8168_init_hw_phy_mcu(dev);

    if (tp->mcfg == CFG_METHOD_18) {
        if (tp->HwHasWrRamCodeToMicroP == TRUE) {
            mdio_write(tp, 0x1F, 0x0005);
            mdio_write(tp, 0x05, 0x8b80);
            gphy_val = mdio_read(tp, 0x06);
            gphy_val |= BIT_2 | BIT_1;
            mdio_write(tp, 0x06, gphy_val);
            mdio_write(tp, 0x1F, 0x0000);
        }

        mdio_write(tp, 0x1f, 0x0007);
        mdio_write(tp, 0x1e, 0x002D);
        gphy_val = mdio_read(tp, 0x18);
        gphy_val |= BIT_4;
        mdio_write(tp, 0x18, gphy_val);
        mdio_write(tp, 0x1f, 0x0000);
        gphy_val = mdio_read(tp, 0x14);
        gphy_val |= BIT_15;
        mdio_write(tp, 0x14, gphy_val);

        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B86);
        gphy_val = mdio_read(tp, 0x06);
        gphy_val |= BIT_0;
        mdio_write(tp, 0x06, gphy_val);
        mdio_write(tp, 0x1f, 0x0000);

        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B85);
        gphy_val = mdio_read(tp, 0x06);
        gphy_val |= BIT_14;
        mdio_write(tp, 0x06, gphy_val);
        mdio_write(tp, 0x1f, 0x0000);

        mdio_write(tp, 0x1F, 0x0003);
        mdio_write(tp, 0x09, 0xA20F);
        mdio_write(tp, 0x1F, 0x0000);

        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B55);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x05, 0x8B5E);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x05, 0x8B67);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x05, 0x8B70);
        mdio_write(tp, 0x06, 0x0000);
        mdio_write(tp, 0x1F, 0x0000);
        mdio_write(tp, 0x1F, 0x0007);
        mdio_write(tp, 0x1E, 0x0078);
        mdio_write(tp, 0x17, 0x0000);
        mdio_write(tp, 0x19, 0x00FB);
        mdio_write(tp, 0x1F, 0x0000);

        mdio_write(tp, 0x1F, 0x0005);
        mdio_write(tp, 0x05, 0x8B79);
        mdio_write(tp, 0x06, 0xAA00);
        mdio_write(tp, 0x1F, 0x0000);

        mdio_write(tp, 0x1f, 0x0003);
        mdio_write(tp, 0x01, 0x328A);
        mdio_write(tp, 0x1f, 0x0000);

        mdio_write(tp, 0x1f, 0x0005);
        mdio_write(tp, 0x05, 0x8B54);
        mdio_write(tp, 0x06, mdio_read(tp, 0x06) & ~BIT_11);
        mdio_write(tp, 0x05, 0x8B5D);
        mdio_write(tp, 0x06, mdio_read(tp, 0x06) & ~BIT_11);
        mdio_write(tp, 0x05, 0x8A7C);
        mdio_write(tp, 0x06, mdio_read(tp, 0x06) & ~BIT_8);
        mdio_write(tp, 0x05, 0x8A7F);
        mdio_write(tp, 0x06, mdio_read(tp, 0x06) | BIT_8);
        mdio_write(tp, 0x05, 0x8A82);
        mdio_write(tp, 0x06, mdio_read(tp, 0x06) & ~BIT_8);
        mdio_write(tp, 0x05, 0x8A85);
        mdio_write(tp, 0x06, mdio_read(tp, 0x06) & ~BIT_8);
        mdio_write(tp, 0x05, 0x8A88);
        mdio_write(tp, 0x06, mdio_read(tp, 0x06) & ~BIT_8);
        mdio_write(tp, 0x1f, 0x0000);

        if (tp->HwHasWrRamCodeToMicroP == TRUE) {
            mdio_write(tp, 0x1f, 0x0005);
            mdio_write(tp, 0x05, 0x8b85);
            mdio_write(tp, 0x06, mdio_read(tp, 0x06) | BIT_15);
            mdio_write(tp, 0x1f, 0x0000);
        }

        if (aspm) {
            if (tp->HwHasWrRamCodeToMicroP == TRUE) {
                mdio_write(tp, 0x1f, 0x0000);
                gphy_val = mdio_read(tp, 0x15);
                gphy_val |= BIT_12;
                mdio_write(tp, 0x15, gphy_val);
            }
        }
    } else if (tp->mcfg == CFG_METHOD_21) {
        mdio_write(tp, 0x1F, 0x0A46);
        gphy_val = mdio_read(tp, 0x10);
        mdio_write(tp, 0x1F, 0x0BCC);
        if (gphy_val & BIT_8)
            gphy_val = mdio_read(tp, 0x12) & ~BIT_15;
        else
            gphy_val = mdio_read(tp, 0x12) | BIT_15;
        mdio_write(tp, 0x1F, 0x0A46);
        gphy_val = mdio_read(tp, 0x13);
        mdio_write(tp, 0x1F, 0x0C41);
        if (gphy_val & BIT_8)
            gphy_val = mdio_read(tp, 0x15) | BIT_1;
        else
            gphy_val = mdio_read(tp, 0x15) & ~BIT_1;

        mdio_write(tp, 0x1F, 0x0A44);
        mdio_write(tp, 0x11, mdio_read(tp, 0x11) | BIT_2 | BIT_3);

        mdio_write(tp, 0x1F, 0x0BCC);
        mdio_write(tp, 0x14, mdio_read(tp, 0x14) & ~BIT_8);
        mdio_write(tp, 0x1F, 0x0A44);
        mdio_write(tp, 0x11, mdio_read(tp, 0x11) | BIT_7);
        mdio_write(tp, 0x11, mdio_read(tp, 0x11) | BIT_6);
        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0x8084);
        mdio_write(tp, 0x14, mdio_read(tp, 0x14) & ~(BIT_14 | BIT_13));
        mdio_write(tp, 0x10, mdio_read(tp, 0x10) | BIT_12);
        mdio_write(tp, 0x10, mdio_read(tp, 0x10) | BIT_1);
        mdio_write(tp, 0x10, mdio_read(tp, 0x10) | BIT_0);

        mdio_write(tp, 0x1F, 0x0A4B);
        mdio_write(tp, 0x11, mdio_read(tp, 0x11) | BIT_2);

        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0x8012);
        mdio_write(tp, 0x14, mdio_read(tp, 0x14) | BIT_15);

        mdio_write(tp, 0x1F, 0x0C42);
        gphy_val = mdio_read(tp, 0x11);
        gphy_val |= BIT_14;
        gphy_val &= ~BIT_13;
        mdio_write(tp, 0x11, gphy_val);

        mdio_write(tp, 0x1F, 0x0A43);
        mdio_write(tp, 0x13, 0x809A);
        mdio_write(tp, 0x14, 0x8022);
        mdio_write(tp, 0x13, 0x80A0);
        gphy_val = mdio_read(tp, 0x14) & 0x00FF;
        gphy_val |= 0x1000;
        mdio_write(tp, 0x14, gphy_val);
        mdio_write(tp, 0x13, 0x8088);
        mdio_write(tp, 0x14, 0x9222);

        mdio_write(tp, 0x1F, 0x0BCD);
        mdio_write(tp, 0x14, 0x5065);
        mdio_write(tp, 0x14, 0xD065);
        mdio_write(tp, 0x1F, 0x0BC8);
        mdio_write(tp, 0x11, 0x5655);
        mdio_write(tp, 0x1F, 0x0BCD);
        mdio_write(tp, 0x14, 0x1065);
        mdio_write(tp, 0x14, 0x9065);
        mdio_write(tp, 0x14, 0x1065);

        if (aspm) {
            if (tp->HwHasWrRamCodeToMicroP == TRUE) {
                mdio_write(tp, 0x1F, 0x0A43);
                mdio_write(tp, 0x10, mdio_read(tp, 0x10) | BIT_2);
            }
        }

        mdio_write(tp, 0x1F, 0x0000);
    } 

#ifdef CONFIG_HW_VER_RLE0745
	/* Adjust built-in phy RX SNR */
	mdio_write(tp, 0x1F, 0x0001);
	mdio_write(tp, 0x1D, 0x000F);
	mdio_write(tp, 0x1F, 0x0001);
	mdio_write(tp, 0x10, 0x7181);
#endif

#ifdef CONFIG_HW_VER_RTS3901
	mdio_write(tp, 0x1F, 0x0001);
	mdio_write(tp, 0x13, 0x7400);
#endif

    mdio_write(tp, 0x1F, 0x0000);

    spin_unlock_irqrestore(&tp->phy_lock, flags);

    if (tp->HwHasWrRamCodeToMicroP == TRUE) {
        if (eee_enable == 1)
            rtl8168_enable_EEE(tp);
        else
            rtl8168_disable_EEE(tp);
    }
}


static inline void rtl8168_delete_link_timer(struct net_device *dev, struct timer_list *timer)
{
    del_timer_sync(timer);
}

static inline void rtl8168_request_link_timer(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    struct timer_list *timer = &tp->link_timer;

    init_timer(timer);
    timer->expires = jiffies + RTL8168_LINK_TIMEOUT;
    timer->data = (unsigned long)(dev);
    timer->function = rtl8168_link_timer;
    add_timer(timer);
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void
rtl8168_netpoll(struct net_device *dev)
{
    disable_irq(dev->irq);
    rtl8168_interrupt(dev->irq, dev);
    enable_irq(dev->irq);
}
#endif


static void
rtl8168_init_software_variable(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);

	tp->HwSuppDashVer = 0;

    if (HW_DASH_SUPPORT_DASH(tp) && rtl8168_check_dash(tp))
        tp->DASH = 1;
    else
        tp->DASH = 0;

    tp->intr_mask = RxDescUnavail | TxOK | RxOK | SWInt;
    tp->timer_intr_mask = PCSTimeout;


#ifdef ENABLE_DASH_SUPPORT
    if(tp->DASH) {
        if( HW_DASH_SUPPORT_TYPE_2( tp ) ) {
            tp->timer_intr_mask |= ( ISRIMR_DASH_INTR_EN | ISRIMR_DASH_INTR_CMAC_RESET);
            tp->intr_mask |= ( ISRIMR_DASH_INTR_EN | ISRIMR_DASH_INTR_CMAC_RESET);
        } else {
            tp->timer_intr_mask |= ( ISRIMR_DP_DASH_OK | ISRIMR_DP_HOST_OK | ISRIMR_DP_REQSYS_OK );
            tp->intr_mask |= ( ISRIMR_DP_DASH_OK | ISRIMR_DP_HOST_OK | ISRIMR_DP_REQSYS_OK );
        }
    }
#endif

    tp->max_jumbo_frame_size = rtl_chip_info[tp->chipset].jumbo_frame_sz;

	tp->use_timer_interrrupt = TRUE;

    if (tp->HwIcVerUnknown) {
        tp->NotWrRamCodeToMicroP = TRUE;
        tp->NotWrMcuPatchCode = TRUE;
    }

    rtl8168_get_hw_wol(dev);
}

static void
rtl8168_release_board(struct platform_device *pdev,
                      struct net_device *dev,
                      void __iomem *ioaddr)
{
    struct rtl8168_private *tp = netdev_priv(dev);
	
    rtl8168_rar_set(tp, tp->org_mac_addr);
    tp->wol_enabled = WOL_DISABLED;

    rtl8168_phy_power_down(dev);

#ifdef ENABLE_DASH_SUPPORT
    if(tp->DASH)
        FreeAllocatedDashShareMemory(dev);
#endif

    iounmap(ioaddr);
	release_mem_region(tp->addr_res->start, sizeof(struct rtl8168_private));
    free_netdev(dev);
}

static int
rtl8168_get_mac_address(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    int i;

	/*
	* try to get mac address in following order:
	*
	* 1) mac registers set by bootloader
	*/
    for (i = 0; i < MAC_ADDR_LEN; i++) {
        dev->dev_addr[i] = RTL_R8(MAC0 + i);
        tp->org_mac_addr[i] = dev->dev_addr[i]; /* keep the original MAC address */
    }

	if (!is_valid_ether_addr(dev->dev_addr)) {
		netdev_err(dev, "Get invalid MAC address from flash!\n");
	} else {
		netdev_info(dev, "Use MAC address: %pM\n", dev->dev_addr);
		rtl8168_rar_set(tp, dev->dev_addr);
	}

    memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);

    return 0;
}

/**
 * rtl8168_set_mac_address - Change the Ethernet Address of the NIC
 * @dev: network interface device structure
 * @p:   pointer to an address structure
 *
 * Return 0 on success, negative on failure
 **/
static int
rtl8168_set_mac_address(struct net_device *dev,
                        void *p)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    struct sockaddr *addr = p;
    unsigned long flags;

    if (!is_valid_ether_addr(addr->sa_data))
        return -EADDRNOTAVAIL;

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

    spin_lock_irqsave(&tp->lock, flags);

    rtl8168_rar_set(tp, dev->dev_addr);

    spin_unlock_irqrestore(&tp->lock, flags);

    return 0;
}

/******************************************************************************
 * rtl8168_rar_set - Puts an ethernet address into a receive address register.
 *
 * tp - The private data structure for driver
 * addr - Address to put into receive address register
 *****************************************************************************/
void
rtl8168_rar_set(struct rtl8168_private *tp,
                uint8_t *addr)
{
    void __iomem *ioaddr = tp->mmio_addr;
    uint32_t rar_low = 0;
    uint32_t rar_high = 0;

    rar_low = ((uint32_t) addr[0] |
               ((uint32_t) addr[1] << 8) |
               ((uint32_t) addr[2] << 16) |
               ((uint32_t) addr[3] << 24));

    rar_high = ((uint32_t) addr[4] |
                ((uint32_t) addr[5] << 8));

    RTL_W8(Cfg9346, Cfg9346_Unlock);
    RTL_W32(MAC0, rar_low);
    RTL_W32(MAC4, rar_high);
    RTL_W8(Cfg9346, Cfg9346_Lock);
}


static int
rtl8168_do_ioctl(struct net_device *dev,
                 struct ifreq *ifr,
                 int cmd)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    struct mii_ioctl_data *data = if_mii(ifr);
    int ret;
    unsigned long flags;

    ret = 0;
    switch (cmd) {
    case SIOCGMIIPHY:
        data->phy_id = 32; /* Internal PHY */
        break;

    case SIOCGMIIREG:
        spin_lock_irqsave(&tp->phy_lock, flags);
        mdio_write(tp, 0x1F, 0x0000);
        data->val_out = mdio_read(tp, data->reg_num);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;

    case SIOCSMIIREG:
        if (!capable(CAP_NET_ADMIN))
            return -EPERM;
        spin_lock_irqsave(&tp->phy_lock, flags);
        mdio_write(tp, 0x1F, 0x0000);
        mdio_write(tp, data->reg_num, data->val_in);
        spin_unlock_irqrestore(&tp->phy_lock, flags);
        break;

    case SIOCDEVPRIVATE_RTLASF:
        if (!netif_running(dev)) {
            ret = -ENODEV;
            break;
        }

        ret = rtl8168_asf_ioctl(dev, ifr);
        break;

#ifdef ENABLE_DASH_SUPPORT
    case SIOCDEVPRIVATE_RTLDASH:
        if (!netif_running(dev)) {
            ret = -ENODEV;
            break;
        }

        ret = rtl8168_dash_ioctl(dev, ifr);
        break;
#endif

    case SIOCRTLTOOL:
        ret = rtltool_ioctl(tp, ifr);
        break;

    default:
        ret = -EOPNOTSUPP;
        break;
    }

    return ret;
}

static void
rtl8168_phy_power_up (struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u32 csi_tmp;
    unsigned long flags;

    spin_lock_irqsave(&tp->phy_lock, flags);
    mdio_write(tp, 0x1F, 0x0000);
    switch (tp->mcfg) {
    case CFG_METHOD_21:
        csi_tmp = rtl8168_eri_read(ioaddr, 0x1AB, 1, ERIAR_ExGMAC);
        csi_tmp |=  ( BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7 );
        rtl8168_eri_write(ioaddr, 0x1AB, 1, csi_tmp, ERIAR_ExGMAC);
        break;
    default:
        break;
    }
    mdio_write(tp, MII_BMCR, BMCR_ANENABLE);
    spin_unlock_irqrestore(&tp->phy_lock, flags);
}

static void
rtl8168_phy_power_down (struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u32 csi_tmp;
    unsigned long flags;

    spin_lock_irqsave(&tp->phy_lock, flags);
    mdio_write(tp, 0x1F, 0x0000);
    switch (tp->mcfg) { 
    case CFG_METHOD_21:
        mdio_write(tp, MII_BMCR, BMCR_ANENABLE | BMCR_PDOWN);

        csi_tmp = rtl8168_eri_read(ioaddr, 0x1AB, 1, ERIAR_ExGMAC);
        csi_tmp &= ~( BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7 );
        rtl8168_eri_write(ioaddr, 0x1AB, 1, csi_tmp, ERIAR_ExGMAC);

        RTL_W8(0xD0, RTL_R8(0xD0) & ~BIT_6);
        break;
    default:
        mdio_write(tp, MII_BMCR, BMCR_PDOWN);
        break;
    }
    spin_unlock_irqrestore(&tp->phy_lock, flags);
}

static int __devinit
rtl8168_init_board(struct platform_device *pdev,
                   struct net_device **dev_out,
                   void __iomem **ioaddr_out,
                   struct resource **irq_out)
{
    void __iomem *ioaddr;
    struct net_device *dev;
    struct rtl8168_private *tp;
    int rc = 0, i;
	struct resource *addr_res, *irq_res;
	struct clk *clk;

    assert(ioaddr_out != NULL);

    /* dev zeroed in alloc_etherdev */
    dev = alloc_etherdev(sizeof (*tp));
    if (dev == NULL) {
        if (netif_msg_drv(&debug))
            dev_err(&pdev->dev, "unable to alloc new ethernet\n");
		rc = -1;
        goto err_out;
    }

    SET_MODULE_OWNER(dev);
    SET_NETDEV_DEV(dev, &pdev->dev);
    tp = netdev_priv(dev);
    tp->dev = dev;
    tp->msg_enable = netif_msg_init(debug.msg_enable, R8168_MSG_DEFAULT);

	addr_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if(addr_res == NULL || irq_res == NULL){
		dev_err(&pdev->dev,
				"insufficient resources\n");
		rc = -2;
		goto err_out_free;
	}

	if (resource_size(addr_res) < R8168_REGS_SIZE) {
		dev_err(&pdev->dev,
				"MMIO Resource too small\n");
		rc = -3;
		goto err_out_free;
	}

	if (!request_mem_region(addr_res->start, resource_size(addr_res),MODULENAME)) {
		dev_err(&pdev->dev,
				"cannot claim address reg area\n");
		rc = -4;
		goto err_out_free;
	}

	tp->addr_res = addr_res;
	tp->irq_res = irq_res;


    /* ioremap MMIO region */
    ioaddr = ioremap(addr_res->start, R8168_REGS_SIZE);
    if (ioaddr == NULL) {
        if (netif_msg_probe(tp))
            dev_err(&pdev->dev, "cannot remap MMIO, aborting\n");
        rc = -5;
        goto err_out_res;
    }

#ifndef CONFIG_SOC_FPGA_CODE
	rts_sys_force_reset(FORCE_RESET_ETHERNET);
	clk = clk_get(NULL, "ethernet_ck");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "ethernet_ck get failed!\n");
		goto err_out_res;
	}

	/* reset mac */
	if (clk_prepare(clk)) {
		dev_err(&pdev->dev, "failed to prepare clock!\n");
		goto err_out_res;
	}
	/* wait 20ms until reset is done */
	msleep(20);
	/* release mac */
	if (clk_enable(clk)) {
		dev_err(&pdev->dev, "failed to enable clock!\n");
		goto err_out_res;
	}

	msleep(20);
	rts_sys_force_reset(FORCE_RESET_ETHERNET_CLR);
	msleep(20);
	rts_efuse_set_done();
#endif

    /* Identify chip attached to board */
    rtl8168_get_mac_version(tp, ioaddr);

    rtl8168_print_mac_version(tp);

    for (i = ARRAY_SIZE(rtl_chip_info) - 1; i >= 0; i--) {
        if (tp->mcfg == rtl_chip_info[i].mcfg)
            break;
    }

    if (i < 0) {
        /* Unknown chip: assume array element #0, original RTL-8168 */
        if (netif_msg_probe(tp))
            dev_printk(KERN_DEBUG, &pdev->dev, "unknown chip version, assuming %s\n", rtl_chip_info[0].name);
        i++;
    }

    tp->chipset = i;

    *ioaddr_out = ioaddr;
    *dev_out = dev;
	*irq_out = irq_res;
out:
    return rc;

err_out_res:
		release_mem_region(addr_res->start, sizeof(struct rtl8168_private));
err_out_free:
		free_netdev(dev);	
err_out:
    *ioaddr_out = NULL;
    *dev_out = NULL;
	*irq_out = NULL;
    goto out;
}


static void
rtl8168_link_timer(unsigned long __opaque)
{
    struct net_device *dev = (struct net_device *)__opaque;
    struct rtl8168_private *tp = netdev_priv(dev);
    struct timer_list *timer = &tp->link_timer;
    unsigned long flags;

    spin_lock_irqsave(&tp->lock, flags);
    rtl8168_check_link_status(dev);
    spin_unlock_irqrestore(&tp->lock, flags);

    mod_timer(timer, jiffies + RTL8168_LINK_TIMEOUT);
}


static const struct net_device_ops rtl8168_netdev_ops = {
    .ndo_open       = rtl8168_open,
    .ndo_stop       = rtl8168_close,
    .ndo_get_stats      = rtl8168_get_stats,
    .ndo_start_xmit     = rtl8168_start_xmit,
    .ndo_tx_timeout     = rtl8168_tx_timeout,
    .ndo_change_mtu     = rtl8168_change_mtu,
    .ndo_set_mac_address    = rtl8168_set_mac_address,
    .ndo_do_ioctl       = rtl8168_do_ioctl,
    .ndo_set_rx_mode    = rtl8168_set_rx_mode,
    .ndo_fix_features   = rtl8168_fix_features,
    .ndo_set_features   = rtl8168_set_features,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller    = rtl8168_netpoll,
#endif
};


static void __devinit rtl8168_init_led(struct rtl8168_private *tp)
{
	struct platform_device *pdev = tp->platform_dev;
	struct rtl8168_platform_data *pdata;
	void __iomem *ioaddr = tp->mmio_addr;
	int i;
	u32 reg;

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		dev_info(&pdev->dev, "get invalid platform data, skip\n");
		return;
	}

	reg = 0;
	for (i = 0; i < 3; i++) {
		u32 bank = 0;
		struct customled_config *led = pdata->led_config + i;

		if (led->led_pin > 2 || led->led_pin < 0)
			continue;
		switch (led->link_mode) {
		case 10:
			bank |= LINK10M;
			break;
		case 100:
			bank |= LINK100M;
			break;
		case 1000:
			bank |= LINK1000M;
			break;
		default:
			break;
		}
		if (led->act_full)
			bank |= ACT_FULL;
		reg |= bank << (i * 4);
		if (led->act_high_active)
			reg |= BIT(12 + i);
	}
	RTL_W16(CustomLED, reg);
}

static int __devinit
rtl8168_init_one(struct platform_device *pdev)
{
    struct net_device *dev = NULL;
    struct rtl8168_private *tp;
    void __iomem *ioaddr = NULL;
    static int board_idx = -1;
	struct resource *irq_res = NULL;

    int rc;

    assert(pdev != NULL);
    assert(ent != NULL);
    board_idx++;

    if (netif_msg_drv(&debug))
        printk(KERN_INFO "%s Gigabit Ethernet driver %s loaded\n",
               MODULENAME, RTL8168_VERSION);

    rc = rtl8168_init_board(pdev, &dev, &ioaddr, &irq_res);
    if (rc)
        return rc;
    tp = netdev_priv(dev);
    assert(ioaddr != NULL);

    tp->mmio_addr = ioaddr;
    tp->set_speed = rtl8168_set_speed_xmii;
    tp->get_settings = rtl8168_gset_xmii;
    tp->phy_reset_enable = rtl8168_xmii_reset_enable;
    tp->phy_reset_pending = rtl8168_xmii_reset_pending;
    tp->link_ok = rtl8168_xmii_link_ok;


    RTL_NET_DEVICE_OPS(rtl8168_netdev_ops);

    SET_ETHTOOL_OPS(dev, &rtl8168_ethtool_ops);

    dev->watchdog_timeo = RTL8168_TX_TIMEOUT;
    dev->irq = irq_res->start;
    dev->base_addr = (unsigned long) ioaddr;

#ifdef CONFIG_R8168_NAPI
    RTL_NAPI_CONFIG(dev, tp, rtl8168_poll, R8168_NAPI_WEIGHT);
#endif

#ifdef CONFIG_R8168_VLAN
    if (tp->mcfg != CFG_METHOD_DEFAULT) 
        dev->features |= NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX;
#endif

    tp->cp_cmd |= RTL_R16(CPlusCmd);
    if (tp->mcfg != CFG_METHOD_DEFAULT) {
        dev->features |= NETIF_F_IP_CSUM;
        dev->features |= NETIF_F_RXCSUM | NETIF_F_SG;
        dev->hw_features = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_TSO |
                           NETIF_F_RXCSUM | NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX;
        dev->vlan_features = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_TSO |
                             NETIF_F_HIGHDMA;
    }

    tp->platform_dev = pdev;

    spin_lock_init(&tp->lock);

    spin_lock_init(&tp->phy_lock);

    rtl8168_init_software_variable(dev);

#ifdef ENABLE_DASH_SUPPORT
    if(tp->DASH)
        AllocateDashShareMemory(dev);
#endif

    rtl8168_exit_oob(dev);

    rtl8168_hw_init(dev);

    rtl8168_hw_reset(dev);

    /* Get production from EEPROM */
    if ((tp->mcfg == CFG_METHOD_21 ) && (mac_ocp_read(tp, 0xDC00) & BIT_3))
        tp->eeprom_type = EEPROM_TYPE_NONE;
    else
        rtl_eeprom_type(tp);

    if (tp->eeprom_type == EEPROM_TYPE_93C46 || tp->eeprom_type == EEPROM_TYPE_93C56)
        rtl_set_eeprom_sel_low(ioaddr);

    rtl8168_get_mac_address(dev);

    platform_set_drvdata(pdev, dev);

    if (netif_msg_probe(tp)) {
        printk(KERN_INFO "%s: 0x%lx, "
               "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x, "
               "IRQ %d\n",
               dev->name,
               dev->base_addr,
               dev->dev_addr[0], dev->dev_addr[1],
               dev->dev_addr[2], dev->dev_addr[3],
               dev->dev_addr[4], dev->dev_addr[5], dev->irq);
    }

    rtl8168_link_option(board_idx, (u8*)&autoneg, (u16*)&speed, (u8*)&duplex);

	rtl8168_init_led(tp);

    rc = register_netdev(dev);
    if (rc) {
        rtl8168_release_board(pdev, dev, ioaddr);
        return rc;
    }

    //printk(KERN_INFO "%s: This product is covered by one or more of the following patents: US6,570,884, US6,115,776, and US6,327,625.\n", MODULENAME);

    netif_carrier_off(dev);

    //printk("%s", GPL_CLAIM);

    return 0;
}

static int __devexit
rtl8168_remove_one(struct platform_device *pdev)
{
    struct net_device *dev = platform_get_drvdata(pdev);
    struct rtl8168_private *tp = netdev_priv(dev);

    assert(dev != NULL);
    assert(tp != NULL);


    flush_scheduled_work();

    unregister_netdev(dev);
    rtl8168_release_board(pdev, dev, tp->mmio_addr);
    platform_set_drvdata(pdev, NULL);

	return 0;
}

static void
rtl8168_set_rxbufsize(struct rtl8168_private *tp,
                      struct net_device *dev)
{
    unsigned int mtu = dev->mtu;

    tp->rx_buf_sz = (mtu > ETH_DATA_LEN) ? mtu + ETH_HLEN + 8 + 1 : RX_BUF_SIZE;
}

static int rtl8168_open(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    int retval;

    tp->in_open_fun = TRUE;

    retval = -ENOMEM;

    rtl8168_set_rxbufsize(tp, dev);
    /*
     * Rx and Tx descriptors needs 256 bytes alignment.
     * pci_alloc_consistent provides more.
     */
    tp->TxDescArray = dma_alloc_coherent(&tp->platform_dev->dev, R8168_TX_RING_BYTES,
                                           &tp->TxPhyAddr, GFP_KERNEL);
    if (!tp->TxDescArray)
        goto out;

    tp->RxDescArray = dma_alloc_coherent(&tp->platform_dev->dev, R8168_RX_RING_BYTES,
                                           &tp->RxPhyAddr, GFP_KERNEL);
    if (!tp->RxDescArray)
        goto err_free_tx;

    tp->tally_vaddr = dma_alloc_coherent(&tp->platform_dev->dev, sizeof(*tp->tally_vaddr), &tp->tally_paddr, GFP_KERNEL);
    if (!tp->tally_vaddr)
        goto err_free_rx;

    retval = rtl8168_init_ring(dev);
    if (retval < 0)
        goto err_free_counters;

    INIT_DELAYED_WORK(&tp->task, NULL);

#ifdef  CONFIG_R8168_NAPI
    RTL_NAPI_ENABLE(dev, &tp->napi);
#endif

    rtl8168_exit_oob(dev);

    rtl8168_tally_counter_clear(tp);

    rtl8168_hw_init(dev);

    rtl8168_hw_reset(dev);

    rtl8168_powerup_pll(dev);

    rtl8168_hw_ephy_config(dev);

    rtl8168_hw_phy_config(dev);

    rtl8168_hw_start(dev);

    rtl8168_set_speed(dev, autoneg, speed, duplex);

    retval = request_irq(dev->irq, rtl8168_interrupt, (tp->features & RTL_FEATURE_MSI) ? 0 : SA_SHIRQ, dev->name, dev);
    if (retval<0)
        goto err_free_counters;

    rtl8168_request_link_timer(dev);

out:
    tp->in_open_fun = FALSE;

    return retval;

err_free_counters:
    dma_free_coherent(&tp->platform_dev->dev, sizeof(*tp->tally_vaddr), tp->tally_vaddr, tp->tally_paddr);

    tp->tally_vaddr = NULL;
err_free_rx:
    dma_free_coherent(&tp->platform_dev->dev, R8168_RX_RING_BYTES, tp->RxDescArray,
                        tp->RxPhyAddr);
    tp->RxDescArray = NULL;
err_free_tx:
    dma_free_coherent(&tp->platform_dev->dev, R8168_TX_RING_BYTES, tp->TxDescArray,
                        tp->TxPhyAddr);
    tp->TxDescArray = NULL;
    goto out;
}


static void
rtl8168_hw_set_rx_packet_filter(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    u32 mc_filter[2];   /* Multicast hash filter */
    int rx_mode;
    u32 tmp = 0;

	if (dev->flags & IFF_PROMISC) {
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys | AcceptAllPhys;
	} else {
		/* Too many to filter perfectly -- accept all multicasts. */
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
	}
    mc_filter[1] = mc_filter[0] = 0xffffffff;
    tmp = mc_filter[0];
    mc_filter[0] = swab32(mc_filter[1]);
    mc_filter[1] = swab32(tmp);

    tp->rtl8168_rx_config = rtl_chip_info[tp->chipset].RCR_Cfg;
    tmp = tp->rtl8168_rx_config | rx_mode | (RTL_R32(RxConfig) & rtl_chip_info[tp->chipset].RxConfigMask);

    RTL_W32(RxConfig, tmp);
    RTL_W32(MAR0 + 0, mc_filter[0]);
    RTL_W32(MAR0 + 4, mc_filter[1]);
}

static void
rtl8168_set_rx_mode(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned long flags;

    spin_lock_irqsave(&tp->lock, flags);

    rtl8168_hw_set_rx_packet_filter(dev);

    spin_unlock_irqrestore(&tp->lock, flags);
}

static void
rtl8168_hw_start(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    //struct platform_device *pdev = tp->platform_dev;
    //u16 mac_ocp_data;
    u32 csi_tmp;
	int timeout;
    unsigned long flags;

    netif_stop_queue(dev);

    RTL_W32(RxConfig, (RX_DMA_BURST << RxCfgDMAShift));

    rtl8168_hw_reset(dev);

    rtl8168_rx_desc_offset0_init(tp, 1);

    RTL_W8(Cfg9346, Cfg9346_Unlock);
	
    RTL_W8(0xF1, RTL_R8(0xF1) & ~BIT_7);
    RTL_W8(Config2, RTL_R8(Config2) & ~BIT_7);
    RTL_W8(Config5, RTL_R8(Config5) & ~BIT_0);
 
    RTL_W8(MTPS, Reserved1_data);

    tp->cp_cmd |= INTT_1;
    if (tp->use_timer_interrrupt)
        tp->cp_cmd |= PktCntrDisable;
    else
        tp->cp_cmd &= ~PktCntrDisable;

    RTL_W16(IntrMitigate, 0x5f51);

    rtl8168_tally_counter_addr_fill(tp);

    rtl8168_desc_addr_fill(tp);

    /* Set DMA burst size and Interframe Gap Time */
    RTL_W32(TxConfig, (TX_DMA_BURST_128 << TxDMAShift) |
           (InterFrameGap << TxInterFrameGapShift));

    if (tp->mcfg == CFG_METHOD_18) {
       //? set_offset70F(tp, 0x17);
       //?  set_offset79(tp, 0x50);

        rtl8168_eri_write(ioaddr, 0xC8, 4, 0x00100002, ERIAR_ExGMAC);
        rtl8168_eri_write(ioaddr, 0xE8, 4, 0x00100006, ERIAR_ExGMAC);
        RTL_W32(TxConfig, RTL_R32(TxConfig) | BIT_7);
        RTL_W8(0xD3, RTL_R8(0xD3) & ~BIT_7);
        csi_tmp = rtl8168_eri_read(ioaddr, 0xDC, 1, ERIAR_ExGMAC);
        csi_tmp &= ~BIT_0;
        rtl8168_eri_write(ioaddr, 0xDC, 1, csi_tmp, ERIAR_ExGMAC);
        csi_tmp |= BIT_0;
        rtl8168_eri_write(ioaddr, 0xDC, 1, csi_tmp, ERIAR_ExGMAC);

        if (aspm)
            RTL_W8(0xF1, RTL_R8(0xF1) | BIT_7);

        if (dev->mtu > ETH_DATA_LEN) {
            RTL_W8(MTPS, 0x27);

            /* tx checksum offload disable */
            dev->features &= ~NETIF_F_IP_CSUM;
        } else {
            /* tx checksum offload enable */
            dev->features |= NETIF_F_IP_CSUM;
        }

        RTL_W8(TDFNR, 0x8);

        RTL_W8(0xD0, RTL_R8(0xD0) | BIT_6);
        RTL_W8(0xF2, RTL_R8(0xF2) | BIT_6);

        rtl8168_eri_write(ioaddr, 0xC0, 2, 0x0000, ERIAR_ExGMAC);
        rtl8168_eri_write(ioaddr, 0xB8, 4, 0x00000000, ERIAR_ExGMAC);
        csi_tmp = rtl8168_eri_read(ioaddr, 0xD5, 1, ERIAR_ExGMAC);
        csi_tmp |= BIT_3 | BIT_2;
        rtl8168_eri_write(ioaddr, 0xD5, 1, csi_tmp, ERIAR_ExGMAC);
        RTL_W8(0x1B,RTL_R8(0x1B) & ~0x07);

        csi_tmp = rtl8168_eri_read(ioaddr, 0x1B0, 1, ERIAR_ExGMAC);
        csi_tmp |= BIT_4;
        rtl8168_eri_write(ioaddr, 0x1B0, 1, csi_tmp, ERIAR_ExGMAC);
        csi_tmp = rtl8168_eri_read(ioaddr, 0x1d0, 1, ERIAR_ExGMAC);
        csi_tmp |= BIT_4 | BIT_1;
        rtl8168_eri_write(ioaddr, 0x1d0, 1, csi_tmp, ERIAR_ExGMAC);
        rtl8168_eri_write(ioaddr, 0xCC, 4, 0x00000050, ERIAR_ExGMAC);
        rtl8168_eri_write(ioaddr, 0xd0, 4, 0x00000060, ERIAR_ExGMAC);
    } else if (tp->mcfg == CFG_METHOD_25) {
		mdio_write(tp , 0x1f, 0);
		mdio_read(tp, 1);

		csi_tmp = mdio_read(tp, 1);

		RTL_W8(Config0, 0);
		RTL_W8(Config1, 0);
		RTL_W8(Config2, 0x3d);
		RTL_W8(Config3, 0x26);
		RTL_W8(Config4, 0);
		RTL_W8(Config5, 0x02);

		RTL_W8(TDFNR, 0x4);

		tp->cp_cmd = 0x0c61;

		rtl8168_eri_write(ioaddr, 0xC8, 4, 0x00080002, ERIAR_ExGMAC);
		rtl8168_eri_write(ioaddr, 0xCC, 1, 0x38, ERIAR_ExGMAC);
		rtl8168_eri_write(ioaddr, 0xD0, 1, 0x48, ERIAR_ExGMAC);
		rtl8168_eri_write(ioaddr, 0xE8, 4, 0x00100006, ERIAR_ExGMAC);


		csi_tmp = rtl8168_eri_read(ioaddr, 0xDC, 4, ERIAR_ExGMAC);
		csi_tmp &= 0xFFFF0000;	
		csi_tmp |= 0x0d;

		rtl8168_eri_write(ioaddr, 0xDC, 4, csi_tmp, ERIAR_ExGMAC);
		RTL_W32(TxConfig, RTL_R32(TxConfig) | BIT_7);
		while (!(RTL_R32(OOBCTL) & 0x02000000));
	}
	/* csum offload command for RTL8168C/8111C and RTL8168CP/8111CP */
	tp->tx_tcp_csum_cmd = TxIPCS_C | TxTCPCS_C;
	tp->tx_udp_csum_cmd = TxIPCS_C | TxUDPCS_C;
	tp->tx_ip_csum_cmd = TxIPCS_C;

    //other hw parameters
    if (tp->mcfg == CFG_METHOD_21)
        rtl8168_eri_write(ioaddr, 0x2F8, 2, 0x1D8F, ERIAR_ExGMAC);


    if (tp->mcfg == CFG_METHOD_18) {
            u32 gphy_val;

            spin_lock_irqsave(&tp->phy_lock, flags);
            mdio_write(tp, 0x1F, 0x0007);
            mdio_write(tp, 0x1E, 0x002C);
            gphy_val = mdio_read(tp, 0x16);
            gphy_val |= BIT_10;
            mdio_write(tp, 0x16, gphy_val);
            mdio_write(tp, 0x1F, 0x0005);
            mdio_write(tp, 0x05, 0x8B80);
            gphy_val = mdio_read(tp, 0x06);
            gphy_val |= BIT_7;
            mdio_write(tp, 0x06, gphy_val);
            mdio_write(tp, 0x1F, 0x0000);
            spin_unlock_irqrestore(&tp->phy_lock, flags);
        }

    tp->cp_cmd &= ~(EnableBist | Macdbgo_oe | Force_halfdup |
                    Force_rxflow_en | Force_txflow_en | Cxpl_dbg_sel |
                    ASF | Macdbgo_sel);

    RTL_W16(CPlusCmd, tp->cp_cmd);

    for (timeout = 0; timeout < 10; timeout++) {
        if ((rtl8168_eri_read(ioaddr, 0x1AE, 2, ERIAR_ExGMAC) & BIT_13)==0)
            break;
        mdelay(1);
    }
    

    RTL_W16(RxMaxSize, tp->rx_buf_sz);

    rtl8168_disable_rxdvgate(dev);

    if (tp->mcfg == CFG_METHOD_11 || tp->mcfg == CFG_METHOD_12)
        rtl8168_mac_loopback_test(tp);

    /* Set Rx packet filter */
    rtl8168_hw_set_rx_packet_filter(dev);

#ifdef ENABLE_DASH_SUPPORT
    if (tp->DASH)
        NICChkTypeEnableDashInterrupt(tp);
#endif

    
	if (aspm) {
		RTL_W8(Config5, RTL_R8(Config5) | BIT_0);
		RTL_W8(Config2, RTL_R8(Config2) | BIT_7);
	} else {
		RTL_W8(Config2, RTL_R8(Config2) & ~BIT_7);
		RTL_W8(Config5, RTL_R8(Config5) & ~BIT_0);
	}

	if (dev->flags & IFF_PROMISC)
		RTL_W32(RxConfig, 0x80e | AcceptAllPhys);
	else
		RTL_W32(RxConfig, 0x80e);
    RTL_W8(Cfg9346, Cfg9346_Lock);

    if (!tp->in_open_fun) {
        RTL_W8(ChipCmd, CmdTxEnb | CmdRxEnb);

        if (tp->rx_fifo_overflow == 0) {
            /* Enable all known interrupts by setting the interrupt mask. */
            rtl8168_enable_hw_interrupt(tp, ioaddr);
            netif_start_queue(dev);
        }
    }

    udelay(10);
}


static int
rtl8168_change_mtu(struct net_device *dev,
                   int new_mtu)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    int max_mtu;
    int ret = 0;
    unsigned long flags;

    if (tp->mcfg == CFG_METHOD_DEFAULT)
        max_mtu = ETH_DATA_LEN;
    else
        max_mtu = tp->max_jumbo_frame_size - ETH_HLEN - 8;

    if (new_mtu < ETH_ZLEN)
        return -EINVAL;
    else if (new_mtu > max_mtu)
        new_mtu = max_mtu;

    if (!netif_running(dev))
        goto out;

    rtl8168_down(dev);

    spin_lock_irqsave(&tp->lock, flags);

    dev->mtu = new_mtu;

    rtl8168_set_rxbufsize(tp, dev);

    ret = rtl8168_init_ring(dev);

    if (ret < 0) {
        spin_unlock_irqrestore(&tp->lock, flags);
        goto out;
    }

#ifdef CONFIG_R8168_NAPI
    RTL_NAPI_ENABLE(dev, &tp->napi);
#endif//CONFIG_R8168_NAPI		

    rtl8168_hw_start(dev);
    spin_unlock_irqrestore(&tp->lock, flags);
    rtl8168_set_speed(dev, tp->autoneg, tp->speed, tp->duplex);

    mod_timer(&tp->link_timer, jiffies + RTL8168_LINK_TIMEOUT);

out:
    return ret;
}

static inline void
rtl8168_make_unusable_by_asic(struct RxDesc *desc)
{
    desc->addr = 0x0badbadbadbadbadull;
    desc->opts1 &= ~cpu_to_le32(DescOwn | RsvdMask);
}

static void
rtl8168_free_rx_skb(struct rtl8168_private *tp,
                    struct sk_buff **sk_buff,
                    struct RxDesc *desc)
{

    dma_unmap_single(&tp->platform_dev->dev, le64_to_cpu(desc->addr), tp->rx_buf_sz,
                     DMA_FROM_DEVICE);
    dev_kfree_skb(*sk_buff);
    *sk_buff = NULL;
    rtl8168_make_unusable_by_asic(desc);
}

static inline void
rtl8168_mark_to_asic(struct RxDesc *desc,
                     u32 rx_buf_sz)
{
    u32 eor = le32_to_cpu(desc->opts1) & RingEnd;

    desc->opts1 = cpu_to_le32(DescOwn | eor | rx_buf_sz);
}

static inline void
rtl8168_map_to_asic(struct RxDesc *desc,
                    dma_addr_t mapping,
                    u32 rx_buf_sz)
{
    desc->addr = cpu_to_le64(mapping);
    wmb();
    rtl8168_mark_to_asic(desc, rx_buf_sz);
}

static int
rtl8168_alloc_rx_skb(struct platform_device *pdev,
                     struct sk_buff **sk_buff,
                     struct RxDesc *desc,
                     int rx_buf_sz)
{
    struct sk_buff *skb;
    dma_addr_t mapping;
    int ret = 0;

	skb = alloc_skb(rx_buf_sz + RTK_RX_ALIGN + NET_SKB_PAD, GFP_ATOMIC);
    if (!skb)
        goto err_out;

    skb_reserve(skb, RTK_RX_ALIGN);
    *sk_buff = skb;

    mapping = dma_map_single(&pdev->dev, skb->data, rx_buf_sz,
                             DMA_FROM_DEVICE);

    rtl8168_map_to_asic(desc, mapping, rx_buf_sz);

out:
    return ret;

err_out:
    ret = -ENOMEM;
    rtl8168_make_unusable_by_asic(desc);
    goto out;
}

static void
rtl8168_rx_clear(struct rtl8168_private *tp)
{
    int i;

    for (i = 0; i < NUM_RX_DESC; i++) {
        if (tp->Rx_skbuff[i])
            rtl8168_free_rx_skb(tp, tp->Rx_skbuff + i,
                                tp->RxDescArray + i);
    }
}

static u32
rtl8168_rx_fill(struct rtl8168_private *tp,
                struct net_device *dev,
                u32 start,
                u32 end)
{
    u32 cur;

    for (cur = start; end - cur > 0; cur++) {
        int ret, i = cur % NUM_RX_DESC;

        if (tp->Rx_skbuff[i])
            continue;

        ret = rtl8168_alloc_rx_skb(tp->platform_dev, tp->Rx_skbuff + i,
                                   tp->RxDescArray + i, tp->rx_buf_sz);
        if (ret < 0)
            break;
    }
    return cur - start;
}

static inline void
rtl8168_mark_as_last_descriptor(struct RxDesc *desc)
{
    desc->opts1 |= cpu_to_le32(RingEnd);
}

static void
rtl8168_desc_addr_fill(struct rtl8168_private *tp)
{
    void __iomem *ioaddr = tp->mmio_addr;

    if (!tp->TxPhyAddr || !tp->RxPhyAddr)
        return;

    RTL_W32(TxDescStartAddrLow, ((u64) tp->TxPhyAddr & DMA_BIT_MASK(32)));
	RTL_W32(TxDescStartAddrHigh, ((u64) tp->TxPhyAddr >> 32));
    RTL_W32(RxDescAddrLow, ((u64) tp->RxPhyAddr & DMA_BIT_MASK(32)));
    RTL_W32(RxDescAddrHigh, ((u64) tp->RxPhyAddr >> 32));
}

static void
rtl8168_tx_desc_init(struct rtl8168_private *tp)
{
    int i = 0;

    memset(tp->TxDescArray, 0x0, NUM_TX_DESC * sizeof(struct TxDesc));

    for (i = 0; i < NUM_TX_DESC; i++) {
        if (i == (NUM_TX_DESC - 1))
            tp->TxDescArray[i].opts1 = cpu_to_le32(RingEnd);
    }
}

static void
rtl8168_rx_desc_offset0_init(struct rtl8168_private *tp, int own)
{
    int i = 0;
    int ownbit = 0;

    if (own)
        ownbit = DescOwn;

    for (i = 0; i < NUM_RX_DESC; i++) {
        if (i == (NUM_RX_DESC - 1))
            tp->RxDescArray[i].opts1 = cpu_to_le32((ownbit | RingEnd) | (unsigned long)tp->rx_buf_sz);
        else
            tp->RxDescArray[i].opts1 = cpu_to_le32(ownbit | (unsigned long)tp->rx_buf_sz);
    }
}

static void
rtl8168_rx_desc_init(struct rtl8168_private *tp)
{
    memset(tp->RxDescArray, 0x0, NUM_RX_DESC * sizeof(struct RxDesc));
}

static int
rtl8168_init_ring(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    rtl8168_init_ring_indexes(tp);

    memset(tp->tx_skb, 0x0, NUM_TX_DESC * sizeof(struct ring_info));
    memset(tp->Rx_skbuff, 0x0, NUM_RX_DESC * sizeof(struct sk_buff *));

    rtl8168_tx_desc_init(tp);
    rtl8168_rx_desc_init(tp);

    if (rtl8168_rx_fill(tp, dev, 0, NUM_RX_DESC) != NUM_RX_DESC)
        goto err_out;

    rtl8168_mark_as_last_descriptor(tp->RxDescArray + NUM_RX_DESC - 1);

    return 0;

err_out:
    rtl8168_rx_clear(tp);
    return -ENOMEM;
}

static void
rtl8168_unmap_tx_skb(struct platform_device *pdev,
                     struct ring_info *tx_skb,
                     struct TxDesc *desc)
{
    unsigned int len = tx_skb->len;

    dma_unmap_single(&pdev->dev, le64_to_cpu(desc->addr), len, DMA_TO_DEVICE);
    desc->opts1 = 0x00;
    desc->opts2 = 0x00;
    desc->addr = 0x00;
    tx_skb->len = 0;
}

static void
rtl8168_tx_clear(struct rtl8168_private *tp)
{
    unsigned int i;
    struct net_device *dev = tp->dev;


    for (i = tp->dirty_tx; i < tp->dirty_tx + NUM_TX_DESC; i++) {
        unsigned int entry = i % NUM_TX_DESC;
        struct ring_info *tx_skb = tp->tx_skb + entry;
        unsigned int len = tx_skb->len;

        if (len) {
            struct sk_buff *skb = tx_skb->skb;

            rtl8168_unmap_tx_skb(tp->platform_dev, tx_skb,
                                 tp->TxDescArray + entry);
            if (skb) {
                dev_kfree_skb(skb);
                tx_skb->skb = NULL;
            }
            RTLDEV->stats.tx_dropped++;
        }
    }
    tp->cur_tx = tp->dirty_tx = 0;
}

static void rtl8168_schedule_work(struct net_device *dev, work_func_t task)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    PREPARE_DELAYED_WORK(&tp->task, task);
    schedule_delayed_work(&tp->task, 4);
}


static void
rtl8168_wait_for_quiescence(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

    synchronize_irq(dev->irq);

    /* Wait for any pending NAPI task to complete */
#ifdef CONFIG_R8168_NAPI
    RTL_NAPI_DISABLE(dev, &tp->napi);
#endif//CONFIG_R8168_NAPI

    rtl8168_irq_mask_and_ack(tp, ioaddr);

#ifdef CONFIG_R8168_NAPI
    RTL_NAPI_ENABLE(dev, &tp->napi);
#endif//CONFIG_R8168_NAPI
}


static void rtl8168_reset_task(struct work_struct *work)
{
    struct rtl8168_private *tp =
        container_of(work, struct rtl8168_private, task.work);
    struct net_device *dev = tp->dev;
    unsigned long flags;

    if (!netif_running(dev))
        return;

    rtl8168_wait_for_quiescence(dev);

    rtl8168_rx_interrupt(dev, tp, tp->mmio_addr, ~(u32)0);

    spin_lock_irqsave(&tp->lock, flags);

    rtl8168_tx_clear(tp);

    if (tp->dirty_rx == tp->cur_rx) {
        rtl8168_init_ring_indexes(tp);
        rtl8168_hw_start(dev);
        rtl8168_set_speed(dev, tp->autoneg, tp->speed, tp->duplex);
        netif_wake_queue(dev);
        spin_unlock_irqrestore(&tp->lock, flags);
    } else {
        spin_unlock_irqrestore(&tp->lock, flags);
        if (net_ratelimit()) {
            struct rtl8168_private *tp = netdev_priv(dev);

            if (netif_msg_intr(tp)) {
                printk(PFX KERN_EMERG
                       "%s: Rx buffers shortage\n", dev->name);
            }
        }
        rtl8168_schedule_work(dev, rtl8168_reset_task);
    }
}

static void
rtl8168_tx_timeout(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned long flags;

    spin_lock_irqsave(&tp->lock, flags);
    rtl8168_hw_reset(dev);
    spin_unlock_irqrestore(&tp->lock, flags);

    /* Let's wait a bit while any (async) irq lands on */
    rtl8168_schedule_work(dev, rtl8168_reset_task);
}

static int
rtl8168_xmit_frags(struct rtl8168_private *tp,
                   struct sk_buff *skb,
                   u32 opts1,
                   u32 opts2)
{
    struct skb_shared_info *info = skb_shinfo(skb);
    unsigned int cur_frag, entry;
    struct TxDesc *txd = NULL;

    entry = tp->cur_tx;
    for (cur_frag = 0; cur_frag < info->nr_frags; cur_frag++) {
        skb_frag_t *frag = info->frags + cur_frag;
        dma_addr_t mapping;
        u32 status, len;
        void *addr;

        entry = (entry + 1) % NUM_TX_DESC;

        txd = tp->TxDescArray + entry;
        len = skb_frag_size(frag);
        addr = skb_frag_address(frag);
        mapping = dma_map_single(&tp->platform_dev->dev, addr, len, DMA_TO_DEVICE);

        /* anti gcc 2.95.3 bugware (sic) */
        status = opts1 | len | (RingEnd * !((entry + 1) % NUM_TX_DESC));

        txd->addr = cpu_to_le64(mapping);

        tp->tx_skb[entry].len = len;

        txd->opts1 = cpu_to_le32(status);
        txd->opts2 = cpu_to_le32(opts2);
    }

    if (cur_frag) {
        tp->tx_skb[entry].skb = skb;
        wmb();
        txd->opts1 |= cpu_to_le32(LastFrag);
    }

    return cur_frag;
}

static inline u32
rtl8168_tx_csum(struct sk_buff *skb,
                struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    const struct iphdr *ip = ip_hdr(skb);

    if (skb->ip_summed == CHECKSUM_PARTIAL) {
        if (ip->protocol == IPPROTO_TCP)
            return tp->tx_tcp_csum_cmd;
        else if (ip->protocol == IPPROTO_UDP)
            return tp->tx_udp_csum_cmd;
        else if (ip->protocol == IPPROTO_IP)
            return tp->tx_ip_csum_cmd;

        WARN_ON(1); /* we need a WARN() */

    }

    return 0;
}

static int
rtl8168_start_xmit(struct sk_buff *skb,
                   struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned int frags, entry = tp->cur_tx % NUM_TX_DESC;
    struct TxDesc *txd = tp->TxDescArray + entry;
    void __iomem *ioaddr = tp->mmio_addr;
    dma_addr_t mapping;
    u32 len;
    u32 opts1;
    u32 opts2;
    int ret = NETDEV_TX_OK;
    unsigned long flags, large_send;

    spin_lock_irqsave(&tp->lock, flags);

    //Work around for rx fifo overflow
    if (tp->rx_fifo_overflow == 1){
		goto err_stop;
	}

    if (unlikely(TX_BUFFS_AVAIL(tp) < skb_shinfo(skb)->nr_frags)) {
        if (netif_msg_drv(tp)) {
            printk(KERN_ERR
                   "%s: BUG! Tx Ring full when queue awake!\n",
                   dev->name);
        }
        goto err_stop;
    }

    if (unlikely(le32_to_cpu(txd->opts1) & DescOwn))
        goto err_stop;

    opts1 = DescOwn;
    opts2 = rtl8168_tx_vlan_tag(tp, skb);

    large_send = 0;
	
    if (dev->features & NETIF_F_TSO) {
        u32 mss = skb_shinfo(skb)->gso_size;

        /* TCP Segmentation Offload (or TCP Large Send) */
        if (mss) {
            if ((tp->mcfg == CFG_METHOD_1) ||
                (tp->mcfg == CFG_METHOD_2) ||
                (tp->mcfg == CFG_METHOD_3)) {
                opts1 |= LargeSend | ((mss & MSSMask) << 16);
            } else if ((tp->mcfg == CFG_METHOD_11) ||
                       (tp->mcfg == CFG_METHOD_12) ||
                       (tp->mcfg == CFG_METHOD_13)) {
                opts2 |= LargeSend_DP | ((mss & MSSMask) << 18);
            } else {
                opts1 |= LargeSend;
                opts2 |= (mss & MSSMask) << 18;
            }
            large_send = 1;
        }
    }

    if (large_send == 0) {
        if (dev->features & NETIF_F_IP_CSUM) {
            if ((tp->mcfg == CFG_METHOD_1) || (tp->mcfg == CFG_METHOD_2) || (tp->mcfg == CFG_METHOD_3))
                opts1 |= rtl8168_tx_csum(skb, dev);
            else
                opts2 |= rtl8168_tx_csum(skb, dev);
        }
    }
    frags = rtl8168_xmit_frags(tp, skb, opts1, opts2);
    if (frags) {
        len = skb_headlen(skb);
        opts1 |= FirstFrag;
    } else {
        len = skb->len;

        if ((tp->mcfg == CFG_METHOD_16|| tp->mcfg == CFG_METHOD_17)&& len < 60) {
            if (opts2 & 0xE0000000) {
                skb_checksum_help(skb);

                opts2 &= ~0xE0000000;
            }
            len = 60;
        }
        opts1 |= FirstFrag | LastFrag;

        tp->tx_skb[entry].skb = skb;
    }

    opts1 |= len | (RingEnd * !((entry + 1) % NUM_TX_DESC));
    mapping = dma_map_single(&tp->platform_dev->dev, skb->data, len, DMA_TO_DEVICE);
    tp->tx_skb[entry].len = len;
    txd->addr = cpu_to_le64(mapping);
    txd->opts2 = cpu_to_le32(opts2);
    txd->opts1 = cpu_to_le32(opts1&~DescOwn);
    wmb();
    txd->opts1 = cpu_to_le32(opts1);

    dev->trans_start = jiffies;

    tp->cur_tx += frags + 1;
    wmb();

    RTL_W8(TxPoll, NPQ);    /* set polling bit */
    if (TX_BUFFS_AVAIL(tp) < MAX_SKB_FRAGS) {
        netif_stop_queue(dev);
        smp_rmb();
        if (TX_BUFFS_AVAIL(tp) >= MAX_SKB_FRAGS)
            netif_wake_queue(dev);
    }

    spin_unlock_irqrestore(&tp->lock, flags);
out:
    return ret;
err_stop:
    netif_stop_queue(dev);
    ret = NETDEV_TX_BUSY;
    RTLDEV->stats.tx_dropped++;

    spin_unlock_irqrestore(&tp->lock, flags);
    goto out;
}

static void
rtl8168_tx_interrupt(struct net_device *dev,
                     struct rtl8168_private *tp,
                     void __iomem *ioaddr)
{
    unsigned int dirty_tx, tx_left;

    assert(dev != NULL);
    assert(tp != NULL);
    assert(ioaddr != NULL);

    dirty_tx = tp->dirty_tx;
    smp_rmb();
    tx_left = tp->cur_tx - dirty_tx;
    while (tx_left > 0) {
        unsigned int entry = dirty_tx % NUM_TX_DESC;
        struct ring_info *tx_skb = tp->tx_skb + entry;
        u32 len = tx_skb->len;
        u32 status;

        rmb();
        status = le32_to_cpu(tp->TxDescArray[entry].opts1);
        if (status & DescOwn)
            break;

        RTLDEV->stats.tx_bytes += len;
        RTLDEV->stats.tx_packets++;

        rtl8168_unmap_tx_skb(tp->platform_dev,
                             tx_skb,
                             tp->TxDescArray + entry);

        if (tx_skb->skb!=NULL) {
            dev_kfree_skb_irq(tx_skb->skb);
            tx_skb->skb = NULL;
        }
        dirty_tx++;
        tx_left--;
    }

    if (tp->dirty_tx != dirty_tx) {
        tp->dirty_tx = dirty_tx;
        smp_wmb();
        if (netif_queue_stopped(dev) &&
            (TX_BUFFS_AVAIL(tp) >= MAX_SKB_FRAGS)) {
            netif_wake_queue(dev);
        }
        smp_rmb();
        if (tp->cur_tx != dirty_tx)
            RTL_W8(TxPoll, NPQ);
    }
}

static inline int
rtl8168_fragmented_frame(u32 status)
{
    return (status & (FirstFrag | LastFrag)) != (FirstFrag | LastFrag);
}

static inline void
rtl8168_rx_csum(struct rtl8168_private *tp,
                struct sk_buff *skb,
                struct RxDesc *desc)
{
    u32 opts1 = le32_to_cpu(desc->opts1);
    u32 opts2 = le32_to_cpu(desc->opts2);
    u32 status = opts1 & RxProtoMask;

    if ((tp->mcfg == CFG_METHOD_1) ||
        (tp->mcfg == CFG_METHOD_2) ||
        (tp->mcfg == CFG_METHOD_3)) {
        /* rx csum offload for RTL8168B/8111B */
        if (((status == RxProtoTCP) && !(opts1 & RxTCPF)) ||
            ((status == RxProtoUDP) && !(opts1 & RxUDPF)) ||
            ((status == RxProtoIP) && !(opts1 & RxIPF)))
            skb->ip_summed = CHECKSUM_UNNECESSARY;
        else
            skb->ip_summed = CHECKSUM_NONE;
    } else {
        /* rx csum offload for RTL8168C/8111C and RTL8168CP/8111CP */
        if (((status == RxTCPT) && !(opts1 & RxTCPF)) ||
            ((status == RxUDPT) && !(opts1 & RxUDPF)) ||
            ((status == 0) && (opts2 & RxV4F) && !(opts1 & RxIPF)))
            skb->ip_summed = CHECKSUM_UNNECESSARY;
        else
            skb->ip_summed = CHECKSUM_NONE;
    }
}

static inline int
rtl8168_try_rx_copy(struct sk_buff **sk_buff,
                    int pkt_size,
                    struct RxDesc *desc,
                    int rx_buf_sz)
{
    int ret = -1;
/*
   if (pkt_size < rx_copybreak) {
        struct sk_buff *skb;

        skb = dev_alloc_skb(pkt_size + RTK_RX_ALIGN);
        if (skb) {
            skb_reserve(skb, RTK_RX_ALIGN);
            eth_copy_and_sum(skb, sk_buff[0]->data, pkt_size, 0);
            *sk_buff = skb;
            rtl8168_mark_to_asic(desc, rx_buf_sz);
            ret = 0;
        }
    }
*/
    return ret;
}

static inline void
rtl8168_rx_skb(struct rtl8168_private *tp,
               struct sk_buff *skb)
{
#ifdef CONFIG_R8168_NAPI
    napi_gro_receive(&tp->napi, skb);
#else
    netif_rx(skb);
#endif
}

static int
rtl8168_rx_interrupt(struct net_device *dev,
                     struct rtl8168_private *tp,
                     void __iomem *ioaddr, u32 budget)
{
    unsigned int cur_rx, rx_left;
    unsigned int delta, count = 0;
    u32 rx_quota = RTL_RX_QUOTA(dev, budget);

    assert(dev != NULL);
    assert(tp != NULL);
    assert(ioaddr != NULL);

    cur_rx = tp->cur_rx;
    rx_left = NUM_RX_DESC + tp->dirty_rx - cur_rx;
    rx_left = rtl8168_rx_quota(rx_left, (u32) rx_quota);

    if ((tp->RxDescArray == NULL) || (tp->Rx_skbuff == NULL))
        goto rx_out;

    for (; rx_left > 0; rx_left--, cur_rx++) {
        unsigned int entry = cur_rx % NUM_RX_DESC;
        struct RxDesc *desc = tp->RxDescArray + entry;
        u32 status;

        rmb();
        status = le32_to_cpu(desc->opts1);

        if (status & DescOwn)
            break;
        if (unlikely(status & RxRES)) {
            if (netif_msg_rx_err(tp)) {
                printk(KERN_INFO
                       "%s: Rx ERROR. status = %08x\n",
                       dev->name, status);
            }

            RTLDEV->stats.rx_errors++;

            if (status & (RxRWT | RxRUNT))
                RTLDEV->stats.rx_length_errors++;
            if (status & RxCRC)
                RTLDEV->stats.rx_crc_errors++;
            rtl8168_mark_to_asic(desc, tp->rx_buf_sz);
        } else {
            struct sk_buff *skb = tp->Rx_skbuff[entry];
            int pkt_size = (status & 0x00003FFF) - 4;

       /*
             * The driver does not support incoming fragmented
             * frames. They are seen as a symptom of over-mtu
             * sized frames.
             */
            if (unlikely(rtl8168_fragmented_frame(status))) {
                RTLDEV->stats.rx_dropped++;
                RTLDEV->stats.rx_length_errors++;
                rtl8168_mark_to_asic(desc, tp->rx_buf_sz);
                continue;
            }

            if (tp->cp_cmd & RxChkSum)
                rtl8168_rx_csum(tp, skb, desc);

            dma_sync_single_for_cpu(&tp->platform_dev->dev,
                                        le64_to_cpu(desc->addr), tp->rx_buf_sz,
                                        DMA_FROM_DEVICE);

            if (rtl8168_try_rx_copy(&skb, pkt_size, desc,
                                    tp->rx_buf_sz)) {
                dma_unmap_single(&tp->platform_dev->dev, le64_to_cpu(desc->addr),
                       	tp->rx_buf_sz, DMA_FROM_DEVICE);
                tp->Rx_skbuff[entry] = NULL;
            }else{

            	dma_sync_single_for_device(&tp->platform_dev->dev, le64_to_cpu(desc->addr),
                       	tp->rx_buf_sz, DMA_FROM_DEVICE);
            }

            skb->dev = dev;
            skb_put(skb, pkt_size);

#ifdef CONFIG_RTL8168_DMA_TEST
			rtl8168_start_xmit(skb, dev);
#else
            skb->protocol = eth_type_trans(skb, dev);
            if (rtl8168_rx_vlan_skb(tp, desc, skb) < 0)
                rtl8168_rx_skb(tp, skb);
#endif       
			dev->last_rx = jiffies;
            RTLDEV->stats.rx_bytes += pkt_size;
            RTLDEV->stats.rx_packets++;
        }
    }

    count = cur_rx - tp->cur_rx;
    tp->cur_rx = cur_rx;

    delta = rtl8168_rx_fill(tp, dev, tp->dirty_rx, tp->cur_rx);
    if (!delta && count && netif_msg_intr(tp))
        printk(KERN_INFO "%s: no Rx buffer allocated\n", dev->name);
    tp->dirty_rx += delta;

    /*
     * FIXME: until there is periodic timer to try and refill the ring,
     * a temporary shortage may definitely kill the Rx process.
     * - disable the asic to try and avoid an overflow and kick it again
     *   after refill ?
     * - how do others driver handle this condition (Uh oh...).
     */
    if ((tp->dirty_rx + NUM_RX_DESC == tp->cur_rx) && netif_msg_intr(tp))
        printk(KERN_EMERG "%s: Rx buffers exhausted\n", dev->name);

rx_out:
    return count;
}

/*
 *The interrupt handler does all of the Rx thread work and cleans up after
 *the Tx thread.
 */
static irqreturn_t rtl8168_interrupt(int irq, void *dev_instance)
{
    struct net_device *dev = (struct net_device *) dev_instance;
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;
    int status;
    int handled = 0;

    do {
        status = RTL_R16(IntrStatus);

        /* hotplug/major error/no more work/shared irq */
        if ((status == 0xFFFF) || !status)
            break;

        if (!(status & (tp->intr_mask | tp->timer_intr_mask)))
            break;

        if (unlikely(!netif_running(dev))) {
	    /*add for ifconfig down cause nobody cared interrupt*/
            RTL_W16(IntrStatus, status);
            handled = 1;
            break;
        }

        handled = 1;

        rtl8168_disable_hw_interrupt(tp, ioaddr);

        switch (tp->mcfg) {
        case CFG_METHOD_9:
        case CFG_METHOD_10:
        case CFG_METHOD_11:
        case CFG_METHOD_12:
        case CFG_METHOD_13:
        case CFG_METHOD_14:
        case CFG_METHOD_15:
        case CFG_METHOD_16:
        case CFG_METHOD_17:
        case CFG_METHOD_18:
        case CFG_METHOD_19:
        case CFG_METHOD_20:
        case CFG_METHOD_21:
        case CFG_METHOD_22:
        case CFG_METHOD_23:
        case CFG_METHOD_24:
        case CFG_METHOD_25:
        case CFG_METHOD_26:
        case CFG_METHOD_27:
        case CFG_METHOD_28:
            /* RX_OVERFLOW RE-START mechanism now HW handles it automatically*/
            RTL_W16(IntrStatus, status&~RxFIFOOver);
            break;
        default:
            RTL_W16(IntrStatus, status);
            break;
        }

        //Work around for rx fifo overflow
        if (unlikely(status & RxFIFOOver)) {
            if (tp->mcfg == CFG_METHOD_1) {
                tp->rx_fifo_overflow = 1;
                netif_stop_queue(dev);
                udelay(300);
                rtl8168_rx_clear(tp);
                rtl8168_init_ring(dev);
                rtl8168_hw_start(dev);
                RTL_W16(IntrStatus, RxFIFOOver);
                netif_wake_queue(dev);
                tp->rx_fifo_overflow = 0;
            }
        }

#ifdef ENABLE_DASH_SUPPORT
        if ( tp->DASH ) {
            if( HW_DASH_SUPPORT_TYPE_2( tp ) ) {
                u8 DashIntType2Status;

                DashIntType2Status = RTL_R8(IBISR0);
                if (DashIntType2Status & ISRIMR_DASH_TYPE2_ROK) {
                    tp->RcvFwDashOkEvt = TRUE;
                }
                if (DashIntType2Status & ISRIMR_DASH_TYPE2_TOK) {
                    tp->SendFwHostOkEvt = TRUE;
                }
                if(DashIntType2Status & ISRIMR_DASH_TYPE2_RX_DISABLE_IDLE) {
                    tp->DashFwDisableRx = TRUE;
                }

                RTL_W8(IBISR0, DashIntType2Status);

                //hau_dbg
                //printk("status = %X DashIntType2Status = %X.\n", status, DashIntType2Status);
            } else {
                if (IntrStatus & ISRIMR_DP_REQSYS_OK) {
                    tp->RcvFwReqSysOkEvt = TRUE;
                }
                if (IntrStatus & ISRIMR_DP_DASH_OK) {
                    tp->RcvFwDashOkEvt = TRUE;
                }
                if (IntrStatus & ISRIMR_DP_HOST_OK) {
                    tp->SendFwHostOkEvt = TRUE;
                }
            }
        }
#endif

#ifdef CONFIG_R8168_NAPI
        if (status & tp->intr_mask || tp->keep_intr_cnt > 0) {
            if (tp->keep_intr_cnt > 0) tp->keep_intr_cnt--;

            if (likely(RTL_NETIF_RX_SCHEDULE_PREP(dev, &tp->napi)))
                __RTL_NETIF_RX_SCHEDULE(dev, &tp->napi);
            else if (netif_msg_intr(tp))
                printk(KERN_INFO "%s: interrupt %04x in poll\n",
                       dev->name, status);
        } else {
            tp->keep_intr_cnt = RTK_KEEP_INTERRUPT_COUNT;
            rtl8168_switch_to_hw_interrupt(tp, ioaddr);
        }
#else
        if (status & tp->intr_mask || tp->keep_intr_cnt > 0) {
            if (tp->keep_intr_cnt > 0) tp->keep_intr_cnt--;

            rtl8168_rx_interrupt(dev, tp, tp->mmio_addr, ~(u32)0);
            rtl8168_tx_interrupt(dev, tp, ioaddr);

#ifdef ENABLE_DASH_SUPPORT
            if ( tp->DASH ) {
                struct net_device *dev = tp->dev;

                HandleDashInterrupt(dev);
            }
#endif

            rtl8168_switch_to_timer_interrupt(tp, ioaddr);
        } else {
            tp->keep_intr_cnt = RTK_KEEP_INTERRUPT_COUNT;
            rtl8168_switch_to_hw_interrupt(tp, ioaddr);
        }
#endif

    } while (false);

    return IRQ_RETVAL(handled);
}

#ifdef CONFIG_R8168_NAPI
static int rtl8168_poll(napi_ptr napi, napi_budget budget)
{
    struct rtl8168_private *tp = RTL_GET_PRIV(napi, struct rtl8168_private);
    void __iomem *ioaddr = tp->mmio_addr;
    RTL_GET_NETDEV(tp)
    unsigned int work_to_do = RTL_NAPI_QUOTA(budget, dev);
    unsigned int work_done;
    unsigned long flags;


    work_done = rtl8168_rx_interrupt(dev, tp, ioaddr, (u32) budget);

    spin_lock_irqsave(&tp->lock, flags);
    rtl8168_tx_interrupt(dev, tp, ioaddr);
    spin_unlock_irqrestore(&tp->lock, flags);

    RTL_NAPI_QUOTA_UPDATE(dev, work_done, budget);

    if (work_done < work_to_do) {
#ifdef ENABLE_DASH_SUPPORT
        if ( tp->DASH ) {
            struct net_device *dev = tp->dev;

            spin_lock_irqsave(&tp->lock, flags);
            HandleDashInterrupt(dev);
            spin_unlock_irqrestore(&tp->lock, flags);
        }
#endif

        RTL_NETIF_RX_COMPLETE(dev, napi);
        /*
         * 20040426: the barrier is not strictly required but the
         * behavior of the irq handler could be less predictable
         * without it. Btw, the lack of flush for the posted pci
         * write is safe - FR
         */
        smp_wmb();

        rtl8168_switch_to_timer_interrupt(tp, ioaddr);
    }

    return RTL_NAPI_RETURN_VALUE;
}
#endif//CONFIG_R8168_NAPI

static void rtl8168_sleep_rx_enable(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    void __iomem *ioaddr = tp->mmio_addr;

    if ((tp->mcfg == CFG_METHOD_1) || (tp->mcfg == CFG_METHOD_2)) {
        RTL_W8(ChipCmd, CmdReset);
        rtl8168_rx_desc_offset0_init(tp, 0);
        RTL_W8(ChipCmd, CmdRxEnb);
    } else if (tp->mcfg == CFG_METHOD_14 || tp->mcfg == CFG_METHOD_15) {
        rtl8168_ephy_write(ioaddr, 0x19, 0xFF64);
        RTL_W32(RxConfig, RTL_R32(RxConfig) | AcceptBroadcast | AcceptMulticast | AcceptMyPhys);
    }
}

static void rtl8168_down(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);
    unsigned long flags;

    rtl8168_delete_link_timer(dev, &tp->link_timer);

#ifdef CONFIG_R8168_NAPI
    napi_disable(&tp->napi);
#endif

    netif_stop_queue(dev);

    /* Give a racing hard_start_xmit a few cycles to complete. */
    synchronize_sched();  /* FIXME: should this be synchronize_irq()? */


    netif_carrier_off(dev);

    spin_lock_irqsave(&tp->lock, flags);

    rtl8168_hw_reset(dev);

    spin_unlock_irqrestore(&tp->lock, flags);

    synchronize_irq(dev->irq);

    spin_lock_irqsave(&tp->lock, flags);

    rtl8168_tx_clear(tp);

    rtl8168_rx_clear(tp);

    rtl8168_sleep_rx_enable(dev);

    spin_unlock_irqrestore(&tp->lock, flags);
}

static int rtl8168_close(struct net_device *dev)
{
    struct rtl8168_private *tp = netdev_priv(dev);

    if (tp->TxDescArray!=NULL && tp->RxDescArray!=NULL) {
        rtl8168_down(dev);

        rtl8168_powerdown_pll(dev);

        free_irq(dev->irq, dev);

        dma_free_coherent(&tp->platform_dev->dev, R8168_RX_RING_BYTES, tp->RxDescArray,
                            tp->RxPhyAddr);
        dma_free_coherent(&tp->platform_dev->dev, R8168_TX_RING_BYTES, tp->TxDescArray,
                            tp->TxPhyAddr);
        tp->TxDescArray = NULL;
        tp->RxDescArray = NULL;

        if (tp->tally_vaddr!=NULL) {
            dma_free_coherent(&tp->platform_dev->dev, sizeof(*tp->tally_vaddr), tp->tally_vaddr, tp->tally_paddr);
            tp->tally_vaddr = NULL;
        }
    }

    return 0;
}

static void rtl8168_shutdown(struct platform_device *pdev)
{
    struct net_device *dev = platform_get_drvdata(pdev);
    struct rtl8168_private *tp = netdev_priv(dev);


    rtl8168_rar_set(tp, tp->org_mac_addr);

    if (s5wol == 0)
        tp->wol_enabled = WOL_DISABLED;

    rtl8168_close(dev);
}


/**
 *  rtl8168_get_stats - Get rtl8168 read/write statistics
 *  @dev: The Ethernet Device to get statistics for
 *
 *  Get TX/RX statistics for rtl8168
 */
static struct
net_device_stats *rtl8168_get_stats(struct net_device *dev) {

    return &RTLDEV->stats;
}

#ifdef CONFIG_PM


static int
rtl8168_suspend(struct platform_device *pdev, pm_message_t state)

{
    struct net_device *dev = platform_get_drvdata(pdev);
    struct rtl8168_private *tp = netdev_priv(dev);

    unsigned long flags;

    if (!netif_running(dev))
        goto out;

    rtl8168_delete_link_timer(dev, &tp->link_timer);

    netif_stop_queue(dev);

    netif_carrier_off(dev);

    netif_device_detach(dev);

    spin_lock_irqsave(&tp->lock, flags);

    rtl8168_hw_reset(dev);

    rtl8168_sleep_rx_enable(dev);

    rtl8168_powerdown_pll(dev);

    spin_unlock_irqrestore(&tp->lock, flags);

out:
    return 0;
}

static int
rtl8168_resume(struct platform_device *pdev)
{
    struct net_device *dev = platform_get_drvdata(pdev);
    struct rtl8168_private *tp = netdev_priv(dev);
	

    /* restore last modified mac address */
    rtl8168_rar_set(tp, dev->dev_addr);

    if (!netif_running(dev))
        goto out;

    rtl8168_exit_oob(dev);

    rtl8168_rx_desc_offset0_init(tp, 1);

    rtl8168_hw_init(dev);

    rtl8168_powerup_pll(dev);

    rtl8168_hw_ephy_config(dev);

    rtl8168_hw_phy_config(dev);

    rtl8168_schedule_work(dev, rtl8168_reset_task);

    netif_device_attach(dev);

    mod_timer(&tp->link_timer, jiffies + RTL8168_LINK_TIMEOUT);
out:
    return 0;
}

#endif /* CONFIG_PM */

static struct platform_driver rtl8168_platform_driver = {
	.driver		= {
				.name	= MODULENAME,
				.owner	= THIS_MODULE,
	},
    .probe      = rtl8168_init_one,
    .remove     = __devexit_p(rtl8168_remove_one),
    .shutdown   = rtl8168_shutdown,
#ifdef CONFIG_PM
    .suspend    = rtl8168_suspend,
    .resume     = rtl8168_resume,
#endif
};

static int __init
rtl8168_init_module(void)
{
	return platform_driver_register(&rtl8168_platform_driver);
}

static void __exit
rtl8168_cleanup_module(void)
{
	platform_driver_unregister(&rtl8168_platform_driver);
}

module_init(rtl8168_init_module);
module_exit(rtl8168_cleanup_module);
