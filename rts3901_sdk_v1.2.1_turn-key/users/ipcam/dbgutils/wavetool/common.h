#ifndef _MPLAN_COMMON_H
#define _MPLAN_COMMON_H

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
//#include <assert.h>

#define PROGRAM_VERSION			"0.05 (10/06/2014)"

#define ETH_MAC_LEN				ETH_ALEN        /* 6 */
#define ETH_HEADER_LEN			ETH_HLEN        /* 14 */
#define MIN_ETH_FRAME_LEN		ETH_ZLEN        /* 60 */
#define MAX_ETH_FRAME_LEN		ETH_FRAME_LEN   /* 1514 */
#define MAX_PAYLOAD_LEN			ETH_DATA_LEN    /* 1500 */


/* raw socket protocol */
#define ETH_P_LOOPBACK			0x10ec

#ifndef SIOCETHTOOL
#define SIOCETHTOOL				0x8946
#endif

#define PROC_NET_DEV			"/proc/net/dev"
#define SYS_CARRIER(buf,name)	sprintf(buf, "/sys/class/net/%s/carrier", name)
#define TMPDIR					"/tmp"
#define PROG_LOCKFILE			TMPDIR"/rtmplan.pid"

#define MAX_CARD_NUM		12

#define DEBUG_FILENAME "rtwftool_dbg"

#define BIT_0       0x0001
#define BIT_1       0x0002
#define BIT_2       0x0004
#define BIT_3       0x0008
#define BIT_4       0x0010
#define BIT_5       0x0020
#define BIT_6       0x0040
#define BIT_7       0x0080
#define BIT_8       0x0100
#define BIT_9       0x0200
#define BIT_10      0x0400
#define BIT_11      0x0800
#define BIT_12      0x1000
#define BIT_13      0x2000
#define BIT_14      0x4000
#define BIT_15      0x8000
#define BIT_16      0x010000
#define BIT_17      0x020000
#define BIT_18      0x040000
#define BIT_19      0x080000
#define BIT_20      0x100000
#define BIT_21      0x200000
#define BIT_22      0x400000
#define BIT_23      0x800000
#define BIT_24      0x01000000
#define BIT_25      0x02000000
#define BIT_26      0x04000000
#define BIT_27      0x08000000
#define BIT_28      0x10000000
#define BIT_29      0x20000000
#define BIT_30      0x40000000
#define BIT_31      0x80000000

#define BIT0       0x0001
#define BIT1       0x0002
#define BIT2       0x0004
#define BIT3       0x0008
#define BIT4       0x0010
#define BIT5       0x0020
#define BIT6       0x0040
#define BIT7       0x0080
#define BIT8       0x0100
#define BIT9       0x0200
#define BIT10      0x0400
#define BIT11      0x0800
#define BIT12      0x1000
#define BIT13      0x2000
#define BIT14      0x4000
#define BIT15      0x8000
#define BIT16      0x010000
#define BIT17      0x020000
#define BIT18      0x040000
#define BIT19      0x080000
#define BIT20      0x100000
#define BIT21      0x200000
#define BIT22      0x400000
#define BIT23      0x800000
#define BIT24      0x01000000
#define BIT25      0x02000000
#define BIT26      0x04000000
#define BIT27      0x08000000
#define BIT28      0x10000000
#define BIT29      0x20000000
#define BIT30      0x40000000
#define BIT31      0x80000000

#define PRINT_PAD		"   "
#define NORMAL			"\033[0m"
#define BOLD			"\033[1m"
#define RED				"\033[31m"
#define GREEN			"\033[32m"
#define YELLOW			"\033[33m"
#define CYAN			"\033[36m"

#ifndef u8
typedef	uint8_t u8;
#endif
#ifndef u16
typedef uint16_t u16;
#endif
#ifndef u32
typedef uint32_t u32;
#endif

#define MPL_ERR		0
#define MPL_MSG		MPL_ERR
#define MPL_DBG		1
#define MPL_INFO	2

#define dbgprint(level, fmt, args...) \
{ \
	if (level <= MPL_MSG) \
	{ \
		printf(fmt, ##args); \
	} \
}

#define dbgperror(level, fmt) \
{ \
	if (level <= MPL_MSG) \
	{ \
		perror(fmt); \
	} \
}


#ifndef bool
typedef enum {
        FALSE = 0,
        TRUE = 1
} bool;
#endif

enum {
        ECONFIG = 1,
        EFOPEN,
        ERIGHTS,
        EALREADYEXE,
        ECURSESUI,
        EFINDNIC,
        ECREATESOCKET,
        EBINDSOCKET,
        EIOCTL,
        EMEMALLOC,
        ELINKFAIL,
        ESETSPEED,
        ELBKERR,
        EPHYRESET,
};

enum {
        INDEX_1000 = 0,
        INDEX_100,
        INDEX_10,
        SPEED_NUM
};

#define UNKNOWN_NIC	0x0000
#define PCI_NIC		BIT_0
#define	USB_NIC		BIT_1
#define FE_NIC	    BIT_2
#define GIGA_NIC	BIT_3

#define MAX_FILE_NAME	100

#define UNKNOWN_TEST		0x0000
#define SPEED1000_TEST		BIT_0
#define SPEED100_TEST		BIT_1
#define SPEED10_TEST		BIT_2
#define SPEED_ALL_TEST		(SPEED10_TEST | SPEED100_TEST | SPEED1000_TEST)
#define REGISTER_RW_TEST	BIT_3
#define ADAPTER_LIST_TEST	BIT_4

/* input arguments */
typedef struct rt_nic_info_s {
        char name[IFNAMSIZ];
        u32 ChipType;
        u16 busfeature;
        u8 macaddr[ETH_MAC_LEN];
        int ifindex;
        char drv_name[128];
        char drv_version[128];
        char bus_info[128];
        u16 vid;
        u16 did;
        u16 svid;
        u16 ssid;
        u16 iobase;
        u16 status;
        u32 linkcap;
        u8 irq;
        char desc[256];

        bool Nic8168DChangeTo8104;
        bool Nic8168EChangeTo8105;
        bool Nic8168FChangeTo8168E;
        bool Nic8411ChangeTo8411AAR;
        bool Nic8401ChangeTo8103EVL;
        bool Nic8168HChangeTo8107E;
        bool Nic8168HChangeTo8118AS;
	bool Nic8168FBChangeTo8119;
} rt_nic_info;

typedef struct _MP_ADAPTER {

        char devname[IFNAMSIZ];
        rt_nic_info allnicinfo[MAX_CARD_NUM];
        rt_nic_info *testnicinfo;
        int nicnum;
        int curridx;
        u32 ChipType;
        int wave_form_test_mode;
        bool kee_send_packet;
        bool bShowDbgMsg;
}

MP_ADAPTER, *PMP_ADAPTER;

rt_nic_info allnicinfo[MAX_CARD_NUM];

/* functions */
int mdi_loopback_test(rt_nic_info *, int);
int reg_rw_test(rt_nic_info *);
void print_file_result(int);
void print_console_result(void);

int is_link(int, rt_nic_info *);
int get_link_speed(int, rt_nic_info *);
int find_interface(rt_nic_info *);
void print_pkt(void *, int);
//int ipow(int, int);
//int isqrt(int, int);
int create_control_socket();
int create_raw_socket(int);
int bind_rawsocket_to_interface(rt_nic_info *, int, int);
int collect_info(rt_nic_info *);


int set_ifr_flags(char *, int, int);
int test_ifr_flags(char *, int);
int execute_lock();
void execute_unlock();

int enable_rtl_diag(rt_nic_info *nicinfo, u8 enable_diag);

#endif
