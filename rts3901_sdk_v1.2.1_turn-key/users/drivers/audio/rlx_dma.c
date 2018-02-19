#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <linux/clk.h>
#include "rlx_hw_id.h"
#include "rlx_dma.h"

#ifdef CONFIG_SND_SOC_RLX_DEBUG
#define DBG(args...)	pr_emerg("%s: %s", __func__, args)
#else
#define DBG(args...)
#endif

static const struct snd_pcm_hardware rlx_dma_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED |
	    SNDRV_PCM_INFO_BLOCK_TRANSFER |
	    SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats = SNDRV_PCM_FMTBIT_U8 |
	    SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_3LE,
	.rates = SNDRV_PCM_RATE_8000 |
	    SNDRV_PCM_RATE_16000 |
	    SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_KNOT,
	.rate_min = 8000,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.period_bytes_min = 32,
	.period_bytes_max = 64 * 1024,
	.periods_min = 2,
	.periods_max = 255,
	.buffer_bytes_max = 128 * 1024,
	.fifo_size = 16,
};

static int rlx_pcm_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	u32 reg_val, val;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	unsigned long totbytes = params_buffer_bytes(params);
	int stream = substream->stream;
	int format;

	DBG("rlx pcm hw params\n");

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data->subdata[stream].substream = substream;

		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_CTL);
		/* data format */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_U8:
			val = 0x0;
			format = 1;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
			val = 0x1;
			format = 2;
			break;
		case SNDRV_PCM_FORMAT_S24_3LE:
			val = 0x2;
			format = 3;
			break;
		default:
			return -EINVAL;
		}
		reg_val = reg_val & ~((u32) 0x3 << RLX_BIT_OUT_DATA_WIDTH);
		reg_val = reg_val | (val << RLX_BIT_OUT_DATA_WIDTH);
		/* channels */
		switch (params_channels(params)) {
		case 1:
			switch (dma_data->subdata[stream].mono_mode) {
			case RLX_SND_DMA_MONO_OUT:
				val = 0x2;
				break;
			case RLX_SND_DMA_MONO_TO_STEREO_OUT:
				val = 0x3;
				break;
			default:
				return -EINVAL;
			}
			break;
		case 2:
			switch (dma_data->subdata[stream].stereo_mode) {
			case RLX_SND_DMA_NORMAL_STEREO_OUT:
				val = 0x0;
				break;
			case RLX_SND_DMA_EXCHANGE_STEREO_OUT:
				val = 0x1;
				break;
			default:
				return -EINVAL;
			}
			val = 0x0;
			break;
		default:
			return -EINVAL;
		}
		reg_val = reg_val & ~((u32) 0x3 << RLX_BIT_AUDIO_OUT_MODE);
		reg_val = reg_val | (val << RLX_BIT_AUDIO_OUT_MODE);
		/* source select */
		switch (dma_data->subdata[stream].codec_sel) {
		case 0:
			val = 0x0;
			break;
		case 1:
			val = 0x1;
			break;
		default:
			return -EINVAL;
		}
		reg_val = reg_val & ~((u32) 0x1 << RLX_BIT_OUT_SOURCE_SELECT);
		reg_val = reg_val | (val << RLX_BIT_OUT_SOURCE_SELECT);
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_CTL);
		dma_data->subdata[stream].format = format;

		/* start address */
		reg_val = runtime->dma_addr;
		reg_val = reg_val << RLX_BIT_TX_SA;
		writel(reg_val, dma_data->addr + RLX_REG_TX_SA);
		/* buffer len */
		reg_val = (totbytes / format) << RLX_COMMON_FORMAT_SHIFT;
		writel(reg_val, dma_data->addr + RLX_REG_TX_LEN);
		/* tx buffer threshold */
		reg_val = reg_val >> 1;
		writel(reg_val, dma_data->addr + RLX_REG_TX_BUFFER_THRESHOLD);
		/* out timer */
		writel(0, dma_data->addr + RLX_REG_TX_TIMER_COUNT);
		reg_val = params_period_size(params);
		writel(reg_val, dma_data->addr + RLX_REG_TX_TIMER_THRESHOLD);
		/* enable out interupt & clear sts */
		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_INT_STS);
		reg_val = reg_val & 0xF0;
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_INT_STS);
		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_INT_EN);
		reg_val = reg_val | (1 << RLX_BIT_AUDIO_OUT_TIMER);
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_INT_EN);
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		dma_data->subdata[stream].substream = substream;

		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_CTL);
		/* data format */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_U8:
			val = 0x0;
			format = 1;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
			val = 0x1;
			format = 2;
			break;
		case SNDRV_PCM_FORMAT_S24_3LE:
			val = 0x2;
			format = 3;
			break;
		default:
			return -EINVAL;
		}
		reg_val = reg_val & ~((u32) 0x3 << RLX_BIT_IN_DATA_WIDTH);
		reg_val = reg_val | (val << RLX_BIT_IN_DATA_WIDTH);
		/* channels */
		switch (params_channels(params)) {
		case 1:
			switch (dma_data->subdata[stream].mono_mode) {
			case RLX_SND_DMA_MONO_LEFT_IN:
				val = 0x2;
				break;
			case RLX_SND_DMA_MONO_RIGHT_IN:
				val = 0x3;
				break;
			case RLX_SND_DMA_MONO_MIX_IN:
				val = 0x4;
				break;
			default:
				return -EINVAL;
			}
			break;
		case 2:
			switch (dma_data->subdata[stream].stereo_mode) {
			case RLX_SND_DMA_NORMAL_STEREO_IN:
				val = 0x0;
				break;
			case RLX_SND_DMA_EXCHANGE_STEREO_IN:
				val = 0x1;
				break;
			default:
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
		reg_val = reg_val & ~((u32) 0x7 << RLX_BIT_AUDIO_IN_MODE);
		reg_val = reg_val | (val << RLX_BIT_AUDIO_IN_MODE);
		/* source select */
		switch (dma_data->subdata[stream].codec_sel) {
		case 0:
			val = 0x0;
			break;
		case 1:
			val = 0x1;
			break;
		default:
			return -EINVAL;
		}
		reg_val = reg_val & ~((u32) 0x1 << RLX_BIT_IN_SOURCE_SELECT);
		reg_val = reg_val | (val << RLX_BIT_IN_SOURCE_SELECT);
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_CTL);
		dma_data->subdata[stream].format = format;

		/* start address */
		reg_val = runtime->dma_addr;
		reg_val = reg_val << RLX_BIT_RX_SA;
		writel(reg_val, dma_data->addr + RLX_REG_RX_SA);
		/* buffer len */
		reg_val = (totbytes / format) << RLX_COMMON_FORMAT_SHIFT;
		writel(reg_val, dma_data->addr + RLX_REG_RX_LEN);
		/* rx buffer threshold */
		reg_val = reg_val >> 1;
		writel(reg_val, dma_data->addr + RLX_REG_RX_BUFFER_THRESHOLD);
		/* in timer */
		writel(0, dma_data->addr + RLX_REG_RX_TIMER_COUNT);
		reg_val = params_period_size(params);
		writel(reg_val, dma_data->addr + RLX_REG_RX_TIMER_THRESHOLD);
		/* enable in interupt & clear sts */
		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_INT_STS);
		reg_val = reg_val & 0xF;
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_INT_STS);
		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_INT_EN);
		reg_val = reg_val | (1 << RLX_BIT_AUDIO_IN_TIMER);
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_INT_EN);
	} else {
		return -EINVAL;
	}

	runtime->dma_bytes = (totbytes / format) << RLX_COMMON_FORMAT_SHIFT;

	return 0;
}

