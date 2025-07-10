/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch_features.h>
#include <common_def.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <plat_topology.h>
#include <platform.h>
#include <power_management.h>
#include "rmi_spm_tests.h"
#include <test_helpers.h>

static test_result_t host_multi_cpu_payload_dev_del_undel(void);

/* Test 2MB of PCIe memory region */
#define PCIE_MEM_TEST_SIZE	SZ_2M

/* Number of dev granules to test */
#define NUM_DEV_GRANULES	((PCIE_MEM_TEST_SIZE / GRANULE_SIZE) / \
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
test_result_t host_init_buffer_dev_del(unsigned int num_reg)
{
	size_t dev_size;

	/* Retrieve platform PCIe memory region */
	int res = plat_get_dev_region((uint64_t *)&bufferdelegate, &dev_size,
					DEV_MEM_NON_COHERENT, num_reg);

	if ((res != 0) || (dev_size < PCIE_MEM_TEST_SIZE)) {
		return TEST_RESULT_SKIPPED;
	}

	tftf_testcase_printf("Testing PCIe memory region %u 0x%lx-0x%lx\n",
				num_reg, (uintptr_t)bufferdelegate,
				(uintptr_t)bufferdelegate + PCIE_MEM_TEST_SIZE - 1UL);

	/* Seed the random number generator */
	assert(is_feat_rng_present());
	srand((unsigned int)read_rndr());

	for (unsigned int i = 0; i < (NUM_DEV_GRANULES * PLATFORM_CORE_COUNT) ; i++) {
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

	return TEST_RESULT_SUCCESS;
}

/*
 * Delegate and Undelegate Non-Secure Device Granules
 */
test_result_t host_dev_mem_delegate_undelegate(void)
{
	u_register_t rmi_feat_reg0;
	unsigned int num_reg = 0U;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	host_rmi_init_cmp_result();

	while (true) {
		size_t dev_size;
		u_register_t retrmm;

		/* Retrieve platform PCIe memory region */
		int res = plat_get_dev_region((uint64_t *)&bufferdelegate,
						&dev_size, DEV_MEM_NON_COHERENT,
						num_reg);

		if ((res != 0) || (dev_size < PCIE_MEM_TEST_SIZE)) {
			break;
		}

		tftf_testcase_printf("Testing PCIe memory region %u 0x%lx-0x%lx\n",
					num_reg, (uintptr_t)bufferdelegate,
					(uintptr_t)bufferdelegate + PCIE_MEM_TEST_SIZE - 1UL);

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

		++num_reg;
	}

	/* No memory regions for test found */
	if (num_reg == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	return host_cmp_result();
}

/*
 * Select all CPU's to randomly delegate/undelegate
 * dev granule pages to stress the delegate mechanism
 */
test_result_t host_dev_mem_delundel_multi_cpu(void)
{
	u_register_t rmi_feat_reg0, lead_mpid;
	unsigned int num_reg = 0U;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	host_rmi_init_cmp_result();

	while (true) {
		u_register_t target_mpid, retrmm;
		unsigned int cpu_node;

		retrmm = host_init_buffer_dev_del(num_reg);
		if (retrmm == TEST_RESULT_SKIPPED) {
			break;
		} else if (retrmm == TEST_RESULT_FAIL) {
			return TEST_RESULT_FAIL;
		}

		for_each_cpu(cpu_node) {
			int32_t ret;

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

		++num_reg;
	}

	/* No memory regions for test found */
	if (num_reg == 0U) {
		return TEST_RESULT_SKIPPED;
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
	unsigned int cpu_node;

	cpu_node = platform_get_core_pos(read_mpidr_el1() & MPID_MASK);

	host_rmi_init_cmp_result();

	for (unsigned int i = 0U; i < NUM_DEV_GRANULES; i++) {
		u_register_t retrmm;

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
	u_register_t rmi_feat_reg0;
	unsigned int num_reg = 0U;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	host_rmi_init_cmp_result();

	while (true) {
		size_t dev_size;
		u_register_t retrmm;

		/* Retrieve platform PCIe memory region */
		int res = plat_get_dev_region((uint64_t *)&bufferdelegate, &dev_size,
						DEV_MEM_NON_COHERENT, num_reg);

		if ((res != 0) || (dev_size < PCIE_MEM_TEST_SIZE)) {
			break;
		}

		tftf_testcase_printf("Testing PCIe memory region %u 0x%lx-0x%lx\n",
					num_reg, (uintptr_t)bufferdelegate,
					(uintptr_t)bufferdelegate + PCIE_MEM_TEST_SIZE - 1UL);

		retrmm = host_rmi_granule_delegate((u_register_t)&bufferdelegate[0]);
		if (retrmm != RMI_SUCCESS) {
			tftf_testcase_printf(
				"Delegate operation does not pass as"
				"expected for double delegation, 0x%lx\n",
				retrmm);
			return TEST_RESULT_FAIL;
		}

		retrmm = host_rmi_granule_delegate((u_register_t)&bufferdelegate[0]);
		if (retrmm == RMI_SUCCESS) {
			tftf_testcase_printf(
				"Delegate operation does not fail as"
				"expected for double delegation, 0x%lx\n",
				retrmm);
			return TEST_RESULT_FAIL;
		}

		retrmm = host_rmi_granule_undelegate((u_register_t)&bufferdelegate[1]);
		if (retrmm == RMI_SUCCESS) {
			tftf_testcase_printf(
				"Undelegate operation does not fail for misaligned address,"
				" 0x%lx\n", retrmm);
			return TEST_RESULT_FAIL;
		}

		retrmm = host_rmi_granule_undelegate((u_register_t)&bufferdelegate[0]);
		if (retrmm != RMI_SUCCESS) {
			tftf_testcase_printf(
				"Undelegate operation fails for cleanup, 0x%lx\n", retrmm);
			return TEST_RESULT_FAIL;
		}

		++num_reg;
	}

	/* No memory regions for test found */
	if (num_reg == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	return host_cmp_result();
}
