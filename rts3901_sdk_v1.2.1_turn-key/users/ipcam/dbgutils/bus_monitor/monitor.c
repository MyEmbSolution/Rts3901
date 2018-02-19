/***************************************************************************
    filename:            : mon_init.c
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#include "define.h"
#include "monitor.h"

static int dbg_ioread32(int fd, u32 addr, u32 *val)
{
	int err = 0;
	u8 buf[4];

	buf[0] = (u8)(addr >> 24);
	buf[1] = (u8)(addr >> 16);
	buf[2] = (u8)(addr >> 8);
	buf[3] = (u8)addr;

	err = ioctl(fd, DBG_IOMEM_IOC_IOREAD32, buf);
	if (err < 0)
		return err;

	if (val)
		*val = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
				((u32)buf[2] << 8) | buf[3];

	return 0;
}

static int dbg_iowrite32(int fd, u32 addr, u32 val)
{
	int err = 0;
	u8 buf[8];

	buf[0] = (u8)(addr >> 24);
	buf[1] = (u8)(addr >> 16);
	buf[2] = (u8)(addr >> 8);
	buf[3] = (u8)addr;
	buf[4] = (u8)(val >> 24);
	buf[5] = (u8)(val >> 16);
	buf[6] = (u8)(val >> 8);
	buf[7] = (u8)val;

	err = ioctl(fd, DBG_IOMEM_IOC_IOWRITE32, buf);
	if (err < 0)
		return err;

	return 0;
}

static u32 read_reg(u32 addr)
{
	u32 rdata;
	int fd, ret = 0;

	fd = open("/dev/dbg_iomem", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open device fail!\n");
		exit(EXIT_FAILURE);
	}
	ret = dbg_ioread32(fd, addr, &rdata);

	if (ret < 0)
		goto out;

	(void)close(fd);
	return rdata;

out:
	(void)close(fd);
	return (u32)ret;

}

static u32 write_reg(u32 addr, u32 value)
{
	u32 rdata;
	int fd, ret = 0;

	fd = open("/dev/dbg_iomem", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open device fail!\n");
		exit(EXIT_FAILURE);
	}
	ret = dbg_iowrite32(fd, addr, value);

	if (ret < 0)
		goto out;

	(void)close(fd);
	return 0;

out:
	(void)close(fd);
	return (u32)ret;

}

int monitor_init(void)
{
	u32 temp, wdata, rdata;
	rdata = read_reg(DESIGN_NAME_ADR);
	printf("RXI300_DES_NAME1          = 0x%x\n", read_reg(DESIGN_NAME_ADR));
	printf("RXI300_IMPL_YEAR1         = 0x%x\n", read_reg(IMPL_YEAR_ADR));
	printf("RXI300_IMPL_DATE1         = 0x%x\n", read_reg(IMPL_DATE_ADR));

	printf("========== Initialize performance monitors ==========\n");
	printf("MON_G0_PRESENT           = 0x%x\n", read_reg(MON_G0_PRESENT_ADR));
/*	printf("MON_G1_PRESENT           = 0x%x\n", read_reg(MON_G1_PRESENT_ADR));*/
	printf("MON_G0_STATUS            = 0x%x\n", read_reg(MON_G0_STATUS_ADR));
/*	printf("MON_G1_STATUS            = 0x%x\n", read_reg(MON_G1_STATUS_ADR));*/
	printf("MON_G0_INTR              = 0x%x\n", read_reg(MON_G0_INTR_ADR));
/*	printf("MON_G1_INTR              = 0x%x\n", read_reg(MON_G1_INTR_ADR));*/

