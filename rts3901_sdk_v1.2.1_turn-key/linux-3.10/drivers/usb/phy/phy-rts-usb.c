#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <asm/ioctl.h>
#include <linux/delay.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/usb/phy.h>
#include <linux/cdev.h>

#include <linux/rts_sysctl.h>


struct rts_usb_dev_phy_regs {
	u32 reserved[5];

	u32 dphy_cfg1;

	u32 dphy_cfg2;

	u32 dev_cfg;
};

struct rts_usb_phy_regs {
	u32 phy_mdio;

	u32 aphy_cfg1;

	u32 aphy_cfg2;
#define UPHY_HST_HS_CKSEL               0x08

	u32 aphy_cfg3;

	u32 aphy_cfg4;

	u32 aphy_cfg5;

	u32 aphy_cfg6;
#define UPHY_U2_APHY_RST_N		0x02

	u32 dphy_cfg1;

	u32 dphy_cfg2;
#define UPHY_U2_DPHY_CHECK_CHIRPK 0x100

	u32 host_cfg;
#define UPHY_PORT_OC_EN			0x01
#define UPHY_HST_FORCE_PORT_PWR_EN	0x02
#define UPHY_HST_AUTO_PPD_ON_OC		0x08
#define UPHY_PORT_PWR_POLARITY_SEL_HI	0x20000
#define UPHY_PORT_OC_POLARITY_SEL_HI	0x10000
};

struct rts_usb_phy {
	struct usb_phy				phy;
	struct device				*dev;
	struct rts_usb_phy_regs __iomem		*regs;
	struct rts_usb_dev_phy_regs __iomem	*dev_regs;

	struct mutex				mutex;

#ifdef CONFIG_USB_RTS_PHY_DEBUG
	struct cdev				cdev;
#endif
};

uint32_t rts_usb_read_phy_int(struct usb_phy *phy)
{
	struct rts_usb_phy *rts = container_of(phy, struct rts_usb_phy, phy);
	uint32_t int_val;

	int_val = readl(&rts->dev_regs->dev_cfg);
	return int_val;
}
EXPORT_SYMBOL(rts_usb_read_phy_int);

void rts_usb_clear_phy_int(struct usb_phy *phy, uint32_t int_val, uint32_t clr_msk)
{
	struct rts_usb_phy *rts = container_of(phy, struct rts_usb_phy, phy);

	clr_msk = ~clr_msk;
	writel(int_val & clr_msk, &rts->dev_regs->dev_cfg);
}
EXPORT_SYMBOL(rts_usb_clear_phy_int);

#ifdef CONFIG_SOC_FPGA_CODE
#define DEV_PORT			0
#define HST_PORT			1

#define PORT_CFG0(port)			((port) << 4)
#define PORT_CFG1(port)			(((port) << 4) | 0x01)
#define PORT_CFG2(port)			(((port) << 4) | 0x02)
#define PORT_CFG3(port)			(((port) << 4) | 0x03)

#define USB2_CLK_CFG0			0x100
#define USB2_CLK_CFG1			0x101
#define USB2_CLK_CFG2			0x102
#define USB2_CLK_CFG3			0x103

#define APHY_RPDEN			0x40

#define APHY_CALEN_DEV			0x10
#define APHY_NSQ_DEV			0x20
#define APHY_CALEN_HST			0x400
#define APHY_NSQ_HST			0x800
#define APHY_SD_CAL_DEV_SHIFT		0
#define APHY_SD_CAL_DEV_MASK		0x0F
#define APHY_SD_CAL_DEV_INIT		(0x08 << APHY_SD_CAL_DEV_SHIFT)
#define APHY_SD_CAL_HST_SHIFT		6
#define APHY_SD_CAL_HST_MASK		0x3C0
#define APHY_SD_CAL_HST_INIT		(0x08 << APHY_SD_CAL_HST_SHIFT)
#define APHY_CALEN_INIT			(APHY_SD_CAL_HST_INIT | APHY_SD_CAL_DEV_INIT)


