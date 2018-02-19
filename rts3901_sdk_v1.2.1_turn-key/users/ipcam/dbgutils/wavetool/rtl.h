#ifndef _RTL_H
#define _RTL_H

//pci/e ioctl
enum rtl_pci_cmd {
        RTLTOOL_READ_MAC=0,
        RTLTOOL_WRITE_MAC,
        RTLTOOL_READ_PHY,
        RTLTOOL_WRITE_PHY,
        RTLTOOL_READ_EPHY,
        RTLTOOL_WRITE_EPHY,
        RTLTOOL_READ_ERI,
        RTLTOOL_WRITE_ERI,
        RTLTOOL_READ_PCI,
        RTLTOOL_WRITE_PCI,
        RTLTOOL_READ_EEPROM,
        RTLTOOL_WRITE_EEPROM,

        RTL_READ_OOB_MAC,
        RTL_WRITE_OOB_MAC,

        RTL_ENABLE_PCI_DIAG,
        RTL_DISABLE_PCI_DIAG,

        RTLTOOL_INVALID
};

struct rtltool_pci_cmd {
        u32	cmd;
        u32	offset;
        u32	len;
        u32	data;
        struct sockaddr ifru_addr;
        struct sockaddr ifru_netmask;
        struct sockaddr ifru_hwaddr;
};

//usb ioctl
enum rtl_usb_cmd {
        RTLTOOL_PLA_OCP_READ_DWORD=0,
        RTLTOOL_PLA_OCP_WRITE_DWORD,
        RTLTOOL_USB_OCP_READ_DWORD,
        RTLTOOL_USB_OCP_WRITE_DWORD,
        RTLTOOL_PLA_OCP_READ,
        RTLTOOL_PLA_OCP_WRITE,
        RTLTOOL_USB_OCP_READ,
        RTLTOOL_USB_OCP_WRITE,
        RTLTOOL_USB_INFO,

        RTL_ENABLE_USB_DIAG,
        RTL_DISABLE_USB_DIAG,

        RTLTOOL_USB_INVALID
};

struct usb_device_info {
        __u16           idVendor;
        __u16           idProduct;
        __u16           bcdDevice;
        __u8            dev_addr[8];
        char            devpath[16];
};

struct rtltool_usb_cmd {
        __u32   cmd;
        __u32   offset;
        __u32   byteen;
        __u32   data;
        void    *buf;
        struct usb_device_info nic_info;
        struct sockaddr ifru_addr;
        struct sockaddr ifru_netmask;
        struct sockaddr ifru_hwaddr;
};

typedef enum _NIC_CHIP_TYPE {
        RTLUNKNOWN = 0,
        RTL8169,        // NO SUPPORT
        RTL8169S1,
        RTL8169S2,
        RTL8169SB,
        RTL8169SC,
        RTL8169SC_REV_E,    // REV E or REV F
        RTL8168B,
        RTL8168B_REV_E,
        RTL8168B_REV_F,         // add 6 wakeup pattern , fix some HW bug
        RTL8136,            // 8168B REV B
        RTL8136_REV_E,          // 8168B REV E
        RTL8101,            // different with 8168B , 8101E REV B
        RTL8101_REV_C,          // 8101E REV C , 8101E REV D (fix unicast autoload bug)
        RTL8168C,           // new IC, buggy, only for sys vendor evaluate
        RTL8168C_REV_G,     // Rev G, Rev J (J = FIX CLK REG)
        RTL8168C_REV_K,     // Rev K (FIX chksum/lso tcp/ip option bug) (PHY FIX earily link on)
        RTL8168C_REV_M,     // Rev M (FIX ESD , and IMPROVE EPHY)
        RTL8102E,
        RTL8102EL,
        RTL8102E_EL_REV_B,
        RTL8102E_EL_REV_C,
        RTL8103,            // rev A
        RTL8103E_EL_REV_B,
        RTL8103E_EL_REV_C,
        RTL8401, //RTL8103 rev A
        RTL8105E_REV_C,
        RTL8105E_REV_D,
        RTL8105E_REV_F,  //RL6151F
        RTL8105E_REV_G,  //RL6151G
        RTL8105E_REV_H, //RL6151H
        RTL8106E_REV_A, //RL6245A
        RTL8106E_REV_B, //RL6245B
        RTL8168D,
        RTL8168D_REV_B,
        RTL8168D_REV_C_REV_D,
        RTL8168E,
        RTL8168E_REV_B,
        RTL8168E_REV_C,
        RTL8168F,      // RL6210A
        RTL8168F_REV_B, //RL6210B
        RTL8168FB,  //RL6220A 16K
        RTL8168FB_REV_B,  //RL6220B 16K
        RTL8168G,      // RL6223A
        RTL8168G_REV_B,      // RL6223B
        RTL8168GU,      // RLE0568
        RTL8168GU_REV_B,      // RL6303
        RTL8168H,  //RL6362
        RTL8168CP,          // new IC, buggy, only for sys vendor evaluate
        RTL8168CP_REV_B,    // formal release 8168CP
        RTL8168CP_REV_C,
        RTL8168CP_REV_D,
        RTL8168DP,
        RTL8168DP_REV_B,
        RTL8168DP_REV_E,
        RTL8168DP_REV_F,
        RTL8168EP, //RLE0527
        RTL8168EP_REV_B,  //RL6287
        RTL8168EP_REV_C,  //RL6287B
        RTL8411,  //RL6208 (similar with RL6220)
        RTL8411B,  //RL6301
        RTL8402,  //RL6209

        RTL8152_REV_A = 0xD0,
        RTL8152_REV_B,
        RTL8153_REV_A,
        RTL8153_REV_B,
        RTL8153_REV_C,


} NIC_CHIP_TYPE, *PNIC_CHIP_TYPE;
// please add new chiptype from up to down