/*	printf("GLOBAL_MON_TW_SIZE       = %d\n", read_reg(GLOBAL_MON_TW_SIZE_ADR));*/
	write_reg(GLOBAL_MON_TW_SIZE_ADR, 10000);
	printf("GLOBAL_MON_TW_SIZE       = %d\n", read_reg(GLOBAL_MON_TW_SIZE_ADR));

	write_reg(MON_G0_0_EVENT_ADR, (0x0 << 24) | (0x01 << 16) | (0x01 << 8) | (DHS_HANDSHAKE << 0));
	printf("MON_G0_0_EVENT           = 0x%x\n", read_reg(MON_G0_0_EVENT_ADR));
	write_reg(MON_G0_0_TAGID_ADR, 0x10000);
	printf("MON_G0_0_TAGID           = 0x%x\n", read_reg(MON_G0_0_TAGID_ADR));

	write_reg(MON_G0_1_EVENT_ADR,  (0x0 << 24) | (0x02 << 16) | (0x02 << 8) | (DHS_HANDSHAKE << 0));
	printf("MON_G0_1_EVENT           = 0x%x\n", read_reg(MON_G0_1_EVENT_ADR));
	write_reg(MON_G0_1_TAGID_ADR, 0x20000);
	printf("MON_G0_1_TAGID           = 0x%x\n", read_reg(MON_G0_1_TAGID_ADR));

	write_reg(MON_G0_2_EVENT_ADR, (0x0 << 24) | (0x03 << 16) | (0x03 << 8) | (DHS_HANDSHAKE << 0));
	printf("MON_G0_2_EVENT           = 0x%x\n", read_reg(MON_G0_2_EVENT_ADR));
	write_reg(MON_G0_2_TAGID_ADR, 0x30000);
	printf("MON_G0_2_TAGID           = 0x%x\n", read_reg(MON_G0_2_TAGID_ADR));

	write_reg(MON_G0_3_EVENT_ADR, (0x0 << 24) | (0x04 << 16) | (0x04 << 8) | (DHS_HANDSHAKE << 0));
	printf("MON_G0_3_EVENT           = 0x%x\n", read_reg(MON_G0_3_EVENT_ADR));
	write_reg(MON_G0_3_TAGID_ADR, 0x40000);
	printf("MON_G0_3_TAGID           = 0x%x\n", read_reg(MON_G0_3_TAGID_ADR));
/*
	write_reg(MON_G0_4_EVENT_ADR,  (0x0 << 24) | (0x04 << 16) | (0x04 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G0_4_EVENT           = 0x%x\n", read_reg(MON_G0_4_EVENT_ADR));
	write_reg(MON_G0_4_TAGID_ADR, 0x0);
	printf("MON_G0_4_TAGID           = 0x%x\n", read_reg(MON_G0_4_TAGID_ADR));

	write_reg(MON_G0_5_EVENT_ADR, (0x0 << 24) | (0x05 << 16) | (0x05 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G0_5_EVENT           = 0x%x\n", read_reg(MON_G0_5_EVENT_ADR));
	write_reg(MON_G0_5_TAGID_ADR, 0x0);
	printf("MON_G0_5_TAGID           = 0x%x\n", read_reg(MON_G0_5_TAGID_ADR));

	write_reg(MON_G0_6_EVENT_ADR, (0x0 << 24) | (0x06 << 16) | (0x06 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G0_6_EVENT           = 0x%x\n", read_reg(MON_G0_6_EVENT_ADR));
	write_reg(MON_G0_6_TAGID_ADR,  0x0);
	printf("MON_G0_6_TAGID           = 0x%x\n", read_reg(MON_G0_6_TAGID_ADR));

	write_reg(MON_G1_0_EVENT_ADR, (0x0 << 24) | (0x10 << 16) | (0x10 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G1_0_EVENT           = 0x%x\n", read_reg(MON_G1_0_EVENT_ADR));
	write_reg(MON_G1_0_TAGID_ADR, 0x0);
	printf("MON_G1_0_TAGID           = 0x%x\n", read_reg(MON_G1_0_TAGID_ADR));

	write_reg(MON_G1_1_EVENT_ADR, (0x0 << 24) | (0x11 << 16) | (0x11 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G1_1_EVENT           = 0x%x\n", read_reg(MON_G1_1_EVENT_ADR));
	write_reg(MON_G1_1_TAGID_ADR, 0x0);
	printf("MON_G1_1_TAGID           = 0x%x\n", read_reg(MON_G1_1_TAGID_ADR));

	write_reg(MON_G1_2_EVENT_ADR, (0x0 << 24) | (0x12 << 16) | (0x12 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G1_2_EVENT           = 0x%x\n", read_reg(MON_G1_2_EVENT_ADR));
	write_reg(MON_G1_2_TAGID_ADR, 0x0);
	printf("MON_G1_2_TAGID           = 0x%x\n", read_reg(MON_G1_2_TAGID_ADR));

	write_reg(MON_G1_3_EVENT_ADR, (0x0 << 24) | (0x13 << 16) | (0x13 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G1_3_EVENT           = 0x%x\n", read_reg(MON_G1_3_EVENT_ADR));
	write_reg(MON_G1_3_TAGID_ADR, 0x0);
	printf("MON_G1_3_TAGID           = 0x%x\n", read_reg(MON_G1_3_TAGID_ADR));

	write_reg(MON_G1_4_EVENT_ADR, (0x0 << 24) | (0x14 << 16) | (0x14 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G1_4_EVENT           = 0x%x\n", read_reg(MON_G1_4_EVENT_ADR));
	write_reg(MON_G1_4_TAGID_ADR, 0x0);
	printf("MON_G1_4_TAGID           = 0x%x\n", read_reg(MON_G1_4_TAGID_ADR));

	write_reg(MON_G1_5_EVENT_ADR, (0x0 << 24) | (0x15 << 16) | (0x15 << 8) | (RES_HANDSHAKE << 0));
	printf("MON_G1_5_EVENT           = 0x%x\n", read_reg(MON_G1_5_EVENT_ADR));
	write_reg(MON_G1_5_TAGID_ADR, 0x0);
	printf("MON_G1_5_TAGID           = 0x%x\n", read_reg(MON_G1_5_TAGID_ADR));
*/
	printf("========== Enable performance monitors ==========\n");
	printf("monitor0--CPU\n");
	printf("monitor1--DRAMC\n");
	printf("monitor2--ISP\n");
	printf("monitor3--XB2\n");
	/*write_reg(MON_G0_ENABLE_TRACE_ADR, (0x0 << 6) | (0x0 << 5) | (0x0 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0));*/
	write_reg(MON_G0_ENABLE_TRACE_ADR, (0x0 << 6) | (0x0 << 5) | (0x0 << 0) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0));
	printf("MON_G0_ENABLE_TRACE      = 0x%x\n", read_reg(MON_G0_ENABLE_TRACE_ADR));
