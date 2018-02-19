/*
 * Realtek Semiconductor Corp.
 *
 * Jim Cao (jim_cao@realsil.com.cn)
 * July. 6, 2015
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sys/signal.h>
#include <ctype.h>
#include "data_rand.h"

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define IP_HDR_LEN (sizeof(struct ip))
#define ETH_HDR_LEN (sizeof(struct ether_header))
#define MAXBUF 2048
#define BUFNUM 1
struct eth_data {
	int sockfd;
	int ifindex;
	unsigned char mac[6];
};

static volatile int exiting;
static volatile int status_snapshot;
sem_t full_sem;
pthread_spinlock_t spinlock;
char buf_recv[MAXBUF];
char *packet_buf[100];
char mac_ip_header[20 + 14];
long long int test_fail;
long long int test_recv;
long long int test_xmit;
long long int last_recv;
long long int cur_recv;
int queue;

static void finish(void)
{
	putchar('\n');
	fflush(stdout);
	printf("--- loop test statistics ---\n");
	sleep(1);
	printf("%lld packets transmitted, ", test_xmit);
	printf("%lld received, ", test_recv);
	printf("%lld failed", test_fail);
	putchar('\n');
	exit(1);
}

static void status(void)
{
	status_snapshot = 0;
	fprintf(stderr, "\r%lld/%lld packets, %lld lost, %lld fail\n",
		test_xmit, test_recv, test_xmit - test_recv, test_fail);
}

static inline void set_signal(int signo, void (*handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));

	sa.sa_handler = (void (*)(int))handler;
#ifdef SA_INTERRUPT
	sa.sa_flags = SA_INTERRUPT;
#endif
	sigaction(signo, &sa, NULL);
}


static void sigexit(int signo)
{
	exiting = 1;
}

static void sigstatus(int signo)
{
	status_snapshot = 1;
}

static void sigalrm(int signo)
{
	last_recv = cur_recv;
	cur_recv = test_recv;
	if (cur_recv - last_recv == 0) {
		status();
		if (sem_init(&full_sem, 0, queue) != 0) {
			fprintf(stderr, "sem_init failed!\n");
			exit(1);
		}
	}
}

static void print_help(void)
{
	fprintf(stdout, "Usage: eth_looptest   [-i interface] [-q queuenum] [-d sec]...\n");
	fprintf(stdout, "    -i interface  [e.g. eth0, eth1....]\n");
	fprintf(stdout, "    -q queuenum   [typically 20]\n");
	fprintf(stdout, "    -d sec        [print log per 'sec' seconds]\n");
}
int set_timer(int sec)
{
	struct itimerval it;

	it.it_interval.tv_sec = sec;
	it.it_interval.tv_usec = 0;
	it.it_value.tv_sec = sec;
	it.it_value.tv_usec = 0;
	return setitimer(ITIMER_REAL, &it, NULL);
}



int svr_sock_create_bind(char *eth_if, struct eth_data *data)
{
	struct sockaddr_ll sock_addr;
	struct ifreq ifr;
	int sockfd = data->sockfd;

	if (NULL == eth_if || NULL == data) {
		fprintf(stderr, "interface error\n");
		exit(1);
	}
	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if (-1 == sockfd) {
		perror("Could not create a socket!\n");
		exit(1);
	}

	memset(&sock_addr, 0, sizeof(struct sockaddr_ll));
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, eth_if);

	/* get eth if index */
	if (-1 == ioctl(sockfd, SIOCGIFINDEX, &ifr)) {
		perror("Get dev index error!\n");
		exit(1);
	}
	/* bind to interface */
	sock_addr.sll_family = AF_PACKET;
	sock_addr.sll_protocol = htons(ETH_P_IP);
	sock_addr.sll_ifindex = ifr.ifr_ifindex;
	data->ifindex = ifr.ifr_ifindex;
	if (-1 == bind(sockfd, (struct sockaddr *)&sock_addr,
		sizeof(sock_addr))) {
		perror("Bind to interface failed!");
		exit(1);
	}

	/* get eth mac addr */
	if (-1 == ioctl(sockfd, SIOCGIFHWADDR, &ifr)) {
		fprintf(stderr, "Get mac addr error\n");
		exit(1);
	}
	memcpy(data->mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	data->sockfd = sockfd;
	return sockfd;
}

