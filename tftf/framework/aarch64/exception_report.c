/*
 * Copyright (c) 2019-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdio.h>

#include <arch_helpers.h>
#include <debug.h>
#include <platform.h>
#include <utils_def.h>

/* We save x0-x30. */
#define GPREGS_CNT 31

/* Set of registers saved by the crash_dump() assembly function. */
struct cpu_context {
	u_register_t regs[GPREGS_CNT];
	u_register_t sp;
};

/*
 * Read the EL1 or EL2 version of a register, depending on the current exception
 * level.
 */
#define read_sysreg(_name)			\
	(IS_IN_EL2() ? read_##_name##_el2() : read_##_name##_el1())

void __dead2 print_exception(const struct cpu_context *ctx)
{
	u_register_t mpid = read_mpidr_el1();
	uintptr_t elr = read_sysreg(elr);
	uintptr_t lr = ctx->regs[30]; /* LR is x30 */

	/*
	 * The instruction barrier ensures we don't read stale values of system
	 * registers.
	 */
	isb();

#if ENABLE_PAUTH
	/* If PAUTH enabled then remove PAC from ELR and LR with xpaci. */
	xpaci(elr);
	xpaci(lr);
#endif /* ENABLE_PAUTH */

	printf("Unhandled exception on CPU%u.\n", platform_get_core_pos(mpid));

	/* Dump some interesting system registers. */
	printf("System registers:\n");
	printf("  MPIDR=0x%lx\n", mpid);
	printf("  ESR=0x%lx  ELR=0x%lx  FAR=0x%lx\n", read_sysreg(esr),
	       elr, read_sysreg(far));
	printf("  SCTLR=0x%lx  SPSR=0x%lx  DAIF=0x%lx\n",
	       read_sysreg(sctlr), read_sysreg(spsr), read_daif());

	/* Dump general-purpose registers. */
	printf("General-purpose registers:\n");
	for (int i = 0; i < GPREGS_CNT; ++i) {
		printf("  x%u=0x%lx\n", i, ctx->regs[i]);
	}
	printf("  LR=0x%lx\n", lr);
	printf("  SP=0x%lx\n", ctx->sp);

	while (1)
		wfi();
}
