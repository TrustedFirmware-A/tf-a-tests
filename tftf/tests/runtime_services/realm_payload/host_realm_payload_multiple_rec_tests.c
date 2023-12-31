/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <drivers/arm/arm_gic.h>
#include <debug.h>
#include <platform.h>
#include <plat_topology.h>
#include <power_management.h>
#include <psci.h>
#include <sgi.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_pmu.h>
#include <host_shared_data.h>

static uint64_t is_secondary_cpu_on;
static struct realm realm;

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

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, MAX_REC_COUNT)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	for (unsigned int i = 0; i < MAX_REC_COUNT; i++) {
		host_shared_data_set_host_val(&realm, i, HOST_ARG1_INDEX, 10U);
		ret1 = host_enter_realm_execute(&realm, REALM_SLEEP_CMD,
				RMI_EXIT_HOST_CALL, i);
		if (!ret1) {
			break;
		}
	}

	ret2 = host_destroy_realm(&realm);

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
	bool ret1, ret2;
	u_register_t ret;
	unsigned int host_call_result;
	u_register_t exit_reason;
	unsigned int rec_num;
	struct rmi_rec_run *run;
	/* Create 3 rec Rec 0 and 2 are runnable, Rec 1 in not runnable */
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_NOT_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 3U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_MULTIPLE_REC_PSCI_DENIED_CMD,
			RMI_EXIT_PSCI, 0U);
	run = (struct rmi_rec_run *)realm.run[0];

	if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
		ERROR("Host did not receive CPU ON request\n");
		ret1 = false;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], &realm);
	if (rec_num != 1U) {
		ERROR("Invalid mpidr requested\n");
		ret1 = false;
		goto destroy_realm;
	}
	INFO("Requesting PSCI Complete Status Denied REC %d\n", rec_num);
	ret = host_rmi_psci_complete(realm.rec[0], realm.rec[rec_num],
			(unsigned long)PSCI_E_DENIED);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete failed\n");
		ret1 = false;
		goto destroy_realm;
	}

	/* Enter rec1, should fail */
	ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 1U);
	if (ret == RMI_SUCCESS) {
		ERROR("Rec1 enter should have failed\n");
		ret1 = false;
		goto destroy_realm;
	}
	ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);

	if (run->exit.gprs[0] != SMC_PSCI_AFFINITY_INFO_AARCH64) {
		ERROR("Host did not receive PSCI_AFFINITY_INFO request\n");
		ret1 = false;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], &realm);
	if (rec_num != 1U) {
		ERROR("Invalid mpidr requested\n");
		goto destroy_realm;
	}

	INFO("Requesting PSCI Complete Affinity Info REC %d\n", rec_num);
	ret = host_rmi_psci_complete(realm.rec[0], realm.rec[rec_num],
			(unsigned long)PSCI_E_SUCCESS);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete failed\n");
		ret1 = false;
		goto destroy_realm;
	}

	/* Re-enter REC0 complete PSCI_AFFINITY_INFO */
	ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);


	if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
		ERROR("Host did not receive CPU ON request\n");
		ret1 = false;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], &realm);
	if (rec_num != 2U) {
		ERROR("Invalid mpidr requested\n");
		ret1 = false;
		goto destroy_realm;
	}

	INFO("Requesting PSCI Complete Status Denied REC %d\n", rec_num);
	/* PSCI_DENIED should fail as rec2 is RMI_RUNNABLE */
	ret = host_rmi_psci_complete(realm.rec[0], realm.rec[rec_num],
			(unsigned long)PSCI_E_DENIED);
	if (ret == RMI_SUCCESS) {
		ret1 = false;
		ERROR("host_rmi_psci_complete should have failed\n");
		goto destroy_realm;
	}

	ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("Rec0 re-enter failed\n");
		ret1 = false;
		goto destroy_realm;
	}

destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

/* Lock used to avoid concurrent accesses to the secondary_cpu_on counter */
spinlock_t secondary_cpu_lock;