#define	SIOCRTLPCITOOL 		SIOCDEVPRIVATE+1
#define	SIOCRTLUSBTOOL 		SIOCDEVPRIVATE

#define RTK_PCI_GBE		"Realtek PCIe GBE Ethernet controller"
#define RTK_PCI_FE		"Realtek PCIe FE Ethernet controller"
#define RTK_USB_GBE		"Realtek USB GBE Ethernet controller"
#define RTK_USB_FE		"Realtek USB FE Ethernet controller"

#define MAC0				0x00
#define MAC4				0x04
#define MAR0				0x08
#define CounterAddrLow		0x10
#define CounterAddrHigh		0x14
#define TxConfig			0x40
#define RxConfig			0x44
#define Cfg9346				0x50
#define Config0				0x51
#define Config1				0x52
#define Config2				0x53
#define Config3				0x54
#define Config4				0x55
#define Config5				0x56
#define PHYstatus			0x6C

#define Cfg9346_Lock		0x00
#define Cfg9346_Unlock		0xC0

/* ERI access */
#define ERIDR				0x70
#define ERIAR				0x74
#define ERIAR_Flag			0x80000000
#define ERIAR_Write			0x80000000
#define ERIAR_Read			0x00000000
#define ERIAR_Addr_Align	4
#define ERIAR_ExGMAC		0
#define ERIAR_MSIX			1
#define ERIAR_ASF			2
#define ERIAR_Type_shift	16
#define ERIAR_ByteEn		0x0f
#define ERIAR_ByteEn_shift	12

//USB
#define PLAMCU_OCP	1
#define USBMCU_OCP	0

#define TCR_0				0xE610

#define SET_PCI_READ_CMD(reg, bytes, ops) \
{ \
	cmd.offset = reg; \
	cmd.len = bytes; \
	cmd.cmd = ops; \
}

#define SET_PCI_WRITE_CMD(reg, bytes, value, ops) \
{ \
	cmd.offset = reg; \
	cmd.len = bytes; \
	cmd.data = value; \
	cmd.cmd = ops; \
}

#define SET_USB_WRITE_CMD(reg, byte, len, value, ops) \
{ \
	cmd.offset = reg; \
	cmd.byteen = byte; \
	cmd.data = len; \
	cmd.buf = value; \
	cmd.cmd = ops; \
}

#define SET_USB_READ_CMD(reg, len, value, ops) \
{ \
	cmd.offset = reg; \
	cmd.data = len; \
	cmd.buf = value; \
	cmd.cmd = ops; \
}


/* PLA_TCR1 */
#define VERSION_MASK            0x7cf0

