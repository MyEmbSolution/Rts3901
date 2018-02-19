/*
 * rts_hconf.c
 *
 * Copyright(C) 2015 Micky Ching, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/user.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include <linux/mtd/rts_hconf.h>

static DEFINE_MUTEX(hconf_mutex);
#define hconf_lock()			\
	mutex_lock(&hconf_mutex)
#define hconf_unlock()			\
	mutex_unlock(&hconf_mutex)

typedef long copyfunc(void *to, const void *from, size_t size);

static uint hconf_mtd_idx = 0;
module_param(hconf_mtd_idx, uint, 0400);
MODULE_PARM_DESC(hconf_mtd_idx,  "hconf partition mtd index number");

static long kfromk(void *to, const void *from, size_t size)
{
	memcpy(to, from, size);

	return 0;
}

static long kfromu(void *to, const void *from, size_t size)
{
	int err;

	err = copy_from_user(to, from, size);
	if (err)
		hconf_err("copy from user failed\n");

	return err;
}

static long ufromk(void *to, const void *from, size_t size)
{
	int err;

	err = copy_to_user(to, from, size);
	if (err)
		hconf_err("copy to user failed\n");

	return err;
}

/* ref: http://www.opensourceforu.com/2012/01/working-with-mtd-devices/ */
static struct mtd_info *hconf_mtd_get(int index)
{
	struct mtd_info *mtd = NULL;

	mtd = get_mtd_device(mtd, index);
	if (IS_ERR(mtd)) {
		pr_err("mtd%d not exist\n", index);
		return NULL;
	}
	if (mtd->type == MTD_ABSENT) {
		pr_err("mtd%d absent\n", index);
		put_mtd_device(mtd);
		return NULL;
	}
	if (strcmp(mtd->name, HCONF_MTD_NAME)) {
		pr_err("mtd%d name is %s", index, mtd->name);
		put_mtd_device(mtd);
		return NULL;
	}
	pr_info("%s type %u, size 0x%llx, ebsize 0x%x\n",
		mtd->name, mtd->type, mtd->size, mtd->erasesize);
	return mtd;

	return NULL;
}

static void hconf_mtd_put(struct mtd_info *mtd)
{
	put_mtd_device(mtd);
}

static void *hconf_cache_expand(struct rts_hconf *hconf, int num)
{
	struct rts_hconf_header *header = &hconf->header;
	struct rts_hconf_entry *entry = header->entry;

	if (!header->cap_num) {
		header->cap_num = HCONF_ENTRY_NUM_MAX;
		entry = kzalloc(header->cap_num * sizeof(*entry), GFP_KERNEL);
	}

	if (num > header->cap_num)
		hconf_err("---------- EXPAND NOT IMPLEMENTED ----------\n");

	return entry;
}

static void hconf_erase_callback(struct erase_info *ei)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *)ei->priv;
	wake_up(wait_q);
}

static int __rts_hconf_mtd_block_erase(struct rts_hconf *hconf, int index)
{
	int err;
	struct erase_info ei = {0};
	wait_queue_head_t wq;

	init_waitqueue_head(&wq);
	ei.mtd = hconf->mtd;
	ei.addr = hconf->mtd->erasesize * index;
	ei.len = hconf->mtd->erasesize;
	ei.callback = hconf_erase_callback;
	ei.priv = (u_long)&wq;

	err = mtd_erase(hconf->mtd, &ei);
	if (err) {
		hconf_err("Erase block %d failed\n", index);
		return err;
	}

	err = wait_event_interruptible(wq, ei.state == MTD_ERASE_DONE ||
		ei.state == MTD_ERASE_FAILED);
	if (err) {
		hconf_err("Erase block %d interrupted\n", index);
		return err;
	}

	if (ei.state == MTD_ERASE_FAILED) {
		hconf_err("Erase block %d failed\n", index);
		return err;
	}

	return 0;
}

int rts_hconf_mtd_block_erase(struct rts_hconf *hconf, int index)
{
	int err;

	hconf_lock();
	err = __rts_hconf_mtd_block_erase(hconf, index);
	hconf_unlock();

	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_mtd_block_erase);

static int hconf_block_io(struct rts_hconf *hconf, void *buf, int index,
	int write)
{
	int err;
	char *pos;
	size_t size = hconf->mtd->erasesize, retlen = 0;
	loff_t from = index * size;

	pos = buf;
	while (size) {
		if (!write)
			err = mtd_read(hconf->mtd, from, size, &retlen, pos);
		else
			err = mtd_write(hconf->mtd, from, size, &retlen, pos);
		if (err)
			return err;

		from += retlen;
		size -= retlen;
		pos += retlen;
	}

	return 0;
}

