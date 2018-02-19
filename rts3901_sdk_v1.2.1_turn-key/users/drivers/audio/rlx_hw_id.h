#ifndef __SND_SOC_RLX_HW_ID_H
#define __SND_SOC_RLX_HW_ID_H

enum {
	TYPE_RLE0745 = 1,
	TYPE_RTS3901 = 2,
	TYPE_RTS3903 = 3,

	TYPE_FPGA = (1 << 16),
};

#define RTS_ASOC_HW_ID(type)		((type) & 0xff)

#endif
