#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <asm/ioctl.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/delay.h>

struct rts_efuse_regs {
	u32 efuse_cfg;

	u32 efuse_ctl;
#define EFUSE_DONE		0x01

	u32 efuse_cnt;

	u32 efuse_read_dat;

	u32 efuse_dat[6];
};

struct rts_efuse {
	struct device				*dev;
	struct rts_efuse_regs __iomem		*regs;
	struct mutex				mutex;
	struct cdev				cdev;
};

/* cfg bits */
#define EFUSE_EN_B_MASK (1 << 14)
#define EFUSE_POR_MASK (3 << 12)
#define EFUSE_PROG_MASK  (1 << 11)
#define EFUSE_READ_MASK (1 << 10)
#define EFUSE_TMRF_MASK (3 << 8)
#define EFUSE_ADDR_MASK 0xff
#define EFUSE_CNT_MASK 0x7ff
#define EFUSE_POW_ON_MASK (1 << 15)

/* Test Pattern
unsigned char efuse_in[24] = {
	0x01,0x02,0x4,0x8,0x10,0x20,0x40,0x80,
	0x11,0x12,0x14,0x18,0x21,0x22,0x24,0x28,
	0x41,0x42,0x44,0x48,0x81,0x82,0x84,0x88
}; */

static void efuse_init(uint32_t pgm_cnt,
	uint32_t tmrf, struct rts_efuse *rts_pt)
{
	uint32_t efuse_cfg_reg;
	tmrf <<= 8;
	tmrf &= EFUSE_TMRF_MASK;
	efuse_cfg_reg = readl(&rts_pt->regs->efuse_cfg) & ~EFUSE_TMRF_MASK;
	efuse_cfg_reg |= tmrf;

	/* set EFUSE_TMRF bits */
	writel(efuse_cfg_reg, &rts_pt->regs->efuse_cfg);

	/* set EFUSE_CNT register */
	writel(pgm_cnt & EFUSE_CNT_MASK, &rts_pt->regs->efuse_cnt);
}

static uint8_t efuse_read_byte(uint8_t byte_index, struct rts_efuse *rts_pt)
{
	uint32_t reg;
	uint8_t rdata;

	/* POWER ON */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg |= EFUSE_POW_ON_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* por enable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg |= EFUSE_POR_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);
	udelay(1);

	/* en_b enable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~(EFUSE_PROG_MASK);
	reg &= ~(EFUSE_EN_B_MASK);
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* assign new addr */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~EFUSE_ADDR_MASK;
	reg |= byte_index;
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* read active */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg |= EFUSE_READ_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* read data */
	rdata = readl(&rts_pt->regs->efuse_read_dat);

	/* read disable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~(EFUSE_READ_MASK);
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* en_b disable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg |= EFUSE_EN_B_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);
	udelay(1);

	/* por disable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~(EFUSE_POR_MASK);
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* power off */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~EFUSE_POW_ON_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* set read done */
	reg = 1;
	writel(reg, &rts_pt->regs->efuse_ctl);

	return rdata;
}

static void efuse_write_byte(uint8_t byte_val,
	uint8_t byte_index, struct rts_efuse *rts_pt)
{
	uint32_t reg;
	uint8_t addr_tmp;
	uint32_t i;

	/* POWER ON */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg |= EFUSE_POW_ON_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* POR active */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg |= EFUSE_POR_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* read disable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~(EFUSE_READ_MASK);
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* en_b active */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~(EFUSE_EN_B_MASK);
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* write bit-by-bit  24byte = 192 bit*/
	byte_index *= 8;
	for (i = 0; i < 8; i++) {
		if (byte_val & 1) {
			addr_tmp = 0;

			/* mask the low 3 bits */
			addr_tmp |= byte_index >> 3;
			addr_tmp &= 0x1f;
			addr_tmp |= byte_index << 5;

			/* set new addr */
			reg = readl(&rts_pt->regs->efuse_cfg);
			reg &= ~EFUSE_ADDR_MASK;
			reg |= addr_tmp;
			writel(reg, &rts_pt->regs->efuse_cfg);

			/* program active */
			reg |= EFUSE_PROG_MASK;
			writel(reg, &rts_pt->regs->efuse_cfg);
			udelay(10);
		}
		byte_val = byte_val >> 1;
		byte_index++;
	}

	/* en_b disable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg |= EFUSE_EN_B_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);
	udelay(1);

	/* por disable */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~(EFUSE_POR_MASK);
	writel(reg, &rts_pt->regs->efuse_cfg);

	/* power off */
	reg = readl(&rts_pt->regs->efuse_cfg);
	reg &= ~EFUSE_POW_ON_MASK;
	writel(reg, &rts_pt->regs->efuse_cfg);
}


static int rts_efuse_open(struct inode *inode, struct file *file)
{
	struct rts_efuse *rts =
		container_of(inode->i_cdev, struct rts_efuse, cdev);

	file->private_data = rts;
	return 0;
}

static int rts_efuse_release(struct inode *inode, struct file *file)
{
	return 0;
}

#define RTS_EFUSE_IOC_MAGIC		0x75

