/*
 * Realtek Semiconductor Corp.
 *
 * ../../../../include/uapi/linux/rts_camera_jpgenc.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _U_RTS_CAMERA_JPGENC_H
#define _U_RTS_CAMERA_JPGENC_H

#define RTSJPGENC_IOC_MAGIC	'j'

#define RTSJPGENC_IOCGHWOFFSET		_IOR(RTSJPGENC_IOC_MAGIC, 1, unsigned long *)
#define RTSJPGENC_IOCGHWIOSIZE		_IOR(RTSJPGENC_IOC_MAGIC, 2, unsigned int *)
#define RTSJPGENC_IOCHWRESET		_IO(RTSJPGENC_IOC_MAGIC, 3)

#define RTSJPGENC_IOC_MAXNR	3

#endif
