/*
 * gpiolib support for Wolfson WM831x PMICs
 *
 * Copyright 2009 Wolfson Microelectronics PLC.
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/mfd/core.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <net/sock.h>
#include <linux/netlink.h>

#define WPS_NETLINK 18
#define WPS_GPIO_NUM	CONFIG_WPS_GPIO_NUM
static struct sock *netlinkfd;

struct {
	int pid;
} user_process;

static int netlink_send(char *info)
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

	return retval;
}

static void netlink_receive(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh = NULL;

	skb = skb_get(__skb);
	if (skb->len >= sizeof(struct nlmsghdr)) {
		nlh = (struct nlmsghdr *)skb->data;
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr))
			&& (__skb->len >= nlh->nlmsg_len))
			user_process.pid = nlh->nlmsg_pid;
	}
	kfree_skb(skb);
}

static int netlink_init(void)
{
	struct netlink_kernel_cfg nlcfg = {
		.input = netlink_receive,
	};

	netlinkfd = netlink_kernel_create(&init_net, WPS_NETLINK, &nlcfg);
	if (!netlinkfd) {
		pr_err("can not create a netlink socket\n");
		return -1;
	}

	return 0;
}

static int netlink_exit(void)
{
	if (netlinkfd)
		sock_release(netlinkfd->sk_socket);

	return 0;
}

static irqreturn_t  wps_gpio_handler(int irq, void *dev_id)
{
	int ret = 0;
	char *info = "wps_pbc";

	if (netlinkfd)
		ret = netlink_send(info);
	if (ret <= 0)
		pr_err("wps-pbc signal send fail\n");

	return IRQ_HANDLED;
}

static int __init wps_gpio_init(void)
{
	int ret;

	gpio_request(WPS_GPIO_NUM, "wps-gpio");
	gpio_direction_input(WPS_GPIO_NUM);
	ret = request_irq(gpio_to_irq(WPS_GPIO_NUM), wps_gpio_handler,
		IRQF_TRIGGER_RISING, "wps-gpio", NULL);
	if (ret < 0)
		pr_err("wps-gpio req irq fail\n");

	ret = netlink_init();
	if (ret < 0)
		pr_err("netlink socket init fail\n");

	return ret;
}

static void __exit wps_gpio_exit(void)
{
	free_irq(gpio_to_irq(WPS_GPIO_NUM), NULL);
	gpio_free(WPS_GPIO_NUM);
	netlink_exit();
}

module_init(wps_gpio_init);
module_exit(wps_gpio_exit);
MODULE_LICENSE("GPL");