static int rlx_pcm_hw_free(struct snd_pcm_substream *substream)
{
	u32 reg_val;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	int stream = substream->stream;

	DBG("rlx pcm hw free\n");

	snd_pcm_set_runtime_buffer(substream, NULL);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data->subdata[stream].substream = NULL;
		/* disable out interupt */
		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_INT_EN);
		reg_val = reg_val & ~(1 << RLX_BIT_AUDIO_OUT_TIMER);
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_INT_EN);
		/* flush tx fifo */
		reg_val = readl(dma_data->addr + RLX_REG_FIFO_CTL);
		reg_val = reg_val | ((u32) 0x1 << RLX_BIT_TX_FIFO_FLUSH);
		writel(reg_val, dma_data->addr + RLX_REG_FIFO_CTL);
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		dma_data->subdata[stream].substream = NULL;
		/* disable in interupt */
		reg_val = readl(dma_data->addr + RLX_REG_AUDIO_INT_EN);
		reg_val = reg_val & ~(1 << RLX_BIT_AUDIO_IN_TIMER);
		writel(reg_val, dma_data->addr + RLX_REG_AUDIO_INT_EN);
		/* flush rx fifo */
		reg_val = readl(dma_data->addr + RLX_REG_FIFO_CTL);
		reg_val = reg_val | ((u32) 0x1 << RLX_BIT_RX_FIFO_FLUSH);
		writel(reg_val, dma_data->addr + RLX_REG_FIFO_CTL);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int rlx_pcm_prepare(struct snd_pcm_substream *substream)
{
	u32 reg_val;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	int stream = substream->stream;

	DBG("rlx pcm prepare\n");

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* clear rp to tx buffer start address */
		reg_val = runtime->dma_addr;
		reg_val = reg_val << RLX_BIT_TX_RP;
		writel(reg_val, dma_data->addr + RLX_REG_TX_RP);
		reg_val = runtime->dma_addr;
		reg_val = reg_val << RLX_BIT_TX_WP;
		writel(reg_val, dma_data->addr + RLX_REG_TX_WP);
		/* flush tx fifo */
		reg_val = readl(dma_data->addr + RLX_REG_FIFO_CTL);
		reg_val = reg_val | ((u32) 0x1 << RLX_BIT_TX_FIFO_FLUSH);
		writel(reg_val, dma_data->addr + RLX_REG_FIFO_CTL);
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		/* clear rp & wp to tx buffer start address */
		reg_val = runtime->dma_addr;
		reg_val = reg_val << RLX_BIT_RX_RP;
		writel(reg_val, dma_data->addr + RLX_REG_RX_RP);
		reg_val = runtime->dma_addr;
		reg_val = reg_val << RLX_BIT_RX_WP;
		writel(reg_val, dma_data->addr + RLX_REG_RX_WP);
		/* flush rx fifo */
		reg_val = readl(dma_data->addr + RLX_REG_FIFO_CTL);
		reg_val = reg_val | ((u32) 0x1 << RLX_BIT_RX_FIFO_FLUSH);
		writel(reg_val, dma_data->addr + RLX_REG_FIFO_CTL);
	} else {
		return -EINVAL;
	}

	return 0;
}