int cli_sock_create_bind(char *eth_if, struct eth_data *data)
{
	struct sockaddr_ll sock_addr;
	struct ifreq ifr;
	int sockfd;

	if (NULL == eth_if || NULL == data) {
		fprintf(stderr, "interface error!\n");
		exit(1);
	}

	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if (-1 == sockfd) {
		perror("Could not create a socket!\n");
		exit(1);
	}

	memset(&sock_addr, 0, sizeof(struct sockaddr_ll));
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, eth_if);

	/* get eth if index */
	if (-1 == ioctl(sockfd, SIOCGIFINDEX, &ifr)) {
		perror("Get dev index error!\n");
		exit(1);
	}
	/* bind to interface */
	sock_addr.sll_family = AF_PACKET;
	sock_addr.sll_protocol = htons(ETH_P_IP);
	sock_addr.sll_ifindex = ifr.ifr_ifindex;
	data->ifindex = ifr.ifr_ifindex;
	if (-1 == bind(sockfd, (struct sockaddr *)&sock_addr,
		sizeof(sock_addr))) {
		perror("Bind to interface failed!");
		exit(1);
	}

	/* get eth mac addr */
	if (-1 == ioctl(sockfd, SIOCGIFHWADDR, &ifr)) {
		fprintf(stderr, "Get mac addr error\n");
		exit(1);
	}
	memcpy(data->mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	data->sockfd = sockfd;
	return sockfd;
}

int svr_sock_recv(struct eth_data *pdata, char *buf, int len)
{
	return recvfrom(pdata->sockfd, buf, len, 0, NULL, NULL);
}
void gen_mac_ip_header(struct eth_data *pdata, char *buf, int data_len)
{
	struct ether_header *eth;
	struct ip *ip;

	if (NULL == pdata || NULL == buf) {
		fprintf(stderr, "invalid pointer!\n");
		exit(1);
	}

	/* fill in mac header */
	eth = (struct ether_header *)buf;
	memcpy(eth->ether_dhost, pdata->mac, ETH_ALEN);
	memcpy(eth->ether_shost, pdata->mac, ETH_ALEN);
	eth->ether_type = htons(ETHERTYPE_IP);

	/* fill in ip header */
	ip = (struct ip *) (buf + ETH_HDR_LEN);
	ip->ip_v = IPVERSION;
	ip->ip_hl = IP_HDR_LEN >> 2;
	ip->ip_tos = 0;
	ip->ip_len = htons(IP_HDR_LEN + data_len);
	ip->ip_id = 0;
	ip->ip_off = 0;
	ip->ip_ttl = MAXTTL;
	ip->ip_p = IPPROTO_RAW;
	ip->ip_sum = 0;
	ip->ip_dst.s_addr = inet_addr("192.168.0.101");
	ip->ip_src.s_addr = inet_addr("192.168.0.123");
}

int send_packet(struct eth_data *pdata, char *buf, int len)
{
	struct sockaddr_ll sock_addr;

	if (NULL == buf) {
		fprintf(stderr, "buf is empty!\n");
		exit(1);
	}

	memset(&sock_addr, 0, sizeof(struct sockaddr_ll));
	sock_addr.sll_ifindex = pdata->ifindex;
	sock_addr.sll_family = AF_PACKET;

	return sendto(pdata->sockfd, buf, len, 0,
			(struct sockaddr *)&sock_addr, sizeof(sock_addr));
}

void *udp_xmit(void *ptr)
{
	int index;
	struct eth_data *cli_data = ptr;
	const int len = IP_HDR_LEN +
		ETH_HDR_LEN + sizeof(random_data[0]);

	while (1) {
		int ret;
		/* replace sem_wait to improve performance */
		while (sem_trywait(&full_sem) != 0)
			usleep(100);

		pthread_spin_lock(&spinlock);
		index = test_xmit++ % 100;
		pthread_spin_unlock(&spinlock);

		ret = send_packet(cli_data, packet_buf[index], len);
		if (unlikely(ret == -1)) {
			perror("Could not send msg!\n");
			exit(1);
		} else if (unlikely(ret != len)) {
			fprintf(stderr, "Socket buffer is too small!\n");
			exit(1);
		}
		if (exiting)
			break;
	}
	return NULL;
}