/*	write_reg(MON_G1_ENABLE_TRACE_ADR, (0x0 << 6) | (0x1 << 5) | (0x1 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0));
	printf("MON_G1_ENABLE_TRACE      = 0x%x\n", read_reg(MON_G1_ENABLE_TRACE_ADR));
*/
	write_reg(MON_G0_ENABLE_ADR, (0x0 << 6) | (0x0 << 5) | (0x0 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0));
	printf("MON_G0_ENABLE            = 0x%x\n", read_reg(MON_G0_ENABLE_ADR));
/*	write_reg(MON_G1_ENABLE_ADR, (0x0 << 6) | (0x1 << 5) | (0x1 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0));
	printf("MON_G1_ENABLE            = 0x%x\n", read_reg(MON_G1_ENABLE_ADR));
*/
	write_reg(GLOBAL_MON_CTRL0_ADR, (0x0 << 24) | (0x0 << 16) | (0x0 << 8) | (0x0 << 0));
	printf("GLOBAL_MON_CTRL0         = 0x%x\n", read_reg(GLOBAL_MON_CTRL0_ADR));

	printf("MON_G0_STATUS            = 0x%x\n", read_reg(MON_G0_STATUS_ADR));
/*	printf("MON_G1_STATUS            = 0x%x\n", read_reg(MON_G1_STATUS_ADR));*/

	return 0;
}


int monitor_start(void)
{
	printf("========== sofeware start performance monitors ==========\n");
	write_reg(GLOBAL_MON_CTRL0_ADR, (0x1 << 24) | (0x0 << 16) | (0x1 << 8) | (0x0 << 0));
	printf("GLOBAL_MON_CTRL0         = 0x%x\n", read_reg(GLOBAL_MON_CTRL0_ADR));
	return 0;
}

int monitor_stop(void)
{
	printf("========== sofeware stop performance monitors ==========\n");
	write_reg(GLOBAL_MON_CTRL0_ADR, (0x0 << 24) | (0x0 << 16) | (0x0 << 8) | (0x0 << 0));
	printf("GLOBAL_MON_CTRL0         = 0x%x\n", read_reg(GLOBAL_MON_CTRL0_ADR));
	return 0;
}

int monitor_report(void)
{
	printf("========== Report status of performance monitors ==========\n");

	printf("GLOBAL_MON_CTRL0         = 0x%x\n", read_reg(GLOBAL_MON_CTRL0_ADR));

	printf("GLOBAL_MON_CTRL1         = 0x%x\n", read_reg(GLOBAL_MON_CTRL1_ADR));
	printf("MON_G0_ENABLE_TRACE      = 0x%x\n", read_reg(MON_G0_ENABLE_TRACE_ADR));

	printf("MON_G0_ENABLE            = 0x%x\n", read_reg(MON_G0_ENABLE_ADR));

	printf("MON_G0_STATUS            = 0x%x\n", read_reg(MON_G0_STATUS_ADR));

	printf("MON_G0_INTR              = 0x%x\n", read_reg(MON_G0_INTR_ADR));

	printf("========== Report counters of performance monitors ==========\n");

	printf("CPU:MON_G0_0_REG0		= 0x%x\n", read_reg(MON_G0_0_REG0_ADR));
	printf("CPU:MON_G0_0_REG1		= 0x%x\n", read_reg(MON_G0_0_REG1_ADR));

	printf("DRAMC:MON_G0_1_REG0		= 0x%x\n", read_reg(MON_G0_1_REG0_ADR));
	printf("DRAMC:MON_G0_1_REG1		= 0x%x\n", read_reg(MON_G0_1_REG1_ADR));

	printf("ISP:MON_G0_2_REG0		= 0x%x\n", read_reg(MON_G0_2_REG0_ADR));
	printf("ISP:MON_G0_2_REG1		= 0x%x\n", read_reg(MON_G0_2_REG1_ADR));

	printf("XB2:MON_G0_3_REG0		= 0x%x\n", read_reg(MON_G0_3_REG0_ADR));
	printf("XB2:MON_G0_3_REG1		= 0x%x\n", read_reg(MON_G0_3_REG1_ADR));

	return 0;
}

