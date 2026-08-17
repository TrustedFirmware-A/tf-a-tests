/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_pggs.h"

#include <events.h>
#include <platform.h>
#include <power_management.h>
#include <test_helpers.h>

#define RESTART_CYCLES			5U

/*
 * The reboot counter lives in a PGGS scratch register that survives
 * resets but is not guaranteed to be zero on a cold boot.  Tag the
 * stored value so a stale, unrelated value left in the register is not
 * mistaken for a valid counter: only a value carrying PGGS_COUNTER_TAG
 * in its upper bits is treated as a counter, anything else restarts the
 * count from zero.  Without this, a stale value >= RESTART_CYCLES would
 * make the test report PASS without performing any restart.
 */
#define PGGS_COUNTER_TAG		0x5A5A0000U
#define PGGS_COUNTER_TAG_MASK		0xFFFF0000U
#define PGGS_COUNTER_VALUE_MASK		0x0000FFFFU

/*
 * Maximum time to wait for the secondary CPUs to reach the OFF state
 * after they have signaled completion, before giving up.
 */
#define SECONDARY_CPUS_OFF_TIMEOUT_MS	1000U

static event_t cpu_has_entered_test[PLATFORM_CORE_COUNT];

static test_result_t non_lead_cpu_fn(void)
{
	u_register_t mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);

	tftf_send_event(&cpu_has_entered_test[core_pos]);

	return TEST_RESULT_SUCCESS;
}

/*
 * Shared restart routine.
 *
 * Powers on all secondary cores, then resets the platform via the PSCI
 * SYSTEM_RESET path with the requested shutdown scope (subsystem or
 * system).  A PGGS register accessed through platform-specific IOCTL
 * helpers persists across restarts and counts iterations across reboots.
 */
static test_result_t do_restart_test(uint32_t shutdown_subtype)
{
	u_register_t lead_mpid, cpu_mpid, cpu_node;
	int32_t status, psci_ret;
	uint32_t pggs_val, count;
	unsigned int core_pos;

	status = xpm_pggs_read(&pggs_val);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("ERROR reading PGGS scratch register, Status: 0x%x\n",
				     status);
		return TEST_RESULT_FAIL;
	}

	/*
	 * Only treat the scratch value as a counter if it carries our tag;
	 * otherwise it is a stale/cold-boot value and the count starts at 0.
	 */
	if ((pggs_val & PGGS_COUNTER_TAG_MASK) == PGGS_COUNTER_TAG) {
		count = pggs_val & PGGS_COUNTER_VALUE_MASK;
	} else {
		count = 0U;
	}

	tftf_testcase_printf("PGGS restart counter = %u (raw 0x%x)\n", count, pggs_val);
	/*
	 * The PGGS scratch register is preserved across reboots and is used
	 * as the iteration counter for this test.  Each reboot re-enters the
	 * test and bumps the counter; once it reaches RESTART_CYCLES the
	 * platform has successfully completed the requested number of restart
	 * cycles, so report PASS and stop triggering further resets
	 * (otherwise the test would loop forever on the target).
	 */
	if (count >= RESTART_CYCLES) {
		/*
		 * Reset the PGGS counter so this test (and any other
		 * restart test sharing the counter) starts from a clean
		 * state on the next invocation.
		 */
		status = xpm_pggs_write(0U);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("ERROR resetting PGGS scratch register, Status: 0x%x\n",
					     status);
			return TEST_RESULT_FAIL;
		}
		return TEST_RESULT_SUCCESS;
	}

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	for (core_pos = 0U; core_pos < PLATFORM_CORE_COUNT; core_pos++) {
		tftf_init_event(&cpu_has_entered_test[core_pos]);
	}

	for_each_cpu(cpu_node) {
		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
		if (cpu_mpid == lead_mpid) {
			continue;
		}

		psci_ret = tftf_cpu_on(cpu_mpid, (uintptr_t)non_lead_cpu_fn, 0);
		if (psci_ret != PSCI_E_SUCCESS && psci_ret != PSCI_E_ALREADY_ON) {
			tftf_testcase_printf("Failed to power on CPU 0x%lx (%d)\n",
					     (unsigned long)cpu_mpid, psci_ret);
			return TEST_RESULT_FAIL;
		}
	}

	for_each_cpu(cpu_node) {
		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
		if (cpu_mpid == lead_mpid) {
			continue;
		}

		core_pos = platform_get_core_pos(cpu_mpid);
		tftf_wait_for_event(&cpu_has_entered_test[core_pos]);
	}

	/*
	 * Wait for each secondary CPU to actually reach the OFF state
	 * before triggering the reset.  Polling affinity_info with a
	 * generous overall timeout works reliably on both slow and fast
	 * targets, unlike a fixed settling delay.
	 */
	for_each_cpu(cpu_node) {
		uint32_t poll_ms = 0U;
		int32_t aff_state;

		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
		if (cpu_mpid == lead_mpid) {
			continue;
		}

		aff_state = tftf_psci_affinity_info(cpu_mpid, MPIDR_AFFLVL0);
		while (aff_state != PSCI_STATE_OFF) {
			if (aff_state < 0) {
				tftf_testcase_printf("ERROR: affinity_info for CPU 0x%lx failed (%d)\n",
						     (unsigned long)cpu_mpid, aff_state);
				return TEST_RESULT_FAIL;
			}
			if (poll_ms >= SECONDARY_CPUS_OFF_TIMEOUT_MS) {
				tftf_testcase_printf("ERROR: CPU 0x%lx did not reach OFF state\n",
						     (unsigned long)cpu_mpid);
				return TEST_RESULT_FAIL;
			}
			waitms(1U);
			poll_ms++;
			aff_state = tftf_psci_affinity_info(cpu_mpid, MPIDR_AFFLVL0);
		}
	}

	/*
	 * Persist the incremented iteration counter into the PGGS scratch
	 * register before triggering the reset.  PGGS retains its value
	 * across the upcoming restart, so on the next boot the test reads
	 * back this updated value, recognises that another cycle has
	 * completed and either triggers the next reset or, once
	 * RESTART_CYCLES is reached, declares the test PASS.
	 */
	count++;
	if (count > PGGS_COUNTER_VALUE_MASK) {
		tftf_testcase_printf("ERROR PGGS restart counter exceeds the value field\n");
		return TEST_RESULT_FAIL;
	}

	status = xpm_pggs_write(PGGS_COUNTER_TAG | count);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("ERROR writing PGGS scratch register, Status: 0x%x\n",
				     status);
		return TEST_RESULT_FAIL;
	}

	status = xpm_system_shutdown(PM_SHUTDOWN_TYPE_SETSCOPE_ONLY,
				     shutdown_subtype);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("ERROR setting shutdown scope, Status: 0x%x\n",
				     status);
		return TEST_RESULT_FAIL;
	}

	psci_system_reset();

	/* psci_system_reset() should never return */
	tftf_testcase_printf("ERROR: psci_system_reset() returned unexpectedly\n");
	return TEST_RESULT_FAIL;
}

/* Subsystem-scoped restart test. */
test_result_t test_subsystem_restart(void)
{
	return do_restart_test(PM_SHUTDOWN_SUBTYPE_RST_SUBSYSTEM);
}

/* System-scoped restart test. */
test_result_t test_system_restart(void)
{
	return do_restart_test(PM_SHUTDOWN_SUBTYPE_RST_SYSTEM);
}
