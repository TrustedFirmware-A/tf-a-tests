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

/* Used when catching synchronous exceptions. */
static volatile bool exception_triggered[PLATFORM_CORE_COUNT];

/*
 * The whole test should only be skipped if the test was skipped on all CPUs.
 * The test on each CPU can't return TEST_RESULT_SKIPPED, because the whole test
 * is skipped if any of the CPUs return TEST_RESULT_SKIPPED. Instead, to skip a
 * test, the test returns TEST_RESULT_SUCCESS, then sets a flag in the
 * test_skipped array. This array is checked at the end by the
 * run_asymmetric_test function.
 */
static volatile bool test_skipped[PLATFORM_CORE_COUNT];

/*
 * Test function which is run on each CPU. It is global so it is visible to all
 * CPUS.
 */
static test_result_t (*asymmetric_test_function)(void);

static bool exception_handler(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);

	uint64_t esr_el2 = read_esr_el2();

	if (EC_BITS(esr_el2) == EC_UNKNOWN) {
		/*
		 * This may be an undef injection, or a trap to EL2 due to a
		 * register not being present. Both cases have the same EC
		 * value.
		 */
		exception_triggered[core_pos] = true;
		return true;
	}

	return false;
}

static test_result_t test_trbe(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);
	bool should_trigger_exception = is_trbe_errata_affected_core();

	if (!is_feat_trbe_present()) {
		test_skipped[core_pos] = true;
		return TEST_RESULT_SUCCESS;
	}

	register_custom_sync_exception_handler(exception_handler);
	exception_triggered[core_pos] = false;
	read_trblimitr_el1();
	unregister_custom_sync_exception_handler();

	/**
	 * NOTE: TRBE as an asymmetric feature is as exceptional one.
	 * Even if the hardware supports the feature, TF-A deliberately disables
	 * it at EL3. In this scenario, when the register "TRBLIMITR_EL1" is
	 * accessed, the registered undef injection handler should kick in and
	 * the exception will be handled synchronously at EL2.
	 */
	if (exception_triggered[core_pos] != should_trigger_exception) {
		tftf_testcase_printf("Exception triggered for core = %d "
				     "when accessing TRB_LIMTR\n", core_pos);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t test_spe(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);

	/**
	 * NOTE: SPE as an asymmetric feature, we expect to access the
	 * PMSCR_EL1 register, when supported in the hardware.
	 * If the feature isn't supported, we skip the test.
	 * So on each individual CPU, we verify whether the feature's presence
	 * and based on it we access (if feature supported) or skip the test.
	 */
	if (!is_feat_spe_supported()) {
		test_skipped[core_pos] = true;
		return TEST_RESULT_SUCCESS;
	}

	read_pmscr_el1();

	return TEST_RESULT_SUCCESS;
}

static test_result_t test_tcr2(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);

	if (!is_feat_tcr2_supported()) {
		test_skipped[core_pos] = true;
		return TEST_RESULT_SUCCESS;
	}

	read_tcr2_el1();

	return TEST_RESULT_SUCCESS;
}

/*
 * Runs on one CPU, and runs asymmetric_test_function.
 */
static test_result_t non_lead_cpu_fn(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);
	test_result_t test_result;

	/* Signal to the lead CPU that the calling CPU has entered the test */
	tftf_send_event(&cpu_has_entered_test[core_pos]);

	test_result = asymmetric_test_function();

	/* Ensure that EL3 still functional */
	smc_args args;
	smc_ret_values smc_ret;
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	smc_ret = tftf_smc(&args);

	tftf_testcase_printf("SMCCC Version = %d.%d\n",
			(int)((smc_ret.ret0 >> SMCCC_VERSION_MAJOR_SHIFT) & SMCCC_VERSION_MAJOR_MASK),
			(int)((smc_ret.ret0 >> SMCCC_VERSION_MINOR_SHIFT) & SMCCC_VERSION_MINOR_MASK));

	return test_result;
}

/* Set some variables that are accessible to all CPUs. */
void test_init(test_result_t (*test_function)(void))
{
	int i;

	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		test_skipped[i] = false;
		tftf_init_event(&cpu_has_entered_test[i]);
	}

	asymmetric_test_function = test_function;

	/* Ensure the above writes are seen before any read */
	dmbsy();
}

/*
 * Run the given test function on all CPUs. If the test is skipped on all CPUs,
 * the whole test is skipped. This is checked using the test_skipped array.
 */
test_result_t run_asymmetric_test(test_result_t (*test_function)(void))
{
	unsigned int lead_mpid;
	unsigned int cpu_mpid, cpu_node;
	unsigned int core_pos;
	int psci_ret;
	bool all_cpus_skipped;
	int i;
	uint32_t aff_info;
	test_result_t test_result;

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	SKIP_TEST_IF_LESS_THAN_N_CPUS(2);

	test_init(test_function);

	/* run test on lead CPU */
	test_result = test_function();

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
	}

	/* Wait for all non-lead CPUs to power down */
	for_each_cpu(cpu_node) {
		cpu_mpid = tftf_get_mpidr_from_node(cpu_node);
		/* Skip lead CPU */
		if (cpu_mpid == lead_mpid)
			continue;

		do {
			aff_info = tftf_psci_affinity_info(cpu_mpid,
							   MPIDR_AFFLVL0);
		} while (aff_info != PSCI_STATE_OFF);
	}

	/*
	 * If the test was skipped on all CPUs, the whole test should be
	 * skipped.
	 */

	all_cpus_skipped = true;
	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		if (!test_skipped[i]) {
			all_cpus_skipped = false;
			break;
		}
	}

	if (all_cpus_skipped) {
		return TEST_RESULT_SKIPPED;
	} else {
		return test_result;
	}
}

/* Test Asymmetric Support for FEAT_TRBE */
test_result_t test_trbe_errata_asymmetric(void)
{
	return run_asymmetric_test(test_trbe);
}

/* Test Asymmetric Support for FEAT_SPE */
test_result_t test_spe_asymmetric(void)
{
	return run_asymmetric_test(test_spe);
}

test_result_t test_tcr2_asymmetric(void)
{
	return run_asymmetric_test(test_tcr2);
}