#define RTS_EFUSE_IOC_IOREAD_EFUSE    _IOWR(RTS_EFUSE_IOC_MAGIC, 0xA2, int)
#define RTS_EFUSE_IOC_IOWRITE_EFUSE    _IOWR(RTS_EFUSE_IOC_MAGIC, 0xA3, int)
static long rts_efuse_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	struct rts_efuse *rts = file->private_data;

	mutex_lock(&rts->mutex);

	switch (cmd) {
	case RTS_EFUSE_IOC_IOREAD_EFUSE:
	{
		u32 reg_val, reg_addr;
		u8 buf[4];

		if (copy_from_user(buf, (char *)arg, 4)) {
			retval = -EFAULT;
			break;
		} else
			retval = 0;

		reg_addr = be32_to_cpu(*(u32 *)buf);
		if (reg_addr >= 24) {
			retval = -EINVAL;
			break;
		}

		efuse_init(606, 2, rts);
		reg_val = efuse_read_byte(reg_addr, rts);

		*(u32 *)buf = cpu_to_be32(reg_val);
		if (copy_to_user((char *)arg, buf, 4))
			retval = -EFAULT;
		else
			retval = 0;

		break;
	}
	case RTS_EFUSE_IOC_IOWRITE_EFUSE:
	{
		u32 reg_val, reg_addr;
		u8 buf[8];

		if (copy_from_user(buf, (char *)arg, 8)) {
			retval = -EFAULT;
			break;
		} else
			retval = 0;

		reg_addr = be32_to_cpu(*((u32 *)buf));
		/* efuse data address range check */
		if (reg_addr >= 24) {
			retval = -EINVAL;
			break;
		}

		reg_val = be32_to_cpu(*((u32 *)(buf + 4)));
		/* write password check */
		if ((reg_val & 0xffffff00) != 0xdeadbf00) {
			retval = -EINVAL;
			break;
		}

		efuse_init(606, 2, rts);
		efuse_write_byte((u8)reg_val, (u8)reg_addr, rts);

		break;
	}
	default:
		retval = -EINVAL;
		break;
	}

	mutex_unlock(&rts->mutex);

	return retval;
}

static const struct file_operations rts_efuse_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= rts_efuse_ioctl,
	.open		= rts_efuse_open,
	.release	= rts_efuse_release,
};

dev_t rts_efuse_devno = MKDEV(122, 0);
struct cdev rts_efuse_cdev;

static struct rts_efuse *_rts_efuse;

/*
 * rts_efuse_get_data - Read efuse data
 * @byte_val: char to store the read data
 * @byte_index: int to store the read byte index
 *
 * Return zero if get data succeed
 */
int rts_efuse_get_data(unsigned char *byte_val, unsigned int byte_index)
{
	int status = -1;

	/* efuse addr range 0~23 */
	if (byte_index >= 24 || byte_val == NULL)
		return -1;

	if (_rts_efuse) {
		efuse_init(606, 2, _rts_efuse);
		*byte_val = efuse_read_byte(byte_index, _rts_efuse);
		status = 0;
	}

	return status;
}
EXPORT_SYMBOL_GPL(rts_efuse_get_data);

void rts_efuse_set_done(void)
{
	if (_rts_efuse)
		writel(EFUSE_DONE, &_rts_efuse->regs->efuse_ctl);
}
EXPORT_SYMBOL_GPL(rts_efuse_set_done);


#ifdef CONFIG_RTS_EFUSE_SYSFS
static int start_efuse_write_byte(struct device *dev,
	unsigned char value, unsigned char addr)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rts_efuse *rts = platform_get_drvdata(pdev);

	/* delay 10us, 30MHz xb2 bus-->303, 60MHz xb2 bus-->606*/
	efuse_init(606, 2, rts);
	efuse_write_byte(value, addr, rts);

	return 0;
}

static int start_efuse_read_byte(struct device *dev, unsigned char byte_index)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rts_efuse *rts = platform_get_drvdata(pdev);

	efuse_init(606, 2, rts);
	return efuse_read_byte(byte_index, rts);
}

