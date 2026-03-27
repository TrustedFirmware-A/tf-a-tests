/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <debug.h>
#include <host_realm_pmu.h>

/*
 * Global flag to track PMU interrupt reception.
 * Set to 1 when PMU interrupt is received and handled.
 * Used to verify interrupt delivery in PMU plane tests.
 */
volatile bool pmu_isr_received;
/* EL1 Virtual Timer IRQ */
#define EL1_VIRT_TIMER_IRQ	27U

/* External function to handle timer interrupt */
extern void realm_handle_timer_interrupt(void);

/* Realm interrupt handler */
void realm_interrupt_handler(void)
{
	/* Read INTID and acknowledge interrupt */
	unsigned long iar1_el1 = read_icv_iar1_el1();

	/*
	 * Deactivate interrupt, this will raise maintenance irq.
	 *
	 * Assumption ICV_CTLR_EL1.EOImode is 0 which implies
	 * ICV_EOIR0_EL1 and ICV_EOIR1_EL1 provide both priority drop and
	 * interrupt deactivation functionality. Accesses to ICV_DIR_EL1 are UNPREDICTABLE.
	 */
	write_icv_eoir1_el1(iar1_el1);

	/* Clear PMU interrupt */
	if (iar1_el1 == PMU_VIRQ) {
		/* Clear PMU interrupt */
		write_pmintenclr_el1(read_pmintenset_el1());
		isb();
	} else if (iar1_el1 == EL1_VIRT_TIMER_IRQ) {
		/* Handle timer interrupt */
		realm_handle_timer_interrupt();
	} else {
		panic();
	}
}