static void rlx_enable_timer(struct rlx_dma_data *dma_data,
		int dir, int enable)
{
	u32 reg_val;
	u8 reg, shift;

	if (TYPE_RLE0745 == RTS_ASOC_HW_ID(dma_data->devtype))
		return;

	if (dir == RLX_SND_DMA_TIMER_OUT) {
		/* out timer */
		reg = RLX_REG_TX_TIMER_THRESHOLD;
		shift = RLX_BIT_AUDIO_OUT_TIMER_EN;
	} else {
		/* in timer */
		reg = RLX_REG_RX_TIMER_THRESHOLD;
		shift = RLX_BIT_AUDIO_IN_TIMER_EN;
	}

	reg_val = readl(dma_data->addr + reg);
	if (enable)
		reg_val = reg_val | ((u32) 0x1 << shift);
	else
		reg_val = reg_val & ~((u32) 0x1 << shift);
	writel(reg_val, dma_data->addr + reg);
}

static int rlx_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	u32 reg_val;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	int stream = substream->stream;

	DBG("rlx pcm trigger\n");

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			/* enable out timer */
			rlx_enable_timer(dma_data, RLX_SND_DMA_TIMER_OUT, 1);

			/* enable tx fifo */
			reg_val = readl(dma_data->addr + RLX_REG_FIFO_ENABLE);
			reg_val =
			    reg_val | ((u32) 0x1 << RLX_BIT_TX_FIFO_ENABLE);
			writel(reg_val, dma_data->addr + RLX_REG_FIFO_ENABLE);

			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			/* disable tx fifo */
			reg_val = readl(dma_data->addr + RLX_REG_FIFO_ENABLE);
			reg_val =
			    reg_val & ~((u32) 0x1 << RLX_BIT_TX_FIFO_ENABLE);
			writel(reg_val, dma_data->addr + RLX_REG_FIFO_ENABLE);

			/* disable out timer */
			rlx_enable_timer(dma_data, RLX_SND_DMA_TIMER_OUT, 0);
			break;
		default:
			return -EINVAL;
		}
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			/* enable in timer */
			rlx_enable_timer(dma_data, RLX_SND_DMA_TIMER_IN, 1);

			/* enable rx fifo */
			reg_val = readl(dma_data->addr + RLX_REG_FIFO_ENABLE);
			reg_val =
			    reg_val | ((u32) 0x1 << RLX_BIT_RX_FIFO_ENABLE);
			writel(reg_val, dma_data->addr + RLX_REG_FIFO_ENABLE);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			/* disable rx fifo */
			reg_val = readl(dma_data->addr + RLX_REG_FIFO_ENABLE);
			reg_val =
			    reg_val & ~((u32) 0x1 << RLX_BIT_RX_FIFO_ENABLE);
			writel(reg_val, dma_data->addr + RLX_REG_FIFO_ENABLE);

			/* disable in timer */
			rlx_enable_timer(dma_data, RLX_SND_DMA_TIMER_IN, 0);
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static snd_pcm_uframes_t rlx_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	unsigned long res;
	u32 reg_val1, reg_val2;
	int stream = substream->stream;

	DBG("rlx pcm pointer\n");

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val1 = readl(dma_data->addr + RLX_REG_TX_RP);
		reg_val2 = readl(dma_data->addr + RLX_REG_TX_SA);
		res = (reg_val1 >> RLX_BIT_TX_RP) -
		      (reg_val2 >> RLX_BIT_TX_SA);
		res = (res >> RLX_COMMON_FORMAT_SHIFT) *
		      dma_data->subdata[stream].format;
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		reg_val1 = readl(dma_data->addr + RLX_REG_RX_WP);
		reg_val2 = readl(dma_data->addr + RLX_REG_RX_SA);
		res = (reg_val1 >> RLX_BIT_RX_WP) -
		      (reg_val2 >> RLX_BIT_RX_SA);
		res = (res >> RLX_COMMON_FORMAT_SHIFT) *
		      dma_data->subdata[stream].format;
	} else {
		return SNDRV_PCM_POS_XRUN;
	}

	if (res >= snd_pcm_lib_buffer_bytes(substream)) {
		if (res == snd_pcm_lib_buffer_bytes(substream))
			res = 0;
	}

	return bytes_to_frames(substream->runtime, res);
}

