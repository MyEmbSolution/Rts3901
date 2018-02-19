#include <sound/soc.h>
#include <linux/clk.h>
#include <linux/module.h>
#include "rlx_hw_id.h"
#include "rlx_i2s.h"

#ifdef CONFIG_SND_SOC_RLX_DEBUG
#define DBG(args...)	pr_emerg("%s: %s", __func__, args)
#else
#define DBG(args...)
#endif

static const struct snd_kcontrol_new rlx_i2s_dai_controls[] = {
	SOC_DOUBLE("I2S Playback Mute", RLX_REG_I2S_CTL,
		   RLX_I2S_OUT_LCHANNLE_MUTE, RLX_I2S_OUT_RCHANNLE_MUTE, 1, 1),
	SOC_DOUBLE("I2S Capture Mute", RLX_REG_I2S_CTL,
		   RLX_I2S_IN_LCHANNLE_MUTE, RLX_I2S_IN_RCHANNLE_MUTE, 1, 1),
};

static int rlx_i2s_config_hclk(struct rlx_i2s_dai_data *dai_data,
		unsigned long rfs)
{
	struct clk *clk1;
	struct clk *clk2;
	struct clk *clk3;
	unsigned long mclk, nclk, fclk;

	if (TYPE_FPGA & dai_data->devtype)
		return 0;

	clk1 = clk_get(NULL, "sys_pll3_m");
	clk2 = clk_get(NULL, "sys_pll3_n");
	clk3 = clk_get(NULL, "sys_pll3_f");

	if (IS_ERR(clk1)) {
		pr_err("get pll3 m fail\n");
		return -1;
	}
	if (IS_ERR(clk2)) {
		pr_err("get pll3 n fail\n");
		return -1;
	}
	if (IS_ERR(clk3)) {
		pr_err("get pll3 f fail\n");
		return -1;
	}

	mclk = 25000000 * ((rfs * 2 * 8) / (5 * 1000000));
	nclk = mclk / 5;
	fclk = rfs * 2 * 8;

	clk_set_rate(clk1, mclk);
	clk_set_rate(clk2, nclk);
	clk_set_rate(clk3, fclk);

	clk_put(clk1);
	clk_put(clk2);
	clk_put(clk3);

	return 0;
}

static int rlx_i2s_dai_enable_clk(struct rlx_i2s_dai_data *dai_data)
{
	mutex_lock(&dai_data->clk_mutex);
	if (dai_data->clk_ref == 0) {
		if (dai_data->pll_clk)
			clk_prepare_enable(dai_data->pll_clk);
		clk_prepare_enable(dai_data->i2s_clk);
	}
	dai_data->clk_ref++;
	mutex_unlock(&dai_data->clk_mutex);

	return 0;
}

static int rlx_i2s_dai_disable_clk(struct rlx_i2s_dai_data *dai_data)
{
	mutex_lock(&dai_data->clk_mutex);
	dai_data->clk_ref--;
	if (dai_data->clk_ref == 0) {
		clk_disable(dai_data->i2s_clk);
		if (dai_data->pll_clk)
			clk_disable(dai_data->pll_clk);
	}
	mutex_unlock(&dai_data->clk_mutex);

	return 0;
}

static int rlx_i2s_dai_enable_ldoi2s(struct rlx_i2s_dai_data *dai_data)
{
	u32 reg_val;

	mutex_lock(&dai_data->ldoi2s_mutex);
	if (dai_data->ldoi2s_ref == 0) {
		reg_val = readl(dai_data->addrp + RLX_REG_I2S_OCP_CTL);
		writel(reg_val | ((u32)0x1 << RLX_LDOI2S_POW),
				dai_data->addrp + RLX_REG_I2S_OCP_CTL);
	}
	dai_data->ldoi2s_ref++;
	mutex_unlock(&dai_data->ldoi2s_mutex);

	return 0;
}

static int rlx_i2s_dai_disable_ldoi2s(struct rlx_i2s_dai_data *dai_data)
{
	u32 reg_val;

	mutex_lock(&dai_data->ldoi2s_mutex);
	dai_data->ldoi2s_ref--;
	if (dai_data->ldoi2s_ref == 0) {
		reg_val = readl(dai_data->addrp + RLX_REG_I2S_OCP_CTL);
		writel(reg_val & ~((u32)0x1 << RLX_LDOI2S_POW),
				dai_data->addrp + RLX_REG_I2S_OCP_CTL);
	}
	mutex_unlock(&dai_data->ldoi2s_mutex);

	return 0;
}

