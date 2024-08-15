/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch.h>
#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <events.h>
#include <plat_topology.h>
#include <platform.h>
#include <platform_def.h>
#include <power_management.h>
#include <psci.h>
#include <smccc.h>
#include <sync.h>
#include <test_helpers.h>
#include <tftf_lib.h>

static event_t cpu_has_entered_test[PLATFORM_CORE_COUNT];

static volatile bool undef_injection_triggered;

static unsigned int test_result;

static bool trbe_trap_exception_handler(void)
{
	uint64_t esr_el2 = read_esr_el2();
	if (EC_BITS(esr_el2) == EC_UNKNOWN) {
		undef_injection_triggered = true;
		return true;
	}

	return false;
}

/*
 * Non-lead cpu function that checks if trblimitr_el1 is accessible,
 * on affected cores this causes a undef injection and passes.In cores that
 * are not affected test just passes. It fails in other cases.
 */
static test_result_t non_lead_cpu_fn(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);
	bool check_if_affected = is_trbe_errata_affected_core();

	test_result = TEST_RESULT_SUCCESS;

	/* Signal to the lead CPU that the calling CPU has entered the test */
	tftf_send_event(&cpu_has_entered_test[core_pos]);

	read_trblimitr_el1();

	/* Ensure that EL3 still functional */
	smc_args args;
	smc_ret_values smc_ret;
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	smc_ret = tftf_smc(&args);

	tftf_testcase_printf("SMCCC Version = %d.%d\n",
			(int)((smc_ret.ret0 >> SMCCC_VERSION_MAJOR_SHIFT) & SMCCC_VERSION_MAJOR_MASK),
			(int)((smc_ret.ret0 >> SMCCC_VERSION_MINOR_SHIFT) & SMCCC_VERSION_MINOR_MASK));

	if (undef_injection_triggered == true && check_if_affected == true) {
		test_result = TEST_RESULT_SUCCESS;
		undef_injection_triggered = false;
		tftf_testcase_printf("Undef injection triggered for core = %d\n", core_pos);
	} else if(undef_injection_triggered == false && check_if_affected == false) {
		test_result = TEST_RESULT_SUCCESS;
		tftf_testcase_printf("TRB_LIMITR register accessible for core = %d\n", core_pos);
	} else {
		test_result = TEST_RESULT_FAIL;
	}

	return test_result;
}

/* This function kicks off non-lead cpus and the non-lead cpu function
 * checks if errata is applied or not using the test.
 */
test_result_t test_asymmetric_features(void)
{
	unsigned int lead_mpid;
	unsigned int cpu_mpid, cpu_node;
	unsigned int core_pos;
	int psci_ret;

	undef_injection_triggered = false;

	register_custom_sync_exception_handler(trbe_trap_exception_handler);

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	SKIP_TEST_IF_LESS_THAN_N_CPUS(2);

	/* Power on all CPUs */
	for_each_cpu(cpu_node) {
		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
		/* Skip lead CPU as it is already powered on */
		if (cpu_mpid == lead_mpid)
			continue;

		psci_ret = tftf_cpu_on(cpu_mpid, (uintptr_t) non_lead_cpu_fn, 0);
		if (psci_ret != PSCI_E_SUCCESS) {
			tftf_testcase_printf(
					"Failed to power on CPU 0x%x (%d)\n",
					cpu_mpid, psci_ret);
			return TEST_RESULT_SKIPPED;
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
		if (test_result == TEST_RESULT_FAIL)
			break;
	}

	unregister_custom_sync_exception_handler();

	return test_result;
}
