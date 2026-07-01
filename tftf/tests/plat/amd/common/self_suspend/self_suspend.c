/*
 * Copyright (c) 2025-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <events.h>
#include <plat_topology.h>
#include <platform.h>
#include <power_management.h>
#include <tftf_lib.h>
#include <timer.h>

#include <platform_def.h>

/* Variable used to confirm the CPU is woken up by the Timer IRQ */
static volatile int requested_irq_received[PLATFORM_CORE_COUNT];

static event_t event_received_wake_irq[PLATFORM_CORE_COUNT];

static int requested_irq_handler(void *data)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

	assert(*(unsigned int *)data == tftf_get_timer_irq());
	assert(requested_irq_received[core_pos] == 0);

	requested_irq_received[core_pos] = 1;

	return 0;
}

static test_result_t suspend_non_lead_cpu(void)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());
	uint32_t power_state;
	u_register_t flags;
	int rc;

	/* Record the wake interrupt through the timer handler */
	tftf_timer_register_handler(requested_irq_handler);

	/* IRQs need to be disabled prior to programming the timer */
	flags = read_daif();
	disable_irq();

	/* Arm the wake-up timer; on failure restore state and bail out */
	rc = tftf_program_timer(PLAT_SUSPEND_ENTRY_TIME);
	if (rc != 0) {
		write_daif(flags);
		isb();
		tftf_timer_unregister_handler();
		tftf_send_event(&event_received_wake_irq[core_pos]);
		return TEST_RESULT_FAIL;
	}

	/* Suspend to power-down at affinity level 0 until the timer IRQ fires */
	power_state = tftf_make_psci_pstate(PSTATE_AFF_LVL_0,
					    PSTATE_TYPE_POWERDOWN, 0);
	rc = tftf_cpu_suspend(power_state);

	/* Restore previous DAIF flags */
	write_daif(flags);
	isb();

	/* Wait until the IRQ wake interrupt is received */
	while (!requested_irq_received[core_pos]) {
		continue;
	}

	/* Clean up and signal the lead core that this core has resumed */
	tftf_timer_unregister_handler();
	tftf_send_event(&event_received_wake_irq[core_pos]);

	/* Report the outcome of the suspend call */
	if (rc != PSCI_E_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Suspend a non-lead CPU to power-down at affinity level 0 and
 * resume it via a timer interrupt
 *
 * Power on a non-lead core and have it suspend to power-down at affinity
 * level 0, then wake it with a timer interrupt. The lead core stays running
 * so the cluster does not enter its fully-idle state, allowing the suspended
 * core to resume.
 *
 * This test needs 2 CPUs to run. It will be skipped on a single core platform.
 */
test_result_t test_psci_suspend_powerdown_level0_secondary(void)
{
	unsigned int lead_mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int target_mpid, core_pos;
	int rc;

	/* Pick a non-lead core to suspend; skip if the platform has none */
	target_mpid = tftf_find_any_cpu_other_than(lead_mpid);
	if (target_mpid == INVALID_MPID) {
		return TEST_RESULT_SKIPPED;
	}

	/* Initialise the target core's wake-up state before powering it on */
	core_pos = platform_get_core_pos(target_mpid);
	tftf_init_event(&event_received_wake_irq[core_pos]);
	requested_irq_received[core_pos] = 0;
	dmbsy();

	/* Power on the target core to run the suspend routine */
	rc = tftf_cpu_on(target_mpid, (uintptr_t)suspend_non_lead_cpu, 0);
	if (rc != PSCI_E_SUCCESS) {
		tftf_testcase_printf("Failed to power on CPU 0x%x (%d)\n",
				     target_mpid, rc);
		return TEST_RESULT_FAIL;
	}

	/* Wait until the target core has resumed and signalled completion */
	tftf_wait_for_event(&event_received_wake_irq[core_pos]);

	return TEST_RESULT_SUCCESS;
}
