#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/des.h>
#include <linux/clk.h>
#include <linux/rts_sysctl.h>

#include "rlx_crypto.h"

#if 0
#define DBG(args...)	pr_emerg("%s: %s", __func__, args)
#else
#define DBG(args...)
#endif

#define FLAGS_ENCRYPT		BIT(0)
#define FLAGS_DECRYPT		BIT(1)
#define FLAGS_AES		BIT(2)
#define FLAGS_DES		BIT(3)
#define FLAGS_DES3		BIT(4)
#define FLAGS_ECB		BIT(5)
#define FLAGS_CBC		BIT(6)
#define FLAGS_CBCCS1		BIT(7)
#define FLAGS_CBCCS2		BIT(8)
#define FLAGS_CBCCS3		BIT(9)
#define FLAGS_CTR		BIT(10)

#define RLX_DMA_TABLE_SIZE	256
#define RLX_AES_MIN_KEY_SIZE	AES_MIN_KEY_SIZE
#ifdef CONFIG_HW_VER_RTS3901
#define RLX_KEY_LENGTH		32
#define RLX_AES_MAX_KEY_SIZE    AES_MAX_KEY_SIZE
#else
#define RLX_KEY_LENGTH          24
#define RLX_AES_MAX_KEY_SIZE    AES_MIN_KEY_SIZE
#endif

struct rlx_crypto_data {
	struct platform_device *pdev;
	void __iomem *addr;
	unsigned long size;
	u32 base;
	int irq;
	u32 *table_in;
	dma_addr_t dma_table_in;
	u32 *table_out;
	dma_addr_t dma_table_out;
	struct mutex dma_mutex;
	struct completion crypto_complete;
	struct clk *cipher_clk;
};

struct rlx_crypto_ctx {
	u8 key[RLX_KEY_LENGTH];
	unsigned int keylen;
	u8 *iv;
	int ivflag;
	unsigned int ivlen;
	unsigned int mask;
};

static struct rlx_crypto_data *rlx_cdata;

static unsigned int rlx_crypto_read(struct rlx_crypto_data *cdata,
				    unsigned int reg)
{
	DBG("rlx crypto read\n");

	return readl(cdata->addr + reg);
}

static int rlx_crypto_write(struct rlx_crypto_data *cdata, unsigned int reg,
			    unsigned int value)
{
	DBG("rlx crypto write\n");

	writel(value, cdata->addr + reg);

	return 0;
}

static int rlx_sg_nents(struct scatterlist *sg_list, int nbytes)
{
	struct scatterlist *sg = sg_list;
	int sg_nents = 0;

	while (nbytes > 0) {
		++sg_nents;
		nbytes -= sg->length;
		sg = sg_next(sg);
	}

	return sg_nents;
}

static int rlx_setkey(struct crypto_tfm *tfm, const u8 *key, unsigned int len)
{
	struct rlx_crypto_ctx *ctx = crypto_tfm_ctx(tfm);

	DBG("rlx setkey\n");

	ctx->keylen = len;
	memcpy(ctx->key, key, len);

	return 0;
}

/*
static void rlx_crypto_dumpreg(struct rlx_crypto_data *cdata)
{
	u32 val;
	int i;

	printk("rlx crypto dump reg:\n");
	for (i = 0; i <= 0x64; i += 4) {
		val = rlx_crypto_read(cdata, i);
		printk("reg[0x%x] = 0x%x\n", i, val);
	}
	printk("dump end\n");
}
*/

static int rlx_crypto_do(struct rlx_crypto_data *cdata,
			 struct rlx_crypto_ctx *ctx)
{
	int ret = 0;
	u32 val;
	u32 val1;
	u32 *reg_val;

	DBG("rlx crypto do\n");

	if (ctx->ivflag == 1) {
		/* initial iv */
		reg_val = (u32 *)ctx->iv;
		rlx_crypto_write(rlx_cdata, RLX_REG_IV_IN_DATA0,
				 cpu_to_be32(reg_val[0]));
		rlx_crypto_write(rlx_cdata, RLX_REG_IV_IN_DATA1,
				 cpu_to_be32(reg_val[1]));
		if (ctx->ivlen > 8) {
			rlx_crypto_write(rlx_cdata, RLX_REG_IV_IN_DATA2,
					 cpu_to_be32(reg_val[2]));
			rlx_crypto_write(rlx_cdata, RLX_REG_IV_IN_DATA3,
					 cpu_to_be32(reg_val[3]));
		}
	}

