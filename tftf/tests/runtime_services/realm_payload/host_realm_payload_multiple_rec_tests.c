/*
 * Copyright (c) 2021-2025, Arm Limited. All rights reserved.
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
static struct realm realm1;
static struct pmu_registers pmu_state[PLATFORM_CORE_COUNT];

/*
 * Test tries to create max Rec
 * Enters all Rec from single CPU
 */
test_result_t host_realm_multi_rec_single_cpu(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[MAX_REC_COUNT];
	u_register_t feature_flag = 0U;
	long sl = RTT_MIN_LEVEL;
	unsigned int rec_num;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, 0U, sl, rec_flag, MAX_REC_COUNT, 0U)) {
		return TEST_RESULT_FAIL;
	}

	/* Start random Rec */
	rec_num = (unsigned int)rand() % MAX_REC_COUNT;

	for (unsigned int i = 0; i < MAX_REC_COUNT; i++) {
		host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID,
				rec_num, HOST_ARG1_INDEX, 10U);

		ret1 = host_enter_realm_execute(&realm, REALM_SLEEP_CMD,
				RMI_EXIT_HOST_CALL, rec_num);
		if (!ret1) {
			break;
		}

		/* Increment Rec number */
		if (++rec_num == MAX_REC_COUNT) {
			rec_num = 0U;
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
	u_register_t feature_flag = 0U;
	long sl = RTT_MIN_LEVEL;
	/* Create 3 rec Rec 0 and 2 are runnable, Rec 1 in not runnable */
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_NOT_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, 0U, sl, rec_flag, 3U, 0U)) {
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
	u_register_t other_mpidr, my_mpidr, ret;
	unsigned int cpu_node, rec_count;
	u_register_t feature_flag0 = 0U;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[MAX_REC_COUNT];

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	rec_count = tftf_get_total_cpus_count();
	assert(rec_count <= MAX_REC_COUNT);

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < rec_count; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, rec_count, 0U)) {
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

	INFO("Wait for all CPUs to come up\n");
	while (is_secondary_cpu_on != (rec_count - 1U)) {
		waitms(100U);
	}

destroy_realm:
	tftf_irq_enable(IRQ_NS_SGI_7, GIC_HIGHEST_NS_PRIORITY);
	for (unsigned int i = 1U; i < rec_count; i++) {
		INFO("Raising NS IRQ for rec %u\n", i);
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
 * REC0 checks if all other CPUs are off, via PSCI_AFFINITY_INFO.
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
	unsigned int host_call_result, i;
	u_register_t rec_flag[MAX_REC_COUNT] = {RMI_RUNNABLE};
	u_register_t exit_reason;
	unsigned int cpu_node, rec_count;
	u_register_t feature_flag = 0U;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	rec_count = tftf_get_total_cpus_count();
	assert(rec_count <= MAX_REC_COUNT);

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (i = 1U; i < rec_count; i++) {
		rec_flag[i] = RMI_NOT_RUNNABLE;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, 0U, sl, rec_flag, rec_count, 0U)) {
		return TEST_RESULT_FAIL;
	}

	is_secondary_cpu_on = 0U;
	init_spinlock(&secondary_cpu_lock);
	my_mpidr = read_mpidr_el1() & MPID_MASK;
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG1_INDEX, rec_count);
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

	i = 0U;

	/* Turn on all CPUs */
	for_each_cpu(cpu_node) {
		if (i == (rec_count - 1U)) {
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
		if (rec_num >= rec_count) {
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
	u_register_t feature_flag = 0U;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[MAX_REC_COUNT] = {RMI_RUNNABLE};
	u_register_t exit_reason;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 1U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_NOT_RUNNABLE;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, 0UL, sl, rec_flag, MAX_REC_COUNT, 0U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_activate_realm_payload(&realm2, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, 0U, sl, rec_flag, 1U, 0U)) {
		ret2 = host_destroy_realm(&realm);
		return TEST_RESULT_FAIL;
	}

	/* Realm to request CPU_ON for rec 2 */
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG1_INDEX, 2U);
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

/*
 * Test PMU counters available to each REC matches that programmed by host
 * Test PMU counters are preserved for each rec
 */
static test_result_t cpu_on_handler_pmu(void)
{
	bool ret1;
	unsigned int i;

	spin_lock(&secondary_cpu_lock);
	i = is_secondary_cpu_on++;
	spin_unlock(&secondary_cpu_lock);

	host_set_pmu_state(&pmu_state[i]);

	ret1 = host_enter_realm_execute(&realm, REALM_PMU_COUNTER, RMI_EXIT_HOST_CALL, i);
	if (!ret1) {
		return TEST_RESULT_FAIL;
	}
	ret1 = host_enter_realm_execute(&realm1, REALM_PMU_COUNTER, RMI_EXIT_HOST_CALL, i);
	if (!ret1) {
		return TEST_RESULT_FAIL;
	}
	ret1 = host_enter_realm_execute(&realm, REALM_PMU_PRESERVE, RMI_EXIT_HOST_CALL, i);
	if (!ret1) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm1, REALM_PMU_PRESERVE, RMI_EXIT_HOST_CALL, i);
	if (!ret1) {
		return TEST_RESULT_FAIL;
	}

	if (host_check_pmu_state(&pmu_state[i])) {
		return TEST_RESULT_SUCCESS;
	}

	return TEST_RESULT_FAIL;
}

