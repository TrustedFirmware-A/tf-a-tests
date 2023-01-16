/*
 * Copyright (c) 2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <arch.h>

	.arch	armv8-a+sve
	.globl  sme2_load_zt0_instruction
	.globl  sme2_store_zt0_instruction

/*
 * TODO: Due to the limitation with toolchain, SME2 intrinsics, still not
 * being supported, manually instructions are encoded using the opcodes.
 * Further, when the toolchain supports the requirements, these macros could
 * be refactored.
 */
.macro _check_general_reg_number nr
	.if ((\nr) < 0) || ((\nr) > 30)
		.error "Bad register number \nr."
	.endif
.endm

/*
 * LDR (ZT0)  : Load ZT0 register with 64byte data.
 * Instruction: LDR ZT0, [<Xn|SP>]
 *
 * LDR ZT0, nx
 * Opcode bit field:
 * nx    : 64-bit name of the general-purpose base register
 */
.macro _ldr_zt nx
	_check_general_reg_number \nx
	.inst	0xe11f8000 | (((\nx) & 0x1f) << 5)
.endm

/*
 * STR (ZT0)  : Store the 64-byte ZT0 register to the memory address
 *              provided in the 64-bit base register or stack pointer.
 * Instruction: STR ZT0, [<Xn|SP>]
 *
 * STR ZT0, nx
 * Opcode bit field:
 * nx    : 64-bit name of the general-purpose base register
 */
.macro _str_zt nx
	.inst	0xe13f8000 | (((\nx) & 0x1f) << 5)
.endm

/*
 * void sme2_load_zt0_instruction;
 *
 * This function loads data from input buffer pointed
 * to by X0 register into the 512 bits ZT0 register.
 */
func sme2_load_zt0_instruction
	_ldr_zt	0
	ret
endfunc sme2_load_zt0_instruction

/*
 * void sme2_store_zt0_instruction;
 *
 * This function stores data from the SME2 ZT0 register
 * into the memory section pointed by the x0 register.
 * It copies 512bits of data to the inout memory buffer.
 */
func sme2_store_zt0_instruction
	_str_zt	0
	ret
endfunc sme2_store_zt0_instruction