int rts_hconf_mtd_block_read(struct rts_hconf *hconf, void *buf, int index)
{
	int err;

	hconf_lock();
	err = hconf_block_io(hconf, buf, index, false);
	hconf_unlock();

	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_mtd_block_read);

int rts_hconf_mtd_block_write(struct rts_hconf *hconf, void *buf, int index)
{
	int err;

	hconf_lock();

	err = __rts_hconf_mtd_block_erase(hconf, index);
	if (err)
		goto unlock;

	err = hconf_block_io(hconf, buf, index, true);
	if (err)
		goto unlock;

unlock:
	hconf_unlock();
	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_mtd_block_write);

static int is_header(const void *buf, copyfunc copy)
{
	struct rts_hconf_header header = {0};

	copy(&header, buf, hconf_header_hlen(&header));

	if (header.magic != HCONF_HEADER_MAGIC)
		goto error;

	if (header.num > HCONF_ENTRY_NUM_MAX)
		goto error;

	if (header.len > HCONF_TOTAL_LEN_MAX)
		goto error;

	return 1;

error:
	hconf_err("invalid header: magic 0x%x, num %d, len %d\n",
		header.magic, header.num, header.len);
	return 0;
}

static int is_entry(const void *buf, copyfunc copy)
{
	struct rts_hconf_entry entry = {0};

	copy(&entry, buf, hconf_entry_hlen(&entry));

	if (entry.type > HCONF_ENTRY_DATA_TYPE)
		goto error;

	if (entry.len > HCONF_ENTRY_LEN_MAX)
		goto error;

	return 1;

error:
	hconf_err("invalid entry: id 0x%x, type %d, len %d\n",
		entry.id, entry.type, entry.len);
	return 0;
}

static u32 entry_id(const void *buf, copyfunc copy)
{
	struct rts_hconf_entry entry = {0};

	copy(&entry, buf, hconf_entry_hlen(&entry));

	return entry.id;
}

static int entry_index(const struct rts_hconf *hconf, u32 id)
{
	const struct rts_hconf_header *header = &hconf->header;
	int i;

	for (i = 0; i < header->num; i++) {
		if (id == header->entry[i].id)
			return i;
	}

	return header->num;
}

int __rts_hconf_cache_clear(struct rts_hconf *hconf)
{
	struct rts_hconf_header *header = &hconf->header;
	int i;

	for (i = 0; i < header->num; i++)
		kfree(header->entry[i].data);
	kfree(header->entry);

	header->magic = HCONF_HEADER_MAGIC;
	header->num = 0;
	header->len = hconf_header_hlen(header);
	header->cap_num = 0;
	header->entry = NULL;

	return 0;
}

int rts_hconf_cache_clear(struct rts_hconf *hconf)
{
	int err;

	hconf_lock();
	err = __rts_hconf_cache_clear(hconf);
	hconf_unlock();

	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_cache_clear);

static int __hconf_cache_from_buf(struct rts_hconf *hconf, const void *buf,
	copyfunc copy)
{
	struct rts_hconf_header *header = &hconf->header;
	struct rts_hconf_entry *entry;
	const char *pos = buf;
	int err, i;

	__rts_hconf_cache_clear(hconf);

	if (!is_header(pos, copy))
		return -EINVAL;

	copy(header, pos, hconf_header_hlen(header));
	header->entry = hconf_cache_expand(hconf, header->num);
	if (!header->entry) {
		err = -ENOMEM;
		goto clear;
	}

	pos += hconf_header_hlen(header);
	entry = header->entry;
	for (i = 0; i < header->num; i++) {
		int data_len;

		if (!is_entry(pos, copy))
			goto clear;

		copy(entry, pos, hconf_entry_hlen(entry));
		data_len = entry->len;
		entry->data = kzalloc(data_len, GFP_KERNEL);
		if (!entry->data) {
			err = -ENOMEM;
			goto clear;
		}
		copy(entry->data, pos + hconf_entry_hlen(entry), data_len);

		pos += hconf_entry_len(entry);
		entry += 1;
	}

	return 0;

clear:
	__rts_hconf_cache_clear(hconf);
	return err;
}

int rts_hconf_cache_from_buf(struct rts_hconf *hconf, const void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_cache_from_buf(hconf, buf, kfromk);
	hconf_unlock();

	return err;
}