	/* start crypto */
	val = rlx_crypto_read(cdata, RLX_REG_CIPHER_CTL);
	rlx_crypto_write(cdata, RLX_REG_CIPHER_CTL, val | 0x1);

	ret = wait_for_completion_timeout(&cdata->crypto_complete,
					  msecs_to_jiffies(10000));
	if (ret == 0) {
		pr_err("timed out\n");
		ret = -ETIMEDOUT;
		goto do_out;
	}

	val = rlx_crypto_read(cdata, RLX_REG_CIPHER_INT_FLAG);
	if (val & 0x4) {
		rlx_crypto_write(cdata, RLX_REG_CIPHER_INT_FLAG, 0x4);
		ret = -EAGAIN;
		goto do_out;
	}

	/* crypto status */
	val = rlx_crypto_read(cdata, RLX_REG_CIPHER_STS);
	if (!(val & 0x1)) {
		pr_err("crypto err\n");
		ret = -EAGAIN;
		goto do_out;
	} else {
		DBG("crypto ok\n");
		if (ctx->ivflag == 1) {
			reg_val = (u32 *)ctx->iv;
			reg_val[0] = be32_to_cpu(rlx_crypto_read(cdata,
							RLX_REG_IV_OUT_DATA0));
			reg_val[1] = be32_to_cpu(rlx_crypto_read(cdata,
							RLX_REG_IV_OUT_DATA1));
			if (ctx->ivlen > 8) {
				reg_val[2] = be32_to_cpu(rlx_crypto_read(cdata,
							RLX_REG_IV_OUT_DATA2));
				reg_val[3] = be32_to_cpu(rlx_crypto_read(cdata,
							RLX_REG_IV_OUT_DATA3));
			}
		}
	}
	ret = 0;
do_out:

	do {
		val = rlx_crypto_read(cdata, RLX_REG_DATA_IN_LENGTH);
		val1 = rlx_crypto_read(cdata,
				RLX_REG_FINISH_BLOCK_NUM);
		if (ctx->mask & (FLAGS_ECB | FLAGS_CBC))
			val1--;
		if (ctx->mask & FLAGS_AES)
			val1 = val1 << 4;
		else if ((ctx->mask & FLAGS_DES) ||
			(ctx->mask & FLAGS_DES3))
			val1 = val1 << 3;
	} while (val != val1);

	val = rlx_crypto_read(cdata, RLX_REG_CIPHER_INT_FLAG);
	if (val & 0x4) {
		rlx_crypto_write(cdata, RLX_REG_CIPHER_INT_FLAG, 0x4);
		ret = -EAGAIN;
	}

	/* stop crypto */
	val = rlx_crypto_read(cdata, RLX_REG_CIPHER_CTL);
	rlx_crypto_write(cdata, RLX_REG_CIPHER_CTL, val | 0x2);

	return ret;
}

