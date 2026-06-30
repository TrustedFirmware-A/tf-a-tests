/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

.extern	num_ctx_regs
.extern	ctx_registers

.globl	set_ns_ep_context
.globl	get_ns_ep_context

.globl	atomic_add
.globl	atomic_read

func set_ns_ep_context
	adrp	x2, ctx_registers
	add	x2, x2, :lo12:ctx_registers
	adrp	x3, num_ctx_regs
	ldr	x3, [x3, :lo12:num_ctx_regs]
	mul	x1, x0, x3
	lsl	x1, x1, #3
	add	x1, x1, x2

	stp	x2, x3, [x1, #0]
	stp	x4, x5, [x1, #16]
	stp	x6, x7, [x1, #32]
	stp	x8, x9, [x1, #48]
	stp	x10, x11, [x1, #64]
	stp	x12, x13, [x1, #80]
	stp	x14, x15, [x1, #96]
	stp	x16, x17, [x1, #112]
	stp	x18, x19, [x1, #128]
	stp	x20, x21, [x1, #144]
	stp	x22, x23, [x1, #160]
	stp	x24, x25, [x1, #176]
	stp	x26, x27, [x1, #192]
	stp	x28, x29, [x1, #208]
	mov	x4, sp
	stp	x30, x4, [x1, #224]
	mov	x0, #0

	ret
endfunc set_ns_ep_context

func get_ns_ep_context
	adrp	x2, ctx_registers
	add	x2, x2, :lo12:ctx_registers
	adrp	x3, num_ctx_regs
	ldr	x3, [x3, :lo12:num_ctx_regs]
	mul	x1, x0, x3
	lsl	x1, x1, #3
	add	x1, x1, x2

	ldp	x2, x3, [x1, #0]
	ldp	x6, x7, [x1, #32]
	ldp	x8, x9, [x1, #48]
	ldp	x10, x11, [x1, #64]
	ldp	x12, x13, [x1, #80]
	ldp	x14, x15, [x1, #96]
	ldp	x16, x17, [x1, #112]
	ldp	x18, x19, [x1, #128]
	ldp	x20, x21, [x1, #144]
	ldp	x22, x23, [x1, #160]
	ldp	x24, x25, [x1, #176]
	ldp	x26, x27, [x1, #192]
	ldp	x28, x29, [x1, #208]
	ldp	x30, x4, [x1, #224]
	mov	sp, x4
	ldp	x4, x5, [x1, #16]
	mov	x0, #1
	ret
endfunc get_ns_ep_context

/*
 * x0 : pointer to the integer to add to
 * x1 : value to add
 */
func atomic_add
	mov	x2, x1		/* Move the value to add into x2 */

retry:
	ldxr	w3, [x0]	/* Load current value to w3 */
	add	w3, w3, w2	/* Add the value in x2 to w3 */
	stxr	w4, w3, [x0]	/* Try to store new value. */
	cbnz	w4, retry	/* If the store failed, retry. */

	ret
endfunc atomic_add

func atomic_read
	ldaxr	w0, [x0]
	ret
endfunc atomic_read
