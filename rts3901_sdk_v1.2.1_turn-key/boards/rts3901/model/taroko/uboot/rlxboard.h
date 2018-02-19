/*
 * Realtek Semiconductor Corp.
 *
 * Copyright 2012  Jethro Hsu (jethro@realtek.com)
 * Copyright 2012  Tony Wu (tonywu@realtek.com)
 */

#ifndef __RLXBOARD_H_
#define __RLXBOARD_H_
#define TAG_STRING	"V1.05"

/*#define CONFIG_VM_CODE*/
#define CONFIG_ARCH_CPU_EL
#define _RTS3901A_		0x01
#define _RTS3901B_		0x02
#define _CHIP_ID_		_RTS3901B_

#define CONFIG_YES
#ifdef CONFIG_OLD_TEST
/*#define CONFIG_MEM_TEST*/
#ifdef CONFIG_MEM_TEST
/*#define _MEM_TEST1_*/
/*#define _MEM_TEST2_*/
#define _MEM_TEST3_
#endif
#ifdef _MEM_TEST3_
#undef CONFIG_YES
#endif
/*#define _IMEM_DMA_TEST_*/
/*#define _WMPU_TEST_*/
#endif

/*#define DDR_CTL_V1*/
#define DDR_CTL_V2


/*******************for ASIC test********************/
/*#define ASIC_TEST_CPU*/
#ifdef ASIC_TEST_CPU

/*******wmpu test********/
/*#define WMPU_TEST*/
#ifdef WMPU_TEST
/*#define WMPU_TEST_READ*/
/*#define WMPU_TEST_WRITE*/
/*#define WMPU_TEST_INST*/
#endif

/*******i/dmem test*********/
/*#define IMEM_TEST*/
/*#define IMEMFILL_TEST*/
/*#define DMEM_TEST*/

/******interupt test********/
/*#define INTERRUPT_TEST_TIMER*/
/*#define INTERRUPT_TEST_SPIC*/

/********pwm and watchdog test*********/
/*pwm test*/
/*#define PWM_TEST*/
/*watchdog test*/
#ifdef PWM_TEST
/*#define WATCH_DOG_TEST*/
#ifdef WATCH_DOG_TEST
/*# define WATCH_DOG_FEED*/
#endif
#endif

/*******timer test********/
/*#define TIMER_TEST*/
#ifdef TIMER_TEST
# define CONFIG_TIMER_2		CONFIG_SYS_MIPS_TIMER_FREQ
# define CONFIG_TIMER_1K	(CONFIG_SYS_MIPS_TIMER_FREQ / 2000)
#define CYCLES_PER_JIFFY	(CONFIG_SYS_MIPS_TIMER_FREQ + CONFIG_SYS_HZ / 2) / CONFIG_SYS_HZ
#endif

#endif /*ASIC_TEST_CPU*/
/********************test end*******************/


#define CONFIG_MIPS32		1  /* MIPS32 CPU core	*/

/*#define CONFIG_ETHADDR		DE:AD:BE:EF:01:02 */  /* Ethernet address */
#define CONFIG_ETHADDR		00:00:00:00:00:00
/*#define CONFIG_IPADDR		192.168.1.26*/
/*#define CONFIG_GATEWAYIP	192.168.1.1*/
/*#define CONFIG_SERVERIP 	192.168.1.50*/
/*#define CONFIG_NETMASK	 	255.255.255.0*/
#define CONFIG_BOOTDELAY	3	/* autoboot after 3 seconds	*/
#define CONFIG_BAUDRATE	57600
/*#define CONFIG_TFTPBLKSIZE  512*/

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

#define	CONFIG_TIMESTAMP		/* Print image info with timestamp */
#undef	CONFIG_BOOTARGS

#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"addmisc=setenv bootargs ${bootargs}"				\
	"console=ttyS0,${baudrate}"				\
	"panic=1\0"						\
	"bootfile=/vmlinux.img\0"					\
	"load=tftp 80500000 ${u-boot}\0"				\
	""