static int rlx_crypto(struct blkcipher_desc *desc,
		      struct scatterlist *dst,
		      struct scatterlist *src,
		      unsigned int nbytes,
		      unsigned int mask)
{
	struct rlx_crypto_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	u32 *reg_val;
	u32 val;
	dma_addr_t addr_in, addr_out;
	int ret = 0, i, in_nents, out_nents;
	unsigned int align_mask;
	unsigned int *table_in_ptr = (unsigned int *)rlx_cdata->table_in;
	unsigned int *table_out_ptr = (unsigned int *)rlx_cdata->table_out;
	struct scatterlist *in_sg, *out_sg;

	DBG("rlx crypto\n");

	mutex_lock(&rlx_cdata->dma_mutex);
	clk_prepare_enable(rlx_cdata->cipher_clk);
	rlx_crypto_write(rlx_cdata, RLX_REG_CIPHER_INT_FLAG, 0x5);
	rlx_crypto_write(rlx_cdata, RLX_REG_CIPHER_INT_EN, 0x5);

	/* initialize key, iv, data, mode */
	/* mode */
	ctx->mask = mask;
	val = rlx_crypto_read(rlx_cdata, RLX_REG_CIPHER_CTL);
#ifdef CONFIG_HW_VER_RTS3901
	val = val & 0xFFFFF007;
#else
	val = val & 0xFFFFFC07;
#endif
	align_mask = 0x7;
	if (mask & FLAGS_AES) {
		val = val | ((u32)0x0 << RLX_ALGORITHM_SEL);
#ifdef CONFIG_HW_VER_RTS3901
		if (ctx->keylen == 16)
			val = val | ((u32)0x0 << RLX_AES_MODE_SEL);
		else if (ctx->keylen == 24)
			val = val | ((u32)0x1 << RLX_AES_MODE_SEL);
		else if (ctx->keylen == 32)
			val = val | ((u32)0x2 << RLX_AES_MODE_SEL);
#endif
		align_mask = 0xf;
	} else if (mask & FLAGS_DES) {
		val = val | ((u32)0x1 << RLX_ALGORITHM_SEL);
	} else if (mask & FLAGS_DES3) {
		val = val | ((u32)0x2 << RLX_ALGORITHM_SEL);
	}

	if (nbytes & align_mask) {
		ret = -EINVAL;
		goto out;
	}

	if (mask & FLAGS_ECB)
		val = val | ((u32)0x0 << RLX_OPERATION_MODE);
	else if (mask & FLAGS_CBC)
		val = val | ((u32)0x1 << RLX_OPERATION_MODE);
	else if (mask & FLAGS_CBCCS1)
		val = val | ((u32)0x2 << RLX_OPERATION_MODE);
	else if (mask & FLAGS_CBCCS2)
		val = val | ((u32)0x3 << RLX_OPERATION_MODE);
	else if (mask & FLAGS_CBCCS3)
		val = val | ((u32)0x4 << RLX_OPERATION_MODE);
	else if (mask & FLAGS_CTR)
		val = val | ((u32)0x5 << RLX_OPERATION_MODE);

	if (mask & FLAGS_ECB)
		ctx->ivflag = 0;
	else
		ctx->ivflag = 1;

	if (mask & FLAGS_AES)
		ctx->ivlen = 16;
	else
		ctx->ivlen = 8;

	if (mask & (FLAGS_ECB | FLAGS_CBC))
		val = val | ((u32)0x1 << RLX_PADDING_SEL);
	else
		val = val | ((u32)0x0 << RLX_PADDING_SEL);

	if (mask & FLAGS_ENCRYPT)
		val = val | ((u32)0x0 << RLX_ENCTYPT_DECTYPT_SEL);
	else if (mask & FLAGS_DECRYPT)
		val = val | ((u32)0x1 << RLX_ENCTYPT_DECTYPT_SEL);
	rlx_crypto_write(rlx_cdata, RLX_REG_CIPHER_CTL, val);

	/* key */
	reg_val = (u32 *)ctx->key;
	rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA0, cpu_to_be32(reg_val[0]));
	rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA1, cpu_to_be32(reg_val[1]));
	if (ctx->keylen > 8) {
		rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA2,
				 cpu_to_be32(reg_val[2]));
		rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA3,
				 cpu_to_be32(reg_val[3]));
	}
	if (ctx->keylen > 16) {
		rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA4,
				 cpu_to_be32(reg_val[4]));
		rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA5,
				 cpu_to_be32(reg_val[5]));
	}
#ifdef CONFIG_HW_VER_RTS3901
	if (ctx->keylen > 24) {
		rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA6,
				 cpu_to_be32(reg_val[6]));
		rlx_crypto_write(rlx_cdata, RLX_REG_KEY_DATA7,
				 cpu_to_be32(reg_val[7]));
	}
