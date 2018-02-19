/*
 * Realtek Semiconductor Corp.
 *
 * ../../../../include/linux/platform_data/camera-rtsoc.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _PLATFORM_DATA_CAMERA_RTS_SOC_H
#define _PLATFORM_DATA_CAMERA_RTS_SOC_H

enum mcu_lock_status {
	MCU_LCK_STS_OK = 0,
	MCU_LCK_STS_ERROR = 1,
	MCU_LCK_STS_NO_MCU_DEV = 2,
	MCU_LCK_STS_RESERVED
};

#define RTSCAM_SOC_CAP_FLAG_TDNR	(1<<0)
#define RTSCAM_SOC_CAP_FLAG_LDC		(1<<1)
#define RTSCAM_SOC_CAP_FLAG_DEHAZE	(1<<2)

struct rtscam_soc_pdata {
	u32 capibility;
	phys_addr_t resvd_mem_base;
	size_t resvd_mem_size;
};

#define RTS_MCU_GPIO_NUM			8

enum {
	RTS_MCU_GPIO_IDLE = 0,
	RTS_MCU_GPIO_INUSE = 1
};

enum {
	RTS_MCU_GPIO_INPUT = 0,
	RTS_MCU_GPIO_OUTPUT = 1
};

enum {
	RTS_MCU_GPIO_LOW = 0,
	RTS_MCU_GPIO_HIGH = 1
};

int rtscam_soc_request_mcu_gpio(int gpio);
int rtscam_soc_free_mcu_gpio(int gpio);
int rtscam_soc_set_mcu_gpio_direction(int gpio, int dir);
int rtscam_soc_get_mcu_gpio_direction(int gpio);
int rtscam_soc_set_mcu_gpio_value(int gpio, int val);
int rtscam_soc_get_mcu_gpio_value(int gpio);

#endif
