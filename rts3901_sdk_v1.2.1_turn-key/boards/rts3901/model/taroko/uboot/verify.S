#include <config.h>
#include <asm/regdef.h>
#include <asm/rlxregs.h>
#include "ddr_def.h"
#include "bspchip.h"

	.text
	.set noreorder

#ifdef ASIC_TEST_CPU
	.globl asic_test_cpu
	.ent asic_test_cpu
asic_test_cpu:
	subu	sp, 4
	sw	ra, 0(sp)

#ifdef IMEM_TEST
	la	t9, imem_test
	jalr	t9
	 nop
#endif

#ifdef IMEMFILL_TEST
	 la	t9, imemfill_test
	 jalr	t9
	  nop
#endif

#ifdef PWM_TEST
	 la	t9, pwm_test
	 jalr	t9
	  nop
#endif

#ifdef WMPU_TEST
	la	t9, wmpu_test
	jalr	t9
	 nop
#endif

	lw	ra, 0(sp)
	addu	sp, 4

	jr	ra
	 nop
	.end asic_test_cpu
#endif

#ifdef WMPU_TEST
	.globl wmpu_test
	.ent wmpu_test
wmpu_test:
#ifdef WMPU_TEST_READ
	/*entry 0 , watch point read 0x88000000~0x88000008*/
	li	t0, 0x88000002
	mtc0	t0, CP0_WATCHLO
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI
	/*entry 1 , watch point read 0x88100000~0x88100008*/
	li	t0, 0x88100002
	mtc0	t0, CP0_WATCHLO, 1
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 1
	/*entry 2 , watch point read 0x88200000~0x88200008*/
	li	t0, 0x88200002
	mtc0	t0, CP0_WATCHLO, 2
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 2
	/*entry 3 , watch point read 0x88300000~0x88300008*/
	li	t0, 0x88300002
	mtc0	t0, CP0_WATCHLO, 3
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 3
	/*entry 4 , watch point read 0x88400000~0x88400008*/
	li	t0, 0x88400002
	mtc0	t0, CP0_WATCHLO, 4
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 4
	/*entry 5 , watch point read 0x88500000~0x88500008*/
	li	t0, 0x88500002
	mtc0	t0, CP0_WATCHLO, 5
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 5
	/*entry 6 , watch point read 0x88600000~0x88600008*/
	li	t0, 0x88600002
	mtc0	t0, CP0_WATCHLO, 6
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 6
	/*entry 7 , watch point read 0x88700000~0x88700008*/
	li	t0, 0x88700002
	mtc0	t0, CP0_WATCHLO, 7
	li	t0, 0x0
	mtc0	t0, CP0_WATCHHI, 7

	/*enable entrys, watch point mode*/
	li	t0, 0xFF0002
	mtlxc0	t0, $5

	/*read test*/
	lw	t0, 0x88000000
	lw	t0, 0x88100000
	lw	t0, 0x88200000
	lw	t0, 0x88300000
	lw	t0, 0x88400000
	lw	t0, 0x88500000
	lw	t0, 0x88600000
	lw	t0, 0x88700000
#endif
#ifdef WMPU_TEST_WRITE
	/*entry 0 , watch point write 0x88000000~0x88000008*/
	li	t0, 0x88000001
	mtc0	t0, CP0_WATCHLO
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI
	/*entry 1 , watch point write 0x88100000~0x88100008*/
	li	t0, 0x88100001
	mtc0	t0, CP0_WATCHLO, 1
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 1
	/*entry 2 , watch point write 0x88200000~0x88200008*/
	li	t0, 0x88200001
	mtc0	t0, CP0_WATCHLO, 2
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 2
	/*entry 3 , watch point write 0x88300000~0x88300008*/
	li	t0, 0x88300001
	mtc0	t0, CP0_WATCHLO, 3
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 3
	/*entry 4 , watch point write 0x88400000~0x88400008*/
	li	t0, 0x88400001
	mtc0	t0, CP0_WATCHLO, 4
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 4
	/*entry 5 , watch point write 0x88500000~0x88500008*/
	li	t0, 0x88500001
	mtc0	t0, CP0_WATCHLO, 5
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 5
	/*entry 6 , watch point write 0x88600000~0x88600008*/
	li	t0, 0x88600001
	mtc0	t0, CP0_WATCHLO, 6
	li	t0, 0x80000000
	mtc0	t0, CP0_WATCHHI, 6
	/*entry 7 , watch point write 0x88700000~0x88700008*/
	li	t0, 0x88700001
	mtc0	t0, CP0_WATCHLO, 7
	li	t0, 0x0
	mtc0	t0, CP0_WATCHHI, 7

	/*enable entrys, watch point mode*/
	li	t0, 0xFF0002
	mtlxc0	t0, $5

	/* write test */
	li	t0, 0
	sw	t0, 0x88000000
	sw	t0, 0x88100000
	sw	t0, 0x88200000
	sw	t0, 0x88300000
	sw	t0, 0x88400000
	sw	t0, 0x88500000
	sw	t0, 0x88600000
	sw	t0, 0x88700000
