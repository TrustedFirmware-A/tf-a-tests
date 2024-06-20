/*
 * Copyright (c) 2024, Linaro Limited.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/arm/private_timer.h>
#include <platform.h>
#include <stddef.h>
#include <timer.h>

static const plat_timer_t plat_timers = {
	.program = arm_gen_timer_program,
	.cancel = arm_gen_timer_cancel,
	.handler = arm_gen_timer_handler,
	.timer_step_value = 2,
	.timer_irq = IRQ_PCPU_EL1_TIMER
};

int plat_initialise_timer_ops(const plat_timer_t **timer_ops)
{
	assert(timer_ops != NULL);
	*timer_ops = &plat_timers;

	/* Initialise the generic timer */
	arm_gen_timer_init();

	return 0;
}