/*
 * Test realm creation with more PMU counter than available, expect failure
 * Test realm creation with 0 PMU counter
 * expect failure if FEAT_HPMN0 is not supported
 * expect success if FEAT_HPMN0 is supported
 * Create 2 Realms first one with MAX PMU counters
 * second realm with lesser PMU counter than available
 * Schedule multiple rec on multiple CPU
 * Test PMU counters available to each REC matches that programmed by host
 * Test PMU counters are preserved for each rec
 */
test_result_t host_realm_pmuv3_mul_rec(void)
{
	u_register_t feature_flag = 0U;
	u_register_t rmm_feat_reg0;
	u_register_t rec_flag[MAX_REC_COUNT];
	bool ret1 = false, ret2;
	unsigned int rec_count, i, num_cnts;
	u_register_t other_mpidr, my_mpidr, ret;
	int cpu_node;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	host_rmi_init_cmp_result();

	rec_count = tftf_get_total_cpus_count();
	assert(rec_count <= MAX_REC_COUNT);

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (i = 0U; i < rec_count; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	/* Get number of PMU event counters implemented through RMI_FEATURES */
	if (host_rmi_features(0UL, &rmm_feat_reg0) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return TEST_RESULT_FAIL;
	}

	num_cnts = EXTRACT(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS, rmm_feat_reg0);
	host_set_pmu_state(&pmu_state[0U]);

	is_secondary_cpu_on = 0;
	my_mpidr = read_mpidr_el1() & MPID_MASK;

	if (num_cnts == 0) {
		INFO("No event counters implemented\n");
	} else {
		INFO("Testing %u event counters\n", num_cnts);
	}

	/*
	 * Check that number of event counters is less
	 * than maximum supported by architecture.
	 */
	if (num_cnts < ((1U << RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS_WIDTH) - 1U)) {
		feature_flag |= RMI_FEATURE_REGISTER_0_PMU_EN |
				INPLACE(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS, num_cnts + 1U);

		if (is_feat_52b_on_4k_2_supported()) {
			feature_flag |= RMI_FEATURE_REGISTER_0_LPA2;
		}

		/* Request more event counters than total, expect failure */
		if (host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
							feature_flag, 0UL, sl, rec_flag, 1U, 0U)) {
			ERROR("Realm create should have failed\n");
			host_destroy_realm(&realm);
			return TEST_RESULT_FAIL;
		}
	}

	/* Request Cycle Counter with no event counters */
	feature_flag = RMI_FEATURE_REGISTER_0_PMU_EN |
			INPLACE(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS, 0U);

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag |= RMI_FEATURE_REGISTER_0_LPA2;
	}

	ret1 = host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
						feature_flag, 0U, sl, rec_flag, 1U, 0U);
	host_destroy_realm(&realm);

	if (!get_feat_hpmn0_supported()) {
		if (ret1) {
			ERROR("Realm create with 0 event counters should have failed\n");
			return TEST_RESULT_FAIL;
		}
	} else {
		if (!ret1) {
			ERROR("Realm create with 0 event counters should not have failed\n");
			return TEST_RESULT_FAIL;
		}
	}

	/* Create first realm with number of PMU event counters */
	feature_flag = RMI_FEATURE_REGISTER_0_PMU_EN |
			INPLACE(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS, num_cnts);

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag |= RMI_FEATURE_REGISTER_0_LPA2;
	}

	/* Prepare realm, create recs later */
	if (!host_prepare_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, 0UL, sl, rec_flag, rec_count, 0U)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Second realm1 with less or equal number of event counters.
	 * When no event counters are implemented, only Cycle Counter
	 * will be tested.
	 */
	feature_flag = RMI_FEATURE_REGISTER_0_PMU_EN |
			INPLACE(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS,
			(num_cnts == 0U) ? num_cnts : num_cnts - 1U);

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag |= RMI_FEATURE_REGISTER_0_LPA2;
	}

	ret1 = host_create_activate_realm_payload(&realm1, (u_register_t)REALM_IMAGE_BASE,
					feature_flag, 0U, sl, rec_flag, rec_count, 0U);
	if (!ret1) {
		goto test_exit;
	}

	/* Create realm recs, activate realm0 */
	if (host_realm_rec_create(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_rec_create");
		goto test_exit;
	}

	if (host_realm_init_ipa_state(&realm, sl, 0U, 1ULL << 32)
		!= RMI_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_init_ipa_state");
		goto test_exit;
	}

	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto test_exit;
	}

	/* Pass number of event counters programmed to realms */
	for (unsigned int j = 0U; j < rec_count; j++) {
		host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, j,
						HOST_ARG1_INDEX, num_cnts);
		host_shared_data_set_host_val(&realm1, PRIMARY_PLANE_ID, j,
						HOST_ARG1_INDEX,
						(num_cnts == 0U) ? 0U : num_cnts - 1U);
	}

	/*
	 * Enter realm rec0 test PMU counters available is same as that programmed by host.
	 * Validation is done by the Realm and will return error if the count does not match.
	 */
	ret1 = host_enter_realm_execute(&realm, REALM_PMU_COUNTER, RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		goto test_exit;
	}

	/* Enter realm1 rec0 test PMU counters available is same as that programmed by host */
	ret1 = host_enter_realm_execute(&realm1, REALM_PMU_COUNTER, RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		goto test_exit;
	}

	/* Test if realm rec0 entering/exiting preserves PMU state */
	ret1 = host_enter_realm_execute(&realm, REALM_PMU_PRESERVE, RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		goto test_exit;
	}

	/* Test if realm1 rec0 entering/exiting preserves PMU state */
	ret1 = host_enter_realm_execute(&realm1, REALM_PMU_PRESERVE, RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		goto test_exit;
	}

	if (!host_check_pmu_state(&pmu_state[0U])) {
		goto test_exit;
	}

	i = 0U;

	/* Turn on all CPUs */
	for_each_cpu(cpu_node) {
		if (i == (rec_count - 1U)) {
			break;
		}
		other_mpidr = tftf_get_mpidr_from_node(cpu_node);
		if (other_mpidr == my_mpidr) {
			continue;
		}

		/* Power on the other CPU */
		ret = tftf_try_cpu_on(other_mpidr, (uintptr_t)cpu_on_handler_pmu, 0);
		if (ret != PSCI_E_SUCCESS) {
			ERROR("TFTF CPU ON failed\n");
			goto test_exit;
		}
		i++;
	}

	/* Wait for all CPU to power up */
	while (is_secondary_cpu_on != (rec_count - 1U)) {
		waitms(100);
	}

	/* Wait for all CPU to power down */
	for_each_cpu(cpu_node) {
		other_mpidr = tftf_get_mpidr_from_node(cpu_node) & MPID_MASK;
		if (other_mpidr == my_mpidr) {
			continue;
		}
		while (tftf_psci_affinity_info(other_mpidr, MPIDR_AFFLVL0) != PSCI_STATE_OFF) {
			continue;
		}
	}

test_exit:
	ret2 = host_destroy_realm(&realm1);
	if (!ret1 || !ret2) {
		ERROR("%s() enter=%u destroy=%u\n", __func__, ret1, ret2);
	}

	ret2 = host_destroy_realm(&realm);
	if (!ret1 || !ret2) {
		ERROR("%s() enter=%u destroy=%u\n", __func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}