/*
 *    Bit 3:  MDO (MISO)
 *    Bit 2:  MDI OE (MOSI OE)
 *    Bit 1:  MDI (MOSI)
 *    Bit 0:  MDC
 */

#define MDI_H		0x02
#define MDI_L		0x00
#define MDO_H		0x08
#define MDO_L		0x00
#define MDC_H		0x01
#define MDC_L		0x00
#define MD_OE		0x04

#define MDC_MASK	0x01
#define MDI_MASK	0x02
#define MDO_MASK	0x08

/* Push data at falling edge */
#define MDI_CLK_W(rts, init, bit)				\
do {								\
	(init) |= MD_OE;					\
	(init) |= MDC_H;					\
	writel(init, &(rts)->regs->phy_mdio);			\
	udelay(1);						\
	(init) &= ~(MDC_MASK | MDI_MASK);			\
	(init) |= (bit);					\
	writel(init, &(rts)->regs->phy_mdio);			\
	udelay(5);						\
} while (0)

/* Push data at rising edge */
#define MDI_CLK_W_RISING(rts, init, bit)			\
do {								\
	(init) |= MD_OE;					\
	(init) |= MDC_H;					\
	writel(init, &(rts)->regs->phy_mdio);			\
	(init) &= ~MDI_MASK;					\
	(init) |= (bit);					\
	writel(init, &(rts)->regs->phy_mdio);			\
	udelay(1);						\
	(init) &= ~MDC_MASK;					\
	writel(init, &(rts)->regs->phy_mdio);			\
	udelay(5);						\
} while (0)

/* Sample data at falling edge */
#define MDO_CLK_R(rts, init, bit)				\
do {								\
	u32 _tmp_val;						\
	(init) &= ~MD_OE;					\
	(init) |= MDC_H;					\
	writel(init, &(rts)->regs->phy_mdio);			\
	udelay(1);						\
	(init) &= ~(MDC_MASK | MDI_MASK);			\
	writel(init, &(rts)->regs->phy_mdio);			\
	_tmp_val = readl(&(rts)->regs->phy_mdio);		\
	(bit) = (_tmp_val & MDO_H) ? 1 : 0;			\
	udelay(5);						\
} while (0)

static u16 mdio_read(struct rts_usb_phy *rts, u16 addr)
{
	u32 init = readl(&rts->regs->phy_mdio);
	u16 data = 0;
	int i;

	/* PRE */
	for (i = 0; i < 40; i++)
		MDI_CLK_W(rts, init, MDI_H);

	/* ST */
	MDI_CLK_W(rts, init, MDI_L);
	MDI_CLK_W(rts, init, MDI_H);

	/* OP */
	MDI_CLK_W(rts, init, MDI_H);
	MDI_CLK_W(rts, init, MDI_L);

	/* Address */
	for (i = 9; i >= 0; i--) {
		int hi = (addr >> i) & 0x01;

		if (hi)
			MDI_CLK_W(rts, init, MDI_H);
		else
			MDI_CLK_W(rts, init, MDI_L);
	}

	/* TA */
	MDI_CLK_W(rts, init, MDI_H);
	MDI_CLK_W(rts, init, MDI_L);

	/* Data */
	for (i = 15; i >= 0; i--) {
		u8 bit;
		MDO_CLK_R(rts, init, bit);
		data |= bit << i;
	}

	/* Idle */
	for (i = 0; i < 10; i++)
		MDI_CLK_W_RISING(rts, init, MDI_H);

	return data;
}

static void mdio_write(struct rts_usb_phy *rts, u16 addr, u16 val)
{
	u32 init = readl(&rts->regs->phy_mdio);
	int i;

	/* PRE */
	for (i = 0; i < 40; i++)
		MDI_CLK_W(rts, init, MDI_H);

	/* ST */
	MDI_CLK_W(rts, init, MDI_L);
	MDI_CLK_W(rts, init, MDI_H);

	/* OP */
	MDI_CLK_W(rts, init, MDI_L);
	MDI_CLK_W(rts, init, MDI_H);

	/* Address */
	for (i = 9; i >= 0; i--) {
		int hi = (addr >> i) & 0x01;

		if (hi)
			MDI_CLK_W(rts, init, MDI_H);
		else
			MDI_CLK_W(rts, init, MDI_L);
	}

	/* TA */
	MDI_CLK_W(rts, init, MDI_H);
	MDI_CLK_W(rts, init, MDI_L);

	/* Data */
	for (i = 15; i >= 0; i--) {
		int hi = (val >> i) & 0x01;

		if (hi)
			MDI_CLK_W(rts, init, MDI_H);
		else
			MDI_CLK_W(rts, init, MDI_L);
	}

	/* Idle */
	for (i = 0; i < 10; i++)
		MDI_CLK_W(rts, init, MDI_H);
}

