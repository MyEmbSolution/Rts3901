#include <netinet/in.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/mii.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>

#ifdef COMMON_PCI
#include <pci/pci.h>
#endif

#include "common.h"
#include "rtl.h"

void print_pkt(void *buf, int len)
{
        int k;
        for(k=0; k < len; k++) {
                if (!(k % 16) )
                        dbgprint(MPL_INFO, "\n%04X:  ", k);
                dbgprint(MPL_INFO, "%02X ", ((u8*)buf)[k]);
        }
        dbgprint(MPL_INFO, "\n");
}

int create_control_socket()
{
        int sd;

        if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                dbgperror(MPL_ERR, "socket()");
                return -ECREATESOCKET;
        }
        dbgprint(MPL_INFO, "created control socket (%i): ok\n", sd);
        return sd;
}

int create_raw_socket(int protocol)
{
        int sd;

        if ((sd = socket(AF_PACKET, SOCK_RAW, htons(protocol))) < 0) {
                dbgperror(MPL_ERR, "socket()");
                return -ECREATESOCKET;
        }
        dbgprint(MPL_INFO, "created raw socket (%i): ok\n", sd);
        return sd;
}

int bind_rawsocket_to_interface(rt_nic_info *nicinfo, int socket, int protocol)
{
        struct sockaddr_ll sll;

        bzero(&sll, sizeof(sll));

        sll.sll_family = AF_PACKET;
        sll.sll_ifindex = nicinfo->ifindex;
        sll.sll_protocol = htons(protocol);

        if((bind(socket, (struct sockaddr *)&sll, sizeof(sll))) < 0) {
                dbgprint(MPL_ERR, "bind socket (%d) to interface (%s) failed!!!\n",
                         socket, nicinfo->name);
                return -EBINDSOCKET;
        }
        dbgprint(MPL_INFO, "bind socket (%d) to interface (%s): ok\n",
                 socket, nicinfo->name);

        return 0;
}

int test_ifr_flags(char *name, int flag)
{
        struct ifreq ifr;
        int sd;

        if ((sd = create_control_socket()) < 0)
                return -1;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, name, IFNAMSIZ);
        ioctl(sd, SIOCGIFFLAGS, &ifr);
        close(sd);

        return (ifr.ifr_flags & flag);
}

/*
 * set interface flag in struct ifreq
 * @param name: interface name
 * @param p: do OR operation
 * @param m: do AND NOT operation
 * @return if success, return 0
 */
//static
int set_ifr_flags(char *name, int p, int m)
{
        struct ifreq ifr;
        int ret;
        int sd;

        if ((sd = create_control_socket()) < 0)
                return -1;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, name, IFNAMSIZ);
        ioctl(sd, SIOCGIFFLAGS, &ifr);

        ifr.ifr_flags = (ifr.ifr_flags | p) & ~m;

        ret = ioctl(sd, SIOCSIFFLAGS, &ifr);

        close(sd);
        return ret;
}

int execute_lock()
{
        struct stat st;
        FILE *fp = NULL;
        int ret;

        if (stat(PROG_LOCKFILE, &st) == 0) {
                dbgprint(MPL_ERR, "\n" PRINT_PAD
                         "<<< Another program is already executing!!! >>>\n\n");
                return -EALREADYEXE;
        }

        if (stat(TMPDIR, &st) != 0) {
                if ((ret = mkdir(TMPDIR, 0755)) != 0)
                        return ret;
        }

        if ((fp = fopen(PROG_LOCKFILE, "w")) == 0) {
                dbgperror(MPL_ERR, "fopen():");
                return -EFOPEN;
        }

        fprintf(fp, "%d\n", getpid());
        fclose(fp);

        return 0;
}

void execute_unlock()
{
        remove(PROG_LOCKFILE);
}

/*
 * check if netif_carrier_ok
 * @param nicinfo:
 * @return if success, return 0
 */
int cm_is_connect(rt_nic_info *nicinfo)
{
        FILE *fp = NULL;
        char filename[128] = {0};
        char buf[24] = {0};
        SYS_CARRIER(filename, nicinfo->name);

        if ((fp = fopen(filename, "r")) == NULL) {
                dbgprint(MPL_ERR, "\n" PRINT_PAD
                         "<<< open file (%s) failed!!! >>>\n\n", filename);
                return -1;
        }
        if (!fgets(buf, sizeof(buf), fp)) {
                fclose(fp);
                return -1;
        }

        fclose(fp);
        return (atoi(buf)) ? 0 : -1;
}

#ifdef COMMON_PCI
static struct pci_dev* find_pci_device(struct pci_access *pacc,
                                       struct pci_filter *filt)
{
        struct pci_dev *z, *pdev;

        pdev = malloc(sizeof(struct device *));

        for (z=pacc->devices; z; z=z->next) {
                if (pci_filter_match(filt, z)) {
                        pdev = z;
                        break;
                }
        }

        return pdev;
}

static int get_pci_info(rt_nic_info *nicinfo)
{
        struct pci_access *pacc;
        struct pci_filter filter;
        struct pci_dev *pdev = NULL;
        char slot[128] = {0};

        strcpy(slot, nicinfo->bus_info);

        pacc = pci_alloc();
        pci_init(pacc);
        pci_scan_bus(pacc);

        pci_filter_init(pacc, &filter);

        if (pci_filter_parse_slot(&filter, slot))
                return -1;

        pdev = find_pci_device(pacc, &filter);

        if (!pdev) {
                printf("No device for PCI slot %s\n", nicinfo->bus_info);
                return -1;
        }

        nicinfo->vid = pci_read_word(pdev, 0x00);
        nicinfo->did = pci_read_word(pdev, 0x02);
        nicinfo->svid = pci_read_word(pdev, 0x2C);
        nicinfo->ssid = pci_read_word(pdev, 0x2E);

        return 0;
}

#else

static int get_pci_info(rt_nic_info *nicinfo)
{
        int sd;

        if ((sd = create_control_socket()) < 0)
                return -1;

        nicinfo->vid = (u16)read_pci_cfg(sd, nicinfo, 2, 0x00);
        nicinfo->did = (u16)read_pci_cfg(sd, nicinfo, 2, 0x02);
        nicinfo->svid = (u16)read_pci_cfg(sd, nicinfo, 2, 0x2C);
        nicinfo->ssid = (u16)read_pci_cfg(sd, nicinfo, 2, 0x2E);

        close(sd);
        return 0;
}
#endif /* COMMON_PCI */