static int rlx_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct rlx_dma_data *dma_data;
	int stream = substream->stream;

	DBG("rlx pcm open\n");

	dma_data = snd_soc_platform_get_drvdata(platform);
	if (dma_data->runflag[stream] == 0)
		dma_data->runflag[stream] = 1;
	else
		return -EBUSY;

	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	snd_pcm_hw_constraint_pow2(runtime, 0, SNDRV_PCM_HW_PARAM_PERIODS);
	snd_pcm_hw_constraint_pow2(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
	snd_soc_set_runtime_hwparams(substream, &rlx_dma_hardware);

	runtime->private_data = dma_data;
	return 0;
}

static int rlx_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	int stream = substream->stream;

	DBG("rlx pcm close\n");

	dma_data->runflag[stream] = 0;
	runtime->private_data = NULL;

	return 0;
}

static int rlx_pcm_ack(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	unsigned long appl_ptr = runtime->control->appl_ptr;
	int stream = substream->stream;
	snd_pcm_uframes_t appl_ofs;
	u32 reg_val;

	DBG("rlx pcm ack\n");

	if (appl_ptr == (unsigned long)0) {
		DBG("rlx_pcm_ack, appl_ptr == 0\n");
		appl_ofs = runtime->buffer_size - 1;
	} else {
		appl_ofs = (appl_ptr - 1) % runtime->buffer_size;
	}
	reg_val = frames_to_bytes(runtime, appl_ofs);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val = (reg_val / dma_data->subdata[stream].format) <<
			  RLX_COMMON_FORMAT_SHIFT;
		reg_val = (reg_val + runtime->dma_addr) << RLX_BIT_TX_WP;
		writel(reg_val, dma_data->addr + RLX_REG_TX_WP);
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		reg_val = (reg_val / dma_data->subdata[stream].format) <<
			  RLX_COMMON_FORMAT_SHIFT;
		reg_val = (reg_val + runtime->dma_addr) << RLX_BIT_RX_RP;
		writel(reg_val, dma_data->addr + RLX_REG_RX_RP);
	}

	return 0;
}