/*
 * We will calibration from bit 3 to bit 0
 * idx stands for the bit index
 */
static void rts_usb_phy_calen_dev(struct rts_usb_phy *rts, int port, int idx)
{
	u32 val;
	u16 mask = 0x01 << idx;
	u8 nsq_dev;

	val = mdio_read(rts, PORT_CFG3(port));
	nsq_dev = !!(val & APHY_NSQ_DEV);
	val &= ~mask;
	val |= ((nsq_dev << idx) | (mask >> 1));
	mdio_write(rts, PORT_CFG3(port), val);
}

static void rts_usb_phy_calen_hst(struct rts_usb_phy *rts, int port, int idx)
{
	u32 val;
	u16 mask = 0x40 << idx;
	u8 nsq_hst;

	val = mdio_read(rts, PORT_CFG3(port));
	nsq_hst = !!(val & APHY_NSQ_HST);
	val &= ~mask;
	val |= ((nsq_hst << (idx + APHY_SD_CAL_HST_SHIFT)) |
			((mask >> 1) & APHY_SD_CAL_HST_MASK));
	mdio_write(rts, PORT_CFG3(port), val);
}

static void rts_usb_reset_aphy(struct rts_usb_phy *rts)
{
	u32 cfg;

	cfg = readl(&rts->regs->aphy_cfg6);
	cfg &= ~UPHY_U2_APHY_RST_N;
	writel(cfg, &rts->regs->aphy_cfg6);
	mdelay(1);
	cfg |= UPHY_U2_APHY_RST_N;
	writel(cfg, &rts->regs->aphy_cfg6);
}

static void rts_usb_pd15k(struct rts_usb_phy *rts, int port)
{
	u16 val;

	val = mdio_read(rts, PORT_CFG0(port));
	val |= APHY_RPDEN;
	mdio_write(rts, PORT_CFG0(port), val);
	mdio_read(rts, PORT_CFG0(port));
}

static void rts_usb_disable_cali(struct rts_usb_phy *rts, int port, u16 calen)
{
	u16 val;

	val = mdio_read(rts, PORT_CFG3(port));
	val &= ~calen;
	mdio_write(rts, PORT_CFG3(port), val);
	mdio_read(rts, PORT_CFG3(port));
}

