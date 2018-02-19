#include <common.h>
#include <net.h>

#define GETMEM(addr)	(*(volatile u32 *)(addr))
#define XMK_STR(x) #x
#define MK_STR(x) XMK_STR(x)
#define ETH_MACADDR_ID	0x1000001
#define ETH_IPADDR_ID	0x1000002
#define ETH_GATEWAY_ID	0x1000003
#define ETH_NETMASK_ID	0x1000004

int set_eth_MACADDR(u32 offset, u32 length)
{
	char ethaddr[20];
	u32 data0 = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset);
	u32 data1 = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset + 4);
	u8 mac[6];
	mac[0] = data0 & 0xff;
	mac[1] = (data0 & 0xff00) >> 8;
	mac[2] = (data0 & 0xff0000) >> 16;
	mac[3] = (data0 & 0xff000000) >> 24;
	mac[4] = data1 & 0xff;
	mac[5] = (data1 & 0xff00) >> 8;

	sprintf(ethaddr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	printf("set ethaddr = %s\n", ethaddr);
	setenv("ethaddr", ethaddr);

	return 0;
}

int config_entry_parse_ethaddr(u32 offset)
{
	u32 entry_id = 0;
	u32 entry_len = 0;
	entry_id = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset);
	entry_len = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset + 4);

	if (entry_id != ETH_MACADDR_ID)
		return -1;
	else
		set_eth_MACADDR((offset+8), entry_len);

	return 0;
}

int hw_config_parse_ethaddr(void)
{
	int res = -1;
	u32 total_len = 0;
	u32 offset = 12;
	u32 magic_num;
	u32 tmp;

	/*printf("enter hw_config_parse_ethaddr\n");*/
	magic_num = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET);
	if (magic_num != 0x68636f6e) {
		printf("no hw config header\n");
		return -1;
	}

	total_len = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + 8);
	if (total_len == offset) {
		printf("no hw config\n");
		return -1;
	}

	while (offset < total_len) {
		/*printf("offset is %x, len is %x\n", offset, total_len);*/
		res = config_entry_parse_ethaddr(offset);
		if (res == 0)
			break;
		tmp = ((read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset + 4)) >> 16) + 8;
		offset += tmp;
		/*printf("%x, %x\n", offset, tmp);*/
	}

	if (res != 0)
		printf("no hw ethaddr config\n");
	return res;
}

int set_eth_IPADDR(u32 offset)
{
	char ipaddr[22];
	u32 data = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset);

	ip_to_string(data, ipaddr);
	printf("set ipaddr = %s\n", ipaddr);
	setenv("ipaddr", ipaddr);

	return 0;
}

int set_eth_GATEWAY(u32 offset)
{
	char gateway[22];
	u32 data = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset);

	ip_to_string(data, gateway);
	printf("set gateway = %s\n", gateway);
	setenv("gatewayip", gateway);

	return 0;
}

int set_eth_NETMASK(u32 offset)
{
	char netmask[22];
	u32 data = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset);

	ip_to_string(data, netmask);
	printf("set netmask = %s\n", netmask);
	setenv("netmask", netmask);

	return 0;
}

/*
int config_entry_parse_ipaddr(u32 offset)
{
	u32 entry_id = 0;
	u32 entry_len = 0;
	entry_id = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_SECTOR_SIZE * (SPI_UBOOT_BLK_CNT + SPI_FW_BLK_CNT) + offset);
	entry_len = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_SECTOR_SIZE * (SPI_UBOOT_BLK_CNT + SPI_FW_BLK_CNT) + offset + 4);

	if (entry_id != ETH_IPADDR_ID)
		return -1;
	else
		set_eth_IPADDR((offset+8), entry_len);

	return 0;
}
*/

int hw_config_parse_network(void)
{
	int res = -1;
	u32 total_len = 0;
	u32 offset = 12;
	u32 magic_num;
	u32 entry_id = 0;
	u32 entry_len = 0;

	/*printf("enter hw_config_parse_ethaddr\n");*/
	magic_num = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET);
	if (magic_num != 0x68636f6e) {
		printf("no hw config header\n");
		return -1;
	}

	total_len = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + 8);
	if (total_len == offset) {
		printf("no hw config\n");
		return -1;
	}

	while (offset < total_len) {
		/*printf("offset is %x, len is %x\n", offset, total_len);*/
		entry_id = read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset);
		switch (entry_id) {
		case ETH_IPADDR_ID:
			res = set_eth_IPADDR((offset + 8));
			break;
		case ETH_GATEWAY_ID:
			res = set_eth_GATEWAY((offset + 8));
			break;
		case ETH_NETMASK_ID:
			res = set_eth_NETMASK((offset + 8));
			break;
		default:
			break;
		}
		entry_len = ((read_spi_flash(CONFIG_FLASHBASEADDR + SPI_HW_OFFSET + offset + 4)) >> 16) + 8;
		offset += entry_len;
		/*printf("%x, %x\n", entry_id, entry_len);*/
	}

	return res;
}


