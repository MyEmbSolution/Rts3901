
/************************************************************************
 *
 *  init_cpu_s.S
 *
 *  CPU specific startup code
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

/************************************************************************
 *  Include files
 ************************************************************************/
#define _ASSEMBLER_
#include <asm/sysdefs.h>
#include <asm/mips.h>
#include <asm/init.h>

/************************************************************************
 *  Definitions
 ************************************************************************/
#define SPRAM_BASE_PHY 		0x10000000	//Physical address (for setup DMEM)
#define SPRAM_SIZE 		0x00004000	// (for setup DMEM)


/************************************************************************
 *  Public variables
 ************************************************************************/

/************************************************************************
 *  Static variables
 ************************************************************************/

/************************************************************************
 *  Implementation : Public functions
 ************************************************************************/
	.set noreorder

/************************************************************************
 *  sys_init_processor
 ************************************************************************/
ULEAF(sys_init_processor)



	/* Setup MIPS1074K specifics (implementation dependent fields) */
	MFC0(   t0, C0_Config )
	li	t1, CONFIG0_MIPS32_64_MSK
	and	t0, t1
	li	t1, CONFIG0_MIPS1074K
	or	t0, t1
	MTC0(	t0, C0_Config )

	MFC0(   t0, C0_Status )
	li	t1, STATUS_MIPS32_64_MSK
	and	t0, t1
	li	t1, STATUS_MIPS1074K
	or	t0, t1
	MTC0(	t0, C0_Status )

	/* Check if it is a MIPS32/64 processor */
	li      t0, M_PRIdCoID
	and	t0, k1
	srl	t0, S_PRIdCoID
	li	t1, C0_PRID_COMP_NOT_MIPS32_64
	bne	t0, t1, mips32_64
	nop
	/* Unknown CPU */
	jr	ra
	li	v0, 100


/************************************************************************
 *  MIPS32/MIPS64 specific cpu initialisation
 ************************************************************************/
mips32_64:

	/* Setup generic MIPS32/MIPS64 fields of STATUS register */
	MFC0(   t0, C0_Status )
	li	t1, ~STATUS_MIPS32_64_MSK
	and	t0, t1
	li	t1, STATUS_MIPS32_64
	or	t0, t1
	MTC0(	t0, C0_Status )

	/* Setup generic MIPS32 fields of CONFIG0 register */
	MFC0(   t0, C0_Config )
	li	t1, ~CONFIG0_MIPS32_64_MSK
	and	t0, t1
	li	t1, CONFIG0_MIPS32_64
	or	t0, t1
	MTC0(	t0, C0_Config )

	/* Setup ES field of CONFIG7 register (let sync instruction go out on the external bus) */
	MFC0_SEL_OPCODE(R_t0, R_C0_Config7, R_C0_SelConfig7)
	li	t1, M_Config7ES
	or	t0, t1
	MTC0_SEL_OPCODE(R_t0, R_C0_Config7, R_C0_SelConfig7)


	/********************************************
	 ** Setup DMEM
	 *******************************************/
	# Enable ErrCtl SPR
	mfc0	v0, C0_ErrCtl
	li	v1, 0x10000000  	# set bit 28 = 1
	or	v0, v1
	mtc0	v0, C0_ErrCtl

	# start address, valid bit 7 = 1
	li	v0, SPRAM_BASE_PHY | 0x80
	mtc0	v0, C0_TAGLO, 2
	li	v0, 0x80000000
	cache	DCACHE_INDEX_STORE_TAG, 0(v0)   # Clear VMC model default data bus unknow value
	cache	DCACHE_INDEX_STORE_TAG, 0(v0)   # Re-setup again

	# size
	li	v0, SPRAM_SIZE
	mtc0	v0, C0_TAGLO, 2
	li	v0, 0x80000000
	cache	DCACHE_INDEX_STORE_TAG, 8(v0)

	# Disable ErrCtl SPR
	mfc0	v0, C0_ErrCtl
	li	v1, 0xefffffff 		# set bit 28 = 0
	and	v0, v1
	mtc0	v0, C0_ErrCtl


	jr	ra
	move	v0, zero


END(sys_init_processor)