static int rlx_pcm_copy(struct snd_pcm_substream *substream, int channel,
			snd_pcm_uframes_t pos, void __user *buf,
			snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rlx_dma_data *dma_data = runtime->private_data;
	int count_bytes = frames_to_bytes(runtime, count);
	char __user *apptr = buf;
	unsigned int *hwptr;
	int stream = substream->stream;
	int pos_bytes, format, val, i;
	char val_tmp;

	DBG("rlx pcm copy\n");

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		format = dma_data->subdata[stream].format;
		pos_bytes = (frames_to_bytes(runtime, pos) / format) <<
			    RLX_COMMON_FORMAT_SHIFT;
		hwptr = (unsigned int *)(runtime->dma_area + pos_bytes);
		while (count_bytes > 0) {
			val = 0;
			if (format == 1) {
				get_user(val_tmp, apptr);
				val = val_tmp & 0xff;
				val = (val - 0x80) & 0xff;
			} else {
				for (i = format - 1; i >= 0; i--) {
					get_user(val_tmp, apptr + i);
					val = (val << 8) | (val_tmp & 0xff);
				}
			}
			*hwptr = (val << ((4 - format) * 8)) >> 7;
			hwptr = hwptr + 1;
			apptr = apptr + format;
			count_bytes = count_bytes - format;
		}
	} else if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		format = dma_data->subdata[stream].format;
		pos_bytes = (frames_to_bytes(runtime, pos) / format) <<
			    RLX_COMMON_FORMAT_SHIFT;
		hwptr = (unsigned int *)(runtime->dma_area + pos_bytes);
		while (count_bytes > 0) {
			if (format == 1) {
				val_tmp = (((*hwptr) >> 17) + 0x80) & 0xff;
				put_user(val_tmp, apptr);
			} else {
				val = ((*hwptr) << 7) >> ((4 - format) * 8);
				for (i = 0; i < format; i++) {
					val_tmp = (val >> (i * 8)) & 0xff;
					put_user(val_tmp, apptr + i);
				}
			}
			hwptr = hwptr + 1;
			apptr = apptr + format;
			count_bytes = count_bytes - format;
		}
	}

	return 0;
}

