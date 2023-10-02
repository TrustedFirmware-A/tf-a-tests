/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <debug.h>
#include <power_management.h>
#include <psci.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_pmu.h>
#include <host_shared_data.h>

/*
 * Test tries to create max Rec
 * Enters all Rec from single CPU
 */
test_result_t host_realm_multi_rec_single_cpu(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
	RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, MAX_REC_COUNT)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	for (unsigned int i = 0; i < MAX_REC_COUNT; i++) {
		host_shared_data_set_host_val(i, HOST_ARG1_INDEX, 10U);
		ret1 = host_enter_realm_execute(REALM_SLEEP_CMD, NULL,
				RMI_EXIT_HOST_CALL, i);
		if (!ret1) {
			break;
		}
	}

	ret2 = host_destroy_realm();

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test creates 3 Rec
 * Rec0 requests CPU ON for rec 1
 * Host denies CPU On for rec 1
 * Host tried to enter rec 1 and fails
 * Host re-enters rec 0
 * Rec 0 checks CPU ON is denied
 * Rec0 requests CPU ON for rec 2
 * Host denies CPU On which should fail as rec is runnable
 * Host allows CPU ON and re-enters rec 0
 * Rec 0 checks return already_on
 */
test_result_t host_realm_multi_rec_psci_denied(void)
{
	struct realm *realm_ptr;
	bool ret1, ret2;
	u_register_t ret;
	unsigned int host_call_result;
	u_register_t exit_reason;
	unsigned int rec_num;
	struct rmi_rec_run *run;
	/* Create 3 rec Rec 0 and 2 are runnable, Rec 1 in not runnable */
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_NOT_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 3U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(REALM_MULTIPLE_REC_PSCI_DENIED_CMD, &realm_ptr,
			RMI_EXIT_PSCI, 0U);
	run = (struct rmi_rec_run *)realm_ptr->run[0];

	if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
		ERROR("Host did not receive CPU ON request\n");
		ret1 = false;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
	if (rec_num != 1U) {
		ERROR("Invalid mpidr requested\n");
		ret1 = false;
		goto destroy_realm;
	}
	INFO("Requesting PSCI Complete Status Denied REC %d\n", rec_num);
	ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
			(unsigned long)PSCI_E_DENIED);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete failed\n");
		ret1 = false;
		goto destroy_realm;
	}

	/* Enter rec1, should fail */
	ret = host_realm_rec_enter(realm_ptr, &exit_reason, &host_call_result, 1U);
	if (ret == RMI_SUCCESS) {
		ERROR("Rec1 enter should have failed\n");
		ret1 = false;
		goto destroy_realm;
	}
	ret = host_realm_rec_enter(realm_ptr, &exit_reason, &host_call_result, 0U);

	if (run->exit.gprs[0] != SMC_PSCI_AFFINITY_INFO_AARCH64) {
		ERROR("Host did not receive PSCI_AFFINITY_INFO request\n");
		ret1 = false;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
	if (rec_num != 1U) {
		ERROR("Invalid mpidr requested\n");
		goto destroy_realm;
	}

	INFO("Requesting PSCI Complete Affinity Info REC %d\n", rec_num);
	ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
			(unsigned long)PSCI_E_SUCCESS);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete failed\n");
		ret1 = false;
		goto destroy_realm;
	}

	/* Re-enter REC0 complete PSCI_AFFINITY_INFO */
	ret = host_realm_rec_enter(realm_ptr, &exit_reason, &host_call_result, 0U);


	if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
		ERROR("Host did not receive CPU ON request\n");
		ret1 = false;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
	if (rec_num != 2U) {
		ERROR("Invalid mpidr requested\n");
		ret1 = false;
		goto destroy_realm;
	}

	INFO("Requesting PSCI Complete Status Denied REC %d\n", rec_num);
	/* PSCI_DENIED should fail as rec2 is RMI_RUNNABLE */
	ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
			(unsigned long)PSCI_E_DENIED);
	if (ret == RMI_SUCCESS) {
		ret1 = false;
		ERROR("host_rmi_psci_complete should have failed\n");
		goto destroy_realm;
	}

	ret = host_realm_rec_enter(realm_ptr, &exit_reason, &host_call_result, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("Rec0 re-enter failed\n");
		ret1 = false;
		goto destroy_realm;
	}

destroy_realm:
	ret2 = host_destroy_realm();

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}