int get_usb_info(rt_nic_info *nicinfo)
{
        struct rtltool_usb_cmd cmd;
        struct ifreq ifr;
        int sd;

        if ((sd = create_control_socket()) < 0)
                return -1;

        memset(&ifr, 0, sizeof(ifr));
        memset(&cmd, 0, sizeof(cmd));
        strncpy(ifr.ifr_name, nicinfo->name, IFNAMSIZ);

        ifr.ifr_data = (__caddr_t)&cmd;
        cmd.cmd = RTLTOOL_USB_INFO;

        if (ioctl(sd, SIOCRTLUSBTOOL, &ifr) < 0) {
                dbgperror(MPL_INFO, "SIOCRTLUSBTOOL");
                return -EIOCTL;
        }

        nicinfo->vid = cmd.nic_info.idVendor;
        nicinfo->did = cmd.nic_info.idProduct;

        dbgprint(MPL_INFO, "devpath=%s, bcd=0x%x\n", cmd.nic_info.devpath, cmd.nic_info.bcdDevice);

        close(sd);
        return 0;
}

u16 get_rt_bus_type(char *devname)
{
        struct ethtool_drvinfo info;
        struct ifreq ifr;
        u32 type = UNKNOWN_NIC;
        int sd;

        if ((sd = create_control_socket()) < 0)
                return type;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, devname, IFNAMSIZ);

        info.cmd = ETHTOOL_GDRVINFO;
        ifr.ifr_data = (caddr_t)&info;

        if (ioctl(sd, SIOCETHTOOL, &ifr) < 0) {
                dbgprint(MPL_ERR, "%s(): %s \n", __func__, devname);
                dbgperror(MPL_ERR, "ETHTOOL_GDRVINFO");
                close(sd);
                return type;
        }
        close(sd);

        if (!strncmp(info.driver, "rtl8168", strlen("rtl8168")) || !strncmp(info.driver, "r8169", strlen("r8169")) ||
            !strncmp(info.driver, "r8101", strlen("r8101")) || !strncmp(info.driver, "r8152", strlen("r8152")) ||
            !strncmp(info.driver, "r8153", strlen("r8153"))) {

                if (!strncmp(info.bus_info, "usb", strlen("usb")))
                        type |= USB_NIC;
                else
                        type |= PCI_NIC;
        }

        dbgprint(MPL_DBG, "%s(): %s info.driver %s\n", __func__, devname, info.driver);
        dbgprint(MPL_DBG, "%s(): %s type 0x%x\n", __func__, devname, type);

        return type;
}

int get_drv_info(rt_nic_info *nicinfo)
{
        struct ethtool_drvinfo info;
        struct ifreq ifr;
        int sd;

        if ((sd = create_control_socket()) < 0)
                return -1;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, nicinfo->name, IFNAMSIZ);

        info.cmd = ETHTOOL_GDRVINFO;
        ifr.ifr_data = (caddr_t)&info;

        if (ioctl(sd, SIOCETHTOOL, &ifr) < 0) {
                dbgperror(MPL_ERR, "ETHTOOL_GDRVINFO");
                return -EIOCTL;
        }

        sprintf(nicinfo->drv_name, "%s", info.driver);
        sprintf(nicinfo->drv_version, "%s", info.version);
        sprintf(nicinfo->bus_info, "%s", info.bus_info);

        close(sd);
        return 0;
}

/*
int get_link_cap(rt_nic_info *nicinfo)
{
	struct ethtool_cmd cmd;
	struct ifreq ifr;
	int sd;

	if ((sd = create_control_socket()) < 0)
		return -ECREATESOCKET;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, nicinfo->name, IFNAMSIZ);

	cmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&cmd;

	if (ioctl(sd, SIOCETHTOOL, &ifr) < 0) {
		dbgperror(MPL_ERR, "ETHTOOL_GSET");
		return -EIOCTL;
	}

	if (cmd.supported & SUPPORTED_1000baseT_Full ||
			cmd.supported & SUPPORTED_1000baseT_Half)
		nicinfo->linkcap = 1000;
	else if (cmd.supported & SUPPORTED_100baseT_Full ||
			cmd.supported & SUPPORTED_100baseT_Half)
		nicinfo->linkcap = 100;
	else if (cmd.supported & SUPPORTED_10baseT_Full ||
			cmd.supported & SUPPORTED_10baseT_Half)
		nicinfo->linkcap = 10;

	//printf("%s: link capability is %u\n", nicinfo->name, nicinfo->linkcap);

	close(sd);
	return 0;
}
*/

int get_link_cap_by_chip(rt_nic_info *nicinfo)
{
        switch (nicinfo->ChipType) {
        case RTL8136:
        case RTL8136_REV_E:
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                nicinfo->linkcap = SPEED_100;
                nicinfo->busfeature |= FE_NIC;
                break;
        case RTLUNKNOWN:
                return -1;
        default:
                if(nicinfo->Nic8168DChangeTo8104 ||
                    nicinfo->Nic8168EChangeTo8105 ||
                    nicinfo->Nic8168HChangeTo8107E) {
                        nicinfo->linkcap = SPEED_100;
                        nicinfo->busfeature |= FE_NIC;
                } else {
                        nicinfo->linkcap = SPEED_1000;
                        nicinfo->busfeature |= GIGA_NIC;
                }
                break;
        }

        return 0;
}

void fill_nic_desc(rt_nic_info *nicinfo)
{
        if (nicinfo->busfeature & PCI_NIC) {
                if (nicinfo->busfeature & GIGA_NIC)
                        sprintf(nicinfo->desc, "%s", RTK_PCI_GBE);
                else
                        sprintf(nicinfo->desc, "%s", RTK_PCI_FE);
        } else if (nicinfo->busfeature & USB_NIC) {
                if (nicinfo->busfeature & GIGA_NIC)
                        sprintf(nicinfo->desc, "%s", RTK_USB_GBE);
                else
                        sprintf(nicinfo->desc, "%s", RTK_USB_FE);
        }
}

/*
 * get interface's index, mac address
 * @param nicinfo:
 * @return if success, return 0
 */
