/*
 * Copyright (c) 2021-2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>

	.arch	armv8-a+sve
	.globl	sme_rdvl_1
	.globl	sme_try_illegal_instruction
	.globl  sme_vector_to_ZA
	.globl  sme_ZA_to_vector


/*
 * TODO: Due to the limitation with toolchain, SME intrinsics, still not being
 * supported, instructions are manually encoded using the opcodes.
 * Further, when the toolchain supports the requirements, these macros could
 * be refactored.
 */


/*
 * LDR (Loads a vector (an array of elements ) to ZA array ):
 * LDR ZA[\nw, #\offset], [X\nxbase, #\offset, MUL VL]
 *
 * Arguments/Opcode bit field:
 * nw     : the vector select register W12-W15
 * nxbase : 64-bit name of the general-purpose base register.
 * offset : vector select  and optional memory offset. Default to 0.
 */
.macro _ldr_za nw, nxbase, offset=0
	.inst	0xe1000000			\
		| (((\nw) & 3) << 13)		\
		| ((\nxbase) << 5)		\
		| ((\offset) & 0xf)
.endm

/*
 * STR ( It stores an array of elements from ZA array to a vector ).
 * STR ZA[\nw, #\offset], [X\nxbase, #\offset, MUL VL]
 *
 * Arguments/Opcode bit field:
 * nw     : the vector select register W12-W15
 * nxbase : 64-bit name of the general-purpose base register.
 * offset : vector select  and optional memory offset. Default to 0.
 */
.macro _str_za nw, nxbase, offset=0
	.inst	0xe1200000			\
		| (((\nw) & 3) << 13)		\
		| ((\nxbase) << 5)		\
		| ((\offset) & 0xf)
.endm

/*
 * uint64_t sme_rdvl_1(void);
 *
 * Run rdvl instruction with imm #1.
 */
func sme_rdvl_1
	rdvl	x0, #1
	ret
endfunc sme_rdvl_1

/*
 * void sme_try_illegal_instruction(void);
 *
 * This function tests that illegal instructions are allowed to run when
 * FA64 is supported. RDFFR is explicitly stated to be illegal in the SME
 * specification section F1.1.2 unless FA64 is supported and enabled.
 */
func sme_try_illegal_instruction
	rdffr	p0.b
	ret
endfunc sme_try_illegal_instruction


/**
 * void sme_vector_to_ZA(uint64_t *input_vec)
 *
 * This function loads an vector of elements to an ZA Array storage
 */
func sme_vector_to_ZA
	mov	w12, wzr
	_ldr_za	12, 0		// ZA.H[W12] loaded from [X0 / input_vector]
	ret
endfunc sme_vector_to_ZA

/**
 * void sme_ZA_to_vector(uint64_t *out_vec)
 *
 * This function stores elements from ZA Array storage to an ZA vector
 */
func sme_ZA_to_vector
	mov	w12, wzr
	_str_za	12, 0		// ZA.H[W12] stored to [X0 / out_vector]
	ret
endfunc sme_ZA_to_vector
