#include <sound/soc.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include "rlx_dma.h"
#include "rlx_i2s.h"

#ifdef CONFIG_SND_SOC_RLX_DEBUG
#define DBG(args...)	pr_emerg("%s: %s", __func__, args)
#else
#define DBG(args...)
#endif
#if defined(CONFIG_SND_SOC_RLX_RT5651)
      #define RLX_I2S_CODEC_MODE	RLX_I2S_CODEC_PC
#elif defined(CONFIG_SND_SOC_RLX_RT5658)
      #define RLX_I2S_CODEC_MODE	RLX_I2S_CODEC_CE_512FS
#endif

#ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC
static int rlx_intern_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct rlx_dma_data *dma_data = snd_soc_platform_get_drvdata(platform);
	int i2s_clk_freq = 24000000;

	snd_soc_dai_set_sysclk(cpu_dai, RLX_I2S_CODEC_PC, i2s_clk_freq, 0);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data->subdata[0].codec_sel = 0;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		dma_data->subdata[1].codec_sel = 0;
	else
		return -EINVAL;

	return 0;
}

static struct snd_soc_ops rlx_intern_ops = {
	.hw_params = rlx_intern_hw_params,
};

static struct snd_soc_dai_link rlx_intern_dai_link[] = {
	{
		.name = "RLX INTERN CARD DIGITAL",
		.stream_name = "RLX INTERN PCM1",
		.platform_name = "audio-platform",
		.codec_name = "rlx-codec",
		.cpu_dai_name = "pcm-platform",
		.codec_dai_name = "rlx-codec-digital",
		.ops = &rlx_intern_ops,
	},
	{
		.name = "RLX INTERN CARD ANALOG",
		.stream_name = "RLX INTERN PCM2",
		.platform_name = "audio-platform",
		.codec_name = "rlx-codec",
		.cpu_dai_name = "pcm-platform",
		.codec_dai_name = "rlx-codec-analog",
		.ops = &rlx_intern_ops,
	},
};

static int rlx_amic_extern_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
#ifdef CONFIG_SND_SOC_RLX_AMIC_PMU_RTP
	struct regulator *regulator;
	int ret;
#ifdef CONFIG_SND_SOC_RLX_AMIC_PMU_RTP_LDO1
	regulator = regulator_get(NULL, "LDO1");
#endif
	if (IS_ERR(regulator)) {
		ret = PTR_ERR(regulator);
		pr_err("fail to get regulator\n");
		return ret;
	}

	ret = regulator_set_voltage(regulator, 2800000, 2800000);
	if (ret) {
		pr_err("fail to set requlator\n");
		goto out;
	}

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ret = regulator_enable(regulator);
		if (ret) {
			pr_err("fail to enable regulator\n");
			goto out;
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		ret = regulator_disable(regulator);
		if (ret) {
			pr_err("fail to disable regulator\n");
			goto out;
		}
		break;
	default:
		pr_err("invalid event\n");
		break;
	}
	ret = 0;
out:
	regulator_put(regulator);
	if (ret)
		return ret;
#endif
	return 0;
}

static const struct snd_soc_dapm_widget rlx_intern_dapm_widgets[] = {
	SND_SOC_DAPM_SUPPLY("AMIC EXTERN POWER", SND_SOC_NOPM, 0, 0,
			rlx_amic_extern_power,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
};

static const struct snd_soc_dapm_route rlx_intern_dapm_routes[] = {
	{"AMIC", NULL, "AMIC EXTERN POWER"},
};

static struct snd_soc_card rlx_intern_snd_soc_card = {
	.name = "RLX_INTERN_CARD",
	.dai_link = rlx_intern_dai_link,
	.num_links = ARRAY_SIZE(rlx_intern_dai_link),
	.dapm_widgets = rlx_intern_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(rlx_intern_dapm_widgets),
	.dapm_routes = rlx_intern_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(rlx_intern_dapm_routes),
};
#endif

#ifdef CONFIG_SND_SOC_RLX_EXTERN_CODEC
static int rlx_extern_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
#ifndef CONFIG_SOC_QC_CODE
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
#endif
	struct snd_soc_platform *platform = rtd->platform;
	struct rlx_dma_data *dma_data = snd_soc_platform_get_drvdata(platform);
	int sample_rate = params_rate(params);
	int ret, i2s_clk_freq, mclk_div, bclk_div;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
				  | SND_SOC_DAIFMT_NB_NF);
	if (ret < 0)
		return ret;