int get_nic_info(rt_nic_info *nicinfo)
{
        struct ifreq ifr;
        int i, ret = -1;
        int sd;

        dbgprint(MPL_INFO, YELLOW "%s: retrieve card information..."
                 NORMAL "\n", nicinfo->name);

        if ((sd = create_control_socket()) < 0)
                return -ECREATESOCKET;

        /* get interface index */
        strncpy(ifr.ifr_name, nicinfo->name, IFNAMSIZ);
        if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
                dbgperror(MPL_ERR, "SIOCGIFINDEX");
                ret = -EIOCTL;
                goto get_nic_info_exit;
        }
        nicinfo->ifindex = ifr.ifr_ifindex;

        dbgprint(MPL_INFO, "%s: index = %d\n", nicinfo->name, nicinfo->ifindex);

        /* get interface MAC address */
        if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
                dbgperror(MPL_ERR, "SIOCGIFHWADDR");
                ret = -EIOCTL;
                goto get_nic_info_exit;
        }

        for (i = 0; i < ETH_MAC_LEN; i++)
                nicinfo->macaddr[i] = ifr.ifr_hwaddr.sa_data[i];

        dbgprint(MPL_INFO, "%s: MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n", nicinfo->name,
                 nicinfo->macaddr[0], nicinfo->macaddr[1], nicinfo->macaddr[2],
                 nicinfo->macaddr[3], nicinfo->macaddr[4], nicinfo->macaddr[5]);

        /* get irq, io base addr */
        if (ioctl(sd, SIOCGIFMAP, &ifr) == -1) {
                dbgperror(MPL_ERR, "SIOCGIFMAP");
                ret = -EIOCTL;
                goto get_nic_info_exit;
        }
        nicinfo->irq = ifr.ifr_map.irq;
        nicinfo->iobase = ifr.ifr_map.base_addr;

        ret = 0;

get_nic_info_exit:
        close(sd);
        return ret;
}

#if 0
/*
 * retrieve interface name
 * @param name: return interface name
 * @param p: buffer to be retrieved
 */
void get_dev_name(char *name, char *p)
{
        int namestart = 0, nameend = 0;

        while (isspace(p[namestart]))
                namestart++;
        nameend = namestart;
        while (p[nameend] && p[nameend] != ':' && !isspace(p[nameend]))
                nameend++;
        if (p[nameend] == ':') {
                if ((nameend - namestart) < IFNAMSIZ) {
                        memcpy(name, &p[namestart], nameend - namestart);
                        name[nameend - namestart] = '\0';
                        p = &p[nameend];
                } else {
                        /* Interface name too large */
                        name[0] = '\0';
                }
        } else {
                /* trailing ':' not found - return empty */
                name[0] = '\0';
        }
}

/*
 * find supported interface defined in get_chip()
 * @param ifname: return all supported interface name
 * @param chipver: return chip id with corresponding interface
 * @return available interfaces
 */
int find_interface(rt_nic_info *)
{
        FILE *fp;
        char buf[512];
        char name[IFNAMSIZ];
        NIC_CHIP_TYPE ChipType = RTLUNKNOWN;
        int num = 0, i = 0;
        u8 type;

        dbgprint(MPL_INFO, "%s(): find available interfaces...\n",
                 __func__);

        if ((fp = fopen(PROC_NET_DEV, "r")) == NULL) {
                printf("\n" PRINT_PAD "<<< open (%s) failed!!! >>>\n",
                       PROC_NET_DEV);
                return 0;
        }

        /* ignore first three lines */
        for (i=0; i < 3; i++) {
                if (!fgets(buf, sizeof(buf), fp))
                        goto read_proc_fail;
        }

        num = 0;
        while (fgets(buf, sizeof(buf), fp)) {
                get_dev_name(name, buf);

                if (test_ifr_flags(name, IFF_UP)) {
                        if (!strncmp(name, "lo", strlen("lo")))
                                continue;

                        if ((type = get_rt_bus_type(name)) == UNKNOWN_NIC)
                                continue;

                        if ((ChipType = get_rt_mac_version(name, type)) != RTLUNKNOWN) {
                                strcpy((nicinfo+num)->name, name);
                                (nicinfo+num)->ChipType = ChipType;
                                (nicinfo+num)->busfeature |= type;
                                num++;
                        }
                }

                if(num >= MAX_CARD_NUM) {
                        printf("%s(): too many cards (>%d)!!!\n", __func__,
                               MAX_CARD_NUM);
                        break;
                }
        }

read_proc_fail:
        fclose(fp);
        return num;
}
#else

int find_interface(rt_nic_info *nicinfo)
{
        struct if_nameindex *iflist, *pif;
        NIC_CHIP_TYPE ChipType = RTLUNKNOWN;
        u16 type;
        int num = 0;

        iflist = if_nameindex();
        pif = iflist;

        while (pif->if_index) {
                if (test_ifr_flags(pif->if_name, IFF_UP)) {
                        if (!strncmp(pif->if_name, "lo", strlen("lo"))) {
                                pif++;
                                continue;
                        }

                        if ((type = get_rt_bus_type(pif->if_name)) == UNKNOWN_NIC) {
                                pif++;
                                continue;
                        }

                        if ((ChipType = get_rt_mac_version(pif->if_name, type)) != RTLUNKNOWN) {
                                strcpy((nicinfo+num)->name, pif->if_name);
                                (nicinfo+num)->ChipType = ChipType;
                                (nicinfo+num)->busfeature |= type;
                                get_rt_changed_mac_chiptype((nicinfo+num));
                                num++;
                        }
                }

                if(num >= MAX_CARD_NUM) {
                        printf("%s(): too many cards (>%d)!!!\n", __func__,
                               MAX_CARD_NUM);
                        break;
                }
                pif++;
        }

        if_freenameindex(iflist);
        return num;
}
#endif