static int rts_init_usb_phy(struct usb_phy *phy)
{
	struct rts_usb_phy *rts = container_of(phy, struct rts_usb_phy, phy);
	int i, port;
	u32 cfg;
	u16 val;

	mutex_lock(&rts->mutex);

	/* Reset USB Host */
	rts_sys_force_reset(FORCE_RESET_U2HOST);
	mdelay(1);

	/* Reset USB Dev */
	rts_sys_force_reset(FORCE_RESET_U2DEV);
	mdelay(1);

	/* Reset APHY */
	rts_usb_reset_aphy(rts);
	msleep(100);

	/* Device calibration */
	for (port = 0; port < 2; port++) {
		mdio_write(rts, PORT_CFG3(port), APHY_CALEN_INIT);
		mdio_write(rts, PORT_CFG3(port),
				APHY_CALEN_INIT | APHY_CALEN_DEV);

		for (i = 3; i >= 0; i--)
			rts_usb_phy_calen_dev(rts, port, i);

		rts_usb_disable_cali(rts, port, APHY_CALEN_DEV);
	}

	/* Pull down 15K resistor for downstream ports */
	rts_usb_pd15k(rts, HST_PORT);

	/* Host calibration */
	mdio_write(rts, PORT_CFG3(HST_PORT), APHY_CALEN_INIT);
	mdio_write(rts, PORT_CFG3(HST_PORT), APHY_CALEN_INIT | APHY_CALEN_HST);

	for (i = 3; i >= 0; i--)
		rts_usb_phy_calen_hst(rts, HST_PORT, i);

	rts_usb_disable_cali(rts, HST_PORT, APHY_CALEN_HST);

	/* fixed RLE0485 Z0 value 0b'1010 without Auto-Calibration */
	mdio_write(rts, USB2_CLK_CFG2, 0x128);

	/* Improve the RX sensitivity for downstream port */
	val = mdio_read(rts, PORT_CFG3(HST_PORT));
	val = ((val & 0x0F) < 0x0E) ? (val + 2) : (val | 0x0F);
	mdio_write(rts, PORT_CFG3(HST_PORT), val);

	/* optimize rts3901 version c usb timing */
	if (RTS_SOC_HW_ID.hw_ver == HW_ID_VER_RTS3901 &&
		RTS_SOC_HW_ID.hw_rev == HW_ID_REV_C) {
		u32 cfg;

		cfg = readl(&rts->regs->aphy_cfg2);
		cfg |= UPHY_HST_HS_CKSEL;
		writel(cfg, &rts->regs->aphy_cfg2);
	}

	mutex_unlock(&rts->mutex);
	return 0;
}
#else
static int rts_init_usb_phy(struct usb_phy *phy)
{
	struct rts_usb_phy *rts = container_of(phy, struct rts_usb_phy, phy);
	u32 cfg;

	mutex_lock(&rts->mutex);

	/* Reset USB Host */
	rts_sys_force_reset(FORCE_RESET_U2HOST);
	mdelay(1);

	/* Reset USB Dev */
	rts_sys_force_reset(FORCE_RESET_U2DEV);
	mdelay(1);

	/* check chirp K with cdr_vld for compatibility */
	cfg = readl(&rts->regs->dphy_cfg2);
	cfg |= UPHY_U2_DPHY_CHECK_CHIRPK;
	writel(cfg, &rts->regs->dphy_cfg2);

	/* optimize rts3901 version c usb timing */
	if (RTS_SOC_HW_ID.hw_ver == HW_ID_VER_RTS3901 &&
		RTS_SOC_HW_ID.hw_rev == HW_ID_REV_C) {
		u32 cfg;

		cfg = readl(&rts->regs->aphy_cfg2);
		cfg |= UPHY_HST_HS_CKSEL;
		writel(cfg, &rts->regs->aphy_cfg2);
	}

	mutex_unlock(&rts->mutex);
	return 0;
}
#endif

#ifdef CONFIG_USB_RTS_PHY_DEBUG
static int usb_phy_mdio_open(struct inode *inode, struct file *file)
{
	struct rts_usb_phy *rts =
		container_of(inode->i_cdev, struct rts_usb_phy, cdev);

	file->private_data = rts;
	return 0;
}

static int usb_phy_mdio_release(struct inode *inode, struct file *file)
{
	return 0;
}

#define USB_PHY_MDIO_IOC_MAGIC		0x74

#define USB_PHY_MDIO_IOC_READ		_IOWR(USB_PHY_MDIO_IOC_MAGIC, 0xA2, int)
#define USB_PHY_MDIO_IOC_WRITE		_IOWR(USB_PHY_MDIO_IOC_MAGIC, 0xA3, int)