static test_result_t cpu_on_handler2(void)
{
	bool ret;

	spin_lock(&secondary_cpu_lock);
	is_secondary_cpu_on++;
	spin_unlock(&secondary_cpu_lock);

	ret = host_enter_realm_execute(&realm, REALM_LOOP_CMD,
			RMI_EXIT_IRQ, is_secondary_cpu_on);
	if (!ret) {
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
}

test_result_t host_realm_multi_rec_exit_irq(void)
{
	bool ret1, ret2;
	unsigned int rec_count = MAX_REC_COUNT;
	u_register_t other_mpidr, my_mpidr, ret;
	int cpu_node;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
		RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
		RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	SKIP_TEST_IF_LESS_THAN_N_CPUS(rec_count);

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, rec_count)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	is_secondary_cpu_on = 0U;
	my_mpidr = read_mpidr_el1() & MPID_MASK;
	ret1 = host_enter_realm_execute(&realm, REALM_GET_RSI_VERSION, RMI_EXIT_HOST_CALL, 0U);
	for_each_cpu(cpu_node) {
		other_mpidr = tftf_get_mpidr_from_node(cpu_node);
		if (other_mpidr == my_mpidr) {
			continue;
		}
		/* Power on the other CPU */
		ret = tftf_try_cpu_on(other_mpidr, (uintptr_t)cpu_on_handler2, 0);
		if (ret != PSCI_E_SUCCESS) {
			goto destroy_realm;
		}
	}

	INFO("Wait for all CPU to come up\n");
	while (is_secondary_cpu_on != (rec_count - 1U)) {
		waitms(100U);
	}

destroy_realm:
	tftf_irq_enable(IRQ_NS_SGI_7, GIC_HIGHEST_NS_PRIORITY);
	for (unsigned int i = 1U; i < rec_count; i++) {
		INFO("Raising NS IRQ for rec %d\n", i);
		host_rec_send_sgi(&realm, IRQ_NS_SGI_7, i);
	}
	tftf_irq_disable(IRQ_NS_SGI_7);
	ret2 = host_destroy_realm(&realm);
	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}


static test_result_t cpu_on_handler(void)
{
	bool ret;
	struct rmi_rec_run *run;
	unsigned int i;

	spin_lock(&secondary_cpu_lock);
	i = ++is_secondary_cpu_on;
	spin_unlock(&secondary_cpu_lock);
	ret = host_enter_realm_execute(&realm, REALM_MULTIPLE_REC_MULTIPLE_CPU_CMD,
			RMI_EXIT_PSCI, i);
	if (ret) {
		run = (struct rmi_rec_run *)realm.run[i];
		if (run->exit.gprs[0] == SMC_PSCI_CPU_OFF) {
			return TEST_RESULT_SUCCESS;
		}
	}
	ERROR("Rec %d failed\n", i);
	return TEST_RESULT_FAIL;
}

/*
 * The test creates a realm with MAX recs
 * On receiving PSCI_CPU_ON call from REC0 for all other recs,
 * the test completes the PSCI call and re-enters REC0.
 * Turn ON secondary CPUs upto a max of MAX_REC_COUNT.
 * Each of the secondary then enters Realm with a different REC
 * and executes the test REALM_MULTIPLE_REC_MULTIPLE_CPU_CMD in Realm payload.
 * It is expected that the REC will exit with PSCI_CPU_OFF as the exit reason.
 * REC00 checks if all other CPUs are off, via PSCI_AFFINITY_INFO.
 * Host completes the PSCI requests.
 */
test_result_t host_realm_multi_rec_multiple_cpu(void)
{
	bool ret1, ret2;
	test_result_t ret3 = TEST_RESULT_FAIL;
	int ret = RMI_ERROR_INPUT;
	u_register_t rec_num;
	u_register_t other_mpidr, my_mpidr;
	struct rmi_rec_run *run;
	unsigned int host_call_result, i = 0U;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE,
		RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE,
		RMI_NOT_RUNNABLE};
	u_register_t exit_reason;
	int cpu_node;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	SKIP_TEST_IF_LESS_THAN_N_CPUS(MAX_REC_COUNT);

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, MAX_REC_COUNT)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	is_secondary_cpu_on = 0U;
	init_spinlock(&secondary_cpu_lock);
	my_mpidr = read_mpidr_el1() & MPID_MASK;
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, MAX_REC_COUNT);
	ret1 = host_enter_realm_execute(&realm, REALM_MULTIPLE_REC_MULTIPLE_CPU_CMD,
			RMI_EXIT_PSCI, 0U);
	if (!ret1) {
		ERROR("Host did not receive CPU ON request\n");
		goto destroy_realm;
	}
	while (true) {
		run = (struct rmi_rec_run *)realm.run[0];
		if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
			ERROR("Host did not receive CPU ON request\n");
			goto destroy_realm;
		}
		rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], &realm);
		if (rec_num >= MAX_REC_COUNT) {
			ERROR("Invalid mpidr requested\n");
			goto destroy_realm;
		}
		ret = host_rmi_psci_complete(realm.rec[0], realm.rec[rec_num],
				(unsigned long)PSCI_E_SUCCESS);
		if (ret == RMI_SUCCESS) {
			/* Re-enter REC0 complete CPU_ON */
			ret = host_realm_rec_enter(&realm, &exit_reason,
				&host_call_result, 0U);
			if (ret != RMI_SUCCESS || exit_reason != RMI_EXIT_PSCI) {
				break;
			}
		} else {
			ERROR("host_rmi_psci_complete failed\n");
			goto destroy_realm;
		}
	}
	if (exit_reason != RMI_EXIT_HOST_CALL || host_call_result != TEST_RESULT_SUCCESS) {
		ERROR("Realm failed\n");
		goto destroy_realm;
	}

	/* Turn on all CPUs */
	for_each_cpu(cpu_node) {
		if (i == (MAX_REC_COUNT - 1U)) {
			break;
		}
		other_mpidr = tftf_get_mpidr_from_node(cpu_node);
		if (other_mpidr == my_mpidr) {
			continue;
		}

		/* Power on the other CPU */
		ret = tftf_try_cpu_on(other_mpidr, (uintptr_t)cpu_on_handler, 0);
		if (ret != PSCI_E_SUCCESS) {
			ERROR("TFTF CPU ON failed\n");
			goto destroy_realm;
		}
		i++;
	}

	while (true) {
		/* Re-enter REC0 complete PSCI_AFFINITY_INFO */
		ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
		if (ret != RMI_SUCCESS) {
			ERROR("Rec0 re-enter failed\n");
			goto destroy_realm;
		}
		if (run->exit.gprs[0] != SMC_PSCI_AFFINITY_INFO_AARCH64) {
			break;
		}
		rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], &realm);
		if (rec_num >= MAX_REC_COUNT) {
			ERROR("Invalid mpidr requested\n");
			goto destroy_realm;
		}
		ret = host_rmi_psci_complete(realm.rec[0], realm.rec[rec_num],
				(unsigned long)PSCI_E_SUCCESS);

		if (ret != RMI_SUCCESS) {
			ERROR("host_rmi_psci_complete failed\n");
			goto destroy_realm;
		}
	}

	if (ret == RMI_SUCCESS && exit_reason == RMI_EXIT_HOST_CALL) {
		ret3 = host_call_result;
	}
destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if ((ret != RMI_SUCCESS) || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret, ret2);
		return TEST_RESULT_FAIL;
	}

	return ret3;
}

/*
 * Test creates 2 realms with multiple recs
 * realm1, rec1 requests CPU_ON for rec2
 * Host calls PSCI_COMPLETE with wrong rec3, checks for error
 * Host calls PSCI_COMPLETE with wrong rec from different realm, checks for error
 * Host calls PSCI_COMPLETE with correct rec, checks for success
 * Host attempts to execute rec which is NOT_RUNNABLE, checks for error
 */
test_result_t host_realm_multi_rec_multiple_cpu2(void)
{
	bool ret1, ret2;
	test_result_t ret3 = TEST_RESULT_FAIL;
	int ret = RMI_ERROR_INPUT;
	u_register_t rec_num;
	struct rmi_rec_run *run;
	unsigned int host_call_result;
	struct realm realm2;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE,
		RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE,
		RMI_NOT_RUNNABLE};
	u_register_t exit_reason;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, MAX_REC_COUNT)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_activate_realm_payload(&realm2, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE + PAGE_POOL_MAX_SIZE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		ret2 = host_destroy_realm(&realm);
		return TEST_RESULT_FAIL;
	}

	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
	}

	/* Realm to request CPU_ON for rec 2 */
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, 2U);
	ret1 = host_enter_realm_execute(&realm, REALM_MULTIPLE_REC_MULTIPLE_CPU_CMD,
			RMI_EXIT_PSCI, 0U);
	if (!ret1) {
		ERROR("Host did not receive CPU ON request\n");
		goto destroy_realm;
	}
	run = (struct rmi_rec_run *)realm.run[0];
	if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
		ERROR("Host2 did not receive CPU ON request\n");
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], &realm);
	if (rec_num >= MAX_REC_COUNT) {
		ERROR("Invalid mpidr requested\n");
		goto destroy_realm;
	}

	/* pass wrong target_rec, expect error */
	ret = host_rmi_psci_complete(realm.rec[0], realm.rec[rec_num + 1U],
		(unsigned long)PSCI_E_SUCCESS);
	if (ret == RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete wrong target_rec didn't fail ret=%x\n",
				ret);
		goto destroy_realm;
	}

	/* pass wrong target_rec from different realm, expect error */
	ret = host_rmi_psci_complete(realm.rec[0], realm2.rec[0U],
		(unsigned long)PSCI_E_SUCCESS);
	if (ret == RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete wrong target_rec didn't fail ret=%x\n",
				ret);
		goto destroy_realm;
	}

	ret = host_rmi_psci_complete(realm.rec[0], realm.rec[rec_num],
			(unsigned long)PSCI_E_SUCCESS);

	/* Try to run Rec3(CPU OFF/NOT_RUNNABLE), expect error */
	ret = host_realm_rec_enter(&realm, &exit_reason,
			&host_call_result, 3U);

	if (ret == RMI_SUCCESS) {
		ERROR("Expected error\n");
		goto destroy_realm;
	}
	ret3 = TEST_RESULT_SUCCESS;

destroy_realm:
	ret1 = host_destroy_realm(&realm);
	ret2 = host_destroy_realm(&realm2);

	if (!ret1 || !ret2) {
		ERROR("%s(): failed destroy=%d, %d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}
	return ret3;
}