static int rts_hconf_cache_from_user_buf(struct rts_hconf *hconf,
	const void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_cache_from_buf(hconf, buf, kfromu);
	hconf_unlock();

	return err;
}

int rts_hconf_cache_init(struct rts_hconf *hconf)
{
	void *buf;
	int err;

	buf = kmalloc(HCONF_TOTAL_LEN_MAX, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	err = rts_hconf_mtd_block_read(hconf, buf, 0);
	if (err)
		goto error;

	err = rts_hconf_cache_from_buf(hconf, buf);
	if (err)
		goto error;

	kfree(buf);
	return 0;

error:
	kfree(buf);
	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_cache_init);

static int __hconf_cache_to_buf(struct rts_hconf *hconf, void *buf,
	copyfunc copy)
{
	struct rts_hconf_header *header = &hconf->header;
	void *pos = buf;
	int i;

	copy(pos, header, hconf_header_hlen(header));

	pos += hconf_header_hlen(header);
	for (i = 0; i < header->num; i++) {
		struct rts_hconf_entry *entry = &header->entry[i];

		copy(pos, entry, hconf_entry_hlen(entry));
		copy(pos + hconf_entry_hlen(entry), entry->data,
			entry->len);

		pos += hconf_entry_len(entry);
	}

	return 0;
}

int rts_hconf_cache_to_buf(struct rts_hconf *hconf, void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_cache_to_buf(hconf, buf, kfromk);
	hconf_unlock();
	return err;
}

static int rts_hconf_cache_to_user_buf(struct rts_hconf *hconf, void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_cache_to_buf(hconf, buf, ufromk);
	hconf_unlock();

	return err;
}

static int __hconf_header_from_buf(struct rts_hconf *hconf, const void *buf,
	copyfunc copy, copyfunc chcopy)
{
	if (!is_header(buf, chcopy))
		return -EINVAL;

	/* NOTE: header is not writable */
	return 0;
}

int rts_hconf_header_from_buf(struct rts_hconf *hconf, const void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_header_from_buf(hconf, buf, kfromk, kfromk);
	hconf_unlock();

	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_header_from_buf);

int rts_hconf_header_from_user_buf(struct rts_hconf *hconf, const void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_header_from_buf(hconf, buf, kfromu, kfromu);
	hconf_unlock();

	return err;
}

static int __hconf_header_to_buf(struct rts_hconf *hconf, void *buf,
	copyfunc copy)
{
	copy(buf, &hconf->header, hconf_header_hlen(&hconf->header));
	return 0;
}

int rts_hconf_header_to_buf(struct rts_hconf *hconf, void *buf)
{
	int err;
	hconf_lock();
	err = __hconf_header_to_buf(hconf, buf, kfromk);
	hconf_unlock();
	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_header_to_buf);

int rts_hconf_header_to_user_buf(struct rts_hconf *hconf, void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_header_to_buf(hconf, buf, ufromk);
	hconf_unlock();

	return err;
}

static int __hconf_entry_from_buf(struct rts_hconf *hconf, const void *buf,
	copyfunc copy)
{
	struct rts_hconf_header *header = &hconf->header;
	struct rts_hconf_entry *entry;
	int idx;

	if (!is_entry(buf, copy))
		return -EINVAL;

	idx = entry_index(hconf, entry_id(buf, copy));
	if (idx == header->num) {
		header->entry = hconf_cache_expand(hconf, idx);
		if (!header->entry)
			return -EINVAL;

		entry = &header->entry[idx];
		copy(entry, buf, hconf_entry_hlen(entry));
		entry->data = kzalloc(entry->len, GFP_KERNEL);
		if (!entry->data)
			return -ENOMEM;

		copy(entry->data, buf + hconf_entry_hlen(entry),
			entry->len);

		header->num += 1;
		header->len += hconf_entry_len(entry);
	} else {
		struct rts_hconf_entry old = header->entry[idx];
		struct rts_hconf_entry new = {0};

		copy(&new, buf, hconf_entry_hlen(&new));
		new.data = kzalloc(new.len, GFP_KERNEL);
		if (!new.data)
			return -ENOMEM;

		entry = &header->entry[idx];
		kfree(entry->data);
		entry->data = new.data;

		copy(entry, buf, hconf_entry_hlen(entry));
		copy(entry->data, buf + hconf_entry_hlen(entry),
			entry->len);

		header->len += hconf_entry_len(entry) - hconf_entry_len(&old);
	}

	return 0;
}

int rts_hconf_entry_from_buf(struct rts_hconf *hconf, const void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_entry_from_buf(hconf, buf, kfromk);
	hconf_unlock();

	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_entry_from_buf);

int rts_hconf_entry_from_user_buf(struct rts_hconf *hconf, const void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_entry_from_buf(hconf, buf, kfromu);
	hconf_unlock();

	return err;
}

static int __hconf_entry_to_buf(struct rts_hconf *hconf, void *buf,
	copyfunc copy, copyfunc chcopy)
{
	struct rts_hconf_header *header = &hconf->header;
	struct rts_hconf_entry *entry;
	int idx;

	if (!is_entry(buf, chcopy))
		return -EINVAL;

	idx = entry_index(hconf, entry_id(buf, chcopy));
	if (idx == header->num) {
		hconf_err("entry not found\n");
		return -EIO;
	}

	entry = &header->entry[idx];
	copy(buf, entry, hconf_entry_hlen(entry));
	copy(buf + hconf_entry_hlen(entry), entry->data,
		entry->len);

	return 0;
}

int rts_hconf_entry_to_buf(struct rts_hconf *hconf, void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_entry_to_buf(hconf, buf, kfromk, kfromk);
	hconf_unlock();

	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_entry_to_buf);

int rts_hconf_entry_to_user_buf(struct rts_hconf *hconf, void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_entry_to_buf(hconf, buf, ufromk, kfromu);
	hconf_unlock();

	return err;
}

static int __hconf_del_entry(struct rts_hconf *hconf, void *buf,
	copyfunc chcopy)
{
	struct rts_hconf_header *header = &hconf->header;
	int i, idx, len;

	if (!is_entry(buf, chcopy))
		return -EINVAL;

	idx = entry_index(hconf, entry_id(buf, chcopy));
	if (idx == header->num) {
		hconf_err("entry not found\n");
		return -EIO;
	}

	len = hconf_entry_len(&header->entry[idx]);
	kfree(header->entry[idx].data);
	for (i = idx; i + 1 < header->num; i++)
		memcpy(&header->entry[i], &header->entry[i+1],
			sizeof(header->entry[i]));

	header->num -= 1;
	header->len -= len;
	return 0;
}

int rts_hconf_del_entry(struct rts_hconf *hconf, void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_del_entry(hconf, buf, kfromk);
	hconf_unlock();

	return err;
}

int rts_hconf_del_entry_user(struct rts_hconf *hconf, void *buf)
{
	int err;

	hconf_lock();
	err = __hconf_del_entry(hconf, buf, kfromu);
	hconf_unlock();

	return err;
}

static int hconf_open(struct inode *inode, struct file *file)
{
	struct rts_hconf *hconf = hconf_from_inode(inode);

	hconf_lock();
	file->private_data = hconf;
	hconf_unlock();

	return 0;
}

static int hconf_release(struct inode *inode, struct file *file)
{
	hconf_lock();
	file->private_data = NULL;
	hconf_unlock();

	return 0;
}

static loff_t hconf_lseek(struct file *file, loff_t offset, int orig)
{
	struct rts_hconf *hconf = file->private_data;

	switch (orig) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += file->f_pos;
		break;
	case SEEK_END:
		offset += hconf->mtd->size;
		break;
	default:
		return -EINVAL;
	}

	if (offset < 0 || offset > hconf->mtd->size)
		return -EINVAL;

	file->f_pos = offset;
	return file->f_pos;
}