#endif

	/* iv */
	if (ctx->ivflag == 1)
		ctx->iv = (u8 *)desc->info;

	/* data in & out */
	rlx_crypto_write(rlx_cdata, RLX_REG_IN_TABLE_ADDR,
			 rlx_cdata->dma_table_in);
	rlx_crypto_write(rlx_cdata, RLX_REG_OUT_TABLE_ADDR,
			 rlx_cdata->dma_table_out);

	in_nents = rlx_sg_nents(src, nbytes);
	ret = dma_map_sg(&rlx_cdata->pdev->dev, src,
			in_nents, DMA_TO_DEVICE);
	if (!ret) {
		pr_err("dma map in sg err\n");
		goto out;
	}

	for_each_sg(src, in_sg, ret, i) {
		addr_in = sg_dma_address(in_sg);
		*(table_in_ptr++) = (u32)addr_in;
		*(table_in_ptr++) = sg_dma_len(in_sg);
	}
	rlx_crypto_write(rlx_cdata, RLX_REG_IN_BUF_NUM, ret);

	out_nents = rlx_sg_nents(dst, nbytes);
	ret = dma_map_sg(&rlx_cdata->pdev->dev, dst,
			out_nents, DMA_FROM_DEVICE);
	if (!ret) {
		pr_err("dma map out sg err\n");
		dma_unmap_sg(&rlx_cdata->pdev->dev, src,
				in_nents, DMA_TO_DEVICE);
		goto out;
	}

	for_each_sg(dst, out_sg, ret, i) {
		addr_out = sg_dma_address(out_sg);
		*(table_out_ptr++) = (u32)addr_out;
		*(table_out_ptr++) = sg_dma_len(out_sg);
	}
	rlx_crypto_write(rlx_cdata, RLX_REG_OUT_BUF_NUM, ret);

	rlx_crypto_write(rlx_cdata, RLX_REG_DATA_IN_LENGTH, nbytes);

	ret = rlx_crypto_do(rlx_cdata, ctx);

	/* unmap in & out */
	dma_unmap_sg(&rlx_cdata->pdev->dev, src, in_nents, DMA_TO_DEVICE);
	dma_unmap_sg(&rlx_cdata->pdev->dev, dst, out_nents, DMA_FROM_DEVICE);

out:
	rlx_crypto_write(rlx_cdata, RLX_REG_CIPHER_INT_EN, 0x0);
	clk_disable(rlx_cdata->cipher_clk);
	mutex_unlock(&rlx_cdata->dma_mutex);

	if (-EAGAIN == ret) {
		rts_sys_force_reset(FORCE_RESET_CIPHER);
		ret = rlx_crypto(desc, dst, src, nbytes, mask);
	}

	return ret;
}

/* aes */
static int rlx_aes_ecb_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_AES | FLAGS_ECB);
}

static int rlx_aes_ecb_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_AES | FLAGS_ECB);
}

static int rlx_aes_cbc_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_AES | FLAGS_CBC);
}

static int rlx_aes_cbc_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_AES | FLAGS_CBC);
}

static int rlx_aes_cbccs1_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_AES | FLAGS_CBCCS1);
}

static int rlx_aes_cbccs1_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_AES | FLAGS_CBCCS1);
}

static int rlx_aes_cbccs2_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_AES | FLAGS_CBCCS2);
}

static int rlx_aes_cbccs2_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_AES | FLAGS_CBCCS2);
}

static int rlx_aes_cbccs3_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_AES | FLAGS_CBCCS3);
}

static int rlx_aes_cbccs3_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_AES | FLAGS_CBCCS3);
}

static int rlx_aes_ctr_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_AES | FLAGS_CTR);
}

static int rlx_aes_ctr_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_AES | FLAGS_CTR);
}

/* des */
static int rlx_des_ecb_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES | FLAGS_ECB);
}

static int rlx_des_ecb_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES | FLAGS_ECB);
}

static int rlx_des_cbc_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES | FLAGS_CBC);
}

static int rlx_des_cbc_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES | FLAGS_CBC);
}

static int rlx_des_cbccs1_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES | FLAGS_CBCCS1);
}

static int rlx_des_cbccs1_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES | FLAGS_CBCCS1);
}

static int rlx_des_cbccs2_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES | FLAGS_CBCCS2);
}

static int rlx_des_cbccs2_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES | FLAGS_CBCCS2);
}

static int rlx_des_cbccs3_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES | FLAGS_CBCCS3);
}

static int rlx_des_cbccs3_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES | FLAGS_CBCCS3);
}

static int rlx_des_ctr_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES | FLAGS_CTR);
}

static int rlx_des_ctr_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES | FLAGS_CTR);
}

/* 3des */
static int rlx_des3_ecb_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES3 | FLAGS_ECB);
}

static int rlx_des3_ecb_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES3 | FLAGS_ECB);
}

static int rlx_des3_cbc_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES3 | FLAGS_CBC);
}

static int rlx_des3_cbc_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES3 | FLAGS_CBC);
}

static int rlx_des3_cbccs1_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES3 | FLAGS_CBCCS1);
}

static int rlx_des3_cbccs1_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES3 | FLAGS_CBCCS1);
}

