/*
 * Realtek Semiconductor Corp.
 *
 * bsp/gpio.h:
 *
 * Copyright (C) 2014 Wei WANG (wei_wang@realsil.com.cn)
 */
#ifndef _BSP_GPIO_H
#define _BSP_GPIO_H

#define ARCH_NR_GPIOS	(23)

enum rts_pinconf_pull {
	RTS_PINCONFIG_PULL_NONE,
	RTS_PINCONFIG_PULL_DOWN,
	RTS_PINCONFIG_PULL_UP,
	RTS_PIN_CONFIG_DRIVE_STRENGTH,
	RTS_PIN_CONFIG_SLEW_RATE,
	RTS_PINCONFIG_INPUT,
	RTS_PINCONFIG_OUTPUT,
};

#define PIN_CONF_PACKED(p, a) ((a << 16) | ((unsigned long) p & 0xffffUL))

static inline enum rts_pinconf_pull pinconf_to_config_param(
	unsigned long config)
{
	return (enum rts_pinconf_pull) (config & 0xffffUL);
}

static inline unsigned short pinconf_to_config_argument(
	unsigned long config)
{
	return (enum rts_pinconf_pull) ((config >> 16) & 0xffffUL);
}

static inline unsigned long pinconf_to_config_packed(
	enum rts_pinconf_pull param, unsigned short argument)
{
	return PIN_CONF_PACKED(param, argument);
}

#ifdef CONFIG_GPIOLIB
#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep
#define gpio_to_irq __gpio_to_irq
#else
int gpio_request(unsigned gpio, const char *label);
void gpio_free(unsigned gpio);
int gpio_direction_input(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
void gpio_set_value(unsigned gpio, int value);
#endif
int gpio_to_irq(unsigned gpio);
int irq_to_gpio(unsigned irq);

#include <asm-generic/gpio.h>		/* cansleep wrappers */


#endif /* _BSP_GPIO_H */
