/*
 * rts_hconf.h
 *
 * Copyright(C) 2015 Micky Ching, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef UAPI_RTS_HCONF_H
#define UAPI_RTS_HCONF_H /* UAPI_RTS_HCONF_H */


#define HCONF_CHAR_MAJOR			123
#define HCONF_CHAR_MINOR			0
#define HCONF_CHAR_NUM				1

#define HCONF_IOC_MAGIC				0x76
#define HCONF_IOC_READ_ALL			_IO(HCONF_IOC_MAGIC, 1)
#define HCONF_IOC_WRITE_ALL			_IO(HCONF_IOC_MAGIC, 2)
#define HCONF_IOC_READ_HEADER			_IO(HCONF_IOC_MAGIC, 3)
#define HCONF_IOC_WRITE_HEADER			_IO(HCONF_IOC_MAGIC, 4)
#define HCONF_IOC_READ_ENTRY			_IO(HCONF_IOC_MAGIC, 5)
#define HCONF_IOC_WRITE_ENTRY			_IO(HCONF_IOC_MAGIC, 6)
#define HCONF_IOC_SYNC				_IO(HCONF_IOC_MAGIC, 7)
#define HCONF_IOC_CLEAR				_IO(HCONF_IOC_MAGIC, 8)
#define HCONF_IOC_DEL_ENTRY			_IO(HCONF_IOC_MAGIC, 9)

#define HCONF_MTD_NAME				"hconf"

#define HCONF_HEADER_MAGIC			0x68636f6e
#define HCONF_TOTAL_LEN_MAX			0x10000 /* n * eb_size */
#define HCONF_ENTRY_NUM_MAX			64
#define HCONF_ENTRY_LEN_MAX			128

enum {
	HCONF_ENTRY_DATA_INT,
	HCONF_ENTRY_DATA_STR,
	HCONF_ENTRY_DATA_TYPE,		/* end flag */
};

#endif /* UAPI_RTS_HCONF_H */
