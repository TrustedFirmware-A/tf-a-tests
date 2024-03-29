/*
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <platform_def.h>
#include <drivers/console.h>

	.globl	platform_get_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush

/*----------------------------------------------------------------------
 * unsigned int platform_get_core_pos(unsigned long mpid)
 *
 * Function to calculate the core position.
 * Return 0 to 3 as logical CPU ID.
*/
func platform_get_core_pos
	lsr	x1, x0, #MPIDR_AFF0_SHIFT
	and	x1, x1, #MPIDR_AFFLVL_MASK /* core id */
	lsr	x2, x0, #MPIDR_AFF1_SHIFT
	and	x2, x2, #MPIDR_AFFLVL_MASK /* cluster id */

	/* core_id > PLATFORM_CORES_CLUSTER */
	mov	x0, #-1
	cmp	x1, #(PLATFORM_CORES_PER_CLUSTER - 1)
	b.hi	1f

	/* cluster_id > PLATFORM_CLUSTER_COUNT */
	cmp	x2, #(PLATFORM_CLUSTER_COUNT - 1)
	b.hi	1f

	/* CorePos = CoreId + (ClusterId * cpus per cluster) */
	mov	x3, #PLATFORM_CORES_PER_CLUSTER
	mul	x3, x2, x3
	add	x0, x1, x3

1:
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
	mov_imm	x0, ZYNQMP_UART_BASE
	mov_imm	x1, ZYNQMP_CRASH_UART_CLK_IN_HZ
	mov_imm	x2, ZYNQMP_UART_BAUDRATE
	b	console_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm	x1, ZYNQMP_UART_BASE
	b	console_putc
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
	mov_imm	x1, ZYNQMP_UART_BASE
	b	console_flush
endfunc plat_crash_console_flush