int collect_info(rt_nic_info *nicinfo)
{
        int ret = -1;

        do {
                if (get_nic_info(nicinfo) < 0) {
                        printf(PRINT_PAD "<<< %s: get interface information failed!!! >>>\n", nicinfo->name);
                        break;
                }

                if (get_drv_info(nicinfo) < 0) {
                        printf(PRINT_PAD "<<< %s: get driver information failed!!! >>>\n", nicinfo->name);
                        break;
                }

                //if (get_link_cap(nicinfo) < 0) {
                if (get_link_cap_by_chip(nicinfo) < 0) {
                        printf(PRINT_PAD "<<< %s: get link capability failed!!! >>>\n", nicinfo->name);
                        break;
                }

                if (nicinfo->busfeature & PCI_NIC) {
                        if (get_pci_info(nicinfo) < 0) {
                                printf(PRINT_PAD "<<< %s: get PCI information failed!!! >>>\n", nicinfo->name);
                                break;
                        }
                } else if (nicinfo->busfeature & USB_NIC) {
                        if (get_usb_info(nicinfo) < 0) {
                                printf(PRINT_PAD "<<< %s: get USB information failed!!! >>>\n", nicinfo->name);
                                break;
                        }
                }

                fill_nic_desc(nicinfo);

                ret = 0;
        } while (0);

        return ret;
}

/*
 * RTLTOOL ioctl interface
 * @param sd: socket id
 * @param devname: interface name
 * @param usrcmd: ioctl private data
 * @param output: return value for any read command
 * @return if success, return 0
 */
int rtlpci_io_read(int sd, char *devname, struct rtltool_pci_cmd *usrcmd, u32 *output)
{
        struct ifreq ifr;

        if (!usrcmd) {
                dbgprint(MPL_ERR, "%s: invalid input!!!\n", __func__);
                return -1;
        }
        strncpy(ifr.ifr_name, devname, IFNAMSIZ);

        ifr.ifr_data = (__caddr_t)usrcmd;

        if (ioctl(sd, SIOCRTLPCITOOL, &ifr) < 0) {
                dbgprint(MPL_ERR, "%s: ioctl failed!!!\n", __func__);
                dbgperror(MPL_INFO, "SIOCRTLPCITOOL");
                return -EIOCTL;
        }
        *output = ((struct rtltool_pci_cmd *)ifr.ifr_data)->data;

        dbgprint(MPL_INFO, "cmd=%d, offset=0x%04x, len=%d, data=0x%08x\n",
                 usrcmd->cmd, usrcmd->offset, usrcmd->len, usrcmd->data);

        return 0;
}

int rtlpci_io_write(int sd, char *devname, struct rtltool_pci_cmd *usrcmd)
{
        struct ifreq ifr;

        if (!usrcmd) {
                dbgprint(MPL_ERR, "%s: invalid input!!!\n", __func__);
                return -1;
        }
        strncpy(ifr.ifr_name, devname, IFNAMSIZ);

        ifr.ifr_data = (__caddr_t)usrcmd;

        if (ioctl(sd, SIOCRTLPCITOOL, &ifr) < 0) {
                dbgprint(MPL_ERR, "%s: ioctl failed!!!\n", __func__);
                dbgperror(MPL_INFO, "SIOCRTLPCITOOL");
                return -EIOCTL;
        }

        dbgprint(MPL_INFO, "cmd=%d, offset=0x%04x, len=%d, data=0x%08x\n",
                 usrcmd->cmd, usrcmd->offset, usrcmd->len, usrcmd->data);

        return 0;
}

int rtlusb_io_read(int sd, char *devname, struct rtltool_usb_cmd *usrcmd)
{
        struct ifreq ifr;

        if (!usrcmd) {
                dbgprint(MPL_ERR, "%s: invalid input!!!\n", __func__);
                return -1;
        }
        strncpy(ifr.ifr_name, devname, IFNAMSIZ);

        ifr.ifr_data = (__caddr_t)usrcmd;

        if (ioctl(sd, SIOCRTLUSBTOOL, &ifr) < 0) {
                dbgperror(MPL_INFO, "SIOCRTLUSBTOOL");
                return -EIOCTL;
        }

        dbgprint(MPL_INFO, "cmd=%d, offset=0x%04x, byteen=%d, data=0x%08x\n",
                 usrcmd->cmd, usrcmd->offset, usrcmd->byteen, usrcmd->data);

        return 0;
}

int rtlusb_io_write(int sd, char *devname, struct rtltool_usb_cmd *usrcmd)
{
        struct ifreq ifr;

        if (!usrcmd) {
                dbgprint(MPL_ERR, "%s: invalid input!!!\n", __func__);
                return -1;
        }
        strncpy(ifr.ifr_name, devname, IFNAMSIZ);

        ifr.ifr_data = (__caddr_t)usrcmd;

        if (ioctl(sd, SIOCRTLUSBTOOL, &ifr) < 0) {
                dbgperror(MPL_INFO, "SIOCRTLUSBTOOL");
                return -EIOCTL;
        }

        dbgprint(MPL_INFO, "cmd=%d, offset=0x%04x, byteen=%d, data=0x%08x\n",
                 usrcmd->cmd, usrcmd->offset, usrcmd->byteen, usrcmd->data);

        return 0;
}

u32 read_pci_cfg(int sd, rt_nic_info *nicinfo, int len, u32 offset)
{
        struct rtltool_pci_cmd cmd;
        u32 value;

        if (len != 1 && len != 2 && len != 4)
                return 0;

        SET_PCI_READ_CMD(offset, len, RTLTOOL_READ_PCI);
        rtlpci_io_read(sd, nicinfo->name, &cmd, &value);

        return value;
}

int usb_write_phy(int sd, rt_nic_info *nicinfo, u16 reg, u16 value)
{
        struct mii_ioctl_data data;
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, nicinfo->name, IFNAMSIZ);

        data.reg_num = reg;
        data.val_in = value;
        memcpy(&ifr.ifr_data, &data, sizeof(data));

        if (ioctl(sd, SIOCSMIIREG, &ifr) < 0) {
                dbgperror(MPL_INFO, "SIOCSMIIREG");
                return -EIOCTL;
        }
        dbgprint(MPL_INFO, "%s: addr=0x%x, value=0x%x\n", __func__, reg, value);

        return 0;
}

int usb_read_phy(int sd, rt_nic_info *nicinfo, u16 reg, u16 *value)
{
        struct mii_ioctl_data data;
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, nicinfo->name, IFNAMSIZ);

        data.reg_num = reg;
        memcpy(&ifr.ifr_data, &data, sizeof(data));

        if (ioctl(sd, SIOCGMIIREG, &ifr) < 0) {
                dbgperror(MPL_INFO, "SIOCGMIIREG");
                return -EIOCTL;
        }
        *value = data.val_out;
        dbgprint(MPL_INFO, "%s: addr=0x%x, value=0x%x\n", __func__, reg, *value);

        return 0;
}