/* Boot from NFS root */
/*#define CONFIG_BOOTCOMMAND	"bootp; setenv bootargs root=/dev/nfs rw nfsroot=${serverip}:${rootpath} ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}::off; bootm 0x80001000"*/
#define CONFIG_BOOTCOMMAND	"bootm "
#define CONFIG_FLASHBASEADDR	0xBC000000

#include <generated/sdk_config.h>
#include <generated/mtd_mapping_config.h>

#ifdef MTD_MAPPING_CONFIG
#define SPI_SECTOR_SIZE		RTS_MTD_FLASH_SECTOR_SIZE
#define SPI_UBOOT_OFFSET	RTS_MTD_BOOT_OFFSET
#define SPI_FW_OFFSET		RTS_MTD_MCU_FW_OFFSET
#define SPI_HW_OFFSET	RTS_MTD_HCONF_OFFSET
#define SPI_SW_OFFSET	RTS_MTD_USERDATA_OFFSET
#define SPI_KERNEL_OFFSET	RTS_MTD_KERNEL_OFFSET
#ifdef RTS_MTD_ROOTFS_OFFSET
#define SPI_ROOTFS_OFFSET	RTS_MTD_ROOTFS_OFFSET
#endif
#define SPI_LDC_OFFSET	RTS_MTD_LDC_OFFSET
#else

#define SPI_SECTOR_SIZE		0x10000
#define SPI_UBOOT_OFFSET	0
#define SPI_FW_OFFSET		0x40000
#define SPI_HW_OFFSET		0x60000
#define SPI_SW_OFFSET		0xa0000
#define SPI_KERNEL_OFFSET	0x120000
#define SPI_ROOTFS_OFFSET	0x520000
#define SPI_LDC_OFFSET	0xd20000
#endif

#define CONFIG_BOOTADDR	(CONFIG_FLASHBASEADDR + SPI_KERNEL_OFFSET)
/*
#define CONFIG_BOOTARGS		"mem=128M console=ttyS0,115200 " \
				"mtdparts=atmel_nand:" \
				"8M(bootstrap/uboot/kernel)ro,-(rootfs) " \
				"root=/dev/mmcblk0p2 " \
				"rw rootfstype=ext4 rootwait"
*/
/*
 * Miscellaneous configurable options
 */
#define CONFIG_LOCALVERSION_AUTO 1
#define	CONFIG_SYS_PROMPT		"rlxboot# "	/* Monitor Command Prompt    */
#define	CONFIG_SYS_CBSIZE		256		/* Console I/O Buffer Size   */

/* Print Buffer Size */
#define	CONFIG_SYS_PBSIZE 		(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)
#define	CONFIG_SYS_MAXARGS		16		/* max number of command args*/

#define CONFIG_SYS_MALLOC_LEN		(16 << 20)
#define CONFIG_SYS_BOOTPARAMS_LEN	(32 << 10)

/*cpu frequency setting*/
#ifdef CONFIG_TARGET_ASIC
#if(_CHIP_ID_ &_RTS3901A_)
#define PLL0_N_SETTING			0x14	/*600M, PLL0 output = (N+4)*25000000 */
#define CPU_FREQUCNCY			240000000 /*usb_pll/2*/
#endif
#if(_CHIP_ID_ &_RTS3901B_)
#define PLL0_N_SETTING			0x24	/*1200M, PLL0 output = (N+4)*25000000 */
#define CPU_FREQUCNCY			500000000 /*PLL0/2*/
#endif
#endif
#ifdef CONFIG_TARGET_FPGA
#define CPU_FREQUCNCY			50000000
#endif
#define CONFIG_SYS_MIPS_TIMER_FREQ	CPU_FREQUCNCY
#define CONFIG_SYS_HZ			1000


#define CONFIG_SYS_SDRAM_BASE		0x80000000     /* Cached addr */
#define CONFIG_SYS_SDRAM_SIZE		(256 << 20)

#define	CONFIG_SYS_LOAD_ADDR		0x80800000    /* default load address	*/

#define	CONFIG_LOADADDR			0x82000000
#define CONFIG_SYS_MEMTEST_START	0x80400000
#define CONFIG_SYS_MEMTEST_END		0x80500000

/*-----------------------------------------------------------------------
 * SMP Configuration
 */