static int rlx_des3_cbccs2_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES3 | FLAGS_CBCCS2);
}

static int rlx_des3_cbccs2_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES3 | FLAGS_CBCCS2);
}

static int rlx_des3_cbccs3_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES3 | FLAGS_CBCCS3);
}

static int rlx_des3_cbccs3_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES3 | FLAGS_CBCCS3);
}

static int rlx_des3_ctr_encrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_ENCRYPT | FLAGS_DES3 | FLAGS_CTR);
}

static int rlx_des3_ctr_decrypt(struct blkcipher_desc *desc,
			       struct scatterlist *dst,
			       struct scatterlist *src,
			       unsigned int nbytes)
{
	return rlx_crypto(desc, dst, src, nbytes,
			  FLAGS_DECRYPT | FLAGS_DES3 | FLAGS_CTR);
}

static struct crypto_alg rlx_algs[] = {
	{
		.cra_name = "ecb(aes)",
		.cra_driver_name = "ecb-aes-rlx",
		.cra_blocksize = AES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = RLX_AES_MIN_KEY_SIZE,
			.max_keysize = RLX_AES_MAX_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_aes_ecb_encrypt,
			.decrypt = rlx_aes_ecb_decrypt,
		},
	}, {
		.cra_name = "cbc(aes)",
		.cra_driver_name = "cbc-aes-rlx",
		.cra_blocksize = AES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = RLX_AES_MIN_KEY_SIZE,
			.max_keysize = RLX_AES_MAX_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_aes_cbc_encrypt,
			.decrypt = rlx_aes_cbc_decrypt,
			.ivsize = AES_MIN_KEY_SIZE,
		},
	}, {
		.cra_name = "cbc-cs1(aes)",
		.cra_driver_name = "cbccs1-aes-rlx",
		.cra_blocksize = AES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = RLX_AES_MIN_KEY_SIZE,
			.max_keysize = RLX_AES_MAX_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_aes_cbccs1_encrypt,
			.decrypt = rlx_aes_cbccs1_decrypt,
			.ivsize = AES_MIN_KEY_SIZE,
		},
	}, {
		.cra_name = "cbc-cs2(aes)",
		.cra_driver_name = "cbccs2-aes-rlx",
		.cra_blocksize = AES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = RLX_AES_MIN_KEY_SIZE,
			.max_keysize = RLX_AES_MAX_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_aes_cbccs2_encrypt,
			.decrypt = rlx_aes_cbccs2_decrypt,
			.ivsize = AES_MIN_KEY_SIZE,
		},
	}, {
		.cra_name = "cbc-cs3(aes)",
		.cra_driver_name = "cbccs3-aes-rlx",
		.cra_blocksize = AES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = RLX_AES_MIN_KEY_SIZE,
			.max_keysize = RLX_AES_MAX_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_aes_cbccs3_encrypt,
			.decrypt = rlx_aes_cbccs3_decrypt,
			.ivsize = AES_MIN_KEY_SIZE,
		},
	}, {
		.cra_name = "ctr(aes)",
		.cra_driver_name = "ctr-aes-rlx",
		.cra_blocksize = AES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = RLX_AES_MIN_KEY_SIZE,
			.max_keysize = RLX_AES_MAX_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_aes_ctr_encrypt,
			.decrypt = rlx_aes_ctr_decrypt,
			.ivsize = AES_MIN_KEY_SIZE,
		},
	},
	{
		.cra_name = "ecb(des)",
		.cra_driver_name = "ecb-des-rlx",
		.cra_blocksize = DES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des_ecb_encrypt,
			.decrypt = rlx_des_ecb_decrypt,
		},
	}, {
		.cra_name = "cbc(des)",
		.cra_driver_name = "cbc-des-rlx",
		.cra_blocksize = DES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des_cbc_encrypt,
			.decrypt = rlx_des_cbc_decrypt,
			.ivsize = DES_BLOCK_SIZE,
		},
	}, {
		.cra_name = "cbc-cs1(des)",
		.cra_driver_name = "cbccs1-des-rlx",
		.cra_blocksize = DES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des_cbccs1_encrypt,
			.decrypt = rlx_des_cbccs1_decrypt,
			.ivsize = DES_BLOCK_SIZE,
		},
	}, {
		.cra_name = "cbc-cs2(des)",
		.cra_driver_name = "cbccs2-des-rlx",
		.cra_blocksize = DES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des_cbccs2_encrypt,
			.decrypt = rlx_des_cbccs2_decrypt,
			.ivsize = DES_BLOCK_SIZE,
		},
	}, {
		.cra_name = "cbc-cs3(des)",
		.cra_driver_name = "cbccs3-des-rlx",
		.cra_blocksize = DES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des_cbccs3_encrypt,
			.decrypt = rlx_des_cbccs3_decrypt,
			.ivsize = DES_BLOCK_SIZE,
		},
	}, {
		.cra_name = "ctr(des)",
		.cra_driver_name = "ctr-des-rlx",
		.cra_blocksize = DES_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des_ctr_encrypt,
			.decrypt = rlx_des_ctr_decrypt,
			.ivsize = DES_BLOCK_SIZE,
		},
	},
	{
		.cra_name = "ecb(des3_ede)",
		.cra_driver_name = "ecb-des3_ede-rlx",
		.cra_blocksize = DES3_EDE_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des3_ecb_encrypt,
			.decrypt = rlx_des3_ecb_decrypt,
		},
	}, {
		.cra_name = "cbc(des3_ede)",
		.cra_driver_name = "cbc-des3_ede-rlx",
		.cra_blocksize = DES3_EDE_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des3_cbc_encrypt,
			.decrypt = rlx_des3_cbc_decrypt,
			.ivsize = DES3_EDE_BLOCK_SIZE,
		},
	}, {
		.cra_name = "cbc-cs1(des3_ede)",
		.cra_driver_name = "cbccs1-des3_ede-rlx",
		.cra_blocksize = DES3_EDE_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des3_cbccs1_encrypt,
			.decrypt = rlx_des3_cbccs1_decrypt,
			.ivsize = DES3_EDE_BLOCK_SIZE,
		},
	}, {
		.cra_name = "cbc-cs2(des3_ede)",
		.cra_driver_name = "cbccs2-des3_ede-rlx",
		.cra_blocksize = DES3_EDE_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des3_cbccs2_encrypt,
			.decrypt = rlx_des3_cbccs2_decrypt,
			.ivsize = DES3_EDE_BLOCK_SIZE,
		},
	}, {
		.cra_name = "cbc-cs3(des3_ede)",
		.cra_driver_name = "cbccs3-des3_ede-rlx",
		.cra_blocksize = DES3_EDE_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des3_cbccs3_encrypt,
			.decrypt = rlx_des3_cbccs3_decrypt,
			.ivsize = DES3_EDE_BLOCK_SIZE,
		},
	}, {
		.cra_name = "ctr(des3_ede)",
		.cra_driver_name = "ctr-des3_ede-rlx",
		.cra_blocksize = DES3_EDE_BLOCK_SIZE,
		.cra_u.blkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.setkey = rlx_setkey,
			.encrypt = rlx_des3_ctr_encrypt,
			.decrypt = rlx_des3_ctr_decrypt,
			.ivsize = DES3_EDE_BLOCK_SIZE,
		},
	}
};

