/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <realm_rsi.h>
#include <arch_helpers.h>
#include <debug.h>
#include <drivers/arm/gic_v3.h>
#include <host_shared_data.h>
#include <tftf_lib.h>

/* Timer interrupt flag */
static volatile bool timer_irq_received;

/* Convert milliseconds to timer ticks */
static uint32_t ms_to_ticks(uint64_t ms)
{
	return ms * read_cntfrq_el0() / 1000;
}

/*
 * Test EL1 virtual timer in realm.
 * This test programs the EL1 virtual timer and waits for the interrupt.
 */
bool test_realm_el1_timer(void)
{
	u_register_t deadline_ms, wait_time_ms;
	u_register_t start_time, end_time, elapsed_ms;
	u_register_t ticks;
	u_register_t priority_bits, priority;

	/* Get timer parameters from host */
	deadline_ms = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	wait_time_ms = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);

	realm_printf("EL1 Timer test: deadline=%lums, wait=%lums\n",
		     deadline_ms, wait_time_ms);

	/* Initialize interrupt flag */
	timer_irq_received = false;

	/* Get the number of priority bits implemented */
	priority_bits = ((read_icv_ctrl_el1() >> ICV_CTLR_EL1_PRIbits_SHIFT) &
				ICV_CTLR_EL1_PRIbits_MASK) + 1UL;

	/* Unimplemented bits are RES0 and start from LSB */
	priority = (0xFFUL << (8UL - priority_bits)) & 0xFFUL;

	/* Set the priority mask register to allow all interrupts */
	write_icv_pmr_el1(priority);

	/* Enable Virtual Group 1 interrupts */
	write_icv_igrpen1_el1(ICV_IGRPEN1_EL1_Enable);

	/* Enable IRQ */
	enable_irq();

	/* Disable the EL1 virtual timer */
	write_cntv_ctl_el0(0);

	/* Program the timer with the deadline */
	ticks = ms_to_ticks(deadline_ms);
	write_cntv_tval_el0(ticks);

	/* Record start time */
	start_time = read_cntpct_el0();

	/* Enable the timer */
	write_cntv_ctl_el0(1);

	/*
	 * Wait for the timer interrupt to be received.
	 * The interrupt handler will set timer_irq_received flag.
	 * Using a busy loop like PMU test instead of WFI.
	 */
	while (!timer_irq_received && (wait_time_ms != 0U)) {
		--wait_time_ms;
		waitms(1);
	}

	/* Record end time */
	end_time = read_cntpct_el0();
	elapsed_ms = ((end_time - start_time) * 1000) / read_cntfrq_el0();

	/* Disable IRQ */
	disable_irq();

	realm_printf("Timer elapsed: %lums, interrupt received: %d\n",
		elapsed_ms, timer_irq_received);

	/* Report results to host */
	realm_shared_data_set_my_realm_val(HOST_ARG1_INDEX,
		timer_irq_received ? 1UL : 0UL);
	realm_shared_data_set_my_realm_val(HOST_ARG2_INDEX, elapsed_ms);

	return timer_irq_received;
}

/*
 * Handle EL1 virtual timer interrupt in realm.
 */
void realm_handle_timer_interrupt(void)
{
	/* Set the flag to indicate interrupt received */
	timer_irq_received = true;

	/* Disable the timer */
	write_cntv_ctl_el0(0);
}