static struct snd_pcm_ops rlx_pcm_ops = {
	.open = rlx_pcm_open,
	.close = rlx_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = rlx_pcm_hw_params,
	.hw_free = rlx_pcm_hw_free,
	.prepare = rlx_pcm_prepare,
	.trigger = rlx_pcm_trigger,
	.pointer = rlx_pcm_pointer,
	.ack = rlx_pcm_ack,
	.copy = rlx_pcm_copy,
};

static irqreturn_t rlx_irq_handler(int irq, void *data)
{
	int reg_val, reg;
	struct rlx_dma_data *dma_data;
	struct snd_pcm_substream *substream;

	dma_data = (struct rlx_dma_data *)data;
	reg_val = readl(dma_data->addr + RLX_REG_AUDIO_INT_STS);
	reg = readl(dma_data->addr + RLX_REG_AUDIO_INT_EN) & reg_val;

	if (reg & (1 << RLX_BIT_AUDIO_IN_TIMER)) {
		if (TYPE_RLE0745 == RTS_ASOC_HW_ID(dma_data->devtype))
			writel(0, dma_data->addr + RLX_REG_RX_TIMER_COUNT);

		substream = dma_data->subdata[1].substream;
		if (substream)
			snd_pcm_period_elapsed(substream);
	}

	if (reg & (1 << RLX_BIT_AUDIO_OUT_TIMER)) {
		if (TYPE_RLE0745 == RTS_ASOC_HW_ID(dma_data->devtype))
			writel(0, dma_data->addr + RLX_REG_TX_TIMER_COUNT);

		substream = dma_data->subdata[0].substream;
		if (substream)
			snd_pcm_period_elapsed(substream);
	}

	writel(reg, dma_data->addr + RLX_REG_AUDIO_INT_STS);

	return IRQ_HANDLED;
}

static void rlx_init_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	struct snd_soc_pcm_runtime *rtd = pcm->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct rlx_dma_data *dma_data;

	DBG("rlx init dma buffer\n");

	dma_data = snd_soc_platform_get_drvdata(platform);

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = platform->dev;
	buf->private_data = NULL;
	buf->area = dma_data->dma_buf[stream].area;
	buf->addr = dma_data->dma_buf[stream].addr;
	buf->bytes = dma_data->dma_buf[stream].bytes;
}

static int rlx_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_pcm *pcm = rtd->pcm;

	DBG("rlx pcm new\n");

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream)
		rlx_init_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream)
		rlx_init_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);

	return 0;
}

static void rlx_pcm_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	DBG("rlx pcm free dma buffer\n");

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		buf->area = NULL;
	}
}

static const char *rlx_dma_mode_enum[] = {"normal stereo out",
					"L/R exchange R/L out",
					"mono out(single channel)",
					"mono out(channel copy stereo)",
					"normal stereo in",
					"L/R exchange R/L in",
					"mono in(select left channel)",
					"mono in(select right channel)",
					"mono in(half of left and right)"};

static const SOC_ENUM_SINGLE_EXT_DECL(rlx_dma_soc_mode_enum,
		rlx_dma_mode_enum);

int rlx_dma_get_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct rlx_dma_data *dma_data = snd_soc_platform_get_drvdata(platform);

	ucontrol->value.enumerated.item[0] =
		(dma_data->subdata[0].stereo_mode * 1000) +
		(dma_data->subdata[0].mono_mode * 100) +
		(dma_data->subdata[1].stereo_mode * 10) +
		(dma_data->subdata[1].mono_mode);

	return 0;
}

