/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

#if __aarch64__

	.globl	mops_cpy

/*
 * void mops_cpy(void *dst, const void *src, size_t len)
 *
 * Copy len bytes from src to dst using FEAT_MOPS CPY instructions.
 *
 * x0: dst  - destination address (modified by CPY* writeback)
 * x1: src  - source address (modified by CPY* writeback)
 * x2: len  - byte count (modified by CPY* writeback)
 *
 * Encoded as raw .inst words for compatibility with older toolchains
 * that do not recognise the FEAT_MOPS mnemonics:
 *   0x19010440  cpyfp [x0]!, [x1]!, x2!
 *   0x19410440  cpyfm [x0]!, [x1]!, x2!
 *   0x19810440  cpyfe [x0]!, [x1]!, x2!
 */
func mops_cpy
	.inst	0x19010440	/* cpyfp [x0]!, [x1]!, x2! */
	.inst	0x19410440	/* cpyfm [x0]!, [x1]!, x2! */
	.inst	0x19810440	/* cpyfe [x0]!, [x1]!, x2! */
	ret
endfunc mops_cpy

#endif /* __aarch64__ */
