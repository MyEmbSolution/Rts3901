
/************************************************************************
 *
 *  init.h
 *
 *  Local definitions for init code
 *
 * ######################################################################
 *
 * mips_start_of_legal_notice
 *
 * Copyright (c) 2003 MIPS Technologies, Inc. All rights reserved.
 *
 *
 * Unpublished rights (if any) reserved under the copyright laws of the
 * United States of America and other countries.
 *
 * This code is proprietary to MIPS Technologies, Inc. ("MIPS
 * Technologies"). Any copying, reproducing, modifying or use of this code
 * (in whole or in part) that is not expressly permitted in writing by MIPS
 * Technologies or an authorized third party is strictly prohibited. At a
 * minimum, this code is protected under unfair competition and copyright
 * laws. Violations thereof may result in criminal penalties and fines.
 *
 * MIPS Technologies reserves the right to change this code to improve
 * function, design or otherwise. MIPS Technologies does not assume any
 * liability arising out of the application or use of this code, or of any
 * error or omission in such code. Any warranties, whether express,
 * statutory, implied or otherwise, including but not limited to the implied
 * warranties of merchantability or fitness for a particular purpose, are
 * excluded. Except as expressly provided in any written license agreement
 * from MIPS Technologies or an authorized third party, the furnishing of
 * this code does not give recipient any license to any intellectual
 * property rights, including any patent rights, that cover this code.
 *
 * This code shall not be exported or transferred for the purpose of
 * reexporting in violation of any U.S. or non-U.S. regulation, treaty,
 * Executive Order, law, statute, amendment or supplement thereto.
 *
 * This code constitutes one or more of the following: commercial computer
 * software, commercial computer software documentation or other commercial
 * items. If the user of this code, or any related documentation of any
 * kind, including related technical data or manuals, is an agency,
 * department, or other entity of the United States government
 * ("Government"), the use, duplication, reproduction, release,
 * modification, disclosure, or transfer of this code, or any related
 * documentation of any kind, is restricted in accordance with Federal
 * Acquisition Regulation 12.212 for civilian agencies and Defense Federal
 * Acquisition Regulation Supplement 227.7202 for military agencies. The use
 * of this code by the Government is further restricted in accordance with
 * the terms of the license agreement(s) and/or applicable contract terms
 * and conditions covering this code from MIPS Technologies or an authorized
 * third party.
 *
 *
 * mips_end_of_legal_notice
 *
 *
 ************************************************************************/

#ifndef INIT_H
#define INIT_H

/************************************************************************
 *  Include files
 ************************************************************************/

#include <asm/mips.h>

/************************************************************************
 *  Definitions
*************************************************************************/

/* MIPS32/MIPS64 specifics */

/*  Setup of STATUS register used for MIPS32/MIPS64 processors
 *  FR field only relevant for MIPS64 (Read only for MIPS32)
 */
#define STATUS_MIPS32_64   (M_StatusBEV | M_StatusFR)

/*  Generic MIPS32/MIPS64 fields of STATUS register (ie the ones not
 *  reserved for implementations)
 */
#define STATUS_MIPS32_64_MSK   0xfffcffff

/* Setup of CONFIG register used for MIPS32/MIPS64 processors */

#ifdef NO_CACHE

#define CONFIG0_MIPS32_64    (K_CacheAttrU << S_ConfigK0)

#else

#ifdef KSEG0_UNCACHED
#define CONFIG0_MIPS32_64    (K_CacheAttrU << S_ConfigK0)
#else
//#define CONFIG0_MIPS32_64    (0 << S_ConfigK0) /* this is cache write through (debug use) */
#define CONFIG0_MIPS32_64    (K_CacheAttrCN << S_ConfigK0) /* this is cache write back */
#endif

#endif

/*  Generic MIPS32/MIPS64 fields of CONFIG0 register (ie the ones not
 *  reserved for implementations)
 */
#define CONFIG0_MIPS32_64_MSK  0x8000ffff


/* MIPS 4K/5K family specifics (excluding generic MIPS32/MIPS64 fields) */
#define STATUS_MIPS4K5K	    0
#define CONFIG0_MIPS4K5K ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU))

/* MIPS 24K specifics */
#define STATUS_MIPS24K	    0
#define CONFIG0_MIPS24K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 34K specifics */
#define STATUS_MIPS34K	    0
#define CONFIG0_MIPS34K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 74K specifics */
#define STATUS_MIPS74K	    0
#define CONFIG0_MIPS74K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 1074K specifics */
#define STATUS_MIPS1074K    0
#define CONFIG0_MIPS1074K  ((K_CacheAttrCN << S_ConfigK23) |\
			    (K_CacheAttrCN << S_ConfigKU)  |\
			    (M_ConfigMM))

/* MIPS 1004K specifics */
#define STATUS_MIPS1004K	    0
#define CONFIG0_MIPS1004K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 20Kc/25Kf specifics (excluding generic MIPS32/MIPS64 fields) */
#ifdef WORKAROUND_20KC_25KF
#define STATUS_MIPS20KC_25KF	(0x1 << 16)
#else
#define STATUS_MIPS20KC_25KF	0
#endif
#define CONFIG0_MIPS20KC_25KF   0

/**** Cpu specific initialisation ****/

#ifdef _ASSEMBLER_

#define MSG( name, s ) \
	.##align 3;      \
name:   .##asciiz  s

#define ERROR_HANDLING						\
								\
error_loop:							\
	b	error_loop;					\
	nop

#define FUNC_CONFIGURE_SDRAM	4


#else  /* #ifdef _ASSEMBLER_ */

/************************************************************************
 *
 *                          arch_core_estimate_busfreq
 *  Description :
 *  -------------
 *
 *  Estimate external bus (SysAD) clock frequency.
 *
 *  Return values :
 *  ---------------
 *
 *  Estimated frequency in Hz.
 *
 ************************************************************************/
UINT32
arch_core_estimate_busfreq( void );

#endif /* #ifdef _ASSEMBLER_ */

#endif /* #ifndef INIT_H */