#endif
#ifdef WMPU_TEST_INST
	la	t0, instruction
	or	t0, 0x4
	/*entry 0 , watch point inst*/
	mtc0	t0, CP0_WATCHLO
	li	t1, 0x80000000
	mtc0	t1, CP0_WATCHHI
	/*entry 1 , watch point inst*/
	addu	t0, 0x8
	mtc0	t0, CP0_WATCHLO, 1
	li	t1, 0x80000000
	mtc0	t1, CP0_WATCHHI, 1
	/*entry 2 , watch point inst*/
	addu	t0, 0x8
	mtc0	t0, CP0_WATCHLO, 2
	li	t1, 0x80000000
	mtc0	t1, CP0_WATCHHI, 2
	/*entry 3 , watch point inst*/
	addu	t0, 0x8
	mtc0	t0, CP0_WATCHLO, 3
	li	t1, 0x80000000
	mtc0	t1, CP0_WATCHHI, 3
	/*entry 4 , watch point inst*/
	addu	t0, 0x8
	mtc0	t0, CP0_WATCHLO, 4
	li	t1, 0x80000000
	mtc0	t1, CP0_WATCHHI, 4
	/*entry 5 , watch point inst*/
	addu	t0, 0x8
	mtc0	t0, CP0_WATCHLO, 5
	li	t1, 0x80000000
	mtc0	t1, CP0_WATCHHI, 5
	/*entry 6 , watch point inst*/
	addu	t0, 0x8
	mtc0	t0, CP0_WATCHLO, 6
	li	t1, 0x80000000
	mtc0	t1, CP0_WATCHHI, 6
	/*entry 7 , watch point inst*/
	addu	t0, 0x8
	mtc0	t0, CP0_WATCHLO, 7
	li	t1, 0x0
	mtc0	t1, CP0_WATCHHI, 7

	/*enable entrys, watch point mode*/
	li	t0, 0xFF0002
	mtlxc0	t0, $5

	.align	3
instruction:
	/* instruction test */
	li	t0, 0
	li	t0, 0
	li	t0, 1
	li	t0, 1
	li	t0, 2
	li	t0, 2
	li	t0, 3
	li	t0, 3
	li	t0, 4
	li	t0, 4
	li	t0, 5
	li	t0, 5
	li	t0, 6
	li	t0, 6
	li	t0, 7
	li	t0, 7
#endif
        jr      ra
         nop
        .end wmpu_test
#endif

#ifdef IMEM_TEST
	.globl imem_test
	.ent imem_test
imem_test:
	/* use cache instruction to fill imem, then test if imem worded */
	mfc0	t0, CP0_STATUS
	or	t0, ST0_CU3
	mtc0	t0, CP0_STATUS
	nop

	la	t0, imem_begin
	addu	t0, 0x2000
	and	t0, ~0x1fff
	subu	t1, t0, 0x80000000 /* get the phy addr */
	mtc3	t1, $2
	addu	t2, t1, 0x1fff
	mtc3	t2, $3
	nop

	mtc0	zero, $20, 1
	li	t3, 0x40
	mtc0	t3, $20, 1
	nop

	/* store to imem, then clear the ram */
	addu	t1, t0, 0x2000
1:
	cache	0xa, 0(t0)
	sw	zero, 0(t0)
	addu	t0, 4
	sw	zero, 0(t0)
	addu	t0, 4

	blt	t0, t1, 1b
	 nop

	move	t0, ra
	li	a0, 3
	bal	pwm_call
	 nop
	move	ra, t0

	jr	ra
	 nop
imem_begin:
	.end imem_test
#endif

