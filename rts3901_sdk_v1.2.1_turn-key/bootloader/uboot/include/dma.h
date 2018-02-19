#ifndef __DMA_H__
#define __DMA_H__

/*DMA register begin*/
#define DMA_BASE        0xb8020000

#define SAR0                    (DMA_BASE + 0x0)
#define DAR0                    (DMA_BASE + 0x8)
#define CTL0                    (DMA_BASE + 0x18)
#define CFG0                    (DMA_BASE + 0x40)
#define RAWTFR                  (DMA_BASE + 0x2C0)
#define CLEARTFR                (DMA_BASE + 0x338)
#define CLEARBLOCK              (DMA_BASE + 0x340)
#define CLEARSRCTRAN            (DMA_BASE + 0x348)
#define CLEARDSTTRAN            (DMA_BASE + 0x350)
#define CLEARERR                (DMA_BASE + 0x358)
#define DMACFGREG               (DMA_BASE + 0x398)
#define CHENREG                 (DMA_BASE + 0x3A0)
#define CH0_INT                 0x01

void dma_copy(u32 src_addr, u32 dst_addr, u32 trans_length);

#endif

