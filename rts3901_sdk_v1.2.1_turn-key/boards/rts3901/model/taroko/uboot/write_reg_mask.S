/* Memory sub-system initialization code */

#include <config.h>
#include <asm/regdef.h>

	.text
	.set noreorder
	.globl write_reg_mask
	.ent write_reg_mask
/************************************************************************
 *  write_reg_mask
 ************************************************************************/
/*writereg_mask(addr, mask, value)
ew $tmp1 = @@$addr
ew $tmp1 = @$tmp1 & @$mask
ew $value = @$tmp1 | (@$value & (0xffffffff - @$mask))
ew @$addr = @$value*/


write_reg_mask:
	lw t0, 0(a0)
	and t0, a1
	li t1, 0xffffffff
	sub t1, t1, a1
	and t1, a2
	or t0, t1
	sw t0, 0(a0)
	jr ra
	nop
	.end write_reg_mask
