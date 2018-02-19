#ifndef _FT2_TEST_ETH_H
#define _FT2_TEST_ETH_H

#define _PATH_ETH0_OPERSTATE	"/sys/class/net/eth0/operstate"
#define _PATH_SYSNET_SPEED	"/sys/class/net/eth0/speed"
#define MODE_10			0
#define MODE_100		1
#define MODE_UNKNOWN	2


int set_ip_address(char *ipaddr);
int check_eth_speed();

#endif