static int rlx_i2s_dai_startup(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	struct rlx_i2s_dai_data *dai_data;

	DBG("rlx i2s dai startup\n");

	dai_data = snd_soc_dai_get_drvdata(dai);
	rlx_i2s_dai_enable_ldoi2s(dai_data);

	return 0;
}

static void rlx_i2s_dai_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct rlx_i2s_dai_data *dai_data;

	DBG("rlx i2s dai shutdown\n");

	dai_data = snd_soc_dai_get_drvdata(dai);
	rlx_i2s_dai_disable_ldoi2s(dai_data);
}

static int rlx_pcm_dai_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct rlx_i2s_dai_data *dai_data;

	DBG("rlx pcm dai startup\n");

	dai_data = snd_soc_dai_get_drvdata(dai);

	if (TYPE_RLE0745 == RTS_ASOC_HW_ID(dai_data->devtype))
		rlx_i2s_dai_enable_ldoi2s(dai_data);

	return 0;
}

static void rlx_pcm_dai_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct rlx_i2s_dai_data *dai_data;

	DBG("rlx pcm dai shutdown\n");

	dai_data = snd_soc_dai_get_drvdata(dai);

	if (TYPE_RLE0745 == RTS_ASOC_HW_ID(dai_data->devtype))
		rlx_i2s_dai_disable_ldoi2s(dai_data);
}

static int rlx_i2s_dai_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct rlx_i2s_dai_data *dai_data;
	u32 reg_val, val;

	DBG("rlx i2s dai hw params\n");

	dai_data = snd_soc_dai_get_drvdata(dai);
	reg_val = readl(dai_data->addr + RLX_REG_I2S_CFG);
	switch (params_rate(params)) {
	case 8000:
		val = 0x0;
		break;
	case 16000:
		val = 0x1;
		break;
	case 32000:
		val = 0x2;
		break;
	case 44100:
		val = 0x3;
		break;
	case 48000:
		val = 0x4;
		break;
	default:
		pr_err("invalid rate(%d)\n", params_rate(params));
		return -EINVAL;
	}
	reg_val = reg_val & ~((u32) 0x7 << RLX_I2S_SAMPLE_RATE);
	reg_val = reg_val | (val << RLX_I2S_SAMPLE_RATE);
	writel(reg_val, dai_data->addr + RLX_REG_I2S_CFG);

	dai_data->channels = params_channels(params);

	return 0;
}

static int rlx_i2s_dai_trigger(struct snd_pcm_substream *substream,
			       int cmd, struct snd_soc_dai *dai)
{
	struct rlx_i2s_dai_data *dai_data;
	u32 reg_val;

	DBG("rlx i2s dai trigger\n");

	dai_data = snd_soc_dai_get_drvdata(dai);
	reg_val = readl(dai_data->addr + RLX_REG_I2S_CTL);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			rlx_i2s_dai_enable_clk(dai_data);
			reg_val = reg_val | ((u32) 0x1 << RLX_I2S_OUT_GO);
			writel(reg_val, dai_data->addr + RLX_REG_I2S_CTL);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			reg_val = reg_val & ~((u32) 0x1 << RLX_I2S_OUT_GO);
			writel(reg_val, dai_data->addr + RLX_REG_I2S_CTL);
			rlx_i2s_dai_disable_clk(dai_data);
			break;
		default:
			return -EINVAL;
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			rlx_i2s_dai_enable_clk(dai_data);
			reg_val = reg_val | ((u32) 0x1 << RLX_I2S_IN_GO);
			writel(reg_val, dai_data->addr + RLX_REG_I2S_CTL);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			reg_val = reg_val & ~((u32) 0x1 << RLX_I2S_IN_GO);
			writel(reg_val, dai_data->addr + RLX_REG_I2S_CTL);
			rlx_i2s_dai_disable_clk(dai_data);
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int rlx_pcm_dai_trigger(struct snd_pcm_substream *substream,
			       int cmd, struct snd_soc_dai *dai)
{
	struct rlx_i2s_dai_data *dai_data;

	DBG("rlx pcm dai trigger\n");

	dai_data = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			rlx_i2s_dai_enable_clk(dai_data);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			rlx_i2s_dai_disable_clk(dai_data);
			break;
		default:
			return -EINVAL;
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			rlx_i2s_dai_enable_clk(dai_data);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			rlx_i2s_dai_disable_clk(dai_data);
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int rlx_i2s_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct rlx_i2s_dai_data *dai_data;
	u32 reg_val, val;

	DBG("rlx i2s dai set fmt\n");

	dai_data = snd_soc_dai_get_drvdata(dai);
	reg_val = readl(dai_data->addr + RLX_REG_I2S_CFG);
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
		if (dai_data->channels == 1)
			val = 0x0;
		else if (dai_data->channels == 2)
			val = 0x2;
		else
			return -EINVAL;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		if (dai_data->channels == 1)
			val = 0x1;
		else if (dai_data->channels == 2)
			val = 0x3;
		else
			return -EINVAL;
		break;
	case SND_SOC_DAIFMT_I2S:
		val = 0x4;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val = 0x5;
		break;
	default:
		return -EINVAL;
	}

	reg_val = reg_val & ~((u32) 0x7 << RLX_I2S_PCM_FORMAT);
	reg_val = reg_val | (val << RLX_I2S_PCM_FORMAT);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		val = 0x0;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		val = 0x3;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		val = 0x1;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		val = 0x2;
		break;
	default:
		return -EINVAL;
	}

	reg_val = reg_val & ~((u32) 0x3 << RLX_BCLK_POLARITY);
	reg_val = reg_val | (val << RLX_BCLK_POLARITY);
	writel(reg_val, dai_data->addr + RLX_REG_I2S_CFG);

	return 0;
}

