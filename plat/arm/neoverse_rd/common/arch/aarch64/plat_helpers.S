/*
 * Copyright (c) 2018-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <drivers/arm/pl011.h>
#include <platform_def.h>

	.globl	platform_get_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush

/*----------------------------------------------------------------------
 * unsigned int platform_get_core_pos(unsigned long mpid)
 *
 * Function to calculate the core position on Neoverse RD platforms.
 *
 * (ClusterId * NRD_MAX_CPUS_PER_CLUSTER * NRD_MAX_PE_PER_CPU) +
 * (CPUId * NRD_MAX_PE_PER_CPU) +
 * ThreadId
 *
 * which can be simplified as:
 *
 * ((ClusterId * NRD_MAX_CPUS_PER_CLUSTER + CPUId) * NRD_MAX_PE_PER_CPU)
 * + ThreadId
 * ---------------------------------------------------------------------
 */
func platform_get_core_pos
	/*
	 * Check for MT bit in MPIDR. If not set, shift MPIDR to left to make it
	 * look as if in a multi-threaded implementation.
	 */
	tst	x0, #MPIDR_MT_MASK
	lsl	x3, x0, #MPIDR_AFFINITY_BITS
	csel	x3, x3, x0, eq

	/* Extract individual affinity fields from MPIDR */
	ubfx	x0, x3, #MPIDR_AFF0_SHIFT, #MPIDR_AFFINITY_BITS
	ubfx	x1, x3, #MPIDR_AFF1_SHIFT, #MPIDR_AFFINITY_BITS
	ubfx	x2, x3, #MPIDR_AFF2_SHIFT, #MPIDR_AFFINITY_BITS

	/* Compute linear position */
	mov	x3, #NRD_MAX_CPUS_PER_CLUSTER
	madd	x1, x2, x3, x1
	mov	x3, #NRD_MAX_PE_PER_CPU
	madd	x0, x1, x3, x0
	ret
endfunc platform_get_core_pos

/* ---------------------------------------------
 * int plat_crash_console_init(void)
 * Function to initialize the crash console
 * without a C Runtime to print crash report.
 * Clobber list : x0 - x4
 * ---------------------------------------------
 */
func plat_crash_console_init
	mov_imm	x0, PLAT_ARM_UART_BASE
	mov_imm	x1, PLAT_ARM_UART_CLK_IN_HZ
	mov_imm	x2, PL011_BAUDRATE
	b	console_core_init
endfunc plat_crash_console_init

/* ---------------------------------------------
 * int plat_crash_console_putc(int c)
 * Function to print a character on the crash
 * console without a C Runtime.
 * Clobber list : x1, x2
 * ---------------------------------------------
 */
func plat_crash_console_putc
	mov_imm	x1, PLAT_ARM_UART_BASE
	b	console_core_putc
endfunc plat_crash_console_putc

/* ---------------------------------------------
 * int plat_crash_console_flush()
 * Function to force a write of all buffered
 * data that hasn't been output.
 * Out : return -1 on error else return 0.
 * Clobber list : r0 - r1
 * ---------------------------------------------
 */
func plat_crash_console_flush
	mov_imm	x1, PLAT_ARM_UART_BASE
	b	console_core_flush
endfunc plat_crash_console_flush
