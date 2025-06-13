/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch_features.h>
#include <common_def.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <pcie_doe.h>
#include <plat_topology.h>
#include <platform.h>
#include <power_management.h>
#include "rmi_spm_tests.h"
#include <test_helpers.h>

static test_result_t host_multi_cpu_payload_dev_del_undel(void);

/* Test 2MB of PCIe memory region 2 */
#define PCIE_MEM_2_TEST_SIZE	SZ_2M

/* Number of dev granules to test */
#define NUM_DEV_GRANULES	((PCIE_MEM_2_TEST_SIZE / GRANULE_SIZE) / \
					PLATFORM_CORE_COUNT)

/* Buffer to delegate and undelegate */
const char *bufferdelegate;
static char bufferstate[NUM_DEV_GRANULES * PLATFORM_CORE_COUNT];

/*
 * Overall test for Host in three sections:
 * 1. Delegate and Undelegate Non-Secure dev granule via
 * SMC call to realm payload.
 * 2. Multi CPU delegation where random assignment of states
 * (realm, non-secure) is assigned to a set of granules.
 * Each CPU is given a number of dev granules to delegate in
 * parallel with the other CPUs.
 * 3. Fail testing of delegation parameters such as
 * attempting to perform a delegation on the same dev granule
 * twice and then testing a misaligned address.
 */
static test_result_t host_init_buffer_dev_del(void)
{
	__unused size_t dev_size;

	host_rmi_init_cmp_result();

	/* Retrieve platform PCIe memory region 2 */
	if (plat_get_dev_region((uint64_t *)&bufferdelegate, &dev_size,
				DEV_MEM_NON_COHERENT, 1U) != 0) {
		tftf_testcase_printf("Cannot retrieve PCIe memory region 2\n");
		return TEST_RESULT_SKIPPED;
	}

	assert(dev_size >= PCIE_MEM_2_TEST_SIZE);

	for (uint32_t i = 0; i < (NUM_DEV_GRANULES * PLATFORM_CORE_COUNT) ; i++) {
		if ((rand() & 1) == 0) {
			u_register_t retrmm = host_rmi_granule_delegate(
					(u_register_t)&bufferdelegate[i * GRANULE_SIZE]);

			if (retrmm != RMI_SUCCESS) {
				tftf_testcase_printf("Delegate operation returns 0x%lx\n",
						retrmm);
				return TEST_RESULT_FAIL;
			}
			bufferstate[i] = B_DELEGATED;
		} else {
			bufferstate[i] = B_UNDELEGATED;
		}
	}

	return host_cmp_result();
}

/*
 * Delegate and Undelegate Non-Secure Dev Granule
 */
test_result_t host_dev_mem_delegate_undelegate(void)
{
	__unused size_t dev_size;
	u_register_t retrmm, rmi_feat_reg0;

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);

	host_rmi_init_cmp_result();

	/* Retrieve platform PCIe memory region 2 */
	if (plat_get_dev_region((uint64_t *)&bufferdelegate, &dev_size,
				DEV_MEM_NON_COHERENT, 1U) != 0) {
		tftf_testcase_printf("Cannot retrieve PCIe memory region 2\n");
		return TEST_RESULT_SKIPPED;
	}

	assert(dev_size >= PCIE_MEM_2_TEST_SIZE);

	retrmm = host_rmi_granule_delegate((u_register_t)bufferdelegate);
	if (retrmm != RMI_SUCCESS) {
		tftf_testcase_printf("Delegate operation returns 0x%lx\n",
					retrmm);
		return TEST_RESULT_FAIL;
	}
	retrmm = host_rmi_granule_undelegate((u_register_t)bufferdelegate);
	if (retrmm != RMI_SUCCESS) {
		tftf_testcase_printf("Undelegate operation returns 0x%lx\n",
					retrmm);
		return TEST_RESULT_FAIL;
	}
	tftf_testcase_printf("Delegate and undelegate of buffer 0x%lx succeeded\n",
				(u_register_t)bufferdelegate);

	return host_cmp_result();
}

/*
 * Select all CPU's to randomly delegate/undelegate
 * dev granule pages to stress the delegate mechanism
 */
test_result_t host_dev_mem_delundel_multi_cpu(void)
{
	u_register_t lead_mpid, target_mpid, retrmm, rmi_feat_reg0;
	int32_t ret;
	unsigned int cpu_node;

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	host_rmi_init_cmp_result();

	retrmm = host_init_buffer_dev_del();
	if (retrmm != TEST_RESULT_SUCCESS) {
		return retrmm;
	}

	for_each_cpu(cpu_node) {
		target_mpid = (u_register_t)tftf_get_mpidr_from_node(cpu_node) & MPID_MASK;

		if (lead_mpid == target_mpid) {
			continue;
		}

		ret = tftf_cpu_on(target_mpid,
			(uintptr_t)host_multi_cpu_payload_dev_del_undel, 0UL);

		if (ret != PSCI_E_SUCCESS) {
			ERROR("CPU ON failed for 0x%lx\n", target_mpid);
			return TEST_RESULT_FAIL;
		}

	}

	for_each_cpu(cpu_node) {
		target_mpid = (u_register_t)tftf_get_mpidr_from_node(cpu_node) & MPID_MASK;

		if (lead_mpid == target_mpid) {
			continue;
		}

		while (tftf_psci_affinity_info(target_mpid, MPIDR_AFFLVL0) !=
				PSCI_STATE_OFF) {
			continue;
		}
	}

	/*
	 * Cleanup to set all dev granules back to undelegated
	 */
	for (unsigned int i = 0U; i < (NUM_DEV_GRANULES * PLATFORM_CORE_COUNT) ; i++) {
		if (bufferstate[i] == B_DELEGATED) {
			retrmm = host_rmi_granule_undelegate(
				(u_register_t)&bufferdelegate[i * GRANULE_SIZE]);
			if (retrmm != RMI_SUCCESS) {
				tftf_testcase_printf("Undelegate operation returns 0x%lx\n",
						retrmm);
				return TEST_RESULT_FAIL;
			}
			bufferstate[i] = B_UNDELEGATED;
		}
	}

	return host_cmp_result();
}