#ifdef IMEMFILL_TEST
	.globl imemfill_test
	.ent imemfill_test
imemfill_test:
	mfc0	t0, CP0_STATUS
	or	t0, ST0_CU3
	mtc0	t0, CP0_STATUS
	nop

	/*set IMEM0 from 0x0000000~0x0001fff*/
	lui	t0, 0x000
	mtc3	t0, $2
	lui	t0, 0x000
	ori	t0, 0x1fff
	mtc3	t0, $3

	mtc0	zero, $20, 1
	li	t0, 0x10
	mtc0	t0, $20, 1
	nop

	/*first turn off imem0 then turn on*/
	mtc0	zero, $20, 1
	li	t0, 0x20
	mtc0	t0, $20, 1
	nop
	mtc0	zero, $20, 1
	li	t0, 0x40
	mtc0	t0, $20, 1
	nop

	jr	ra
	 nop

	.end imemfill_test
#endif

#ifdef PWM_TEST
	.globl pwm_test
	.ent pwm_test
pwm_test:
	li	t0, GPIO_ETN_LED_SEL_REG
	li	t1, GPIO_ETN_LED_SEL_PWM
	sw	t1, 0(t0)

	li	t0, PWM_NUM_REG
	li	t1, 4
	sw	t1, 0(t0)

	li	t0, PWM_HIGH_REG
	li	t1, 0x7fffff
	sw	t1, 0(t0)
	li	t0, PWM_LOW_REG
	li	t1, 0x7fffff
	sw	t1, 0(t0)

	li	t0, PWM_EN_REG
	li	t1, 0
	sw	t1, 0(t0)
	 nop
	li	t1, 1
	sw	t1, 0(t0)

# ifdef WATCH_DOG_TEST
	li	t0, WATCH_DOG_CFG_REG
	li	t1, 0x203
	sw	t1, 0(t0)
# endif
1:
# ifdef WATCH_DOG_FEED
	li	t2, WATCH_DOG_CTL_REG
	li	t3, 1
	sw	t3, 0(t2)
# endif
	b	1b
	 nop

	jr	ra
	 nop

	.end pwm_test
#endif

#ifdef ASIC_TEST_CPU
	/* pwm_call for test*/
	.globl pwm_call
	.ent pwm_call
pwm_call:
	li	a1, PWM_NUM_REG
	sw	a0, 0(a1)
	move	a2, a0

	li	a0, GPIO_ETN_LED_SEL_REG
	li	a1, GPIO_ETN_LED_SEL_PWM
	sw	a1, 0(a0)

	li	a0, PWM_HIGH_REG
	li	a1, 0x7fffff
	sw	a1, 0(a0)
	li	a0, PWM_LOW_REG
	li	a1, 0xffffff
	sw	a1, 0(a0)
	li	a0, PWM_INV_REG
	li	a1, 0x1
	sw	a1, 0(a0)

	li	a0, PWM_EN_REG
	li	a1, 0
	sw	a1, 0(a0)
	 nop
	li	a1, 1
	sw	a1, 0(a0)
	 nop
2:
	li	a0, 0
	li	a1, 0x4000000
1:
	addu	a0, 1
	bne	a0, a1, 1b
	 nop
	subu	a2, 1
	bnez	a2, 2b
	 nop

	li	a0, 0x4000000
1:
	bnez	a0, 1b
	 subu	a0, 1
	jr	ra
	 nop
	.end pwm_call

	/*interrupt handle*/
	.globl gen_exception
	.ent gen_exception
gen_exception:
#ifdef WMPU_TEST
	/*this exception test is before gd(k0) set and C language*/
	subu	sp, 8
	sw	ra, 4(sp)
	sw	a0, 0(sp)

	mfc0	a0, $13
	mfc0	k1, $14
	and	a0, 0x7C
	srl	a0, 2 /*get ExtCode*/
	subu	a0, 23
1:	bnez	a0, 1b
	addu	k1, 4

	li	a0, 1
	bal	pwm_call
	 nop

	lw	ra, 4(sp)
	lw	a0, 0(sp)
	addu	sp, 8

	jr	k1
	 rfe