void *udp_recv(void *ptr)
{
	struct eth_data *svr_data = ptr;
	int len_recv;
	int index;
	const int len = IP_HDR_LEN + ETH_HDR_LEN + sizeof(random_data[0]);

	while (1) {
		len_recv = svr_sock_recv(svr_data, buf_recv, MAXBUF);
		if (unlikely(len_recv == -1)) {
			perror("Can't receive packets!\n");
			exit(1);
		}

		test_recv++;

		index = ntohs(((struct ip *)(buf_recv +
			ETH_HDR_LEN))->ip_id);
		if (unlikely(len_recv != len) ||
			unlikely(memcmp(buf_recv,
				packet_buf[index], len_recv) != 0)) {
			test_fail++;
			printf("Fail! SENT:%lld, RECV:%lld, Fail:%lld, packlen:%d\n",
				test_xmit, test_recv, test_fail, len_recv);
			exit(1);
		}
		sem_post(&full_sem);
		if (status_snapshot)
			status();
	}
}

int main(int argc, char *argv[])
{
	struct eth_data cli_data, svr_data;
	pthread_t thread_xmit, thread_recv, thread_xmit_copy;
	int ret, i;
	char *eth_if = NULL;
	int debug = 0;
	int interval;

	while ((ret = getopt(argc, argv, ":i:q:d:")) != -1) {
		switch (ret) {
		case 'i':
			eth_if = optarg;
			break;
		case 'q':
			queue = atoi(optarg);
			break;
		case 'd':
			debug = 1;
			interval = atoi(optarg);
			break;
		case '?':
			fprintf(stderr, "Illegal option:-%c\n",
				isprint(optopt) ? optopt : '#');
			print_help();
			exit(EXIT_FAILURE);
			break;
		default:
			fprintf(stderr, "Not supported Option\n");
			print_help();
			exit(EXIT_FAILURE);
			break;
		}
	}
	/* init control var */
	status_snapshot = 0;
	exiting = 0;
	test_fail = 0;
	test_recv = 0;
	test_xmit = 0;
	last_recv = 0;
	cur_recv = 0;

	if (eth_if == NULL || queue == 0) {
		print_help();
		exit(1);
	}

	if (sem_init(&full_sem, 0, queue) != 0)	{
		fprintf(stderr, "sem_init failed!\n");
		return -1;
	}

	set_signal(SIGINT, sigexit);
	if (debug) {
		if (set_timer(interval) != 0) {
			fprintf(stderr, "Set timer error!\n");
			exit(1);
		}
		set_signal(SIGALRM, sigalrm);
	}
	set_signal(SIGQUIT, sigstatus);

	printf("Press Ctrl + '\\' to see statistics\n");
	printf("Press Ctrl + 'c' to interrupt loop test\n");
	printf("Ethernet Loopback test will be interrupted as long as error detected!\n");

	cli_sock_create_bind(eth_if, &cli_data);
	svr_sock_create_bind(eth_if, &svr_data);
	gen_mac_ip_header(&cli_data, mac_ip_header, sizeof(random_data[i]));

	for (i = 0; i < 100; i++) {
		packet_buf[i] = malloc(sizeof(random_data[i]) +
			IP_HDR_LEN + ETH_HDR_LEN);
		if (packet_buf[i] == NULL) {
			fprintf(stderr, "buffer allocat failed!\n");
			exit(1);
		}
		memcpy(packet_buf[i], mac_ip_header, IP_HDR_LEN + ETH_HDR_LEN);
		memcpy(packet_buf[i] + IP_HDR_LEN + ETH_HDR_LEN,
			random_data[i], sizeof(random_data[i]));
		((struct ip *)(packet_buf[i] + ETH_HDR_LEN))->ip_id = htons(i);
	}




	ret = pthread_create(&thread_recv, NULL, udp_recv, &svr_data);
	if (ret) {
		fprintf(stderr, "thread_recv create error!\n");
		exit(1);
	}

	/* wait for udp server start */
	sleep(1);

	ret = pthread_create(&thread_xmit, NULL, udp_xmit, &cli_data);
	if (ret) {
		fprintf(stderr, "thread_xmit create error!\n");
		exit(1);
	}

	ret = pthread_create(&thread_xmit_copy, NULL, udp_xmit, &cli_data);
	if (ret) {
		fprintf(stderr, "thread_xmit create error!\n");
		exit(1);
	}

	pthread_join(thread_xmit, NULL);
	pthread_join(thread_xmit_copy, NULL);
	finish();
	pthread_join(thread_recv, NULL);
	exit(EXIT_SUCCESS);
}