static irqreturn_t rlx_crypto_irq_handler(int irq, void *data)
{
	struct rlx_crypto_data *crypto_data;
	u32 val;

	crypto_data = (struct rlx_crypto_data *)data;

	val = rlx_crypto_read(crypto_data, RLX_REG_CIPHER_INT_FLAG);

	val = (~val) & 0x5;
	rlx_crypto_write(crypto_data, RLX_REG_CIPHER_INT_EN, val);
	complete(&crypto_data->crypto_complete);

	return IRQ_HANDLED;
}

static int rlx_crypto_probe(struct platform_device *pdev)
{
	int ret, i, j;
	struct resource *res;
	struct rlx_crypto_data *crypto_data;

	DBG("rlx crypto probe\n");

	crypto_data = devm_kzalloc(&pdev->dev,
				   sizeof(struct rlx_crypto_data),
				   GFP_KERNEL);
	if (crypto_data == NULL) {
		pr_err("Unable to alloc crypto data\n");
		return -ENOMEM;
	}
	crypto_data->pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("Unable to get crypto address\n");
		ret = -ENXIO;
		goto mem_err;
	}

	if (!request_mem_region(res->start,
				resource_size(res), "rlx-crypto")) {
		pr_err("Unable to request mem region\n");
		ret = -EBUSY;
		goto mem_err;
	}

	crypto_data->base = res->start;
	crypto_data->size = res->end - res->start + 1;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		pr_err("Unable to get crypto irq\n");
		ret = -ENXIO;
		goto err;
	}

	crypto_data->irq = res->start;

	crypto_data->addr = ioremap(crypto_data->base, crypto_data->size);
	if (crypto_data->addr == NULL) {
		pr_err("failed to iorepam\n");
		ret = -ENXIO;
		goto err;
	}

	crypto_data->table_in
	    = dma_alloc_coherent(&pdev->dev, RLX_DMA_TABLE_SIZE,
				 &crypto_data->dma_table_in, GFP_KERNEL);
	if (!crypto_data->table_in) {
		pr_err("Unable to allocate dma table in buffer\n");
		ret = -ENOMEM;
		goto dma_err;
	}

	crypto_data->table_out
	    = dma_alloc_coherent(&pdev->dev, RLX_DMA_TABLE_SIZE,
				 &crypto_data->dma_table_out, GFP_KERNEL);
	if (!crypto_data->table_out) {
		pr_err("Unable to allocate dma table out buffer\n");
		ret = -ENOMEM;
		goto dma_err;
	}

	for (i = 0; i < ARRAY_SIZE(rlx_algs); i++) {
		rlx_algs[i].cra_priority = 300;
		rlx_algs[i].cra_flags = CRYPTO_ALG_TYPE_BLKCIPHER,
		rlx_algs[i].cra_ctxsize = sizeof(struct rlx_crypto_ctx);
		rlx_algs[i].cra_alignmask = 3;
		rlx_algs[i].cra_type = &crypto_blkcipher_type;
		rlx_algs[i].cra_module = THIS_MODULE;

		ret = crypto_register_alg(&rlx_algs[i]);
		if (ret)
			goto reg_err;
	}

	ret = request_irq(crypto_data->irq, rlx_crypto_irq_handler,
			  IRQF_DISABLED, "rlx-crypto", crypto_data);

	rlx_cdata = crypto_data;

	mutex_init(&rlx_cdata->dma_mutex);
	init_completion(&rlx_cdata->crypto_complete);

	rlx_cdata->cipher_clk = clk_get(NULL, "cipher_ck");

	return 0;