#ifndef CONFIG_SOC_QC_CODE
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
				  | SND_SOC_DAIFMT_NB_NF
				  | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
#endif
	switch (RLX_I2S_CODEC_MODE) {
	case RLX_I2S_CODEC_PC:
		i2s_clk_freq = 24000000;
		mclk_div = 1;
		bclk_div = 0;
		break;
	case RLX_I2S_CODEC_CE_256FS:
		switch (sample_rate) {
		case 8000:
			i2s_clk_freq = 16384000;
			mclk_div = 8;
			bclk_div = 32;
			break;
		case 16000:
			i2s_clk_freq = 16384000;
			mclk_div = 4;
			bclk_div = 16;
			break;
		case 32000:
			i2s_clk_freq = 16384000;
			mclk_div = 2;
			bclk_div = 8;
			break;
		case 44100:
			i2s_clk_freq = 22579200;
			mclk_div = 2;
			bclk_div = 8;
			break;
		case 48000:
			i2s_clk_freq = 24576000;
			mclk_div = 2;
			bclk_div = 8;
			break;
		default:
			pr_err("invalid rate(%d)\n", sample_rate);
			return -EINVAL;
		}
		break;
	case RLX_I2S_CODEC_CE_512FS:
		switch (sample_rate) {
		case 8000:
			i2s_clk_freq = 16384000;
			mclk_div = 4;
			bclk_div = 32;
			break;
		case 16000:
			i2s_clk_freq = 16384000;
			mclk_div = 2;
			bclk_div = 16;
			break;
		case 32000:
			i2s_clk_freq = 16384000;
			mclk_div = 1;
			bclk_div = 8;
			break;
		case 44100:
			i2s_clk_freq = 22579200;
			mclk_div = 1;
			bclk_div = 8;
			break;
		case 48000:
			i2s_clk_freq = 24576000;
			mclk_div = 1;
			bclk_div = 8;
			break;
		default:
			pr_err("invalid rate(%d)\n", sample_rate);
			return -EINVAL;
		}
		break;
	}
	snd_soc_dai_set_sysclk(cpu_dai, RLX_I2S_CODEC_MODE, i2s_clk_freq, 0);
#ifndef CONFIG_SOC_QC_CODE
	snd_soc_dai_set_sysclk(codec_dai, 0, i2s_clk_freq, 0);
#endif
	if (mclk_div > 0)
		snd_soc_dai_set_clkdiv(cpu_dai, RLX_I2S_DIV_MCLK, mclk_div);
	if (bclk_div > 0)
		snd_soc_dai_set_clkdiv(cpu_dai, RLX_I2S_DIV_BCLK, bclk_div);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data->subdata[0].codec_sel = 1;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		dma_data->subdata[1].codec_sel = 1;
	else
		return -EINVAL;

	return 0;
}

static struct snd_soc_ops rlx_extern_ops = {
	.hw_params = rlx_extern_hw_params,
};

