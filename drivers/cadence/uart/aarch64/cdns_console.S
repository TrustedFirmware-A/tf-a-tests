/*
 * Copyright (c) 2016-2020, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <drivers/console.h>
#include <drivers/cadence/cdns_uart.h>
#include <platform_def.h>

	/*
	 * "core" functions are low-level implementations that don't require
	 * writable memory and are thus safe to call in BL1 crash context.
	 */
	.globl console_core_init
	.globl console_core_putc
	.globl console_core_getc
	.globl console_core_flush

	.globl  console_init
	.globl  console_putc
	.globl  console_getc
	.globl  console_flush

        /*
         *  The console base is in the data section and not in .bss
         *  even though it is zero-init. In particular, this allows
         *  the console functions to start using this variable before
         *  the runtime memory is initialized for images which do not
         *  need to copy the .data section from ROM to RAM.
         */
        .section .data.console_base
        .align 3
console_base: .quad 0x0


func console_init
	adrp x3, console_base
	str	x0, [x3, :lo12:console_base]
	b	console_core_init
endfunc console_init

	/* -----------------------------------------------
	 * int console_core_init(uintptr_t base_addr)
	 * Function to initialize the console without a
	 * C Runtime to print debug information. This
	 * function will be accessed by console_init and
	 * crash reporting.
	 * We assume that the bootloader already set up
	 * the HW (baud, ...) and only enable the trans-
	 * mitter and receiver here.
	 * In: x0 - console base address
	 * Out: return 1 on success else 0 on error
	 * Clobber list : x1, x2, x3
	 * -----------------------------------------------
	 */
func console_core_init
	/* Check the input base address */
	cbz	x0, core_init_fail

	/* RX/TX enabled & reset */
	mov	w3, #(R_UART_CR_TX_EN | R_UART_CR_RX_EN | R_UART_CR_TXRST | R_UART_CR_RXRST)
	str	w3, [x0, #R_UART_CR]

	mov	w0, #1
	ret
core_init_fail:
	mov	w0, wzr
	ret
endfunc console_core_init


	/* --------------------------------------------------------
	 * int console_cdns_core_putc(int c, uintptr_t base_addr)
	 * Function to output a character over the console. It
	 * returns the character printed on success or -1 on error.
	 * In : w0 - character to be printed
	 *      x1 - console base address
	 * Out : return -1 on error else return character.
	 * Clobber list : x2
	 * --------------------------------------------------------
	 */
func console_core_putc
	cbz	x1, putc_error
	/* Prepend '\r' to '\n' */
	cmp	w0, #0xA
	b.ne	2f
1:
	/* Check if the transmit FIFO is empty */
	ldr	w2, [x1, #R_UART_SR]
	tbz	w2, #UART_SR_INTR_TEMPTY_BIT, 1b
	mov	w2, #0xD
	str	w2, [x1, #R_UART_TX]
2:
	/* Check if the transmit FIFO is empty */
	ldr	w2, [x1, #R_UART_SR]
	tbz	w2, #UART_SR_INTR_TEMPTY_BIT, 2b
	str	w0, [x1, #R_UART_TX]
	ret
putc_error:
	mov	w0, #ERROR_NO_VALID_CONSOLE
	ret
endfunc console_core_putc

	/* --------------------------------------------------------
	 * int console_cdns_putc(int c, console_t *cdns)
	 * Function to output a character over the console. It
	 * returns the character printed on success or -1 on error.
	 * In : w0 - character to be printed
	 *      x1 - pointer to console_t structure
	 * Out : return -1 on error else return character.
	 * Clobber list : x2
	 * --------------------------------------------------------
	 */
func console_putc
	adrp x1, console_base
	ldr	x1, [x1, :lo12:console_base]
	b	console_core_putc
endfunc console_putc

	/* ---------------------------------------------
	 * int console_cdns_core_getc(uintptr_t base_addr)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 if no character is available.
	 * In : x0 - console base address
	 * Out: w0 - character if available, else -1
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_core_getc
	adr	x0, console_base
	ldr	x0, [x0]

	/* Check if the receive FIFO is empty */
	ldr	w1, [x0, #R_UART_SR]
	tbnz	w1, #UART_SR_INTR_REMPTY_BIT, no_char
	ldr	w1, [x0, #R_UART_RX]
	mov	w0, w1
	ret
no_char:
	mov	w0, #ERROR_NO_PENDING_CHAR
	ret
endfunc console_core_getc

	/* ---------------------------------------------
	 * int console_cdns_getc(console_t *console)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 if no character is available.
	 * In : x0 - pointer to console_t structure
	 * Out: w0 - character if available, else -1
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_getc
	adrp	x0, console_base
	ldr	x0, [x0, :lo12:console_base]
	b	console_core_getc
endfunc console_getc

	/* ---------------------------------------------
	 * int console_cdns_core_flush(uintptr_t base_addr)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * In : x0 - console base address
	 * Out : void
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_core_flush
	cbz	x0, flush_error
	/* Loop until the transmit FIFO is empty */
1:
	ldr     w2, [x1, #R_UART_SR]
	tbz     w2, #UART_SR_INTR_TEMPTY_BIT, 1b
	str     w0, [x1, #R_UART_TX]
	ret
flush_error:
	mov     w0, #ERROR_NO_VALID_CONSOLE
	ret
endfunc console_core_flush

	/* ---------------------------------------------
	 * void console_cdns_flush(console_t *console)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * In : x0 - pointer to console_t structure
	 * Out : void.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_flush
	adrp	x0, console_base
	ldr	x0, [x0, :lo12:console_base]
	b	console_core_flush
endfunc console_flush
