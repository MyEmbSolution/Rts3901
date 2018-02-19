#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_sc.h>

#define NETLINK_TEST 17

static struct sock *netlinkfd;

struct {
	int pid;
} user_process;

struct {
	unsigned int	sc_debug;
	unsigned int	sc_status;
	unsigned int	sc_start_time;
	unsigned int	sc_pass_time;
	unsigned int	sc_startlen;
	unsigned int	sc_stoplen;
	unsigned int	sc_control_ip;
	unsigned char	sc_control_mac[6];
} rtk_sc_ctx;

void super_link_init(union recv_frame *precv_frame)
{
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib *pattrib = &precv_frame->u.hdr.attrib;

	rtk_sc_ctx.sc_status = 1;
	rtk_sc_ctx.sc_startlen = pattrib->pkt_len;
	rtk_sc_ctx.sc_stoplen = pattrib->pkt_len + 0x42;
	rtk_sc_ctx.sc_start_time = rtw_get_current_time();
	if (rtk_sc_ctx.sc_debug == 1) {
		printk("sc_status = 1, start simple config!\r\n");
		printk("startLen = %d, stopLen = %d\r\n",
			rtk_sc_ctx.sc_startlen, rtk_sc_ctx.sc_stoplen);
	}
}

void rtl8192cu_super_link(_adapter *adapter, union recv_frame *precv_frame)
{
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib *pattrib = &precv_frame->u.hdr.attrib;
	u8 appaddr[6] = {0x01, 0x00, 0x5e, 0x7f, 0xff, 0xff};
	static u8 buff[0xa0];
	int i = 0;
	char info[200] = {0x00};
	u16 framelen = pattrib->pkt_len;

	if (_rtw_memcmp(appaddr, pattrib->dst, 4)) {
		if (_rtw_memcmp(appaddr, pattrib->dst, 6) &&
			(rtk_sc_ctx.sc_status == 0)) {
			if (rtk_sc_ctx.sc_debug == 1)
				printk("Qos = %02x, framelen = %02x \r\n",
					ptr[0], framelen);
			if (((ptr[0]&0x80) && (framelen == 0x52)) ||
				((!(ptr[0]&0x80)) && (framelen == 0x50))) {
				memset(buff, 0x00, 0xa0);
				buff[0] = 0x30;
				buff[1] = 0x0d;
				super_link_init(precv_frame);
			} else if (((ptr[0]&0x80) && (framelen == 0x5a)) ||
				((!(ptr[0]&0x80)) && (framelen == 0x58))) {
				memset(buff, 0x00, 0xa0);
				if (ptr[0] & 0x80)
					buff[0] = 0x40 + (ptr[29]>>6 & 0x03);
				else
					buff[0] = 0x40 + (ptr[27]>>6 & 0x03);
				buff[1] = 0x0d;
				super_link_init(precv_frame);
			} else if (((ptr[0]&0x80) && (framelen == 0x66)) ||
				((!(ptr[0]&0x80)) && (framelen == 0x64))) {
				memset(buff, 0x00, 0xa0);
				buff[0] = 0x50;
				buff[1] = 0x0d;
				super_link_init(precv_frame);
			}

			if ((pattrib->pkt_len == rtk_sc_ctx.sc_stoplen)
				&& (rtk_sc_ctx.sc_status > 0)) {
				rtk_sc_ctx.sc_status = 0;
				if (rtk_sc_ctx.sc_debug == 1)
					printk("sc_status = 0, stop simple config!\r\n");
			}

		}

		if ((pattrib->pkt_len > rtk_sc_ctx.sc_startlen)
			&& (pattrib->pkt_len < rtk_sc_ctx.sc_stoplen)
			&& (rtk_sc_ctx.sc_status == 1)) {
			buff[(pattrib->pkt_len - rtk_sc_ctx.sc_startlen)*2] = pattrib->dst[4];
			buff[(pattrib->pkt_len - rtk_sc_ctx.sc_startlen)*2+1] = pattrib->dst[5];
			if (rtw_checksum((u16 *)(buff))) {
				rtk_sc_ctx.sc_status = 2;
				if (rtk_sc_ctx.sc_debug == 1) {
					for (i = 0; i < 0xa0; i++) {
						printk("0x%02x ", buff[i]);
						if ((i+1)%8 == 0)
							printk("\r\n");
					}
					printk("\r\n");
					printk("sc_status = 2, SoftApp start config wifi! -->passtime = %dms\r\n",
						rtw_get_passing_time_ms(rtk_sc_ctx.sc_start_time));
				}
				rtw_sc_send((char *)buff);
			}
		}

	}
}

