/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 * Copyright (c) 2024, 2026, Linaro Limited.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <platform.h>

/*******************************************************************************
 * Data structure to keep track of per-cpu secure generic timer context across
 * power management operations.
 ******************************************************************************/
typedef struct timer_context {
	uint64_t cval;
	uint32_t ctl;
} timer_context_t;

static timer_context_t pcpu_timer_context[PLATFORM_CORE_COUNT];

/*******************************************************************************
 * This function initializes the generic timer to fire every `timeo` ms.
 ******************************************************************************/
void private_timer_start(unsigned long timeo)
{
	uint64_t cval, freq;
	uint32_t ctl = 0;

	/* Any previous pending timer activation will be disabled. */
	cval = read_cntpct_el0();
	freq = read_cntfrq_el0();
	cval += (freq * timeo) / 1000;
	write_cnthp_cval_el2(cval);

	/* Enable the secure physical timer */
	set_cntp_ctl_enable(ctl);
	write_cnthp_ctl_el2(ctl);
}

/*******************************************************************************
 * This function deasserts the timer interrupt prior to cpu power down
 ******************************************************************************/
void private_timer_stop(void)
{
	/* Disable the timer */
	write_cnthp_ctl_el2(0);
}

/*******************************************************************************
 * This function saves the timer context prior to cpu suspension
 ******************************************************************************/
void private_timer_save(void)
{
	uint32_t linear_id = platform_get_core_pos(read_mpidr_el1());

	pcpu_timer_context[linear_id].cval = read_cnthp_cval_el2();
	pcpu_timer_context[linear_id].ctl = read_cnthp_ctl_el2();
	flush_dcache_range((uintptr_t) &pcpu_timer_context[linear_id],
			   sizeof(pcpu_timer_context[linear_id]));
}

/*******************************************************************************
 * This function restores the timer context post cpu resummption
 ******************************************************************************/
void private_timer_restore(void)
{
	uint32_t linear_id = platform_get_core_pos(read_mpidr_el1());

	write_cnthp_cval_el2(pcpu_timer_context[linear_id].cval);
	write_cnthp_ctl_el2(pcpu_timer_context[linear_id].ctl);
}

/*
 * Program arm generic timer to fire and interrupt after time_out_ms
 * milliseconds.
 */
int arm_gen_timer_program(unsigned long time_out_ms)
{
	unsigned int cntp_ctl;
	unsigned long long count_val;
	unsigned int freq;

	count_val = read_cntpct_el0();
	freq = read_cntfrq_el0();
	count_val += (freq * time_out_ms) / 1000;

	/*
	 * Ensure that we have programmed a timer interrupt for a time in
	 * future. Else we will have to wait for the gentimer to rollover
	 * for the interrupt to fire (which is 64 years).
	 */
	if (count_val < read_cntpct_el0())
		return -1;

	write_cntp_cval_el0(count_val);

	/* Enable the timer */
	cntp_ctl = read_cntp_ctl_el0();
	set_cntp_ctl_enable(cntp_ctl);
	clr_cntp_ctl_imask(cntp_ctl);
	write_cntp_ctl_el0(cntp_ctl);

	VERBOSE("%s : interrupt requested at sys_counter: %llu "
		"time_out_ms: %ld\n", __func__, count_val, time_out_ms);

	return 0;
}

static void disable_gentimer(void)
{
	uint32_t val;

	/* Deassert and disable the timer interrupt */
	val = 0;
	set_cntp_ctl_imask(val);
	write_cntp_ctl_el0(val);
}

/*
 * Cancel the currently programmed arm generic timer interrupt
 */
int arm_gen_timer_cancel(void)
{
	disable_gentimer();
	return 0;
}

/*
 * Handler to acknowledge and de-activate the arm generic timer interrupt
 */
int arm_gen_timer_handler(void)
{
	disable_gentimer();
	return 0;
}

/*
 * Initializes the arm generic timer so that it can be used for programming
 * timer interrupt.
 */
int arm_gen_timer_init(void)
{
	/* Disable the timer as the reset value is unknown */
	disable_gentimer();

	/* Initialise CVAL to zero */
	write_cntp_cval_el0(0);

	return 0;
}


