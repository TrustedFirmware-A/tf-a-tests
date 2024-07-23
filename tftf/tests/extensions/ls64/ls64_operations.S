/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

#if __aarch64__

        .arch armv8.7-a
	.globl	ls64_store
	.globl	ls64_load

/*
 * Function to store 64 bytes of data from consecutive registers into a memory
 * location in single-copy atomic operation via st64b instruction.
 *
 * x0:     Holds the base address of the input array of 8 64-bit integers.
 * x1:     Holds the base address of the destination/output array of 8 64-bit
 *         integers, where st64b does the single-copy atomic 64-byte store.
 * x8-x15: Consecutive registers loaded with input array.
 *
 */
func ls64_store
	ldp	x8, x9, [x0, #0]	/* x0: Base address of Input Array */
	ldp	x10, x11, [x0, #16]
	ldp	x12, x13, [x0, #32]
	ldp	x14, x15, [x0, #48]
	st64b	x8, [x1]		/* x1: Address where 64-byte data to be stored */
	ret
endfunc ls64_store

/*
 * Function to load 64-byte of data from a memory location to eight consecutive
 *  64-bit registers in single-copy atomic operation via ld64b instruction.
 *
 * x0: Holds the address of memory from where 64-byte of data to be loaded.
 * x1: Holds the base address of the destination/output array of 8 64-bit integers.
 * x9-x16: consecutive registers into which data will be copied with ld64b inst.
 */

func ls64_load
	ld64b	x4, [x0]
	stp	x4, x5, [x1, #0]	/* Base address of destination buffer */
	stp	x6, x7, [x1, #16]
	stp	x8, x9, [x1, #32]
	stp	x10, x11, [x1, #48]
	ret
endfunc ls64_load

#endif /* __aarch64__ */