#endif
#ifdef INTERRUPT_TEST_SPIC
	mfc0	k1, $14

	subu	sp, 12
	sw	ra, 8(sp)
	sw	t9, 4(sp)
	sw	a0, 0(sp)

	la	t9, printf_spic_status
	jalr	t9
	 nop

	li	a0, 3
	bal	pwm_call
	 nop

	lw	ra, 8(sp)
	lw	t9, 4(sp)
	lw	a0, 0(sp)
	addu	sp, 12

	jr	k1
	 rfe
#endif
1:	b	1b
	 nop
	.end gen_exception

#ifdef INTERRUPT_TEST_TIMER
	.globl timer_interrupt
	.ent timer_interrupt
timer_interrupt:
	subu	sp, 8
	sw	t0, 4(sp)
	sw	t1, 0(sp)

	mfc0	t0, CP0_COMPARE
	addu	t0, 0x8000000
	mtc0	t0, CP0_COMPARE

	/* turn on or turn off led */
	li	t0, 0xb8800058
	lw	t1, 0(t0)
	xor	t1, 2
	sw	t1, 0(t0)

	lw	t0, 4(sp)
	lw	t1, 0(sp)
	addu	sp, 8

	mfc0	k1, $14
	nop
	jr	k1
	 rfe
	.end timer_interrupt
#endif
#endif


#ifdef CONFIG_OLD_TEST
	.globl old_test
	.ent old_test
old_test:
#ifdef _WMPU_TEST_
	/*entry 0 , watch point read 0xb8810200~0xb8810208 */
	li	a0, 0xb8810202
	mtc0	a0, CP0_WATCHLO

	li	a0, 0x08
	mtc0	a0, CP0_WATCHHI

	/*entry 1, watch point write 0xb8810000~0xb8810008*/
	li	a0, 0xb8810001
	mtc0	a0, CP0_WATCHLO, 1

	li	a0, 0x08
	mtc0	a0, CP0_WATCHHI, 1

	/*enable entry0 & entry 1, watch point mode*/
	li	a0, 0x30002
	mtlxc0	a0, $5

	li	a0, 0x02
	mtlxc0	a0, $6
#endif

#ifdef CONFIG_MEM_TEST
	mfc0	t0, CP0_STATUS
	or	t0, ST0_CU3
	mtc0	t0, CP0_STATUS
	nop

	/*set DMEM0 from 0xf00000~0xf03fff*/
	lui	a0, 0xf0
	mtc3	a0, $4
	lui	a0, 0xF0
	ori	a0, 0x3fff
	mtc3	a0, $5

	mtc0	zero, $20
	li	a0, 0x400
	mtc0	a0, $20
	nop

	/*set IMEM0 from 0x1000000~0x1007fff*/
	lui	a0, 0x100
	mtc3	a0, $0
	lui	a0, 0x100
	ori	a0, 0x7fff
	mtc3	a0, $1

	mtc0	zero, $20
	li	a0, 0x40
	mtc0	a0, $20
	nop
#endif

#ifdef _IMEM_DMA_TEST_
	mfc0	t0, CP0_STATUS
	or	t0, ST0_CU3
	mtc0	t0, CP0_STATUS
	nop

	/*set IMEM0 from 0x0000000~0x0007fff*/
	lui	a0, 0x000
	mtc3	a0, $0
	lui	a0, 0x000
	ori	a0, 0x7fff
	mtc3	a0, $1

	mtc0	zero, $20
	li	a0, 0x40
	mtc0	a0, $20
	nop

	mtc0	zero, $20
	li	a0, 0x10
	mtc0	a0, $20
	nop
#endif

	/*DDR bist test function , set stack point into DMEM*/
#ifdef CONFIG_DDR_BIST
	/*enable CP3*/
	mfc0	a0, CP0_STATUS
	or	a0, ST0_CU3
	mtc0	a0, CP0_STATUS
	nop

	/*set DMEM0 from 0x18900000~0x18901fffff*/
	lui	a0, 0x1890
	mtc3	a0, $4
	lui	a0, 0x1890
	ori	a0, 0x1fff
	mtc3	a0, $5

	/*DMEM on*/
	mtc0	zero, $20
	li	a0, 0x400
	mtc0	a0, $20
	nop

	/*set sp point to the middle of DMEM*/
	li	sp, 0xB8901000

	subu	sp, 4

	sw	ra, 0(sp)
	la	t9, DDR_Bist
	jalr	t9
	 nop
	lw	ra, 0(sp)
#endif

	jr	ra
	 nop
	.end old_test
#endif