int getweight_inword(u32 value, u32 a[])
{
	u32 i, tmp;
	for (i = 0; i < 8; i++)
	{
		tmp = value;
		tmp &= 0xF;
		a[tmp] += 1;
		value >>= 4;
	}
}

int arbiter_report(void)
{
	u32 XB0[12] = {0};
	u32 XB1[7] = {0};

	getweight_inword((read_reg(ARB_XB0_CTRL0_ADDR)), XB0);
	getweight_inword(read_reg(ARB_XB0_CTRL1_ADDR), XB0);
	getweight_inword(read_reg(ARB_XB0_CTRL2_ADDR), XB0);
	getweight_inword(read_reg(ARB_XB0_CTRL3_ADDR), XB0);
	printf("=====Arbiter XB0 setting=====\n");
	printf("%x, %x, %x, %x\n", read_reg(ARB_XB0_CTRL0_ADDR), \
	read_reg(ARB_XB0_CTRL1_ADDR), read_reg(ARB_XB0_CTRL2_ADDR),\
	read_reg(ARB_XB0_CTRL3_ADDR));
	printf("weight is\t0:XB0_H264: %x\n", XB0[0]);
	printf("\t\t1:XB0_CPU: %d\n", XB0[1]);
	printf("\t\t2:XB0_ISP: %d\n", XB0[2]);
	printf("\t\t3:XB1_ETH: %d\n", XB0[3]);
	printf("\t\t4:XB1_AES: %d\n", XB0[4]);
	printf("\t\t5:XB1_Audio: %d\n", XB0[5]);
	printf("\t\t6:XB1_SDIO: %d\n", XB0[6]);
	printf("\t\t7:XB1_EHCI: %d\n", XB0[7]);
	printf("\t\t8:XB1_OTG: %d\n", XB0[8]);
	printf("\t\t9:XB1_OHCI: %d\n", XB0[9]);
	printf("\t\ta:XB0_DMA: %d\n", XB0[10]);
	printf("\t\tb:XB0_MCU: %d\n", XB0[11]);

	getweight_inword(read_reg(ARB_XB1_CTRL0_ADDR), XB1);
	getweight_inword(read_reg(ARB_XB1_CTRL1_ADDR), XB1);
	printf("=====Arbiter XB1 setting=====\n");
	printf("%x, %x\n", read_reg(ARB_XB1_CTRL0_ADDR), read_reg(ARB_XB1_CTRL1_ADDR));
	printf("weight is\t0:XB1_Audio: %d\n", (XB1[0]-4));
	printf("\t\t1:XB1_SDIO: %d\n", XB1[1]);
	printf("\t\t2:XB1_ETH: %d\n", XB1[2]);
	printf("\t\t3:XB1_EHCI: %d\n", XB1[3]);
	printf("\t\t4:XB1_OTG: %d\n", XB1[4]);
	printf("\t\t5:XB1_AES: %d\n", XB1[5]);
	printf("\t\t6:XB1_OHCI: %d\n", XB1[6]);

	return 0;
}

int main(int argc, char * const argv[])
{
	const char *cmd;
	int ret;

	/* need at least two arguments */
	if (argc < 1)
		printf("cmd wrong\n");

	cmd = argv[1];
	--argc;
	++argv;

	if (strcmp(cmd, "init") == 0) {
		ret = monitor_init();
		goto done;
	}

	if (strcmp(cmd, "start") == 0) {
		ret = monitor_start();
		goto done;
	}

	if (strcmp(cmd, "stop") == 0) {
		ret = monitor_stop();
		goto done;
	}

	if (strcmp(cmd, "report") == 0) {
		ret = monitor_report();
		goto done;
	}

	if (strcmp(cmd, "getarb") == 0) {
		ret = arbiter_report();
		goto done;
	}

done:
	 return 0;
}




