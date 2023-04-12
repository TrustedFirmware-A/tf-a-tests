/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

.global	sve_subtract_arrays_interleaved
.global	sve_subtract_arrays

#if __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0)

/*
 * Based on example code from:
 * Arm Compiler Scalable Vector Extension User Guide Version 6.12 [1].
 *
 * [1] https://developer.arm.com/documentation/100891/0612/getting-started-with-the-sve-compiler/compiling-c-and-c---code-for-sve-enabled-targets
 */

/*
 * Subtracts arrays using SVE operations with interleaved callback.
 * dst_array = src_array_1 - src_array_2
 * Inputs:
 *   x0 - dst_array
 *   x1 - src_array_1
 *   x2 - src_array_2
 *   x3 - array size
 *   x4 - callback function pointer
 * Returns:
 *   Callback function's return value
 */
func sve_subtract_arrays_interleaved
.arch_extension sve
	stp	x29, x30, [sp, #-80]!
	mov	x29, sp
	stp	x19, x20, [sp, #16]
	mov	x19, x0
	mov	x20, x1
	stp	x21, x22, [sp, #32]
	mov	x21, x2
	mov	x22, x3
	stp	x23, x24, [sp, #48]
	mov	x23, x4
	mov	x24, x3
	str	x25, [sp, #64]
	mov	x25, 0

	whilelo	p0.s, xzr, x4
.loop:
	ld1w	z0.s, p0/z, [x20, x25, lsl 2]
	ld1w	z1.s, p0/z, [x21, x25, lsl 2]

	/* Invoke the world switch callback */
	blr	x23

	/* Exit loop if callback returns non-zero */
	cmp	w0, #0x0
	bne	.exit_loop

	sub	z0.s, z0.s, z1.s
	st1w	z0.s, p0, [x19, x25, lsl 2]
	incw	x25

	whilelo	p0.s, x25, x24
	bne	.loop
.exit_loop:
	ldp	x19, x20, [sp, #16]
	ldp	x21, x22, [sp, #32]
	ldp	x23, x24, [sp, #48]
	ldr	x25, [sp, #64]
	ldp	x29, x30, [sp], #80
	ret
.arch_extension nosve
endfunc sve_subtract_arrays_interleaved

/*
 * Subtracts arrays using SVE operations.
 * dst_array = src_array_1 - src_array_2
 * Inputs:
 *   x0 - dst_array
 *   x1 - src_array_1
 *   x2 - src_array_2
 *   x3 - array size
 * Returns:
 *   none
 */
func sve_subtract_arrays
.arch_extension sve
	mov	x4, x3
	mov	x5, 0
	whilelo	p0.s, xzr, x3
.sub_loop:
	ld1w	z0.s, p0/z, [x1, x5, lsl 2]
	ld1w	z1.s, p0/z, [x2, x5, lsl 2]
	sub	z0.s, z0.s, z1.s
	st1w	z0.s, p0, [x0, x5, lsl 2]
	incw	x5
	whilelo	p0.s, x5, x4
	bne	.sub_loop
	ret
.arch_extension nosve
endfunc sve_subtract_arrays

#endif /* __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0) */
