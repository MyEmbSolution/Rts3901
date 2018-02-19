/*
 * Realtek Semiconductor Corp.
 *
 * Copyright 2012  Jethro Hsu (jethro@realtek.com)
 * Copyright 2012  Tony Wu (tonywu@realtek.com)
 */

#include <config.h>
#include <common.h>
#include <serial.h>

#include <linux/compiler.h>

#include "bspchip.h"

DECLARE_GLOBAL_DATA_PTR;

/* Uart baudrate */
#define UART_BAUD_RATEL		(UART_BAUD_DEFAULT & 0x00ff)
#define UART_BAUD_RATEH		((UART_BAUD_DEFAULT & 0xff00) >> 8)
#define UART_IOBASE		UART1_IOBASE

/******************************************************************************
*
* dwc_serial_init - initialize a channel
*
* This routine initializes the number of data bits, parity
* and set the selected baud rate. Interrupts are disabled.
* Set the modem control signals if the option is selected.
*
* RETURNS: N/A
*/

static int dwc_serial_init (void)
{
	REG32(UART_IOBASE+UART_IER) = 0;

	REG32(UART_IOBASE+UART_LCR) = UART_LCR_DLAB;
	REG32(UART_IOBASE+UART_DLL) = UART_BAUD_RATEL;
	REG32(UART_IOBASE+UART_DLH) = UART_BAUD_RATEH;

	REG32(UART_IOBASE+UART_LCR) = UART_LCR_DLS;

	REG32(XB2_UART1_PULL_UP_CTRL) |= 0x8; /*RX pull up enable*/
	REG32(XB2_UART1_PULL_DOWN_CTRL) &= (~0x4); /*RX pull down disable*/

	if (REG32(UART_IOBASE+UART_CTR) == 0x44570110) {
		REG32(UART_CLK_CFG_REG) = CLK_ENABLE|CLOCK_SELECT_USB_PLL_5|CLOCK_SELECT_DIV4;
		REG32(UART_TX_EN_REG) = UART1_TX_EN;
	}

	return 0;
}


static void dwc_serial_setbrg (void)
{
	/* Not implemented yet */
	return;
}

static void dwc_serial_putc (const char c)
{
	volatile u32 status = 0;

	while (!status) {
		status = REG32(UART_IOBASE+UART_LSR) & 0x00000020;
	}

	REG32(UART_IOBASE+UART_THR) = c;
	if (c == '\n') {
		status = 0;
		while (!status) {
			status = REG32(UART_IOBASE+UART_LSR) & 0x00000020;
		}
		REG32(UART_IOBASE+UART_THR) = '\r';
	}
}

static void dwc_serial_puts (const char *s)
{
	while (*s) {
		dwc_serial_putc(*s++);
	}
}

static int dwc_serial_getc (void)
{
	volatile u32 status = 0;
	u32 ret;

	while (!status) {
		status = REG32(UART_IOBASE+UART_LSR) & 0x00000001;
	}

	ret = REG32(UART_IOBASE+UART_RBR);
	return (char)ret;
}

static int dwc_serial_tstc (void)
{
	int ret;

	ret = (REG32(UART_IOBASE+UART_LSR) & 0x00000001) ? 1:0;
	return ret;
}

static struct serial_device rlxboard_serial_drv = {
	.name   = "rlxboard_serial",
	.start  = dwc_serial_init,
	.stop   = NULL,
	.setbrg = dwc_serial_setbrg,
	.putc   = dwc_serial_putc,
	.puts   = dwc_serial_puts,
	.getc   = dwc_serial_getc,
	.tstc   = dwc_serial_tstc,
};

void rlxboard_serial_initialize(void)
{
	serial_register(&rlxboard_serial_drv);
}

struct serial_device *default_serial_console(void)
{
	return &rlxboard_serial_drv;
}
