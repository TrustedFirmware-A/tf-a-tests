/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <events.h>
#include <lfa.h>
#include <plat_topology.h>
#include <platform.h>
#include <power_management.h>
#include <psci.h>
#include <spinlock.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#define RMM_X1	UL(0x564bf212a662076c)
#define RMM_X2	UL(0xd90636638fbacb92)

static uint64_t fw_id;

/*
 * Test entry point function for non-lead CPUs.
 * Specified by the lead CPU when bringing up other CPUs.
 */
static test_result_t non_lead_cpu_fn(void)
{
	smc_args args = { .fid = LFA_ACTIVATE, .arg1 = fw_id };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_ACTIVATE error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
}

static test_result_t test_lfa_activate_flow(uint64_t uuid1, uint64_t uuid2)
{
	unsigned int lead_mpid;
	unsigned int cpu_mpid, cpu_node;
	int psci_ret;

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	SKIP_TEST_IF_LESS_THAN_N_CPUS(2);

	smc_args args = { .fid = LFA_GET_INFO, .arg1 = 0 };
	smc_ret_values ret;
	uint64_t i, j;
	bool found = false;

	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_GET_INFO error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}
	j = ret.ret1;

	for (i = 0U; i < j; i++) {
		args.fid = LFA_GET_INVENTORY;
		args.arg1 = i;
		ret = tftf_smc(&args);
		if (ret.ret0 != 0) {
			tftf_testcase_printf("%s: LFA_GET_INVENTORY error: 0x%08lx\n",
					     __func__, ret.ret0);
			return TEST_RESULT_FAIL;
		}
		tftf_testcase_printf("ID %lld: 0x%16lx 0x%16lx - capable: %s, pending: %s\n",
			i, ret.ret1, ret.ret2,
			(ret.ret4 & 0x1) ? "yes" : "no",
			(ret.ret4 & 0x2) ? "yes" : "no");

		if ((ret.ret1 == uuid1) && (ret.ret2 == uuid2)) {
			found = true;
			fw_id = i;
		}
	}

	if (found == false) {
		tftf_testcase_printf("%s: Firmware not found\n", __func__);
		return TEST_RESULT_SKIPPED;
	}

	args.fid = LFA_PRIME;
	args.arg1 = fw_id;
	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_PRIME error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

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

	args.fid = LFA_ACTIVATE;
	args.arg1 = fw_id;
	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: LFA_ACTIVATE error: 0x%08lx\n", __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test RMM Live activation.
 */
test_result_t test_lfa_activate_rmm(void)
{
	return test_lfa_activate_flow(RMM_X1, RMM_X2);
}
