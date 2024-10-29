/*
 * Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <events.h>
#include <plat_topology.h>
#include <platform.h>
#include <power_management.h>
#include <psci.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <timer.h>

static event_t cpu_has_entered_test[PLATFORM_CORE_COUNT];

/*
 * Test entry point function for non-lead CPUs.
 * Specified by the lead CPU when bringing up other CPUs.
 */
static test_result_t non_lead_cpu_fn(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);

	VERBOSE("%s\n", __func__);

	/* Signal to the lead CPU that the calling CPU has entered the test */
	tftf_send_event(&cpu_has_entered_test[core_pos]);

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Code for a test running on multiple CPUs.
 *
 * This "test" powers on all CPUs on the platform and report test success.
 * The function test_multi_core() runs on the lead CPU only.
 * The test entrypoint for other CPUs is non_lead_cpu_fn(), as specified when
 * bringing them up.
 * 1) Power on all secondary cores
 * 2) after a delay power off all secondary cores
 *
 * The test is skipped if an error occurs during the bring-up of non-lead CPUs.
 */
test_result_t test_power_on_cores(void)
{
	unsigned int lead_mpid;
	unsigned int cpu_mpid, cpu_node;
	unsigned int core_pos;
	int psci_ret;

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	VERBOSE("%s: lead_mpid = 0x%x\n", __func__, lead_mpid);

	SKIP_TEST_IF_LESS_THAN_N_CPUS(2);

	/* Power on all CPUs */
	for_each_cpu(cpu_node) {
		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
		/* Skip lead CPU as it is already powered on */
		if (cpu_mpid == lead_mpid)
			continue;

		VERBOSE("%s getting cpu_mpid = 0x%x powered on\n", __func__,
				cpu_mpid);
		psci_ret = tftf_cpu_on(cpu_mpid, (uintptr_t) non_lead_cpu_fn, 0);
		if (psci_ret != PSCI_E_SUCCESS) {
			tftf_testcase_printf("Failed to power on CPU 0x%x (%d)\n",
					cpu_mpid, psci_ret);
			/*return TEST_RESULT_SKIPPED; */
		}
	}

	/* Wait for non-lead CPUs to enter the test */
	for_each_cpu(cpu_node) {
		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
		/* Skip lead CPU */
		if (cpu_mpid == lead_mpid)
			continue;

		core_pos = platform_get_core_pos(cpu_mpid);
		tftf_wait_for_event(&cpu_has_entered_test[core_pos]);
	}

	return TEST_RESULT_SUCCESS;
}
