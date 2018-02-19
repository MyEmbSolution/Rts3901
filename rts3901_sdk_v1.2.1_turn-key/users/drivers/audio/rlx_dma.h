#ifndef __SND_SOC_RLX_DMA_H
#define __SND_SOC_RLX_DMA_H

#define RLX_COMMON_FORMAT_SHIFT			2

#define RLX_REG_RX_FIFO_THRESHOLD		0x00
#define RLX_REG_TX_FIFO_THRESHOLD		0x04
#define RLX_REG_RX_SA				0x08
#define RLX_REG_RX_LEN				0x0C
#define RLX_REG_RX_RP				0x10
#define RLX_REG_RX_WP				0x14
#define RLX_REG_TX_SA				0x18
#define RLX_REG_TX_LEN				0x1C
#define RLX_REG_TX_RP				0x20
#define RLX_REG_TX_WP				0x24
#define RLX_REG_FIFO_ENABLE			0x28
#define RLX_REG_FIFO_CTL			0x2C
#define RLX_REG_AUDIO_CTL			0x30
#define RLX_REG_RX_BUFFER_THRESHOLD		0x34
#define RLX_REG_RX_TIMER_COUNT			0x38
#define RLX_REG_RX_TIMER_THRESHOLD		0x3C
#define RLX_REG_TX_BUFFER_THRESHOLD		0x40
#define RLX_REG_TX_TIMER_COUNT			0x44
#define RLX_REG_TX_TIMER_THRESHOLD		0x48
#define RLX_REG_AUDIO_INT_EN			0x4C
#define RLX_REG_AUDIO_INT_STS			0x50
#define RLX_REG_RX_FIFO_STATUS			0x54
#define RLX_REG_TX_FIFO_STATUS			0x58
#define RLX_REG_FIFO_ACCESS_ADDR		0x5C
#define RLX_REG_FIFO_ACCESS_DATA		0x60

/* RLX_REG_RX_SA */
#define RLX_BIT_RX_SA				(0)

/* RLX_REG_RX_RP */
#define RLX_BIT_RX_RP				(0)

/* RLX_REG_RX_WP */
#define RLX_BIT_RX_WP				(0)

/* RLX_REG_TX_SA */
#define RLX_BIT_TX_SA				(0)

/* RLX_REG_TX_RP */
#define RLX_BIT_TX_RP				(0)

/* RLX_REG_TX_WP */
#define RLX_BIT_TX_WP				(0)

/* RLX_REG_FIFO_ENABLE */
#define RLX_BIT_TX_FIFO_ENABLE			(0)
#define RLX_BIT_RX_FIFO_ENABLE			(1)

/* RLX_REG_FIFO_CTL */
#define RLX_BIT_TX_FIFO_FLUSH			(0)
#define RLX_BIT_RX_FIFO_FLUSH			(1)

/* RLX_REG_AUDIO_CTL */
#define RLX_BIT_IN_SOURCE_SELECT		(0)
#define RLX_BIT_OUT_SOURCE_SELECT		(1)
#define RLX_BIT_AUDIO_IN_MODE			(2)
#define RLX_BIT_AUDIO_OUT_MODE			(5)
#define RLX_BIT_IN_DATA_WIDTH			(7)
#define RLX_BIT_OUT_DATA_WIDTH			(9)

/* RLX_REG_RX_TIMER_THRESHOLD */
#define RLX_BIT_AUDIO_IN_TIMER_EN		(20)

/* RLX_REG_TX_TIMER_THRESHOLD */
#define RLX_BIT_AUDIO_OUT_TIMER_EN		(20)

/* RLX_REG_AUDIO_INT_STS */
#define RLX_BIT_AUDIO_IN_TIMER			(0)
#define RLX_BIT_AUDIO_IN_DDR_THRESHOLD		(1)
#define RLX_BIT_AUDIO_IN_FIFO_FULL		(2)
#define RLX_BIT_AUDIO_IN_DDR_FULL		(3)
#define RLX_BIT_AUDIO_OUT_TIMER			(4)
#define RLX_BIT_AUDIO_OUT_DDR_THRESHOLD		(5)
#define RLX_BIT_AUDIO_OUT_FIFO_EMPTY		(6)
#define RLX_BIT_AUDIO_OUT_DDR_EMPTY		(7)
#define RLX_BIT_OCP_READ_ERR			(8)

#define RLX_SND_DMA_TIMER_IN			0
#define RLX_SND_DMA_TIMER_OUT			1

enum {
	RLX_SND_DMA_NORMAL_STEREO_OUT,
	RLX_SND_DMA_EXCHANGE_STEREO_OUT,
	RLX_SND_DMA_MONO_OUT,
	RLX_SND_DMA_MONO_TO_STEREO_OUT,
	RLX_SND_DMA_NORMAL_STEREO_IN,
	RLX_SND_DMA_EXCHANGE_STEREO_IN,
	RLX_SND_DMA_MONO_LEFT_IN,
	RLX_SND_DMA_MONO_RIGHT_IN,
	RLX_SND_DMA_MONO_MIX_IN,
};

struct rlx_dma_subdata {
	struct snd_pcm_substream *substream;
	int format;
	int codec_sel;
	int mono_mode;
	int stereo_mode;
};

struct rlx_dma_data {
	struct platform_device *pdev;
	void __iomem *addr;
	unsigned long size;
	u32 base;
	int irq;
	int runflag[2];
	struct rlx_dma_subdata subdata[2];
	struct snd_dma_buffer dma_buf[2];
	u32 devtype;
};

#endif /* __SND_SOC_RLX_DMA_H */