#undef CONFIG_SMP 		/* undefine this if it is not a SMP */
#define CORENLOOP 		0xa0000e00
#define SMPBOOT			0xa0000f00
#define CONFIG_BOOT_ADDR	0xbfb070f0

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_FLASH_BOOT		1	/* set '1' to boot from flash */
#define CONFIG_SYS_MAX_FLASH_BANKS	1	/* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT	(128)	/* max number of sectors on one chip */
#ifndef _MEM_TEST3_
#define CONFIG_CMD_SF			1
#define CONFIG_SPI_FLASH		1
#define CONFIG_RLX_SPI			1
#endif
#define CONFIG_SPI_FLASH_MACRONIX		1
#define CONFIG_SPI_FLASH_GIGADEVICE		1
#define CONFIG_SPI_FLASH_WINBOND		1
/*#define CONFIG_SPI_FRAM_RAMTRON		1*/
#define PHYS_FLASH_1			0xbfc00000 /* Flash Bank #1 */

/* The following #defines are needed to get flash environment right */
#define	CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#define	CONFIG_SYS_MONITOR_LEN		(192 << 10)

#if	((defined _MEM_TEST2_) || (defined _MEM_TEST3_))
#define CONFIG_SYS_INIT_SP_OFFSET  0xf03000
#define DMEM_BASE_ADDR1			0xa0000000
#define DMEM_BASE_ADDR2			0xa0001000
#else
#define CONFIG_SYS_INIT_SP_OFFSET	0x1000000
#endif
#define CONFIG_SYS_INIT_SP_SIZE		4096

/* We boot from this flash, selected with dip switch */
#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH_1

/* timeout values are in ticks */
#define CONFIG_SYS_FLASH_ERASE_TOUT	(2 * CONFIG_SYS_HZ) /* Timeout for Flash Erase */
#define CONFIG_SYS_FLASH_WRITE_TOUT	(2 * CONFIG_SYS_HZ) /* Timeout for Flash Write */

#define	CONFIG_ENV_IS_NOWHERE		1
#define   CONFIG_ENV_OVERWRITE			1


/*#define CONFIG_ENV_IS_IN_SPI_FLASH		1*/

/* Address and size of Primary Environment Sector	*/
#define CONFIG_ENV_ADDR			0xB0030000
#define CONFIG_ENV_SIZE			0x20000
#define CONFIG_ENV_OFFSET		0x2000
#define CONFIG_ENV_SECT_SIZE		0x10000
#define CONFIG_FLASH_16BIT
#define CONFIG_NR_DRAM_BANKS		2
#define CONFIG_NET_MULTI
#define CONFIG_MEMSIZE_IN_BYTES
/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CONFIG_SYS_DCACHE_SIZE		32768
#define CONFIG_SYS_ICACHE_SIZE		65536
#define CONFIG_SYS_CACHELINE_SIZE	32

/*-----------------------------------------------------------------------
 * uboot Configuration
 */
#ifndef _MEM_TEST3_
/* Support bootm-ing different OSes */
#define CONFIG_BOOTM_LINUX	1
#define CONFIG_BOOTM_NETBSD	0
#define CONFIG_BOOTM_RTEMS	0

#define CONFIG_GZIP		1
#define CONFIG_ZLIB		1
#define CONFIG_PARTITIONS	1

#define CONFIG_CMD_BOOTM	1
#define CONFIG_CMD_CRC32	1
#define CONFIG_CMD_EXPORTENV	1
#define CONFIG_CMD_IMPORTENV	1
#define CONFIG_CMD_SAVEENV	/* saveenv                      */
#define CONFIG_CMD_EDITENV	/* editenv                      */
#define CONFIG_CMD_GO		1

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/*
 * Command line configuration.
 */
#define CONFIG_CMD_CONSOLE	/* coninfo                      */
#define CONFIG_CMD_ECHO		/* echo arguments               */
#define CONFIG_CMD_NET		/* bootp, tftpboot, rarpboot    */
#define CONFIG_CMD_TFTPSRV
/*#define CONFIG_CMD_DHCP*/
#define CONFIG_RTL8168
#define CONFIG_CMD_PING
#define CONFIG_CMD_SOURCE	/* "source" command support     */
#define CONFIG_CMD_XIMG		/* Load part of Multi Image     */
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_EFUSE

