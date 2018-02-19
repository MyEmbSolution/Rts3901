#ifndef _RTW_SC_H_
#define _RTW_SC_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/netlink.h>


#define SC_DEBUG 0

void super_link_init(union recv_frame *precv_frame);
void rtl8192cu_super_link(_adapter *adapter, union recv_frame *precv_frame);
int check_superlink_state(void);
int rtw_checksum(u16 *buff);
int rtw_sc_send(char *info);
void rtw_sc_receive(struct sk_buff *__skb);
int rtw_sc_init(void);
void rtw_sc_exit(void);


#endif
