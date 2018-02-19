#ifndef __SND_SOC_RLX_I2S_H
#define __SND_SOC_RLX_I2S_H

#define RLX_I2S_DIV_BCLK		0
#define RLX_I2S_DIV_MCLK		1

#define RLX_I2S_CODEC_PC		0
#define RLX_I2S_CODEC_CE_256FS		1
#define RLX_I2S_CODEC_CE_512FS		2

/* i2s reg */
#define RLX_REG_I2S_CTL			0x0
#define RLX_REG_I2S_CFG			0x4

/* i2s power reg */
#define RLX_REG_I2S_OCP_CTL		0x0

/* RLX_REG_I2S_CTL */
#define RLX_I2S_IN_GO			(0)
#define RLX_I2S_OUT_GO			(1)
#define RLX_I2S_IN_LCHANNLE_MUTE	(2)
#define RLX_I2S_IN_RCHANNLE_MUTE	(3)
#define RLX_I2S_OUT_LCHANNLE_MUTE	(4)
#define RLX_I2S_OUT_RCHANNLE_MUTE	(5)

/* RLX_REG_I2S_CFG */
#define RLX_I2S_SAMPLE_RATE		(0)
#define RLX_I2S_IN_STOP_CFG		(3)
#define RLX_I2S_OUT_STOP_CFG		(4)
#define RLX_I2S_PCM_FORMAT		(5)
#define RLX_BCLK_POLARITY		(8)
#define RLX_LRCK_POLARITY		(9)
#define RLX_SCK_GEN_SEL			(15)
#define RLX_BCLK_INT_DIV		(16)
#define RLX_MCLK_DIV_SEL		(24)

/* RLX_REG_I2S_OCP_CTL */
#define RLX_LDOI2S_POW			(0)

#define RLX_I2S_RATES	(SNDRV_PCM_RATE_8000 |\
			 SNDRV_PCM_RATE_16000 |\
			 SNDRV_PCM_RATE_32000 |\
			 SNDRV_PCM_RATE_44100 |\
			 SNDRV_PCM_RATE_48000 |\
			 SNDRV_PCM_RATE_KNOT)

#define RLX_I2S_FORMATS	(SNDRV_PCM_FMTBIT_U8 |\
			 SNDRV_PCM_FMTBIT_S16_LE |\
			 SNDRV_PCM_FMTBIT_S24_3LE)

#define RLX_I2S_ADDR_NUM		2

struct rlx_i2s_dai_data {
	struct platform_device *pdev;
	void __iomem *addr;
	void __iomem *addrp;

	struct clk *i2s_clk;
	struct clk *pll_clk;
	int clk_ref;
	struct mutex clk_mutex;
	int ldoi2s_ref;
	struct mutex ldoi2s_mutex;
	int channels;
	u32 devtype;
};

#endif /* __SND_SOC_RLX_I2S_H */
