/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <tftf.h>

	.globl	tftf_entrypoint
	.globl	tftf_hotplug_entry


/* ----------------------------------------------------------------------------
 * Cold boot entry point for the primary CPU.
 * ----------------------------------------------------------------------------
 */
func tftf_entrypoint
	/* --------------------------------------------------------------------
	 * Save arguments x0-x3 from the previous bootloader.
	 * --------------------------------------------------------------------
	 */
	mov	x20, x0
	mov	x21, x1
	mov	x22, x2
	mov	x23, x3

	bl	arch_init

	/* --------------------------------------------------------------------
	 * Invalidate the RW memory used by TFTF image.
	 * This is done to safeguard against possible corruption of this
	 * memory by dirty cache lines in a system cache as a result of use
	 * by an earlier boot loader stage.
	 * --------------------------------------------------------------------
	 */
	adr	x0, __DATA_START__
	adr	x1, __DATA_END__
	sub	x1, x1, x0
	bl	inv_dcache_range

	/* --------------------------------------------------------------------
	 * This code is expected to be executed only by the primary CPU.
	 * Save the mpid for the first core that executes and if a secondary
	 * CPU has lost its way make it spin forever.
	 * --------------------------------------------------------------------
	 */
	bl	save_primary_mpid

	/* --------------------------------------------------------------------
	 * Zero out NOBITS sections. There are 2 of them:
	 *   - the .bss section;
	 *   - the coherent memory section.
	 * --------------------------------------------------------------------
	 */
	ldr	x0, =__BSS_START__
	ldr	x1, =__BSS_SIZE__
	bl	zeromem16

	ldr	x0, =__COHERENT_RAM_START__
	ldr	x1, =__COHERENT_RAM_UNALIGNED_SIZE__
	bl	zeromem16

	/* --------------------------------------------------------------------
	 * Give ourselves a small coherent stack to ease the pain of
	 * initializing the MMU
	 * --------------------------------------------------------------------
	 */
	mrs	x0, mpidr_el1
	bl	platform_set_coherent_stack

	bl	tftf_early_platform_setup
	bl	tftf_plat_arch_setup

	/* --------------------------------------------------------------------
	 * Give ourselves a stack allocated in Normal -IS-WBWA memory
	 * --------------------------------------------------------------------
	 */
	mrs	x0, mpidr_el1
	bl	platform_set_stack

	/* --------------------------------------------------------------------
	 * Save the fw_config or transfer list and hw_config addresses passed
	 * in registers x0 to x3 from the previous bootloader.
	 * --------------------------------------------------------------------
	 */
	mov	x0, x20
	mov	x1, x21
	mov	x2, x22
	mov	x3, x23
	bl	save_handoff_params

	/* --------------------------------------------------------------------
	 * tftf_cold_boot_main() will perform the remaining architectural and
	 * platform setup, initialise the test framework's state, then run the
	 * tests.
	 * --------------------------------------------------------------------
	 */
	b	tftf_cold_boot_main
endfunc tftf_entrypoint

/* ----------------------------------------------------------------------------
 * Entry point for a CPU that has just been powered up.
 * In : x0 - context_id
 * ----------------------------------------------------------------------------
 */
func tftf_hotplug_entry
	/* --------------------------------------------------------------------
	 * Preserve the context_id in a callee-saved register
	 * --------------------------------------------------------------------
	 */
	mov	x19, x0

	bl	arch_init

	/* --------------------------------------------------------------------
	 * Give ourselves a small coherent stack to ease the pain of
	 * initializing the MMU
	 * --------------------------------------------------------------------
	 */
	mrs	x0, mpidr_el1
	bl	platform_set_coherent_stack

	/* --------------------------------------------------------------------
	 * Enable the MMU
	 * --------------------------------------------------------------------
	 */
	bl	tftf_plat_enable_mmu

	/* --------------------------------------------------------------------
	 * Give ourselves a stack in normal memory.
	 * --------------------------------------------------------------------
	 */
	mrs	x0, mpidr_el1
	bl	platform_set_stack

	/* --------------------------------------------------------------------
	 * Save the context_id for later retrieval by tests
	 * --------------------------------------------------------------------
	 */
	mrs	x0, mpidr_el1
	mov_imm x1, MPID_MASK
	and	x0, x0, x1
	bl	platform_get_core_pos

	mov	x1, x19

	bl	tftf_set_cpu_on_ctx_id

	/* --------------------------------------------------------------------
	 * Jump to warm boot main function
	 * --------------------------------------------------------------------
	 */
	b	tftf_warm_boot_main
endfunc tftf_hotplug_entry

/* ----------------------------------------------------------------------------
 * Saves the mpid of the primary core and if the primary core
 * is already saved then it loops infinitely.
 * ----------------------------------------------------------------------------
 */
func save_primary_mpid
	adrp	x1, tftf_primary_core
	ldr	w0, [x1, :lo12:tftf_primary_core]
	mov	w2, #INVALID_MPID
	cmp	w0, w2
	b.ne	panic
	mov_imm	x2, MPID_MASK
	mrs	x0, mpidr_el1
	and	x0, x0, x2
	str	w0, [x1, :lo12:tftf_primary_core]
	ret
panic:
	/* Primary core MPID already saved */
	b	.
	ret
endfunc save_primary_mpid

/* Initialize architectural state. */
func arch_init
	mrs	x0, CurrentEL
	cmp	x0, #(MODE_EL1 << MODE_EL_SHIFT)
	b.eq	el1_setup

el2_setup:
	/* Set the exception vectors. */
	adr	x0, tftf_vector
	msr	vbar_el2, x0

	/* Enable the instruction cache and alignment checks. */
	mov_imm	x0, (SCTLR_EL2_RES1 | SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	msr	sctlr_el2, x0

	isb
	ret

el1_setup:
	/* Set the exception vectors. */
	adr	x0, tftf_vector
	msr	vbar_el1, x0

	/* Enable the instruction cache and stack pointer alignment checks. */
	mov_imm	x0, (SCTLR_EL1_RES1 | SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	msr	sctlr_el1, x0

	isb
	ret
endfunc arch_init


/* ----------------------------------------------------------------------------
 * Save fw_config or transfer list and hw_config addresses passed in registers
 * x0 to x3 from the previous bootloader.
 * ----------------------------------------------------------------------------
 */
func save_handoff_params
#if TRANSFER_LIST
	adrp	x4, ns_tl
	str	x3, [x4, :lo12:ns_tl]
	str	x1, [x4, :lo12:tl_signature]
	str	x0, [x4, :lo12:hw_config_base]
#else
	adrp	x2, fw_config_base
	str	x0, [x2, :lo12:fw_config_base]
	str	x1, [x2, :lo12:hw_config_base]
#endif
	ret
endfunc save_handoff_params