/*
 * Multi CPU testing of delegate and undelegate of dev granules.
 * The granules are first randomly initialized to either realm or
 * non secure using the function init_buffer_dev_del() and then
 * the function below assigns NUM_DEV_GRANULES to each CPU for delegation
 * or undelgation depending upon the initial state.
 */
static test_result_t host_multi_cpu_payload_dev_del_undel(void)
{
	__unused size_t dev_size;
	u_register_t retrmm;
	unsigned int cpu_node;

	/* Retrieve platform PCIe memory region 2 */
	if (plat_get_dev_region((uint64_t *)&bufferdelegate, &dev_size,
				DEV_MEM_NON_COHERENT, 1U) != 0) {
		tftf_testcase_printf("Cannot retrieve PCIe memory region 2\n");
		return TEST_RESULT_SKIPPED;
	}

	assert(dev_size >= PCIE_MEM_2_TEST_SIZE);

	cpu_node = platform_get_core_pos(read_mpidr_el1() & MPID_MASK);

	host_rmi_init_cmp_result();

	for (unsigned int i = 0U; i < NUM_DEV_GRANULES; i++) {
		if (bufferstate[((cpu_node * NUM_DEV_GRANULES) + i)] == B_UNDELEGATED) {
			retrmm = host_rmi_granule_delegate((u_register_t)
				&bufferdelegate[((cpu_node * NUM_DEV_GRANULES) + i) *
								GRANULE_SIZE]);
			if (retrmm != RMI_SUCCESS) {
				tftf_testcase_printf("Delegate operation returns 0x%lx\n",
							retrmm);
				return TEST_RESULT_FAIL;
			}
			bufferstate[((cpu_node * NUM_DEV_GRANULES) + i)] = B_DELEGATED;
		} else {
			retrmm = host_rmi_granule_undelegate((u_register_t)
				&bufferdelegate[((cpu_node * NUM_DEV_GRANULES) + i) *
								GRANULE_SIZE]);
			if (retrmm != RMI_SUCCESS) {
				tftf_testcase_printf("Undelegate operation returns 0x%lx\n",
							retrmm);
				return TEST_RESULT_FAIL;
			}
			bufferstate[((cpu_node * NUM_DEV_GRANULES) + i)] = B_UNDELEGATED;
		}
	}

	return host_cmp_result();
}

/*
 * Fail testing of delegation process. The first is an error expected
 * for processing the same granule twice and the second is submission of
 * a misaligned address
 */
test_result_t host_fail_dev_mem_del(void)
{
	__unused size_t dev_size;
	u_register_t retrmm, rmi_feat_reg0;

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);

	/* Retrieve platform PCIe memory region 2 */
	if (plat_get_dev_region((uint64_t *)&bufferdelegate, &dev_size,
				DEV_MEM_NON_COHERENT, 1U) != 0) {
		tftf_testcase_printf("Cannot retrieve PCIe memory region 2\n");
		return TEST_RESULT_SKIPPED;
	}

	assert(dev_size >= PCIE_MEM_2_TEST_SIZE);

	host_rmi_init_cmp_result();

	retrmm = host_rmi_granule_delegate((u_register_t)&bufferdelegate[0]);
	if (retrmm != RMI_SUCCESS) {
		tftf_testcase_printf
			("Delegate operation does not pass as expected for double delegation,"
				" 0x%lx\n", retrmm);
		return TEST_RESULT_FAIL;
	}

	retrmm = host_rmi_granule_delegate((u_register_t)&bufferdelegate[0]);
	if (retrmm == RMI_SUCCESS) {
		tftf_testcase_printf
			("Delegate operation does not fail as expected for double delegation,"
				" 0x%lx\n", retrmm);
		return TEST_RESULT_FAIL;
	}

	retrmm = host_rmi_granule_undelegate((u_register_t)&bufferdelegate[1]);
	if (retrmm == RMI_SUCCESS) {
		tftf_testcase_printf
			("Undelegate operation does not return fail for misaligned address,"
				" 0x%lx\n", retrmm);
		return TEST_RESULT_FAIL;
	}

	retrmm = host_rmi_granule_undelegate((u_register_t)&bufferdelegate[0]);
	if (retrmm != RMI_SUCCESS) {
		tftf_testcase_printf
			("Undelegate operation returns fail for cleanup, 0x%lx\n", retrmm);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}