static struct snd_soc_dai_link rlx_extern_dai_link[] = {
	{
		.name = "RLX EXTERN CARD AIF1",
		.stream_name = "RLX EXTERN PCM1",
		.platform_name = "audio-platform",
		.cpu_dai_name = "i2s-platform",
#if defined(CONFIG_SOC_QC_CODE)
		.codec_name = "rlx-codec",
		.codec_dai_name = "rlx-codec-digital",
#elif defined(CONFIG_SND_SOC_RLX_RT5651)
		.codec_name = "rt5651.0-001a",
		.codec_dai_name = "rt5651-aif1",
#elif defined(CONFIG_SND_SOC_RLX_RT5658)
		.codec_name = "rt5658.0-001b",
		.codec_dai_name = "rt5658-aif1",
#endif
		.ops = &rlx_extern_ops,
	 }, {
		.name = "RLX EXTERN CARD AIF2",
		.stream_name = "RLX EXTERN PCM2",
		.platform_name = "audio-platform",
		.cpu_dai_name = "i2s-platform",
#if defined(CONFIG_SOC_QC_CODE)
		.codec_name = "rlx-codec",
		.codec_dai_name = "rlx-codec-analog",
#elif defined(CONFIG_SND_SOC_RLX_RT5651)
		.codec_name = "rt5651.0-001a",
		.codec_dai_name = "rt5651-aif2",
#elif defined(CONFIG_SND_SOC_RLX_RT5658)
		.codec_name = "rt5658.0-001b",
		.codec_dai_name = "rt5658-aif2",
#endif
		.ops = &rlx_extern_ops,
	},
#if defined(CONFIG_SND_SOC_RLX_RT5658)
	{
		.name = "RLX EXTERN CARD AIF3",
		.stream_name = "RLX EXTERN PCM3",
		.platform_name = "audio-platform",
		.cpu_dai_name = "i2s-platform",
#if defined(CONFIG_SOC_QC_CODE)
		.codec_name = "rlx-codec",
		.codec_dai_name = "rlx-codec-analog",
#elif defined(CONFIG_SND_SOC_RLX_RT5658)
		.codec_name = "rt5658.0-001b",
		.codec_dai_name = "rt5658-aif3",
#endif
		.ops = &rlx_extern_ops,
	  },
#endif
};

static struct snd_soc_card rlx_extern_snd_soc_card = {
	.name = "RLX_EXTERN_CARD",
	.dai_link = rlx_extern_dai_link,
	.num_links = ARRAY_SIZE(rlx_extern_dai_link),
};
#endif

#ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC
static struct platform_device *rlx_intern_snd_device;
#endif
#ifdef CONFIG_SND_SOC_RLX_EXTERN_CODEC
static struct platform_device *rlx_extern_snd_device;
#endif

static int __init rlx_snd_init(void)
{
	int ret = 0;
	int num = 0;

	DBG("rlx snd init\n");

#ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC
	rlx_intern_snd_device = platform_device_alloc("soc-audio", num++);
	if (!rlx_intern_snd_device) {
		pr_err("platform device alloc failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(rlx_intern_snd_device, &rlx_intern_snd_soc_card);
	ret = platform_device_add(rlx_intern_snd_device);
	if (ret) {
		pr_err("platform device add failed\n");
		platform_device_put(rlx_intern_snd_device);
	}
#endif

#ifdef CONFIG_SND_SOC_RLX_EXTERN_CODEC
	rlx_extern_snd_device = platform_device_alloc("soc-audio", num++);
	if (!rlx_extern_snd_device) {
		pr_err("platform device alloc failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(rlx_extern_snd_device, &rlx_extern_snd_soc_card);
	ret = platform_device_add(rlx_extern_snd_device);
	if (ret) {
		pr_err("platform device add failed\n");
		platform_device_put(rlx_extern_snd_device);
	}
#endif

	return ret;
}
module_init(rlx_snd_init);

static void __exit rlx_snd_exit(void)
{
#ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC
	platform_device_unregister(rlx_intern_snd_device);
#endif
#ifdef CONFIG_SND_SOC_RLX_EXTERN_CODEC
	platform_device_unregister(rlx_extern_snd_device);
#endif
}
module_exit(rlx_snd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek RLX ALSA soc codec driver");
