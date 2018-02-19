#ifndef __DBGUTILS_DEFINE_H
#define __DBGUTILS_DEFINE_H

typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;

typedef u32		__le32;
typedef u32		dma_addr_t;

typedef u8		__u8;
typedef u16		__u16;
typedef u32		__u32;

typedef u16		__le16;

#define	dword_get_byte(dword, index)			\
	(unsigned char)((dword) >> ((index) * 8))
#define	dword_set_byte(dword, index, value)		\
	((dword) & ~(0xFF << ((index) * 8))) | ((value) << ((index) * 8))

#endif	// __DBGUTILS_DEFINE_H

