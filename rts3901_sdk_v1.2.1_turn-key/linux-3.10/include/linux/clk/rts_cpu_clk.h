#ifndef __LINUX_RTS_CPU_CLK_H_
#define __LINUX_RTS_CPU_CLK_H_

#include <linux/clk.h>

int rts_set_cpu_clk(struct clk *rts_cpu_clk, unsigned long rate);

#endif