void write_phy(int sd, rt_nic_info *nicinfo, u16 reg, u16 value)
{
        struct rtltool_pci_cmd cmd;

        if (nicinfo->busfeature & PCI_NIC) {
                SET_PCI_WRITE_CMD(reg, 4, value, RTLTOOL_WRITE_PHY);
                rtlpci_io_write(sd, nicinfo->name, &cmd);
        } else if (nicinfo->busfeature & USB_NIC) {
                usb_write_phy(sd, nicinfo, reg, value);
        }
}

u16 read_phy(int sd, rt_nic_info *nicinfo, u16 reg)
{
        struct rtltool_pci_cmd cmd;
        u32 value = 0;

        if (nicinfo->busfeature & PCI_NIC) {
                SET_PCI_READ_CMD(reg, 4, RTLTOOL_READ_PHY);
                rtlpci_io_read(sd, nicinfo->name, &cmd, &value);
        } else if (nicinfo->busfeature & USB_NIC) {
                usb_read_phy(sd, nicinfo, reg, (u16 *)&value);
        }

        return (u16)value;
}

int usb_direct_read(int sd, char *devname, u8 reg_type, u16 addr, u8 *value, int len)
{
        struct rtltool_usb_cmd cmd;
        u8 *buffer;
        u16 tmp_addr = addr & ~3;
        u16 offset = addr & 3;
        u16 realsize = (u16)((offset + len + 3) & ~3);
        u8 io_cmd;
        int ret = -1;

        if (realsize != len)
                buffer = (u8 *)malloc(realsize);
        else
                buffer = value;

        if (reg_type == PLAMCU_OCP)
                io_cmd = RTLTOOL_PLA_OCP_READ;
        else if (reg_type == USBMCU_OCP)
                io_cmd = RTLTOOL_USB_OCP_READ;
        else
                return ret;

        SET_USB_READ_CMD(tmp_addr, realsize, (void *)buffer, io_cmd);

        ret = rtlusb_io_read(sd, devname, &cmd);

        if (buffer != value) {
                memcpy(value, &buffer[offset], len);
                free(buffer);
        }

        return ret;
}

int usb_direct_write(int sd, char *devname, u8 reg_type, u16 addr, u8 *value, int len)
{
        struct rtltool_usb_cmd cmd;
        u8 *buffer, io_cmd;
        u16 tmp_addr = addr & ~3;
        u16 offset = addr & 3;
        u16 realsize = (u16)((offset + len + 3) & ~3);
        u16 startMask, stopMask, byteMask, end;
        int ret = -1;

        if (realsize != len) {
                buffer = (u8 *)malloc(realsize);
                memcpy(&buffer[offset], value, len);
        } else {
                buffer = value;
        }

        startMask = (0xf << offset) & 0xf;
        end = (offset + len) & 3;
        stopMask = 0xf >> ((4 - end) & 3);
        if (realsize > 4)
                byteMask = (stopMask << 4) | startMask;
        else
                byteMask = stopMask & startMask;

        if (reg_type == PLAMCU_OCP)
                io_cmd = RTLTOOL_PLA_OCP_WRITE;
        else if (reg_type == USBMCU_OCP)
                io_cmd = RTLTOOL_USB_OCP_WRITE;
        else
                return ret;

        SET_USB_WRITE_CMD(tmp_addr, byteMask, realsize, (void *)buffer, io_cmd);

        ret = rtlusb_io_write(sd, devname, &cmd);

        if (buffer != value) {
                free(buffer);
        }

        return ret;
}

void write_mac(int sd, rt_nic_info *nicinfo, u16 reg, u32 value, int len)
{
        struct rtltool_pci_cmd cmd;

        if (nicinfo->busfeature & PCI_NIC) {
                SET_PCI_WRITE_CMD(reg, len, value, RTLTOOL_WRITE_MAC);
                rtlpci_io_write(sd, nicinfo->name, &cmd);
        } else if (nicinfo->busfeature & USB_NIC) {
                usb_direct_write(sd, nicinfo->name, PLAMCU_OCP, reg, (u8 *)&value, len);
        }
}

u32 read_mac(int sd, rt_nic_info *nicinfo, u16 reg, int len)
{
        struct rtltool_pci_cmd cmd;
        u32 value = 0;

        if (nicinfo->busfeature & PCI_NIC) {
                SET_PCI_READ_CMD(reg, len, RTLTOOL_READ_MAC);

                rtlpci_io_read(sd, nicinfo->name, &cmd, &value);
        } else if (nicinfo->busfeature & USB_NIC) {
                usb_direct_read(sd, nicinfo->name, PLAMCU_OCP, reg, (u8 *)&value, len);
        }

        return value;
}

void write_ephy(int sd, rt_nic_info *nicinfo, u16 reg, u16 value)
{
        struct rtltool_pci_cmd cmd;

        SET_PCI_WRITE_CMD(reg, 4, value, RTLTOOL_WRITE_EPHY);
        rtlpci_io_write(sd, nicinfo->name, &cmd);
}

u32 read_ephy(int sd, rt_nic_info *nicinfo, u16 reg)
{
        struct rtltool_pci_cmd cmd;
        u32 value;

        SET_PCI_READ_CMD(reg, 4, RTLTOOL_READ_EPHY);

        rtlpci_io_read(sd, nicinfo->name, &cmd, &value);

        return value;
}

u32 read_eri(int sd, rt_nic_info *nicinfo, u16 reg, int len)
{
        struct rtltool_pci_cmd cmd;
        u32 value;

        if (len != 1 && len != 2 && len != 4)
                return 0;

        SET_PCI_READ_CMD(reg, len, RTLTOOL_READ_ERI);

        rtlpci_io_read(sd, nicinfo->name, &cmd, &value);

        return value;
}

void write_eri(int sd, rt_nic_info *nicinfo, u16 reg, int len, u32 value)
{
        struct rtltool_pci_cmd cmd;

        if (len != 1 && len != 2 && len != 4)
                return;

        SET_PCI_WRITE_CMD(reg, len, value, RTLTOOL_WRITE_ERI);
        rtlpci_io_write(sd, nicinfo->name, &cmd);
}

/*
 * get mac chip version
 * @param devname: interface name
 * @param mcfg: bus type
 * @return return mcfg
 */
