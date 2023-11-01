/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <realm_def.h>

	.globl	realm_entrypoint

.section .bss.stacks
.align 16
	.fill	REALM_STACK_SIZE * MAX_REC_COUNT
stacks_end:

func realm_entrypoint
	/* Save x0 - context_id */
	mov	x20, x0
	mrs	x0, mpidr_el1
	mov_imm	x1, MPID_MASK
	and	x0, x0, x1

	/* Setup the stack pointer. */
	bl	realm_setup_my_stack

	/* mpidr 0 is assumed to be primary CPU, jump to warmboot otherwise */
	cbnz	x0, realm_warmboot_endpoint

	/* Primary CPU Only */
	/* Clear BSS */
	ldr	x0, =__REALM_BSS_START__
	adr	x1, realm_entrypoint
	add	x0, x1, x0
	ldr	x1, =__REALM_BSS_SIZE__
	bl	zeromem16

	/*
	 * Invalidate the data cache for the whole Realm.
	 * This prevents re-use of stale data cache entries from
	 * prior bootloader stages.
	 */
	adrp	x0, __REALM_TEXT_START__
	add	x0, x0, realm_entrypoint
	adrp	x1, __REALM_BSS_END__
	add	x1, x1, realm_entrypoint
	sub	x1, x1, x0
	bl	inv_dcache_range

	/* Relocate symbols */
pie_fixup:
	ldr	x0, =pie_fixup
	and	x0, x0, #~(PAGE_ALIGNMENT - 1)
	mov	x1, REALM_MAX_LOAD_IMG_SIZE
	add	x1, x1, x0
	bl	fixup_gdt_reloc

	/* Initialize architectural state. */
	bl	arch_init
#if ENABLE_PAUTH
	bl	pauth_init_enable
#endif

loop:
	/* And jump to the C entrypoint. */
	bl	realm_payload_main
	b	loop

realm_warmboot_endpoint:
	/* Initialize architectural state. */
	bl	arch_init
#if ENABLE_PAUTH
	bl	pauth_init_enable
#endif
	mov	x0, x20
	b	realm_secondary_entrypoint
endfunc realm_entrypoint

/*
 * Setup the stack pointer.
 * x0 = mpidr
 * clobbers x1,x2
 */
func realm_setup_my_stack
	adr	x1, stacks_end
	mov	x2, REALM_STACK_SIZE
	mul	x2, x0, x2
	sub	sp, x1, x2
	ret
endfunc	realm_setup_my_stack

/* Initialize architectural state. */
func arch_init
	/* Set the exception vectors. */
	adr	x0, realm_vector
	add	x1, x1, :lo12:realm_vector
	msr	vbar_el1, x0
	isb

	/* Enable the instruction cache and stack pointer alignment checks. */
	mov_imm	x0, (SCTLR_EL1_RES1 | SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	msr	sctlr_el1, x0

	/*
	 * Set CPACR_EL1.FPEN=11 no EL1/0 trapping of
	 * SVE/Adv. SIMD/FP instructions.
	 */
	mov	x1, CPACR_EL1_FPEN(CPACR_EL1_FP_TRAP_NONE)
	mrs	x0, cpacr_el1
	orr	x0, x0, x1
	mov	x1, CPACR_EL1_ZEN(CPACR_EL1_ZEN_TRAP_NONE)
	orr	x0, x0, x1
	mov	x1, CPACR_EL1_SMEN(CPACR_EL1_SMEN_TRAP_NONE)
	orr	x0, x0, x1
	msr	cpacr_el1, x0
	isb

	ret
endfunc arch_init