int check_superlink_state(void)
{
	if (rtk_sc_ctx.sc_status > 0) {
		if (rtw_get_passing_time_ms(rtk_sc_ctx.sc_start_time) >= 30000) {
			if (rtk_sc_ctx.sc_debug == 1) {
				if (rtk_sc_ctx.sc_status < 2)
					printk("simple config timeout, set sc_status = 0!\r\n");
				else
					printk("simple config success, set sc_status = 0!\r\n");
			}
			rtk_sc_ctx.sc_status = 0;
		}
	}
	return 0;
}

int rtw_checksum(u16 *buff)
{
	int i = 0;
	int checksum = 0;

	for (i = 1; i < 0x41; i++)
		checksum += buff[i];
	checksum = (checksum>>16)+(checksum & 0xffff);
	checksum = 0xffff-checksum;
	if (rtk_sc_ctx.sc_debug == 1)
		printk("rx checksum = %x, buff[0x41] = %x\n",
			checksum, buff[0x41]);
	if (checksum == buff[0x41])
		return 1;
	else
		return 0;
}

int rtw_sc_send(char *info)
{
	int size, retval;
	struct sk_buff *skb;
	unsigned char *old_tail;
	struct nlmsghdr *nlh;

	size = NLMSG_SPACE(strlen(info));
	skb = alloc_skb(size, GFP_ATOMIC);
	nlh = nlmsg_put(skb, 0, 0, 0, NLMSG_SPACE(strlen(info))-sizeof(struct nlmsghdr), 0);
	old_tail = skb->tail;
	memcpy(NLMSG_DATA(nlh), info, strlen(info));
	nlh->nlmsg_len = skb->tail - old_tail;
	retval = netlink_unicast(netlinkfd, skb, user_process.pid, MSG_DONTWAIT);
	return 0;
}

void rtw_sc_receive(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh = NULL;
	char *data = "rtw_sc test message from kernel";

	skb = skb_get(__skb);
	if (skb->len >= sizeof(struct nlmsghdr)) {
		nlh = (struct nlmsghdr *)skb->data;
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr))
			&& (__skb->len >= nlh->nlmsg_len)) {
			user_process.pid = nlh->nlmsg_pid;
			if (rtk_sc_ctx.sc_debug == 1)
				printk("[kernel space] user_pid:%d\n", user_process.pid);
		}
	} else {
		if (rtk_sc_ctx.sc_debug == 1)
			printk("[kernel space] data receive from user are:%s\n",
				(char *)NLMSG_DATA(nlmsg_hdr(__skb)));
		}
		kfree_skb(skb);
}

int rtw_sc_init(void)
{
	struct netlink_kernel_cfg nlcfg = {
		.input = rtw_sc_receive,
	};

	netlinkfd = netlink_kernel_create(&init_net, NETLINK_TEST, &nlcfg);
	if (!netlinkfd) {
		printk(KERN_ERR "can not create a netlink socket\n");
		return -1;
	}
	rtk_sc_ctx.sc_debug = SC_DEBUG;
	rtk_sc_ctx.sc_status = 0;
	if (rtk_sc_ctx.sc_debug == 1)
		printk("Enable Realtek WiFi Super_Link\n");
	return 0;
}

void rtw_sc_exit(void)
{
	if (netlinkfd)
		sock_release(netlinkfd->sk_socket);
	if (rtk_sc_ctx.sc_debug == 1)
		printk("Quit Realtek WiFi Super_Link\n");
}