reg_err:
	for (j = 0; j < i; j++)
		crypto_unregister_alg(&rlx_algs[j]);
dma_err:
	if (crypto_data->table_in)
		dma_free_coherent(&pdev->dev, RLX_DMA_TABLE_SIZE,
				  crypto_data->table_in,
				  crypto_data->dma_table_in);
	if (crypto_data->table_out)
		dma_free_coherent(&pdev->dev, RLX_DMA_TABLE_SIZE,
				  crypto_data->table_out,
				  crypto_data->dma_table_out);
err:
	release_mem_region(crypto_data->base, resource_size(res));
mem_err:
	devm_kfree(&pdev->dev, crypto_data);
	crypto_data = NULL;

	return ret;
}

static int rlx_crypto_remove(struct platform_device *pdev)
{
	struct rlx_crypto_data *crypto_data;
	struct resource *res;
	int i;

	for (i = 0; i < ARRAY_SIZE(rlx_algs); i++)
		crypto_unregister_alg(&rlx_algs[i]);

	crypto_data = dev_get_drvdata(&pdev->dev);
	if (crypto_data->table_in)
		dma_free_coherent(&pdev->dev, RLX_DMA_TABLE_SIZE,
				  crypto_data->table_in,
				  crypto_data->dma_table_in);
	if (crypto_data->table_out)
		dma_free_coherent(&pdev->dev, RLX_DMA_TABLE_SIZE,
				  crypto_data->table_out,
				  crypto_data->dma_table_out);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res)
		release_mem_region(crypto_data->base, resource_size(res));

	devm_kfree(&pdev->dev, crypto_data);
	dev_set_drvdata(&pdev->dev, NULL);
	crypto_data = NULL;
	rlx_cdata = NULL;

	return 0;
}

static struct platform_driver rlx_crypto_driver = {
	.probe = rlx_crypto_probe,
	.remove = rlx_crypto_remove,
	.driver = {
		.name = "rlx-crypto",
		.owner = THIS_MODULE,
	},
};
module_platform_driver(rlx_crypto_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek RLX crypto driver");