NIC_CHIP_TYPE get_rt_mac_version(char *devname, u16 type)
{
        rt_nic_info nicinfo;
        u32 TcrValue, MacVer, RevId;
        NIC_CHIP_TYPE ChipType = RTLUNKNOWN;
        int sd;

        memset(&nicinfo, 0, sizeof(rt_nic_info));
        strncpy(nicinfo.name, devname, IFNAMSIZ);
        nicinfo.busfeature |= type;

        if ((sd = create_raw_socket(ETH_P_LOOPBACK)) < 0)
                return ChipType;

        if (type & USB_NIC)
                TcrValue = read_mac(sd, &nicinfo, TCR_0+2, 2);
        else if (type & PCI_NIC)
                TcrValue = read_mac(sd, &nicinfo, TxConfig, 4);

        if (type & USB_NIC) {
                MacVer = TcrValue & 0x7C80;
                RevId = TcrValue & 0x0070;

                switch (MacVer) {
                case 0x4C00:
                        if (RevId == 0x0000)
                                ChipType = RTL8152_REV_A;
                        else if (RevId == 0x0010)
                                ChipType = RTL8152_REV_B;
                        else
                                ChipType = RTL8152_REV_B;
                        break;
                case 0x5C00:
                        if (RevId == 0x0000)
                                ChipType = RTL8153_REV_A;
                        else if (RevId == 0x0010)
                                ChipType = RTL8153_REV_B;
                        else if (RevId == 0x0020)
                                ChipType = RTL8153_REV_C;
                        else
                                ChipType = RTL8153_REV_C;
                        break;
                default:
                        dbgprint(MPL_DBG, "%s: unknown chip version (%x)!!!\n", devname, TcrValue);
                        break;
                }
        } else if (type & PCI_NIC) {
                MacVer = TcrValue;
                RevId = TcrValue;
                MacVer &= 0x7C800000;
                RevId &= ( BIT_20 | BIT_21 | BIT_22 );

                do {
                        if ( MacVer == 0x00000000 ) {
                                ChipType = RTL8169;

                                // NO SUPPORT
                                break;
                        } else if ( MacVer == 0x00800000 ) {
                                ChipType = RTL8169S1;

                                break;
                        } else if ( MacVer == 0x04000000 ) {
                                ChipType = RTL8169S2;

                                break;
                        } else if ( MacVer == 0x10000000 ) {
                                ChipType = RTL8169SB;

                                break;
                        } else if ( MacVer == 0x18000000 ) {
                                // 8169SC could not use rev id to identify
                                // HW design policy could not apply here
                                if ( TcrValue & 0x80000000 ) {
                                        ChipType = RTL8169SC_REV_E;

                                } else {
                                        ChipType = RTL8169SC;

                                }
                                break;
                        } else if ( MacVer == 0x30000000 ) {
                                ChipType = RTL8168B;


                                break;
                        } else if ( MacVer == 0x30800000 ) {
                                ChipType = RTL8136;


                                break;
                        } else if ( MacVer == 0x38000000 ) {


                                if ( RevId == 0x500000 ) {
                                        ChipType = RTL8168B_REV_F;

                                } else if ( RevId == 0x000000 ) {
                                        ChipType = RTL8168B_REV_E;

                                } else {
                                        ChipType = RTL8168B_REV_F;

                                }

                                break;
                        } else if ( MacVer == 0x38800000 ) {
                                ChipType = RTL8136_REV_E;


                                break;
                        } else if ( MacVer == 0x34000000 ) {

                                if ( RevId == 0x300000 ) {
                                        ChipType = RTL8101_REV_C;

                                } else if ( RevId == 0x200000 ) {
                                        ChipType = RTL8101_REV_C;

                                } else if ( RevId == 0x000000 ) {
                                        ChipType = RTL8101;

                                } else {
                                        ChipType = RTL8101_REV_C;

                                }

                                break;
                        } else if ( MacVer == 0x3c000000 ) {

                                if ( RevId == 0x400000 ) {
                                        ChipType = RTL8168C_REV_M;

                                } else if ( RevId == 0x300000 ) {
                                        ChipType = RTL8168C_REV_K;

                                } else if ( RevId == 0x200000 ) {
                                        ChipType = RTL8168C_REV_G;

                                } else if ( RevId == 0x100000 ) {
                                        ChipType = RTL8168C;

                                } else {
                                        ChipType = RTL8168C_REV_M;

                                }
                                break;
                        } else if ( MacVer == 0x34800000 ) {
                                //
                                // 8102E
                                //

                                if ( RevId == 0x600000 ) {

                                        ChipType = RTL8103E_EL_REV_C;

                                } else if ( RevId == 0x500000 ) {

                                        ChipType = RTL8103E_EL_REV_B;

                                } else if ( RevId == 0x400000 ) {

                                        ChipType = RTL8103;

                                } else if ( RevId == 0x300000 ) {
                                        ChipType = RTL8103;
                                } else if (RevId == 0x200000) {
                                        ChipType = RTL8102E_EL_REV_C;

                                } else if (RevId == 0x100000) {
                                        ChipType = RTL8102E_EL_REV_B;

                                } else if (RevId == 0x000000) {
                                        ChipType = RTL8102E;

                                } else {

                                        ChipType = RTL8103E_EL_REV_C;

                                }

                                break;
                        } else if (MacVer == 0x24800000 ) {
                                //
                                // 8102EL
                                //

                                if ( RevId == 0x600000 ) {
                                        ChipType = RTL8103E_EL_REV_C;
                                } else if ( RevId == 0x500000 ) {
                                        ChipType = RTL8103E_EL_REV_B;
                                } else if ( RevId == 0x400000 ) {
                                        ChipType = RTL8103;
                                } else if ( RevId == 0x300000 ) {
                                        ChipType = RTL8103;
                                } else if (RevId == 0x200000) {
                                        ChipType = RTL8102E_EL_REV_C;
                                } else if (RevId == 0x100000) {
                                        ChipType = RTL8102E_EL_REV_B;
                                } else if (RevId == 0x000000) {
                                        ChipType = RTL8102EL;
                                } else {
                                        ChipType = RTL8103E_EL_REV_C;
                                }

                                break;
                        } else if ( MacVer == 0x40800000 ) {
                                //
                                // 8105E Rev.C or later
                                //

                                if (RevId == 0x000000) {
                                        ChipType = RTL8105E_REV_C;
                                } else if (RevId == 0x100000) {
                                        ChipType = RTL8105E_REV_D;
                                } else if (RevId == 0x200000) {
                                        ChipType = RTL8105E_REV_F;
                                } else if (RevId == 0x300000) {
                                        ChipType = RTL8105E_REV_G;
                                } else if (RevId == 0x400000) {
                                        ChipType = RTL8105E_REV_H;
                                } else {
                                        ChipType = RTL8105E_REV_H;
                                }
                                break;
                        } else if ( MacVer == 0x44800000 ) {
                                //
                                // 8106E
                                //

                                if (RevId == 0x000000) {
                                        ChipType = RTL8106E_REV_A;
                                } else if (RevId == 0x100000) {
                                        ChipType = RTL8106E_REV_B;
                                } else {
                                        ChipType = RTL8106E_REV_B;
                                }
                                break;
                        } else if ( MacVer == 0x3c800000 ) {
                                //
                                // 8168CP rev A and rev B MUST write at the same time because one parameter will crash rev B IC
                                //
                                if ( RevId == 0x300000) {
                                        ChipType = RTL8168CP_REV_D;
                                } else if ( RevId == 0x200000 ) {
                                        ChipType = RTL8168CP_REV_C;
                                } else if ( RevId == 0x100000 ) {
                                        ChipType = RTL8168CP_REV_B;
                                } else if ( RevId == 0x000000 ) {
                                        ChipType = RTL8168CP;
                                } else {
                                        ChipType = RTL8168CP_REV_D;
                                }
                                break;
                        } else if ( MacVer == 0x28000000 ) {
                                // for 8168D, if chip rev is newer than driver, use standard PHY
                                if ( RevId == 0x300000 ) {
                                        ChipType = RTL8168D_REV_C_REV_D;
                                } else if (RevId == 0x200000) {
                                        ChipType = RTL8168D_REV_C_REV_D;
                                } else if ( RevId == 0x100000 ) {
                                        ChipType = RTL8168D_REV_B;
                                } else if ( RevId == 0x000000 ) {
                                        ChipType = RTL8168D;
                                } else {
                                        ChipType = RTL8168D_REV_C_REV_D;
                                }

                                break;
                        } else if ( MacVer == 0x24000000 ) {
                                ChipType = RTL8401;

                                break;
                        } else if ( MacVer == 0x28800000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168DP;
                                } else if ( RevId == 0x100000) {
                                        ChipType = RTL8168DP_REV_B;
                                } else if ( RevId == 0x200000) {
                                        ChipType = RTL8168DP_REV_E;
                                } else if ( RevId == 0x300000) {
                                        ChipType = RTL8168DP_REV_F;
                                } else {
                                        ChipType = RTL8168DP_REV_F;
                                }
                                break;
                        } else if ( MacVer == 0x2c000000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168E;
                                } else if ( RevId == 0x100000) {
                                        ChipType = RTL8168E_REV_B;
                                } else if ( RevId == 0x200000) {
                                        ChipType = RTL8168E_REV_C;
                                } else {
                                        ChipType = RTL8168E_REV_C;
                                }
                                break;
                        } else if ( MacVer == 0x2c800000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168F;
                                } else if ( RevId == 0x100000) {
                                        ChipType = RTL8168F_REV_B;
                                } else {
                                        ChipType = RTL8168F_REV_B;

                                }
                                break;
                        } else if ( MacVer == 0x48000000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168FB;
                                } else if ( RevId == 0x100000) {
                                        ChipType = RTL8168FB_REV_B;
                                } else {
                                        ChipType = RTL8168FB_REV_B;
                                }
                                break;
                        } else if ( MacVer == 0x4C000000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168G;
                                } else if ( RevId == 0x100000) {
                                        ChipType = RTL8168G_REV_B;
                                } else {
                                        ChipType = RTL8168G_REV_B;
                                }
                                break;
                        } else if ( MacVer == 0x54000000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168H;
                                } else {
                                        ChipType = RTL8168H;
                                }
                                break;
                        } else if ( MacVer == 0x50800000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168GU;
                                } else if ( RevId == 0x100000) {
                                        ChipType = RTL8168GU_REV_B;
                                } else {
                                        ChipType = RTL8168GU_REV_B;
                                }
                                break;
                        } else if ( MacVer == 0x48800000 ) { //Giga Barrosa
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8411;
                                } else {
                                        ChipType = RTL8411;
                                }
                                break;
                        } else if ( MacVer == 0x5C800000 ) { //RTL8411B

                                if ( RevId == 0x000000) {
                                        ChipType = RTL8411B;
                                } else {
                                        ChipType = RTL8411B;
                                }
                                break;
                        } else if ( MacVer == 0x44000000 ) { //10/100 Barrosa
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8402;
                                } else {
                                        ChipType = RTL8402;
                                }
                                break;
                        } else if ( MacVer == 0x50000000 ) {
                                if ( RevId == 0x000000) {
                                        ChipType = RTL8168EP;
                                } else if(RevId == 0x100000) {
                                        ChipType = RTL8168EP_REV_B;
                                } else if(RevId == 0x200000) {
                                        ChipType = RTL8168EP_REV_C;
                                } else {
                                        ChipType = RTL8168EP_REV_C;
                                }
                                break;
                        }
                } while(FALSE);
        }

        close(sd);

        dbgprint(MPL_DBG, "%s: TCR=0x%x, ChipType=%d\n", devname, TcrValue, ChipType);

        return ChipType;
}

