#ifndef _XB2_H_
#define _XB2_H_

#define MDIO_RD_DONE_INT_IRQ 10
#define MDIO_WR_DONE_INT_IRQ 9
#define SARADC_DONE_INT_IRQ 8
#define RTC_ALARM3_INT_IRQ 7
#define RTC_ALARM2_INT_IRQ 6
#define RTC_ALARM1_INT_IRQ 5
#define RTC_ALARM0_INT_IRQ 4
#define PWM3_DONE_INT_IRQ 3
#define PWM2_DONE_INT_IRQ 2
#define PWM1_DONE_INT_IRQ 1
#define PWM0_DONE_INT_IRQ 0

extern int rts_xb2_to_irq(unsigned offset);
extern int rts_xb2_uart_cw(unsigned int offset, unsigned int value);
extern int rts_xb2_uart_cr(unsigned int offset, unsigned int *value);

#endif