#define   CONFIG_SYS_LONGHELP				/* undef to save memory      */
#define	CONFIG_AUTO_COMPLETE
#define	CONFIG_CMDLINE_EDITING
#define CONFIG_CMD_UPDATE				1
#define CONFIG_CMD_FEPHY
#endif
#undef CONFIG_CMD_AMBAPP
#undef CONFIG_CMD_BDI		/* bdinfo                       */
#undef CONFIG_CMD_BEDBUG
#undef CONFIG_CMD_BOOTD		/* bootd                        */
#undef CONFIG_CMD_IMI		/* iminfo                       */
#undef CONFIG_CMD_ITEST		/* Integer (and string) test    */
#undef CONFIG_CMD_I2C
#undef CONFIG_CMD_FPGA		/* FPGA configuration Support   */
#undef CONFIG_CMD_FLASH		/* flinfo, erase, protect       */
#undef CONFIG_CMD_IMLS		/* List all found images        */
#undef CONFIG_CMD_LOADB		/* loadb                        */
#undef CONFIG_CMD_LOADS		/* loads                        */
#undef CONFIG_CMD_MISC		/* Misc functions like sleep etc*/
#undef CONFIG_CMD_NFS		/* NFS support                  */
#undef CONFIG_CMD_PXE
#undef CONFIG_CMD_UBI
#undef CONFIG_CMD_UBIFS
#undef CONFIG_CMD_RUN          /* run command in env variable  */
#undef CONFIG_CMD_SETGETDCR    /* DCR support on 4xx           */

#undef CONFIG_CMD_ELF
#undef CONFIG_CMD_IDE
#undef CONFIG_CMD_FAT

/*DDR option*/
#ifdef CONFIG_TARGET_ASIC
#ifdef CONFIG_DDR_H5TQ2G
/*#define CONFIG_DDR_H5TQ2G_112M*/
/*#define CONFIG_DDR_H5TQ2G_200M*/
/*#define CONFIG_DDR_H5TQ2G_400M*/
/*#define CONFIG_DDR_H5TQ2G_667M*/
#define CONFIG_DDR_H5TQ2G_800M
#endif

#ifdef CONFIG_DDR_W975116KG
#define CONFIG_DDR_MCM
#define CONFIG_DDR_W9751V6KG_400M
/*#define CONFIG_DDR_W9751V6KG_533M*/
#endif

#ifdef CONFIG_DDR_NT5CC128M16IP
#define CONFIG_DDR_NT5CC128M16IP_800M
#endif

#ifdef CONFIG_DDR_M15F2G16128A
#define CONFIG_DDR_M15F2G16128A_800M
#endif

#ifdef CONFIG_DDR_W632GG6KB
/*#define CONFIG_DDR_W632GG6KB_667M*/
#define CONFIG_DDR_W632GG6KB_800M
#endif

#ifdef CONFIG_DDR_W631GG6KB
#define CONFIG_DDR_W631GG6KB_800M
#endif

#ifdef CONFIG_DDR_NT5CC64M16IP
#define CONFIG_DDR_NT5CC64M16IP_800M
#endif

#ifdef CONFIG_DDR_M15F1G1664A
#define CONFIG_DDR_M15F1G1664A_800M
#endif

#ifdef CONFIG_DDR3_2GBIT_GENERAL
#define CONFIG_DDR3_2GBIT_GENERAL_800M
#endif

#ifdef CONFIG_DDR3_1GBIT_GENERAL
#define CONFIG_DDR3_1GBIT_GENERAL_800M
#endif

#define CONFIG_BGA234_DEMO_BOARD
#ifndef CONFIG_BGA234_DEMO_BOARD
#define CONFIG_BGA234_QA_BOARD
#endif
#endif

#ifdef CONFIG_TARGET_FPGA
#ifdef CONFIG_DDR_H5TQ2G
#define CONFIG_DDR_H5TQ2G_112M
#endif
#endif

#define _SYS_PLL_INIT_

/*MP configuration*/
#undef CONFIG_DDR_BIST	/*DDR bist test*/

#endif	/* __CONFIG_H */