static int rlx_i2s_dai_set_sysclk(struct snd_soc_dai *dai,
				  int clk_id, unsigned int rfs, int dir)
{
	struct rlx_i2s_dai_data *dai_data;
	u32 reg_val;

	DBG("rlx i2s dai set sysclk\n");

	dai_data = snd_soc_dai_get_drvdata(dai);
	reg_val = readl(dai_data->addr + RLX_REG_I2S_CFG);

	switch (clk_id) {
	case RLX_I2S_CODEC_PC:
		/* pc codec */
		reg_val = reg_val & ~((u32) 0x1 << RLX_SCK_GEN_SEL);
		break;
	case RLX_I2S_CODEC_CE_256FS:
	case RLX_I2S_CODEC_CE_512FS:
		/* ce codec */
		reg_val = reg_val | ((u32) 0x1 << RLX_SCK_GEN_SEL);
		break;
	default:
		pr_err("invalid sys clock select(%d)\n", clk_id);
		return -EINVAL;
	}
	writel(reg_val, dai_data->addr + RLX_REG_I2S_CFG);

	/* set i2s_clk freq */
	rlx_i2s_config_hclk(dai_data, rfs);
	clk_set_rate(dai_data->i2s_clk, rfs);

	return 0;
}

static int rlx_pcm_dai_set_sysclk(struct snd_soc_dai *dai,
				  int clk_id, unsigned int rfs, int dir)
{
	struct rlx_i2s_dai_data *dai_data;

	DBG("rlx pcm dai set sysclk\n");

	dai_data = snd_soc_dai_get_drvdata(dai);

	/* set i2s_clk freq */
	rlx_i2s_config_hclk(dai_data, rfs);
	clk_set_rate(dai_data->i2s_clk, rfs);

	return 0;
}

static int rlx_i2s_dai_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct rlx_i2s_dai_data *dai_data;
	u32 reg_val, val;

	DBG("rlx i2s dai set clkdiv\n");

	dai_data = snd_soc_dai_get_drvdata(dai);
	reg_val = readl(dai_data->addr + RLX_REG_I2S_CFG);
	switch (div_id) {
	case RLX_I2S_DIV_BCLK:
		val = div;
		reg_val = reg_val & ~((u32) 0xFF << RLX_BCLK_INT_DIV);
		reg_val = reg_val | (val << RLX_BCLK_INT_DIV);
		writel(reg_val, dai_data->addr + RLX_REG_I2S_CFG);
		break;
	case RLX_I2S_DIV_MCLK:
		switch (div) {
		case 1:
			val = 0x0;
			break;
		case 2:
			val = 0x1;
			break;
		case 3:
			val = 0x2;
			break;
		case 4:
			val = 0x3;
			break;
		case 6:
			val = 0x4;
			break;
		case 8:
			val = 0x5;
			break;
		case 12:
			val = 0x6;
			break;
		case 16:
			val = 0x7;
			break;
		case 32:
			val = 0x8;
			break;
		case 64:
			val = 0x9;
			break;
		case 128:
			val = 0xA;
			break;
		default:
			pr_err("invalid clock divider(%d)\n", div);
			return -EINVAL;
		}
		reg_val = reg_val & ~((u32) 0xF << RLX_MCLK_DIV_SEL);
		reg_val = reg_val | (val << RLX_MCLK_DIV_SEL);
		writel(reg_val, dai_data->addr + RLX_REG_I2S_CFG);
		break;
	default:
		pr_err("invalid clock divider id(%d)\n", div_id);
		return -EINVAL;
	}

	return 0;
}

