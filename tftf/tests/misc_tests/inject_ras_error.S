/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <sdei.h>


#ifdef __aarch64__
	.globl	inject_unrecoverable_ras_error
	.globl	inject_uncontainable_ras_error
	.globl	serror_sdei_event_handler

/*
 * Program fault injection register, and wait for ever for the fault to trigger.
 * Note that the model must be launched with fault inject support.
 *
 * x0: Fault record number to program
 * x1: Injected fault properties
 * x2: Type of error to be generated
 */
func inject_ras_error_record
	/* Choose Error record 0 on the PE */
	msr	ERRSELR_EL1, x0
	isb

	/* Enable error reporting */
	orr	x1, x1, #ERXCTLR_ED_BIT
	msr	ERXCTLR_EL1, x1

	/* Program count down timer to 1 */
	mov	x0, #1
	msr	ERXPFGCDN_EL1, x0

	/* Start count down to generate error */
	orr	x2, x2, #ERXPFGCTL_CDEN_BIT
	msr	ERXPFGCTL_EL1, x2
	isb

	ret
endfunc inject_ras_error_record

/* Inject Unrecoverable error through fault record 0. */
func inject_unrecoverable_ras_error
	/* Inject fault into record 0 */
	mov	x0, #0

	/* Enable error reporting */
	mov	x1, #ERXCTLR_UE_BIT
	msr	ERXCTLR_EL1, x1

	/* Injected fault control */
	mov	x2, #ERXPFGCTL_UEU_BIT

	b	inject_ras_error_record
endfunc inject_unrecoverable_ras_error

/*
 * Inject Uncontainable error through fault record 0. This function doesn't wait
 * as the handling is terminal in EL3.
 */
func inject_uncontainable_ras_error
	/* Inject fault into record 0 */
	mov	x0, #0

	mov	x1, xzr

	/* Injected fault control */
	mov	x2, #ERXPFGCTL_UC_BIT

	b	inject_ras_error_record
endfunc inject_uncontainable_ras_error

/*
 * SDEI event handler for SErrors.
 */
func serror_sdei_event_handler
	stp	x29, x30, [sp, #-16]!
	bl	sdei_handler
	ldp	x29, x30, [sp], #16
	mov_imm	x0, SDEI_EVENT_COMPLETE
	mov	x1, xzr
	smc	#0
	b	.
endfunc serror_sdei_event_handler
#endif
