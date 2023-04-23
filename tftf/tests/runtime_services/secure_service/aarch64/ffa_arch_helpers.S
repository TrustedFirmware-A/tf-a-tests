/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

	.macro service_call _conduit
	/*
	 * Use a callee saved register to point to ffa_value structure after
	 * returning from the conduit.
	 * Although x19 contains an 8-byte value, we are allocating 16 bytes
	 * on the stack to respect the 16-byte stack-alignment.
	 */
	str	x19, [sp, #-16]!

	/*
	 * Save pointed to ffa_value structure into x19, which is a callee saved
	 * register.
	 */
	mov	x19, x0
	/* Load the argument values into the appropriate registers. */
	ldp	x6, x7, [x0, #48]
	ldp	x4, x5, [x0, #32]
	ldp	x2, x3, [x0, #16]
	ldp	x0, x1, [x0, #0]

	\_conduit	#0

	/*
	 * The return values are stored in x0-x17, put them in the ffa_value
	 * return structure. x19 points to the ffa_value structure.
	 */
	stp	x0, x1, [x19, #0]
	stp	x2, x3, [x19, #16]
	stp	x4, x5, [x19, #32]
	stp	x6, x7, [x19, #48]
	stp	x8, x9, [x19, #64]
	stp	x10, x11, [x19, #80]
	stp	x12, x13, [x19, #96]
	stp	x14, x15, [x19, #112]
	stp	x16, x17, [x19, #128]
	ldr	x19, [sp], #16
	.endm

.globl ffa_svc
func ffa_svc
	service_call svc
	ret
endfunc ffa_svc

.globl ffa_smc
func ffa_smc
	service_call smc
	ret
endfunc ffa_smc
