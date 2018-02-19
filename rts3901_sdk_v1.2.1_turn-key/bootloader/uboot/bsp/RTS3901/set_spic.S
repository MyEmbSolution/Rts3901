/* Memory sub-system initialization code */

#include <config.h>
#include <asm/regdef.h>
#include <asm/rlxregs.h>
#include "ddr_def.h"
#include "bspchip.h"

	.text
	.globl set_spic
	.set noreorder
	.ent set_spic
.set_spic:
	li v0, 0xb8030014
	li v1, 0x01
	sw v1, 0(v0)
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	b af_init_ddrc
	nop
	.end set_spic