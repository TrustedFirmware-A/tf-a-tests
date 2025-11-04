/*
 * Copyright (c) 2018-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/arm/gic_v5.h>
#include <drivers/arm/system_timer.h>
#include <platform.h>
#include <stddef.h>
#include <timer.h>

#pragma weak plat_initialise_timer_ops

static plat_timer_t plat_timers = {
	.program = program_systimer,
	.cancel = cancel_systimer,
	.handler = handler_systimer,
	.timer_step_value = 2,
};

int plat_initialise_timer_ops(const plat_timer_t **timer_ops)
{
	assert(timer_ops != NULL);
	*timer_ops = &plat_timers;

	/*
	 * on GICv2/3 platforms give the INTID verbatim (eg SPI 60, ie. INTID
	 * 92) although can be a PPI too
	 * on GICv5 take the same SPI ID but re-use the GICv3 macro for
	 * compatibility.
	 * TODO: currently unclear how non-fvp platforms will use this; refactor
	 * to work elsewhere.
	 */
	if (arm_gic_get_version() != 5) {
		plat_timers.timer_irq = IRQ_CNTPSIRQ1;
	} else {
		plat_timers.timer_irq = (IRQ_CNTPSIRQ1 - 32) | INPLACE(INT_TYPE, INT_SPI);
	}

	/* Initialise the system timer */
	init_systimer(SYS_CNT_BASE1);

	return 0;
}
