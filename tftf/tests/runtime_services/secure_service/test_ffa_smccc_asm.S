/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

	.global test_ffa_smc
	.global test_ffa_smc_ext

	.section	.text, "ax"

/**
 * test_ffa_smc
 *
 * x0 - ptr to a struct ffa_value8
 *
 * This function is used to test FF-A ABIs on top of SMCCCv1.2 for interfaces
 * requiring at most 8 input/output registers.
 * Load 8 GP input registers, move a pattern into x8-x29 and emit an SMC.
 * On return save 8 output registers to the structure and compare x8-x29
 * to the known pattern. If a register was altered it indicates an SMCCC
 * violation and the function returns with a value greater than 0.
 * The function returns 0 on success.
 */
func test_ffa_smc
	/* Save FP/LR. */
	stp	x29, x30, [sp, #-16]!

	/* Save x19-x28 per AAPCS64. */
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!

	/*
	 * Store the struct ffa_value pointer to x30.
	 * x30 is preserved by the SMC callee.
	 */
	mov	x30, x0

	/* Load the SMC service input values. */
	ldp	x0, x1, [x30]
	ldp	x2, x3, [x30, #16]
	ldp	x4, x5, [x30, #32]
	ldp	x6, x7, [x30, #48]
	mov	x8, #0xa8
	add	x9, x8, #1
	add	x10, x8, #2
	add	x11, x8, #3
	add	x12, x8, #4
	add	x13, x8, #5
	add	x14, x8, #6
	add	x15, x8, #7
	add	x16, x8, #8
	add	x17, x8, #9
	add	x18, x8, #10
	add	x19, x8, #11
	add	x20, x8, #12
	add	x21, x8, #13
	add	x22, x8, #14
	add	x23, x8, #15
	add	x24, x8, #16
	add	x25, x8, #17
	add	x26, x8, #18
	add	x27, x8, #19
	add	x28, x8, #20
	add	x29, x8, #21
	smc     #0

	/* Store the SMC service return values. */
	stp	x0, x1, [x30]
	stp	x2, x3, [x30, #16]
	stp	x4, x5, [x30, #32]
	stp	x6, x7, [x30, #48]

	/* Check if SMC callee-preserved registers were altered. */
	mov	w0, wzr
	cmp	x8, #0xa8
	cinc	x0, x0, ne
	cmp	x9, #0xa9
	cinc	x0, x0, ne
	cmp	x10, #0xaa
	cinc	x0, x0, ne
	cmp	x11, #0xab
	cinc	x0, x0, ne
	cmp	x12, #0xac
	cinc	x0, x0, ne
	cmp	x13, #0xad
	cinc	x0, x0, ne
	cmp	x14, #0xae
	cinc	x0, x0, ne
	cmp	x15, #0xaf
	cinc	x0, x0, ne
	cmp	x16, #0xb0
	cinc	x0, x0, ne
	cmp	x17, #0xb1
	cinc	x0, x0, ne
	cmp	x18, #0xb2
	cinc	x0, x0, ne
	cmp	x19, #0xb3
	cinc	x0, x0, ne
	cmp	x20, #0xb4
	cinc	x0, x0, ne
	cmp	x21, #0xb5
	cinc	x0, x0, ne
	cmp	x22, #0xb6
	cinc	x0, x0, ne
	cmp	x23, #0xb7
	cinc	x0, x0, ne
	cmp	x24, #0xb8
	cinc	x0, x0, ne
	cmp	x25, #0xb9
	cinc	x0, x0, ne
	cmp	x26, #0xba
	cinc	x0, x0, ne
	cmp	x27, #0xbb
	cinc	x0, x0, ne
	cmp	x28, #0xbc
	cinc	x0, x0, ne
	cmp	x29, #0xbd
	cinc	x0, x0, ne

	/* Restore x19-x28 per AAPCS64. */
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16

	/* Restore FP/LR. */
	ldp	x29, x30, [sp], #16
	ret
endfunc test_ffa_smc

/**
 * test_ffa_smc_ext
 *
 * x0 - ptr to a struct ffa_value
 *
 * This function is used to test FF-A ABIs on top of SMCCCv1.2 for interfaces
 * requiring at most 18 input/output registers.
 * Load 18 GP input registers, move a pattern into x18-x29 and emit an SMC.
 * On return save 18 output registers to the structure and compare x18-x29
 * to the known pattern. If a register was altered it indicates an SMCCC
 * violation and the function returns with a value greater than 0.
 * The function returns 0 on success.
 */
func test_ffa_smc_ext
	/* Save FP/LR. */
	stp	x29, x30, [sp, #-16]!

	/* Save x19-x28 per AAPCS64. */
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!

	/*
	 * Store the struct ffa_value_ext pointer to x30.
	 * x30 is preserved by the SMC callee.
	 */
	mov	x30, x0

	/* Load the SMC service input values. */
	ldp	x0, x1, [x30]
	ldp	x2, x3, [x30, #16]
	ldp	x4, x5, [x30, #32]
	ldp	x6, x7, [x30, #48]
	ldp	x8, x9, [x30, #64]
	ldp	x10, x11, [x30, #80]
	ldp	x12, x13, [x30, #96]
	ldp	x14, x15, [x30, #112]
	ldp	x16, x17, [x30, #128]
	mov	x18, #0xb2
	add	x19, x18, #1
	add	x20, x18, #2
	add	x21, x18, #3
	add	x22, x18, #4
	add	x23, x18, #5
	add	x24, x18, #6
	add	x25, x18, #7
	add	x26, x18, #8
	add	x27, x18, #9
	add	x28, x18, #10
	add	x29, x18, #11
	smc     #0

	/* Store the SMC service return values. */
	stp	x0, x1, [x30]
	stp	x2, x3, [x30, #16]
	stp	x4, x5, [x30, #32]
	stp	x6, x7, [x30, #48]
	stp	x8, x9, [x30, #64]
	stp	x10, x11, [x30, #80]
	stp	x12, x13, [x30, #96]
	stp	x14, x15, [x30, #112]
	stp	x16, x17, [x30, #128]

	/* Check if SMC callee-preserved registers were altered. */
	mov	w0, wzr
	cmp	x18, #0xb2
	cinc	x0, x0, ne
	cmp	x19, #0xb3
	cinc	x0, x0, ne
	cmp	x20, #0xb4
	cinc	x0, x0, ne
	cmp	x21, #0xb5
	cinc	x0, x0, ne
	cmp	x22, #0xb6
	cinc	x0, x0, ne
	cmp	x23, #0xb7
	cinc	x0, x0, ne
	cmp	x24, #0xb8
	cinc	x0, x0, ne
	cmp	x25, #0xb9
	cinc	x0, x0, ne
	cmp	x26, #0xba
	cinc	x0, x0, ne
	cmp	x27, #0xbb
	cinc	x0, x0, ne
	cmp	x28, #0xbc
	cinc	x0, x0, ne
	cmp	x29, #0xbd
	cinc	x0, x0, ne

	/* Restore x19-x28 per AAPCS64. */
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16

	/* Restore FP/LR. */
	ldp	x29, x30, [sp], #16
	ret
endfunc test_ffa_smc_ext