int rlx_dma_put_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct rlx_dma_data *dma_data = snd_soc_platform_get_drvdata(platform);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int value = ucontrol->value.enumerated.item[0];

	if (value > e->max - 1)
		return -EINVAL;

	switch (value) {
	case RLX_SND_DMA_NORMAL_STEREO_OUT:
	case RLX_SND_DMA_EXCHANGE_STEREO_OUT:
		dma_data->subdata[0].stereo_mode = value;
		break;
	case RLX_SND_DMA_MONO_OUT:
	case RLX_SND_DMA_MONO_TO_STEREO_OUT:
		dma_data->subdata[0].mono_mode = value;
		break;
	case RLX_SND_DMA_NORMAL_STEREO_IN:
	case RLX_SND_DMA_EXCHANGE_STEREO_IN:
		dma_data->subdata[1].stereo_mode = value;
		break;
	case RLX_SND_DMA_MONO_LEFT_IN:
	case RLX_SND_DMA_MONO_RIGHT_IN:
	case RLX_SND_DMA_MONO_MIX_IN:
		dma_data->subdata[1].mono_mode = value;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_kcontrol_new rlx_soc_platform_controls[] = {
	SOC_ENUM_EXT("Audio Mono/Stereo In/Out Mode", rlx_dma_soc_mode_enum,
			rlx_dma_get_enum, rlx_dma_put_enum),
};

static struct snd_soc_platform_driver rlx_dma_platform_driver = {
	.ops = &rlx_pcm_ops,
	.pcm_new = rlx_pcm_new,
	.pcm_free = rlx_pcm_free,
	.controls = rlx_soc_platform_controls,
	.num_controls = ARRAY_SIZE(rlx_soc_platform_controls),
};

static u64 dma_mask = DMA_BIT_MASK(32);

static int rlx_alloc_dma_buffer(struct rlx_dma_data *dma_data)
{
	int i;
	struct snd_dma_buffer *buf;
	struct device *dev = &dma_data->pdev->dev;
	size_t size = rlx_dma_hardware.buffer_bytes_max << 2;

	DBG("rlx alloc dma buffer\n");

	if (!dev->dma_mask)
		dev->dma_mask = &dma_mask;
	if (!dev->coherent_dma_mask)
		dev->coherent_dma_mask = 0xffffffff;

	for (i = 0; i < 2; i++) {
		buf = &dma_data->dma_buf[i];
		buf->area = dma_alloc_coherent(dev, size,
				&buf->addr, GFP_KERNEL);
		if (!buf->area)
			return -ENOMEM;

		buf->bytes = size;
	}

	return 0;
}

static void rlx_free_dma_buffer(struct rlx_dma_data *dma_data)
{
	int i;
	struct snd_dma_buffer *buf;

	DBG("rlx free dma buffer\n");

	for (i = 0; i < 2; i++) {
		buf = &dma_data->dma_buf[i];
		if (buf->area) {
			dma_free_coherent(&dma_data->pdev->dev, buf->bytes,
					buf->area, buf->addr);
			buf->area = NULL;
		}
	}
}

static struct platform_device_id rlx_dma_devtypes[] = {
	{
		.name = "rle0745-fpga-adma",
		.driver_data = TYPE_RLE0745 | TYPE_FPGA,
	}, {
		.name = "rlx0745-adma",
		.driver_data = TYPE_RLE0745,
	}, {
		.name = "rts3901-fpga-adma",
		.driver_data = TYPE_RTS3901 | TYPE_FPGA,
	}, {
		.name = "rts3901-adma",
		.driver_data = TYPE_RTS3901,
	}, {
		.name = "rts3903-fpga-adma",
		.driver_data = TYPE_RTS3903 | TYPE_FPGA,
	}, {
		.name = "rts3903-adma",
		.driver_data = TYPE_RTS3903,
	},
};

static int rlx_dma_probe(struct platform_device *pdev)
{
	int ret, reg_val;
	struct resource *res;
	struct rlx_dma_data *dma_data;
	const struct platform_device_id *id_entry;

	DBG("rlx dma probe\n");

	id_entry = platform_get_device_id(pdev);
	if (!id_entry) {
		pr_err("not support soc audio platform\n");
		return -EINVAL;
	}

	dma_data =
	    devm_kzalloc(&pdev->dev, sizeof(struct rlx_dma_data), GFP_KERNEL);
	if (dma_data == NULL) {
		pr_err("Unable to alloc dma data\n");
		return -ENOMEM;
	}
	dma_data->pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("Unable to get dma address\n");
		ret = -ENXIO;
		goto mem_err;
	}

	if (!request_mem_region(res->start, resource_size(res), "rlx-dma")) {
		pr_err("Unable to request mem region\n");
		ret = -EBUSY;
		goto mem_err;
	}

	dma_data->base = res->start;
	dma_data->size = res->end - res->start + 1;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		pr_err("Unable to get dma irq\n");
		ret = -ENXIO;
		goto err;
	}

	dma_data->irq = res->start;

	dma_data->runflag[0] = 0;
	dma_data->runflag[1] = 0;

	dma_data->subdata[0].substream = NULL;
	dma_data->subdata[1].substream = NULL;
	dma_data->subdata[0].mono_mode = RLX_SND_DMA_MONO_TO_STEREO_OUT;
	dma_data->subdata[1].mono_mode = RLX_SND_DMA_MONO_LEFT_IN;
	dma_data->subdata[0].stereo_mode = RLX_SND_DMA_NORMAL_STEREO_OUT;
	dma_data->subdata[1].stereo_mode = RLX_SND_DMA_NORMAL_STEREO_IN;

	dma_data->dma_buf[0].area = NULL;
	dma_data->dma_buf[1].area = NULL;
	ret = rlx_alloc_dma_buffer(dma_data);
	if (ret) {
		pr_err("failed to alloc dma buffer\n");
		goto dma_err;
	}

	dma_data->addr = ioremap(dma_data->base, dma_data->size);
	if (dma_data->addr == NULL) {
		pr_err("failed to ioremap\n");
		ret = -ENXIO;
		goto dma_err;
	}

	ret = request_irq(dma_data->irq, rlx_irq_handler,
			  0, "audio-platform", dma_data);
	if (ret) {
		pr_err("failed to request irq %d\n", dma_data->irq);
		goto io_err;
	}

	ret = snd_soc_register_platform(&pdev->dev, &rlx_dma_platform_driver);
	if (ret) {
		pr_err("register platform failed");
		goto irq_err;
	}

	dma_data->devtype = id_entry->driver_data;

	dev_set_drvdata(&pdev->dev, dma_data);

	/* init interupt en */
	reg_val = 0;
	writel(reg_val, dma_data->addr + RLX_REG_AUDIO_INT_EN);

	return 0;

