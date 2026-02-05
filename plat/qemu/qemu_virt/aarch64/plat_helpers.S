/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 * Copyright (c) 2024, Linaro Limited.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <drivers/arm/pl011.h>
#include <platform_def.h>

	.globl	platform_get_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush

/*
 * unsigned int platform_get_core_pos(unsigned long mpid)
 *  With this function:
 *  CorePos = (ClusterId * PLATFORM_CORE_COUNT_PER_CLUSTER) + CoreId
 */
func platform_get_core_pos
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #(MPIDR_AFFINITY_BITS -\
				  PLATFORM_CORES_PER_CLUSTER_SHIFT)
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
	mov_imm	x0, CRASH_CONSOLE_BASE
	mov_imm	x1, PL011_UART_CLK_IN_HZ
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
	mov_imm	x1, CRASH_CONSOLE_BASE
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
	mov_imm	x1, CRASH_CONSOLE_BASE
	b	console_core_flush
endfunc plat_crash_console_flush