static long usb_phy_mdio_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct rts_usb_phy *rts = file->private_data;
	int retval = 0;

	mutex_lock(&rts->mutex);

	switch (cmd) {
	case USB_PHY_MDIO_IOC_READ:
	{
		u16 addr, val;
		u8 buf[2];

		if (copy_from_user(buf, (char *)arg, 2)) {
			retval = -EFAULT;
			break;
		} else {
			retval = 0;
		}

		addr = ((u16)buf[0] << 8) | buf[1];

		val = mdio_read(rts, addr);
		buf[0] = (u8)(val >> 8);
		buf[1] = (u8)val;

		if (copy_to_user((char *)arg, buf, 2))
			retval = -EFAULT;
		else
			retval = 0;
		break;
	}

	case USB_PHY_MDIO_IOC_WRITE:
	{
		u16 addr, val;
		u8 buf[4];

		if (copy_from_user(buf, (char *)arg, 4)) {
			retval = -EFAULT;
			break;
		} else {
			retval = 0;
		}

		addr = ((u16)buf[0] << 8) | buf[1];
		val = ((u16)buf[2] << 8) | buf[3];

		mdio_write(rts, addr, val);
		break;
	}

	default:
		return -EINVAL;
	}

	mutex_unlock(&rts->mutex);
	return retval;
}

static const struct file_operations usb_phy_mdio_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= usb_phy_mdio_ioctl,
	.open		= usb_phy_mdio_open,
	.release	= usb_phy_mdio_release,
};

dev_t usb_phy_mdio_devno = MKDEV(121, 0);
struct cdev usb_phy_mdio_cdev;
#endif

static int rts_usb_phy_probe(struct platform_device *pdev)
{
	struct rts_usb_phy *rts;
	struct resource *res;
#ifdef CONFIG_USB_RTS_PHY_DEBUG
	int err;
#endif

	rts = devm_kzalloc(&pdev->dev, sizeof(*rts), GFP_KERNEL);
	if (!rts)
		return -ENOMEM;

	mutex_init(&rts->mutex);

	rts->dev = &pdev->dev;

	rts->phy.dev = rts->dev;
	rts->phy.label = "rtsuphy";
	rts->phy.type = USB_PHY_TYPE_USB2;
	rts->phy.init = rts_init_usb_phy;

	usb_add_phy_dev(&rts->phy);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource provided\n");
		return -ENXIO;
	}

	rts->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rts->regs))
		return PTR_ERR(rts->regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource provided of dev phy\n");
		return -ENXIO;
	}

	rts->dev_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rts->dev_regs))
		return PTR_ERR(rts->dev_regs);

	platform_set_drvdata(pdev, rts);

#ifdef CONFIG_USB_RTS_PHY_DEBUG
	err = register_chrdev_region(usb_phy_mdio_devno, 1, "usb_phy_mdio");
	if (err) {
		dev_err(&pdev->dev, "register_chrdev_region fail\n");
		return -ENODEV;
	}

	cdev_init(&rts->cdev, &usb_phy_mdio_fops);
	usb_phy_mdio_cdev.owner = THIS_MODULE;

	err = cdev_add(&rts->cdev, usb_phy_mdio_devno, 1);
	if (err) {
		dev_err(&pdev->dev, "cdev_add fail\n");
		unregister_chrdev_region(usb_phy_mdio_devno, 1);
		return -ENODEV;
	}
#endif

	dev_info(&pdev->dev, "Initialized Realtek IPCam USB Phy module\n");
	return 0;
}

static int rts_usb_phy_remove(struct platform_device *pdev)
{
#ifdef CONFIG_USB_RTS_PHY_DEBUG
	struct rts_usb_phy *rts = platform_get_drvdata(pdev);

	cdev_del(&rts->cdev);
	unregister_chrdev_region(usb_phy_mdio_devno, 1);
#endif
	return 0;
}

static struct platform_driver rts_usb_phy_driver = {
	.probe		= rts_usb_phy_probe,
	.remove		= rts_usb_phy_remove,
	.driver		= {
		.name	= "usbphy-platform",
		.owner	= THIS_MODULE,
	},
};

static int __init rts_usb_phy_init(void)
{
	return platform_driver_register(&rts_usb_phy_driver);
}
subsys_initcall(rts_usb_phy_init);

static void __exit rts_usb_phy_exit(void)
{
	platform_driver_unregister(&rts_usb_phy_driver);
}
module_exit(rts_usb_phy_exit);

MODULE_AUTHOR("Wei WANG");
MODULE_DESCRIPTION("Realtek IPCam USB Phy");
MODULE_LICENSE("GPL");