static ssize_t hconf_read(struct file *file, char __user *buf, size_t count,
	loff_t *ppos)
{
	struct rts_hconf *hconf = file->private_data;
	int err;

	if (*ppos > 0)
		return 0;

	if (count < hconf->header.len) {
		hconf_err("buffer too short\n");
		return -ENOMEM;
	}

	err = rts_hconf_cache_to_user_buf(hconf, buf);
	if (err)
		return err;

	*ppos += hconf->header.len;

	return hconf->header.len;
}

static ssize_t hconf_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	struct rts_hconf *hconf = file->private_data;
	int err;

	if (*ppos > 0)
		return 0;

	err = rts_hconf_cache_from_user_buf(hconf, buf);
	if (err)
		return err;

	*ppos += hconf->header.len;

	return 0;
}

int rts_hconf_cache_sync(struct rts_hconf *hconf)
{
	char *buf;
	int err;

	buf = kzalloc(HCONF_TOTAL_LEN_MAX, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	err = rts_hconf_cache_to_buf(hconf, buf);
	if (err)
		goto free_buf;

	err = rts_hconf_mtd_block_write(hconf, buf, 0);
	if (err)
		goto free_buf;

	return 0;

free_buf:
	kfree(buf);
	return err;
}
EXPORT_SYMBOL_GPL(rts_hconf_cache_sync);

static long hconf_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct rts_hconf *hconf = file->private_data;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case HCONF_IOC_READ_ALL:
		return rts_hconf_cache_to_user_buf(hconf, argp);
	case HCONF_IOC_WRITE_ALL:
		return rts_hconf_cache_from_user_buf(hconf, argp);
	case HCONF_IOC_READ_HEADER:
		return rts_hconf_header_to_user_buf(hconf, argp);
	case HCONF_IOC_WRITE_HEADER:
		return rts_hconf_header_from_user_buf(hconf, argp);
	case HCONF_IOC_READ_ENTRY:
		return rts_hconf_entry_to_user_buf(hconf, argp);
	case HCONF_IOC_WRITE_ENTRY:
		return rts_hconf_entry_from_user_buf(hconf, argp);
	case HCONF_IOC_SYNC:
		return rts_hconf_cache_sync(hconf);
	case HCONF_IOC_CLEAR:
		return rts_hconf_cache_clear(hconf);
	case HCONF_IOC_DEL_ENTRY:
		return rts_hconf_del_entry_user(hconf, argp);
	default:
		hconf_err("unknown ioctl cmd: 0x%x\n", cmd);
		return -ENOIOCTLCMD;
	}

	return 0;
}

