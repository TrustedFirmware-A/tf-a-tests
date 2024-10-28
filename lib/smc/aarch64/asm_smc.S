/*
 * Copyright (c) 2013-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

	.section        .text, "ax"

	.macro smccc_conduit _conduit

	/*
	 * According to the AAPCS64, x8 is the indirect result location
	 * register. It contains the address of the memory block that the caller
	 * has reserved to hold the result, i.e. the smc_ret_values structure
	 * in our case.
	 * x8 might be clobbered across the SMC call so save it on the stack.
	 * Although x8 contains an 8 byte value, we are allocating 16bytes on the stack
	 * to respect 16byte stack-alignment.
	 */
	str	x8, [sp, #-16]!

	/* "Conduit" arguments are already stored in x0-x7 */
	\_conduit	#0

	/* Pop x8 into a caller-saved register */
	ldr	x9, [sp], #16

	/*
	 * Return values are stored in x0-x7, put them in the 'smc_ret_values'
	 * return structure
	 */
	stp	x0, x1, [x9, #0]
	stp	x2, x3, [x9, #16]
	stp	x4, x5, [x9, #32]
	stp	x6, x7, [x9, #48]

	.endm

	.macro smccc_conduit_nox8 _conduit

	/*
	 * Store address pointing at the smc_ret_values structure in the stack
	 */
	str	x1, [sp, #-16]!

	/*
	 * Store arguments in x0..x17. Start from highest registers so address
	 * pointed by x0 is preserved until it is no longer needed.
	 */
	ldp	x16, x17, [x0, #128]
	ldp	x14, x15, [x0, #112]
	ldp	x12, x13, [x0, #96]
	ldp	x10, x11, [x0, #80]
	ldp	x8, x9, [x0, #64]
	ldp	x6, x7, [x0, #48]
	ldp	x4, x5, [x0, #32]
	ldp	x2, x3, [x0, #16]
	ldp	x0, x1, [x0, #0]

	/* "Conduit" arguments are already stored in x0..x17 */
	\_conduit	#0

	/*
	 * Store value received in x0 as x0 will be used to compute addresses
	 * to store the results.
	 */
	str	x0, [sp, #-16]!

	/* Load address of smc_ret_values structure into x0 */
	ldr	x0, [sp, #16]

	/* Store values x1..x17 in the smc_ret_values structure */
	stp	x16, x17, [x0, #128]
	stp	x14, x15, [x0, #112]
	stp	x12, x13, [x0, #96]
	stp	x10, x11, [x0, #80]
	stp	x8, x9, [x0, #64]
	stp	x6, x7, [x0, #48]
	stp	x4, x5, [x0, #32]
	stp	x2, x3, [x0, #16]
	str	x1, [x0, #8]

	/*
	 * Load previously stored value of x0 into x1 and store it in the
	 * smc_ret_values structure. Return sp to its original position.
	 */
	ldr	x1, [sp], #32
	str	x1, [x0, #0]

	.endm

/* ---------------------------------------------------------------------------
 * smc_ret_values asm_tftf_smc64(uint32_t fid,
 *				 u_register_t arg1,
 *				 u_register_t arg2,
 *				 u_register_t arg3,
 *				 u_register_t arg4,
 *				 u_register_t arg5,
 *				 u_register_t arg6,
 *				 u_register_t arg7);
 * ---------------------------------------------------------------------------
 */
	.globl	asm_tftf_smc64

func asm_tftf_smc64
	smccc_conduit smc
	ret
endfunc asm_tftf_smc64

	.globl asm_tftf_smc64_no_retval_x8

/* ---------------------------------------------------------------------------
 * void asm_tftf_smc64_no_retval_x8(smc_args *args, smc_ret_values *ret);
 * ---------------------------------------------------------------------------
 */
func asm_tftf_smc64_no_retval_x8
	smccc_conduit_nox8 smc
	ret
endfunc asm_tftf_smc64_no_retval_x8

/* ---------------------------------------------------------------------------
 * hvc_ret_values asm_tftf_hvcc64(uint32_t fid,
 *				 u_register_t arg1,
 *				 u_register_t arg2,
 *				 u_register_t arg3,
 *				 u_register_t arg4,
 *				 u_register_t arg5,
 *				 u_register_t arg6,
 *				 u_register_t arg7);
 * ---------------------------------------------------------------------------
 */
	.globl	asm_tftf_hvc64

func asm_tftf_hvc64
	smccc_conduit hvc
	ret
endfunc asm_tftf_hvc64