static int rlx_i2s_dai_probe(struct snd_soc_dai *dai)
{
	DBG("rlx i2s dai probe\n");

	snd_soc_add_dai_controls(dai, rlx_i2s_dai_controls,
				 ARRAY_SIZE(rlx_i2s_dai_controls));

	return 0;
}

static struct snd_soc_dai_ops rlx_i2s_dai_ops = {
	.startup = rlx_i2s_dai_startup,
	.shutdown = rlx_i2s_dai_shutdown,
	.trigger = rlx_i2s_dai_trigger,
	.hw_params = rlx_i2s_dai_hw_params,
	.set_fmt = rlx_i2s_dai_set_fmt,
	.set_clkdiv = rlx_i2s_dai_set_clkdiv,
	.set_sysclk = rlx_i2s_dai_set_sysclk,
};

static struct snd_soc_dai_ops rlx_pcm_dai_ops = {
	.startup = rlx_pcm_dai_startup,
	.shutdown = rlx_pcm_dai_shutdown,
	.set_sysclk = rlx_pcm_dai_set_sysclk,
};

static struct snd_soc_dai_driver rlx_i2s_dai[] = {
	{
		.name = "i2s-platform",
		.probe = rlx_i2s_dai_probe,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_I2S_RATES,
			.formats = RLX_I2S_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_I2S_RATES,
			.formats = RLX_I2S_FORMATS,
		},
		.ops = &rlx_i2s_dai_ops,
	} , {
		.name = "pcm-platform",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_I2S_RATES,
			.formats = RLX_I2S_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_I2S_RATES,
			.formats = RLX_I2S_FORMATS,
		},
		.ops = &rlx_pcm_dai_ops,
	}
};

static const struct snd_soc_component_driver rlx_i2s_component = {
	.name = "dai-platform",
};

static void rlx_i2s_iounmap(struct rlx_i2s_dai_data *dai_data)
{
	struct platform_device *pdev = dai_data->pdev;
	struct resource *res;
	int i;

	if (dai_data->addr) {
		iounmap(dai_data->addr);
		dai_data->addr = NULL;
	}

	if (dai_data->addrp) {
		iounmap(dai_data->addrp);
		dai_data->addrp = NULL;
	}

	for (i = 0; i < RLX_I2S_ADDR_NUM; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res)
			release_mem_region(res->start, resource_size(res));
	}
}

static int rlx_i2s_ioremap(struct rlx_i2s_dai_data *dai_data)
{
	struct platform_device *pdev = dai_data->pdev;
	struct resource *res;
	void __iomem *addr[RLX_I2S_ADDR_NUM];
	int i, ret = 0;
	unsigned long size;
	u32 base;

	for (i = 0; i < RLX_I2S_ADDR_NUM; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			pr_err("Unable to get I2S address-%d\n", i);
			ret = -ENXIO;
			goto out;
		}

		if (!request_mem_region(res->start, resource_size(res),
					"dai-platform")) {
			pr_err("Unable to request mem region\n");
			ret = -EBUSY;
			goto out;
		}

		base = res->start;
		size = res->end - res->start + 1;

		addr[i] = ioremap(base, size);
		if (addr[i] == NULL) {
			pr_err("failed to ioremap-%d\n", i);
			ret = -ENXIO;
			goto out;
		}
	}
out:
	dai_data->addr = addr[0];
	dai_data->addrp = addr[1];

	if (ret)
		rlx_i2s_iounmap(dai_data);

	return ret;
}

static struct platform_device_id rlx_i2s_devtypes[] = {
	{
		.name = "rle0745-fpga-adai",
		.driver_data = TYPE_RLE0745 | TYPE_FPGA,
	}, {
		.name = "rlx0745-adai",
		.driver_data = TYPE_RLE0745,
	}, {
		.name = "rts3901-fpga-adai",
		.driver_data = TYPE_RTS3901 | TYPE_FPGA,
	}, {
		.name = "rts3901-adai",
		.driver_data = TYPE_RTS3901,
	}, {
		.name = "rts3903-fpga-adai",
		.driver_data = TYPE_RTS3903 | TYPE_FPGA,
	}, {
		.name = "rts3903-adai",
		.driver_data = TYPE_RTS3903,
	},
};