static const struct file_operations hconf_fops = {
	.owner = THIS_MODULE,
	.open = hconf_open,
	.release = hconf_release,
	.read = hconf_read,
	.write = hconf_write,
	.llseek = hconf_lseek,
	.unlocked_ioctl = hconf_ioctl,
};

static struct rts_hconf *rts_hconf_ptr;
struct rts_hconf *rts_hconf_pointer(void)
{
	return rts_hconf_ptr;
}
EXPORT_SYMBOL_GPL(rts_hconf_pointer);

static int rts_hconf_init(struct rts_hconf *hconf)
{
	if (hconf_mtd_idx == 0) {
		pr_err("invalid hconf_mtd_idx!\n");
		return -EINVAL;
	}

	hconf->mtd = hconf_mtd_get(hconf_mtd_idx);
	if (!hconf->mtd) {
		return -ENOMEDIUM;
	}

	rts_hconf_cache_init(hconf);

	return 0;
}

static int __init hconf_init(void)
{
	struct rts_hconf *hconf;
	dev_t dev;
	int err;

	dev = MKDEV(HCONF_CHAR_MAJOR, HCONF_CHAR_MINOR);
	err = register_chrdev_region(dev, HCONF_CHAR_NUM, "hconf");
	if (err < 0) {
		pr_err("register hconf failed\n");
		return err;
	}

	hconf = kzalloc(HCONF_CHAR_NUM * sizeof(*hconf), GFP_KERNEL);
	if (!hconf) {
		pr_err("alloc memory failed\n");
		err = -ENOMEM;
		goto unregister;
	}

	err = rts_hconf_init(hconf);
	if (err) {
		pr_err("hconf init failed\n");
		goto free_buf;
	}

	cdev_init(&hconf->cdev, &hconf_fops);
	hconf->cdev.owner = THIS_MODULE;
	err = cdev_add(&hconf->cdev, dev, HCONF_CHAR_NUM);
	if (err) {
		hconf_err("add char device failed\n");
		goto free_buf;
	}

	hconf_dbg("hconf init success\n");
	rts_hconf_ptr = hconf;
	return 0;

free_buf:
	kfree(hconf);
unregister:
	unregister_chrdev_region(dev, HCONF_CHAR_NUM);
	return err;
}

void __exit hconf_exit(void)
{
	struct rts_hconf *hconf = rts_hconf_ptr;

	rts_hconf_cache_clear(hconf);
	hconf_mtd_put(hconf->mtd);
	cdev_del(&hconf->cdev);
	kfree(hconf);
	unregister_chrdev(HCONF_CHAR_MAJOR, "hconf");
}


module_init(hconf_init);
module_exit(hconf_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Micky Ching <micky_ching@realsil.com.cn>");
MODULE_DESCRIPTION("MTD hardware partition config driver");

