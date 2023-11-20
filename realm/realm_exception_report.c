/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <debug.h>

/* We save x0-x30. */
#define GPRS_CNT 31U

/* Set of registers saved by the crash_dump() assembly function in stack. */
struct rec_regs {
	unsigned long gprs[GPRS_CNT];
	unsigned long sp;
};

void __dead2 realm_print_exception(const struct rec_regs *ctx)
{
	u_register_t mpid;

	/*
	 * The instruction barrier ensures we don't read stale values of system
	 * registers.
	 */
	isb();

	mpid = read_mpidr_el1();
	realm_printf("Unhandled exception on REC%u.\n", mpid & MPID_MASK);

	/* Dump some interesting system registers. */
	realm_printf("System registers:\n");
	realm_printf("  MPIDR=0x%lx\n", mpid);
	realm_printf("  ESR=0x%lx  ELR=0x%lx  FAR=0x%lx\n", read_esr_el1(),
		     read_elr_el1(), read_far_el1());
	realm_printf("  SCTLR=0x%lx  SPSR=0x%lx  DAIF=0x%lx\n",
		     read_sctlr_el1(), read_spsr_el1(), read_daif());

	/* Dump general-purpose registers. */
	realm_printf("General-purpose registers:\n");
	for (unsigned int i = 0U; i < GPRS_CNT; i++) {
		realm_printf("  x%u=0x%lx\n", i, ctx->gprs[i]);
	}
	realm_printf("  SP=0x%lx\n", ctx->sp);

	while (1) {
		wfi();
	}
}
