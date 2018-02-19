#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <asm/irq.h>
#include <linux/bitops.h>		/* bit operations */
#include <linux/io.h>
#include <asm/ioctl.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>

#include <linux/rts_sysctl.h>

#ifdef CONFIG_DBG_IOMEM_DMATEST
#define MAX_DMABUF	10
static int dmabuf_cnt = 0;
static void *dmabuf[MAX_DMABUF];
#endif

static int dbg_iomem_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int dbg_iomem_release(struct inode *inode, struct file *file)
{
	return 0;
}

#define DBG_IOMEM_IOC_MAGIC		0x73

#define DBG_IOMEM_IOC_IOREAD32		_IOWR(DBG_IOMEM_IOC_MAGIC, 0xA2, int)
#define DBG_IOMEM_IOC_IOWRITE32		_IOWR(DBG_IOMEM_IOC_MAGIC, 0xA3, int)

#ifdef CONFIG_DBG_IOMEM_DMATEST
#define DBG_IOMEM_IOC_ALLOC_DMABUF	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xAA, int)
#define DBG_IOMEM_IOC_FREE_DMABUF	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xAB, int)
#define DBG_IOMEM_IOC_COPY_DMABUF	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xAC, int)
#endif

static long dbg_iomem_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	switch (cmd) {
	case DBG_IOMEM_IOC_IOREAD32:
	{
		u32 reg_val, reg_addr;
		u8 buf[4];

		if (copy_from_user(buf, (char *)arg, 4)) {
			retval = -EFAULT;
			break;
		} else {
			retval = 0;
		}

		reg_addr = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
			((u32)buf[2] << 8) | buf[3];
		reg_val = readl((void *)reg_addr);

		buf[0] = (u8)(reg_val >> 24);
		buf[1] = (u8)(reg_val >> 16);
		buf[2] = (u8)(reg_val >> 8);
		buf[3] = (u8)reg_val;

		if (copy_to_user((char *)arg, buf, 4))
			retval = -EFAULT;
		else
			retval = 0;
		break;
	}

	case DBG_IOMEM_IOC_IOWRITE32:
	{
		u32 reg_val, reg_addr;
		u8 buf[8];

		if (copy_from_user(buf, (char *)arg, 8)) {
			retval = -EFAULT;
			break;
		} else {
			retval = 0;
		}

		reg_addr = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
			((u32)buf[2] << 8) | buf[3];
		reg_val = ((u32)buf[4] << 24) | ((u32)buf[5] << 16) |
			((u32)buf[6] << 8) | buf[7];

		writel(reg_val, (void *)reg_addr);
		break;
	}

#ifdef CONFIG_DBG_IOMEM_DMATEST
	case DBG_IOMEM_IOC_ALLOC_DMABUF:
	{
		u32 len, phys_addr;
		void *newbuf;
		u8 buf[4];

		if (copy_from_user(buf, (char *)arg, 4)) {
			retval = -EFAULT;
			break;
		} else {
			retval = 0;
		}

		if (dmabuf_cnt >= MAX_DMABUF) {
			retval = -ENOMEM;
			break;
		}

		len = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
			((u32)buf[2] << 8) | buf[3];
		newbuf = kmalloc(len, GFP_KERNEL);
		if (!newbuf) {
			retval = -ENOMEM;
			break;
		}
		dmabuf[dmabuf_cnt++] = newbuf;
		phys_addr = virt_to_phys(newbuf);
		buf[0] = (u8)(phys_addr >> 24);
		buf[1] = (u8)(phys_addr >> 16);
		buf[2] = (u8)(phys_addr >> 8);
		buf[3] = (u8)phys_addr;

		if (copy_to_user((char *)arg, buf, 4))
			retval = -EFAULT;
		else
			retval = 0;
		break;
	}

	case DBG_IOMEM_IOC_FREE_DMABUF:
	{
		int i;

		for (i = 0; i < dmabuf_cnt; i++)
			kfree(dmabuf[i]);
		dmabuf_cnt = 0;

		break;
	}

	case DBG_IOMEM_IOC_COPY_DMABUF:
	{
		u32 src, dst, len;
		u8 buf[12];
		int result = 0;

		if (copy_from_user(buf, (char *)arg, 12)) {
			retval = -EFAULT;
			break;
		} else {
			retval = 0;
		}

		src = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
			((u32)buf[2] << 8) | buf[3];
		dst = ((u32)buf[4] << 24) | ((u32)buf[5] << 16) |
			((u32)buf[6] << 8) | buf[7];
		len = ((u32)buf[8] << 24) | ((u32)buf[9] << 16) |
			((u32)buf[10] << 8) | buf[11];

		result = rts_dma_copy(dst, src, len);
		if (result < 0) {
			buf[0] = 1;
			result = 0 - result;
		} else {
			buf[0] = 0;
		}
		buf[1] = (u8)(result >> 24);
		buf[2] = (u8)(result >> 16);
		buf[3] = (u8)(result >> 8);
		buf[4] = (u8)result;

		if (copy_to_user((char *)arg, buf, 8))
			retval = -EFAULT;
		else
			retval = 0;

		break;
	}
#endif

	default:
		return -EINVAL;
	}

	return retval;
}

static const struct file_operations dbg_iomem_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= dbg_iomem_ioctl,
	.open		= dbg_iomem_open,
	.release	= dbg_iomem_release,
};

dev_t dbg_iomem_devno = MKDEV(120, 0);
struct cdev dbg_iomem_cdev;

int __init dbg_iomem_init(void)
{
	int err;

	printk(KERN_INFO "dbg_iomem initialized!\n");

	err = register_chrdev_region(dbg_iomem_devno, 1, "dbg_iomem");
	if (err) {
		printk(KERN_ERR "register_chrdev_region fail\n");
		return -ENODEV;
	}

	cdev_init(&dbg_iomem_cdev, &dbg_iomem_fops);
	dbg_iomem_cdev.owner = THIS_MODULE;

	err = cdev_add(&dbg_iomem_cdev, dbg_iomem_devno, 1);
	if (err) {
		printk(KERN_ERR "cdev_add fail\n");
		unregister_chrdev_region(dbg_iomem_devno, 1);
		return -ENODEV;
	}

	return 0;
}

static void __exit dbg_iomem_exit(void)
{
	printk(KERN_INFO "dbg_iomem_gpio exit!\n");

	/* De-register */
	cdev_del(&dbg_iomem_cdev);
	unregister_chrdev_region(dbg_iomem_devno, 1);
}

module_init(dbg_iomem_init);
module_exit(dbg_iomem_exit);

MODULE_AUTHOR("wwang <wei_wang@realsil.com.cn>");
MODULE_LICENSE("GPL");