/*
 * get changed mac chip type
 * @param rt_nic_info *nicinfo
 * @return return chip type changed or not
 */
u8 get_rt_changed_mac_chiptype(rt_nic_info *nicinfo)
{
        u8 chiptype_changed = FALSE;
        int sd;

        if ((sd = create_raw_socket(ETH_P_LOOPBACK)) < 0)
                return chiptype_changed;

        switch(nicinfo->ChipType) {
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                if(0x8137 == read_pci_cfg(sd, nicinfo, 2, 0x2E)) {
                        nicinfo->Nic8168DChangeTo8104 = TRUE;
                        chiptype_changed = TRUE;
                }
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                if(BIT0 & read_mac(sd, nicinfo, 0xF1, 1)) {
                        nicinfo->Nic8168EChangeTo8105 = TRUE;
                        chiptype_changed = TRUE;
                }
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                if(0x06 == read_pci_cfg(sd, nicinfo, 1, 0x08)) {
                        nicinfo->Nic8168FChangeTo8168E = TRUE;
                        chiptype_changed = TRUE;
                }
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
                if(0x06 == read_pci_cfg(sd, nicinfo, 1, 0x08)) {
                        nicinfo->Nic8168FChangeTo8168E = TRUE;
                        chiptype_changed = TRUE;
                } else {
                        if(0x04 == read_eri(sd, nicinfo, 0xE6, 1) ||
                            0x05 == read_eri(sd, nicinfo, 0xE6, 1)) {
                                nicinfo->Nic8168FBChangeTo8119 = TRUE;
                                chiptype_changed = TRUE;
                        }
                }
                break;
        case RTL8411:
                if(0x0F == read_pci_cfg(sd, nicinfo, 1, 0x08)) {
                        nicinfo->Nic8411ChangeTo8411AAR = TRUE;
                        chiptype_changed = TRUE;
                }
                break;
        case RTL8168H:
                if(0x10 == read_eri(sd, nicinfo, 0xE6, 1)) {
                        nicinfo->Nic8168HChangeTo8107E = TRUE;
                        chiptype_changed = TRUE;
                } else if((0x02 == read_eri(sd, nicinfo, 0xE6, 1) && 0x16 == read_pci_cfg(sd, nicinfo, 1, 0x08))
                          || (0x03 == read_eri(sd, nicinfo, 0xE6, 1) && 0x17 == read_pci_cfg(sd, nicinfo, 1, 0x08))) {
                        nicinfo->Nic8168HChangeTo8118AS = TRUE;
                        chiptype_changed = TRUE;
                }
                break;
        case RTL8401:
                if(0x09 == read_pci_cfg(sd, nicinfo, 1, 0x08)) {
                        nicinfo->Nic8401ChangeTo8103EVL = TRUE;
                        chiptype_changed = TRUE;
                }
                break;
        }

        close(sd);

        return chiptype_changed;
}

