#include <config.h>
#include <asm/regdef.h>
#include <asm/rlxregs.h>
#include "bspchip.h"

	.text
	.set noreorder

	.globl spic_ctrl
	.ent spic_ctrl
spic_ctrl:
	/**Push the registers that we used in this function to stack to protect
	 * the registers. */
	subu	sp, 20
	sw	t0, 0(sp)
	sw	t1, 4(sp)
	sw	t2, 8(sp)
	sw	t3, 12(sp)
	sw	t8, 16(sp)
	/**copy some code to ddr before relocate,
	 * for speed up reading code from spi flash
	 * the code is defined in symbal "set_spic".*/
	li	t0, SPICCTRL_DDR_ADDR_BASE
	li	t1, SPICCTRL_DDR_ADDR_END
1:	CACHE	0x10, 0(t0)
	addu	t0, 4
	bne	t0, t1, 1b
	 nop
	nop

	la	t8 , af_init_ddrc
	la	t0, set_spic
	li	t1, SPICCTRL_DDR_ADDR_BASE
	move	t2, t0
	addu	t2, 0xa0
1:
	lw	t3, 0(t0)
	sw	t3, 0(t1)
	addu	t0, 4
	blt	t0, t2, 1b
	addu	t1, 4

	li	t0, SPICCTRL_DDR_ADDR_BASE
	li	t1, SPICCTRL_DDR_ADDR_END
1:	CACHE	0x19, 0(t0)
	addu	t0, 4
	bne	t0, t1, 1b
	 nop
	nop

	li	t9, SPICCTRL_DDR_ADDR_BASE
	jr	t9
	 nop

af_init_ddrc:
	/* Pop the register from stack to restore there values */
	lw	t0, 0(sp)
	lw	t1, 4(sp)
	lw	t2, 8(sp)
	lw	t3, 12(sp)
	lw	t8, 16(sp)
	addu	sp, 20

	jr	ra
	 nop
	.end spic_ctrl

set_spic:
	/**XB1 120M PLL0/5 div2
	 * ew 0xb8861000 0x80
	 * ew 0xb8861018 0x126
	 * ew 0xb8861000 0x00 */
	li	v0, CLK_CHANGE
	li	v1, BUS_CLK_CHANGE
	sw	v1, 0(v0)

	li	v0, BUS_CLK_CFG_REG
	li	v1, (CLK_ENABLE | CLOCK_SELECT_PLL0_5 | CLOCK_SELECT_DIV2)
	sw	v1, 0(v0)

	li	v0, CLK_CHANGE
	li	v1, 0x0
	sw	v1, 0(v0)

	/**set spic baudrate to 1,
	 * the spi clock is: XB1_bus_clock/(baudrate * 2) */
	li	v0, SPIC_VA
	li	v1, SPIC_BAUD_VALUE
	sw	v1, SPIC_BAUDR(v0)

	/**Because SPIC do not drive spi bus during dummy cycle, we should use
	 * RD_DUAL_O rather than RD_DUAL_IO */
	/*use RD_DUAL_O auto read for speed up*/
	/**set RD_DUAL_O auto read command, 0x3b
	 * all spi nor flash support it*/
	li	v1, RD_DUAL_O_CMD
	sw	v1, SPIC_READ_DUAL_DATA(v0)

	/*set RD_DUAL_O read dummy cycle*/
	li	v1, SPIC_AUTO_LENGTH_VALUE
	sw	v1, SPIC_AUTO_LENGTH(v0)

	/*set RD_DUAL_O auto read valid*/
	li	v1, SPIC_VALID_CMD_VALUE
	sw	v1, SPIC_VALID_CMD(v0)

	jr	t8
	 nop
	nop
	nop
	nop
	nop
	nop
	nop