#define EFUSE_TEST_BYTE_ATTR(_number)	\
	static ssize_t efuse_read_byte##_number(	\
		struct device *dev, struct device_attribute *attr, char *buf)\
	{					\
		return sprintf(buf, "0x%X\n",	\
			start_efuse_read_byte(dev, _number));	\
	}					\
	static ssize_t efuse_write_byte##_number(struct device *dev,	\
		struct device_attribute *attr, const char *buf, size_t size)\
	{	\
		ssize_t status = -EIO;	\
		long value;		\
					\
		status = strict_strtoul(buf, 0, &value);		\
		if (status == 0) {		\
			if ((value & 0xffffff00) == 0xdeadbf00)	\
				start_efuse_write_byte(dev, value & 0xff, _number);\
			status = size;	\
		}			\
					\
		return status;		\
	}				\
	DEVICE_ATTR(efuse_test_byte##_number, 0644,		\
		efuse_read_byte##_number, efuse_write_byte##_number)	\

EFUSE_TEST_BYTE_ATTR(0);
EFUSE_TEST_BYTE_ATTR(1);
EFUSE_TEST_BYTE_ATTR(2);
EFUSE_TEST_BYTE_ATTR(3);
EFUSE_TEST_BYTE_ATTR(4);
EFUSE_TEST_BYTE_ATTR(5);
EFUSE_TEST_BYTE_ATTR(6);
EFUSE_TEST_BYTE_ATTR(7);
EFUSE_TEST_BYTE_ATTR(8);
EFUSE_TEST_BYTE_ATTR(9);
EFUSE_TEST_BYTE_ATTR(10);
EFUSE_TEST_BYTE_ATTR(11);
EFUSE_TEST_BYTE_ATTR(12);
EFUSE_TEST_BYTE_ATTR(13);
EFUSE_TEST_BYTE_ATTR(14);
EFUSE_TEST_BYTE_ATTR(15);
EFUSE_TEST_BYTE_ATTR(16);
EFUSE_TEST_BYTE_ATTR(17);
EFUSE_TEST_BYTE_ATTR(18);
EFUSE_TEST_BYTE_ATTR(19);
EFUSE_TEST_BYTE_ATTR(20);
EFUSE_TEST_BYTE_ATTR(21);
EFUSE_TEST_BYTE_ATTR(22);
EFUSE_TEST_BYTE_ATTR(23);

static struct attribute *efuse_test_attrs[] = {
	&dev_attr_efuse_test_byte0.attr,
	&dev_attr_efuse_test_byte1.attr,
	&dev_attr_efuse_test_byte2.attr,
	&dev_attr_efuse_test_byte3.attr,
	&dev_attr_efuse_test_byte4.attr,
	&dev_attr_efuse_test_byte5.attr,
	&dev_attr_efuse_test_byte6.attr,
	&dev_attr_efuse_test_byte7.attr,
	&dev_attr_efuse_test_byte8.attr,
	&dev_attr_efuse_test_byte9.attr,
	&dev_attr_efuse_test_byte10.attr,
	&dev_attr_efuse_test_byte11.attr,
	&dev_attr_efuse_test_byte12.attr,
	&dev_attr_efuse_test_byte13.attr,
	&dev_attr_efuse_test_byte14.attr,
	&dev_attr_efuse_test_byte15.attr,
	&dev_attr_efuse_test_byte16.attr,
	&dev_attr_efuse_test_byte17.attr,
	&dev_attr_efuse_test_byte18.attr,
	&dev_attr_efuse_test_byte19.attr,
	&dev_attr_efuse_test_byte20.attr,
	&dev_attr_efuse_test_byte21.attr,
	&dev_attr_efuse_test_byte22.attr,
	&dev_attr_efuse_test_byte23.attr,
	NULL,
};

static struct attribute_group efuse_test_attr_group = {
	.name = "efuse_test",
	.attrs = efuse_test_attrs,
};
#endif

static int efuse_probe(struct platform_device *pdev)
{
	struct rts_efuse *rts;
	struct resource *res;
	int err;

	rts = devm_kzalloc(&pdev->dev, sizeof(*rts), GFP_KERNEL);
	if (!rts)
		return -ENOMEM;

	mutex_init(&rts->mutex);

	rts->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource provided");
		return -ENXIO;
	}

	rts->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rts->regs))
		return PTR_ERR(rts->regs);

	platform_set_drvdata(pdev, rts);

	err = register_chrdev_region(rts_efuse_devno, 1, "efuse");
	if (err) {
		dev_err(&pdev->dev, "register_chrdev_region fail\n");
		return -ENODEV;
	}

	cdev_init(&rts->cdev, &rts_efuse_fops);
	rts_efuse_cdev.owner = THIS_MODULE;

	err = cdev_add(&rts->cdev, rts_efuse_devno, 1);
	if (err) {
		dev_err(&pdev->dev, "cdev_add fail\n");
		unregister_chrdev_region(rts_efuse_devno, 1);
		return -ENODEV;
	}

#ifdef CONFIG_RTS_EFUSE_SYSFS
	err = sysfs_create_group(&pdev->dev.kobj, &efuse_test_attr_group);
	if (err) {
		dev_err(&pdev->dev, "sysfs create kobject entry failed!\n");
		return -ENODEV;
	}
#endif

	_rts_efuse = rts;

	return 0;
}

static int efuse_remove(struct platform_device *pdev)
{
	struct rts_efuse *rts = platform_get_drvdata(pdev);

	cdev_del(&rts->cdev);
	unregister_chrdev_region(rts_efuse_devno, 1);

#ifdef CONFIG_RTS_EFUSE_SYSFS
	sysfs_remove_group(&pdev->dev.kobj, &efuse_test_attr_group);
#endif

	_rts_efuse = NULL;

	return 0;
}

static struct platform_driver rts_efuse_driver = {
	.probe		= efuse_probe,
	.remove		= efuse_remove,
	.driver		= {
		.name = "efuse-platform",
	},
};

module_platform_driver(rts_efuse_driver);
MODULE_LICENSE("GPL");