irq_err:
	free_irq(dma_data->irq, dma_data);
io_err:
	iounmap(dma_data->addr);
dma_err:
	rlx_free_dma_buffer(dma_data);
err:
	release_mem_region(dma_data->base, resource_size(res));
mem_err:
	devm_kfree(&pdev->dev, dma_data);
	dma_data = NULL;

	return ret;
}

static int rlx_dma_remove(struct platform_device *pdev)
{
	struct rlx_dma_data *dma_data;
	struct resource *res;

	dma_data = dev_get_drvdata(&pdev->dev);
	iounmap(dma_data->addr);
	free_irq(dma_data->irq, dma_data);
	rlx_free_dma_buffer(dma_data);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res)
		release_mem_region(dma_data->base, resource_size(res));

	snd_soc_unregister_platform(&pdev->dev);
	devm_kfree(&pdev->dev, dma_data);
	dev_set_drvdata(&pdev->dev, NULL);
	dma_data = NULL;

	return 0;
}

static struct platform_driver rlx_dma_driver = {
	.driver = {
		.name = "audio-platform",
		.owner = THIS_MODULE,
	},
	.probe = rlx_dma_probe,
	.remove = rlx_dma_remove,
	.id_table = rlx_dma_devtypes,
};

static int __init rlx_platform_init(void)
{
	DBG("rlx platform init\n");

	return platform_driver_register(&rlx_dma_driver);
}
module_init(rlx_platform_init);

static void __exit rlx_platform_exit(void)
{
	platform_driver_unregister(&rlx_dma_driver);
}
module_exit(rlx_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek RLX ALSA soc codec driver");