int reg_rw_test(rt_nic_info *nicinfo)
{
        u32 low = 0, high = 0;
        u32 tmplow = 0, tmphigh = 0;
        int sd, ret = -1;

        dbgprint(MPL_MSG, YELLOW "%s: register read/write test" NORMAL "\n",
                 nicinfo->name);

        if ((sd = create_raw_socket(ETH_P_LOOPBACK)) < 0)
                goto regrw_exit;

        /* backup */
        low = read_mac(sd, nicinfo, MAR0, 4);
        high = read_mac(sd, nicinfo, MAR0+4, 4);

        dbgprint(MPL_INFO, "%s: backup %02x:%02x:%02x:%02x:%02x:%02x\n",
                 nicinfo->name, low&0xFF, (low>>8)&0xFF, (low>>16)&0xFF,
                 (low>>24)&0xFF, (high)&0xFF, (high>>8)&0xFF);

        /* write tested */
        write_mac(sd, nicinfo, MAR0, 0x5a5a5a, 4);
        write_mac(sd, nicinfo, MAR0+4, 0x5a5a5a, 4);

        /* read tested */
        tmplow = read_mac(sd, nicinfo, MAR0, 4);
        tmphigh = read_mac(sd, nicinfo, MAR0+4, 4);

        dbgprint(MPL_INFO, "%s: read tested %02x:%02x:%02x:%02x:%02x:%02x\n",
                 nicinfo->name, tmplow&0xFF, (tmplow>>8)&0xFF, (tmplow>>16)&0xFF,
                 (tmplow>>24)&0xFF, (tmphigh)&0xFF, (tmphigh>>8)&0xFF);

        /* restore backup */
        write_mac(sd, nicinfo, MAR0, low, 4);
        write_mac(sd, nicinfo, MAR0+4, high, 4);

        close(sd);

        ret = (tmplow == 0x5a5a5a && tmphigh == 0x5a5a5a) ? 0 : -1;

regrw_exit:

        return ret;
}

int enable_rtl_diag(rt_nic_info *nicinfo, u8 enable_diag)
{
        int sd, ret = -1;
        struct ifreq ifr;

        if (enable_diag) {
                dbgprint(MPL_MSG, YELLOW "%s: enable rtl diag " NORMAL "\n",
                         nicinfo->name);
        } else {
                dbgprint(MPL_MSG, YELLOW "%s: disable rtl diag " NORMAL "\n",
                         nicinfo->name);
        }

        strncpy(ifr.ifr_name, nicinfo->name, IFNAMSIZ);

        do {
                if ((sd = create_raw_socket(ETH_P_LOOPBACK)) < 0)
                        break;

                if (nicinfo->busfeature & PCI_NIC) {
                        struct rtltool_pci_cmd cmd;

                        ifr.ifr_data = (__caddr_t)&cmd;

                        if (enable_diag) {
                                SET_PCI_WRITE_CMD(0, 0, 0, RTL_ENABLE_PCI_DIAG);
                        } else {
                                SET_PCI_WRITE_CMD(0, 0, 0, RTL_DISABLE_PCI_DIAG);
                        }

                        if (ioctl(sd, SIOCRTLPCITOOL, &ifr) < 0) {
                                dbgprint(MPL_ERR, "%s: ioctl failed!!!\n", __func__);
                                dbgperror(MPL_INFO, "SIOCRTLPCITOOL");
                                ret = -EIOCTL;
                                break;
                        }
                } else if (nicinfo->busfeature & USB_NIC) {
                        struct rtltool_usb_cmd cmd;

                        ifr.ifr_data = (__caddr_t)&cmd;

                        if (enable_diag) {
                                SET_USB_WRITE_CMD(0, 0, 0, 0, RTL_ENABLE_USB_DIAG);
                        } else {
                                SET_USB_WRITE_CMD(0, 0, 0, 0, RTL_DISABLE_USB_DIAG);
                        }
                        if (ioctl(sd, SIOCRTLUSBTOOL, &ifr) < 0) {
                                dbgprint(MPL_ERR, "%s: ioctl failed!!!\n", __func__);
                                dbgperror(MPL_INFO, "SIOCRTLUSBTOOL");
                                ret = -EIOCTL;
                                break;
                        }
                } else
                        break;

                ret = 0;

                close(sd);
        } while(FALSE);

        return ret;
}