// For USB NIC
#define PLAMCU_OCP	1
#define USBMCU_OCP	0

/* functions */
u32 read_pci_cfg(int, rt_nic_info *, int, u32);
void write_phy(int, rt_nic_info *, u16, u16);
u16 read_phy(int, rt_nic_info *, u16);
u32 read_mac(int, rt_nic_info *, u16, int);
void write_mac(int, rt_nic_info *, u16, u32, int);
u32 read_eri(int, rt_nic_info *, u16, int);
void write_eri(int, rt_nic_info *, u16, int, u32);
int usb_direct_read(int sd, char *devname, u8 reg_type, u16 addr, u8 *value, int len);
int usb_direct_write(int sd, char *devname, u8 reg_type, u16 addr, u8 *value, int len);

int disable_power_saving_funcs(rt_nic_info *);
int set_rt_speed_para(rt_nic_info *, int);
void rt_giga_lbk_off(rt_nic_info *);
void rt_exit(rt_nic_info *);
int check_rt_cable_connect(rt_nic_info *);
NIC_CHIP_TYPE get_rt_mac_version(char *, u16);
u8 get_rt_changed_mac_chiptype(rt_nic_info *);
int check_link_speed(int, rt_nic_info *, int);


/* WaveFormTool */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long int DWORD;

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long int ULONG;

typedef UCHAR* PUCHAR;
typedef USHORT* PUSHORT;
typedef ULONG* PULONG;

typedef void VOID;

#define Sleep(x) usleep(x*1000)
#define udelay(x) usleep(x)

//Channel Wait Count
#define CHANNEL_WAIT_COUNT (1000)
#define CHANNEL_WAIT_TIME (1)  // 1us
#define CHANNEL_EXIT_DELAY_TIME (20)  //20us

//ERI Type
#define ERI_CHANNEL_TYPE_EXT_GMAC (0)
//#define ERI_CHANNEL_MSIX (1)
//#define ERI_CHANNEL_TYPE_ASF (2)
//#define ERI_CHANNEL_TYPE_OCP (2) //RTL8168EP
//#define ERI_CHANNEL_TYPE_CR_REG (3) //RTL8411B


//define function pointer
typedef void (*RtkDiagTest)(PMP_ADAPTER);

void RTK_10M_Haromonic_Finish(PMP_ADAPTER Adapter);
void RTK_10M_Harmonic(PMP_ADAPTER Adapter);
void RTK_10M_Normal_Finish(PMP_ADAPTER Adapter);
void RTK_10M_Normal(PMP_ADAPTER Adapter);
void RTK_10M_Link_Pluse(PMP_ADAPTER Adapter);
void WaveFormInitAdapter(PMP_ADAPTER Adapter);
void RTK_100M_Channel_A(PMP_ADAPTER Adapter);
void RTK_100M_Finish(PMP_ADAPTER Adapter);
void RTK_100M_Channel_B(PMP_ADAPTER Adapter);
void RTK_10M_Return_loss_Finish(PMP_ADAPTER Adapter);
void RTK_10M_Return_Loss_Channel_A(PMP_ADAPTER Adapter);
void RTK_10M_Return_Loss_Channel_B(PMP_ADAPTER Adapter);
void RTK_100M_Return_loss_Finish(PMP_ADAPTER Adapter);
void RTK_100M_Return_Loss_Channel_A(PMP_ADAPTER Adapter);
void RTK_100M_Return_Loss_Channel_B(PMP_ADAPTER Adapter);
void RTK_1000M_Return_loss_Finish(PMP_ADAPTER Adapter);
void RTK_1000M_Return_Loss(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_1_Finish(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_1(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_2_Finish(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_2(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_3_Finish(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_3(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_4_Finish(PMP_ADAPTER Adapter);
void RTK_1000M_Test_Mode_4(PMP_ADAPTER Adapter);
void send_waveform_test_packet(PMP_ADAPTER Adapter);








struct rtk_wave_form_test_vector {
        char rtk_test_vector_name[64];
        u16 support_bus_feature;
        RtkDiagTest rtk_wave_form_test_start_fun;
        RtkDiagTest rtk_wave_form_test_finish_fun;
};


#endif