static int rlx_i2s_probe(struct platform_device *pdev)
{
	int ret;
	struct rlx_i2s_dai_data *dai_data;
	const struct platform_device_id *id_entry;

	DBG("rlx i2s probe\n");

	id_entry = platform_get_device_id(pdev);
	if (!id_entry) {
		pr_err("not support soc dai platform\n");
		return -EINVAL;
	}

	dai_data =
	    devm_kzalloc(&pdev->dev, sizeof(struct rlx_i2s_dai_data),
			 GFP_KERNEL);
	if (dai_data == NULL) {
		pr_err("Unable to alloc I2S data\n");
		return -ENOMEM;
	}
	dai_data->pdev = pdev;
	dai_data->devtype = id_entry->driver_data;

	ret = rlx_i2s_ioremap(dai_data);
	if (ret)
		goto io_err;

	dai_data->i2s_clk = clk_get(NULL, "i2s_ck");
	if (IS_ERR(dai_data->i2s_clk)) {
		pr_err("failed to get i2s_ck\n");
		ret = -ENOENT;
		goto io_err;
	}

	if (TYPE_FPGA & dai_data->devtype) {
		dai_data->pll_clk = NULL;
		clk_set_parent(dai_data->i2s_clk, clk_get(NULL, "usb_pll_7"));
	} else {
		dai_data->pll_clk = clk_get(NULL, "sys_pll3_f");
		if (IS_ERR(dai_data->pll_clk)) {
			pr_err("failed to get sys_pll3_f\n");
			ret = -ENOENT;
			goto pll_err;
		}
		clk_set_parent(dai_data->i2s_clk, clk_get(NULL, "sys_pll3_f"));
	}
	dai_data->clk_ref = 0;
	mutex_init(&dai_data->clk_mutex);
	dai_data->ldoi2s_ref = 0;
	mutex_init(&dai_data->ldoi2s_mutex);

	if (TYPE_RLE0745 == RTS_ASOC_HW_ID(dai_data->devtype)) {
		rlx_pcm_dai_ops.trigger = rlx_i2s_dai_trigger;
		rlx_pcm_dai_ops.hw_params = rlx_i2s_dai_hw_params;
	} else {
		rlx_pcm_dai_ops.trigger = rlx_pcm_dai_trigger;
	}

	ret = snd_soc_register_component(&pdev->dev,
					 &rlx_i2s_component,
					 rlx_i2s_dai, 2);
	if (ret) {
		pr_err("register component failed\n");
		goto pll_err;
	}

	dev_set_drvdata(&pdev->dev, dai_data);

	return 0;
pll_err:
	if (dai_data->pll_clk) {
		clk_put(dai_data->pll_clk);
		dai_data->pll_clk = NULL;
	}

	if (dai_data->i2s_clk) {
		clk_put(dai_data->i2s_clk);
		dai_data->i2s_clk = NULL;
	}
io_err:
	rlx_i2s_iounmap(dai_data);
	devm_kfree(&pdev->dev, dai_data);
	dai_data = NULL;

	return ret;
}

static int rlx_i2s_remove(struct platform_device *pdev)
{
	struct rlx_i2s_dai_data *dai_data;

	dai_data = dev_get_drvdata(&pdev->dev);
	if (dai_data->i2s_clk) {
		clk_put(dai_data->i2s_clk);
		dai_data->i2s_clk = NULL;
	}
	if (dai_data->pll_clk) {
		clk_put(dai_data->pll_clk);
		dai_data->pll_clk = NULL;
	}
	rlx_i2s_iounmap(dai_data);

	snd_soc_unregister_component(&pdev->dev);
	devm_kfree(&pdev->dev, dai_data);
	dev_set_drvdata(&pdev->dev, NULL);
	dai_data = NULL;

	return 0;
}

static struct platform_driver rlx_i2s_driver = {
	.driver = {
		.name = "dai-platform",
		.owner = THIS_MODULE,
	},
	.probe = rlx_i2s_probe,
	.remove = rlx_i2s_remove,
	.id_table = rlx_i2s_devtypes,
};

static int __init rlx_i2s_init(void)
{
	DBG("rlx i2s init\n");

	return platform_driver_register(&rlx_i2s_driver);
}
module_init(rlx_i2s_init);

static void __exit rlx_i2s_exit(void)
{
	platform_driver_unregister(&rlx_i2s_driver);
}
module_exit(rlx_i2s_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek RLX ALSA soc codec driver");
