/*
 * Copyright (c) 2021-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <assert.h>
#include <arch_features.h>
#include <debug.h>
#include <irq.h>
#include <drivers/arm/arm_gic.h>
#include <drivers/arm/gic_v3.h>
#include <heap/page_alloc.h>
#include <lib/context_mgmt/context_el1.h>
#include <lib/context_mgmt/context_el2.h>
#include <pauth.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_pmu.h>
#include <host_shared_data.h>

#define SLEEP_TIME_MS		2U

extern const char *rmi_exit[];

static struct realm realm[MAX_REALM_COUNT];
static struct pmu_registers pmu_state;
static el1_ctx_regs_t el1_ctx_before = {0};
static el1_ctx_regs_t el1_ctx_after = {0};
static el2_sysregs_t el2_ctx_before = {0};
static el2_sysregs_t el2_ctx_after = {0};

#if ENABLE_PAUTH
static uint128_t pauth_keys_before[NUM_KEYS];
static uint128_t pauth_keys_after[NUM_KEYS];
#endif

/*
 * @Test_Aim@ Test RSI_PLANE_REG_READ/WRITE
 */
test_result_t host_test_realm_create_planes_register_rw(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[MAX_REC_COUNT];
	struct realm realm;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;
	struct rmi_rec_run *run;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!are_planes_supported()) {
		return TEST_RESULT_SKIPPED;
	}

	if (is_feat_52b_on_4k_2_supported() == true) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 1U)) {
		return TEST_RESULT_FAIL;
	}

	/* CMD for Plane N */
	host_shared_data_set_realm_cmd(&realm, REALM_PLANE_N_REG_RW_CMD, 1U, 0U);

	run = (struct rmi_rec_run *)realm.run[0U];

	host_realm_set_aux_plane_args(&realm, 1U, 0U);
	ret1 = host_enter_realm_execute(&realm, REALM_PLANE_N_REG_RW_CMD,
			RMI_EXIT_HOST_CALL, 0U);

	if (run->exit.exit_reason != RMI_EXIT_HOST_CALL) {
		ERROR("Rec0 error exit=0x%lx ret1=%d HPFAR=0x%lx \
				esr=0x%lx far=0x%lx\n",
				run->exit.exit_reason, ret1,
				run->exit.hpfar,
				run->exit.esr, run->exit.far);
	}
	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}
	return host_cmp_result();
}
/*
 * @Test_Aim@ Test realm payload creation with 3 Aux Planes, enter all Planes
 * Host cannot enter Aux Planes directly,
 * Host will enter P0, P0 will enter aux plane
 */
test_result_t host_test_realm_create_planes_enter_multiple_rtt(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[MAX_REC_COUNT];
	struct realm realm;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;
	struct rmi_rec_run *run;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!are_planes_supported()) {
		return TEST_RESULT_SKIPPED;
	}

	if (is_feat_52b_on_4k_2_supported() == true) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	/* Test invalid combination Tree per plane with s2ap indirect */
	if (host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT,
			sl, rec_flag, 1U, MAX_AUX_PLANE_COUNT)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, MAX_AUX_PLANE_COUNT)) {
		return TEST_RESULT_FAIL;
	}

	/* save EL1 registers */
	save_el1_sysregs_context(&el1_ctx_before);
	modify_el1_context_sysregs(&el1_ctx_before, NS_CORRUPT_EL1_REGS);
	save_el1_sysregs_context(&el1_ctx_before);

	/* save EL2 registers */
	el2_save_registers(&el2_ctx_before);

	/* CMD for Plane N */
	for (unsigned int j = 1U; j <= MAX_AUX_PLANE_COUNT; j++) {
		host_shared_data_set_realm_cmd(&realm, REALM_SLEEP_CMD, j, 0U);
		host_shared_data_set_host_val(&realm, j, 0U,
				HOST_ARG1_INDEX, SLEEP_TIME_MS);
	}

	for (unsigned int j = 1U; j <= MAX_AUX_PLANE_COUNT; j++) {
		run = (struct rmi_rec_run *)realm.run[0U];

		host_realm_set_aux_plane_args(&realm, j, 0U);
		ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
				RMI_EXIT_HOST_CALL, 0U);


		if (run->exit.exit_reason != RMI_EXIT_HOST_CALL) {
			ERROR("Rec0 error exit=0x%lx ret1=%d HPFAR=0x%lx \
					esr=0x%lx far=0x%lx\n",
					run->exit.exit_reason, ret1,
					run->exit.hpfar,
					run->exit.esr, run->exit.far);
		}
	}

	/* save EL1 registers */
	save_el1_sysregs_context(&el1_ctx_after);

	INFO("Comparing EL1 registers\n");
	ret2 = compare_el1_contexts(&el1_ctx_before, &el1_ctx_after);
	if (!ret2) {
		ERROR("NS EL1 registers corrupted\n");
		host_destroy_realm(&realm);
		return TEST_RESULT_FAIL;
	}

	/* save EL2 registers */
	el2_save_registers(&el2_ctx_after);

	INFO("Comparing EL2 registers\n");
	if (memcmp(&el2_ctx_before, &el2_ctx_after, sizeof(el2_sysregs_t))) {
		ERROR("NS EL2 registers corrupted\n");
		host_destroy_realm(&realm);
		return TEST_RESULT_FAIL;
	}

	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

/*
 * @Test_Aim@ Test realm payload creation with 3 Aux Planes, enter all Planes
 * Host cannot enter Aux Planes directly,
 * Host will enter P0, P0 will enter aux plane.
 * This test also validates that NS EL1 and NS EL2 registers are preserved by RMM.
 * This test also validates that s2por_el1 register is not accessible in realm.
 */
test_result_t host_test_realm_create_planes_enter_single_rtt(void)
{
	bool ret1, ret2;
	u_register_t rec_flag = RMI_RUNNABLE;
	struct realm realm;
	u_register_t feature_flag0 = 0UL, feature_flag1;
	long sl = RTT_MIN_LEVEL;
	struct rmi_rec_run *run;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!are_planes_supported() || !is_single_rtt_supported()) {
		return TEST_RESULT_SKIPPED;
	}

	if (is_feat_52b_on_4k_2_supported() == true) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	/* use single RTT for all planes */
	feature_flag0 |= INPLACE(RMI_FEATURE_REGISTER_0_PLANE_RTT,
			RMI_PLANE_RTT_SINGLE);

	feature_flag1 = RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, feature_flag1, sl, &rec_flag, 1U, MAX_AUX_PLANE_COUNT)) {
		return TEST_RESULT_FAIL;
	}

	/* CMD for Plane N */
	for (unsigned int j = 1U; j <= MAX_AUX_PLANE_COUNT; j++) {
		host_shared_data_set_realm_cmd(&realm, REALM_SLEEP_CMD, j, 0U);
		host_shared_data_set_host_val(&realm, j, 0U,
			HOST_ARG1_INDEX, SLEEP_TIME_MS);
	}

	save_el1_sysregs_context(&el1_ctx_before);
	modify_el1_context_sysregs(&el1_ctx_before, NS_CORRUPT_EL1_REGS);
	save_el1_sysregs_context(&el1_ctx_before);

	el2_save_registers(&el2_ctx_before);

	for (unsigned int j = 1U; j <= MAX_AUX_PLANE_COUNT; j++) {
		run = (struct rmi_rec_run *)realm.run[0U];

		host_realm_set_aux_plane_args(&realm, j, 0U);
		ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
				RMI_EXIT_HOST_CALL, 0U);

		if (!ret1) {
			ERROR("Rec0 error exit=0x%lx ret1=%d HPFAR=0x%lx \
				esr=0x%lx far=0x%lx\n",
				run->exit.exit_reason, ret1,
				run->exit.hpfar,
				run->exit.esr, run->exit.far);
			goto destroy_realm;
		}
	}

	save_el1_sysregs_context(&el1_ctx_before);

	INFO("Comparing EL1 registers\n");
	ret1 = compare_el1_contexts(&el1_ctx_before, &el1_ctx_after);

	if (!ret1) {
		ERROR("NS EL1 registers corrupted\n");
		goto destroy_realm;
	}

	el2_save_registers(&el2_ctx_after);

	INFO("Comparing EL2 registers\n");
	if (memcmp(&el2_ctx_before, &el2_ctx_after, sizeof(el2_sysregs_t))) {
		ERROR("NS EL2 registers corrupted\n");
		goto destroy_realm;
	}

	/* Test that realm cannot modify s2por_el1 */
	ret1 = host_enter_realm_execute(&realm, REALM_S2POE_ACCESS,
			RMI_EXIT_HOST_CALL, 0U);

	if (!ret1) {
		ERROR("S2POR_EL1 reg access test failed\n");
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

/*
 * @Test_Aim@ Test realm payload creation, execution and destruction iteratively
 */
test_result_t host_test_realm_create_enter(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[MAX_REC_COUNT];
	struct realm realm;
	u_register_t feature_flag0 = 0UL, feature_flag1 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (is_single_rtt_supported()) {
		/* Use s2ap encoding indirect */
		INFO("Using S2ap indirect for single plane\n");
		feature_flag1 = RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	for (unsigned int i = 0U; i < 5U; i++) {
		/* Run random Rec */
		unsigned int run_num = (unsigned int)rand() % MAX_REC_COUNT;

		if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, feature_flag1, sl, rec_flag, MAX_REC_COUNT, 0U)) {
			return TEST_RESULT_FAIL;
		}

		host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, run_num,
				HOST_ARG1_INDEX, SLEEP_TIME_MS);

		ret1 = host_enter_realm_execute(&realm, REALM_SLEEP_CMD, RMI_EXIT_HOST_CALL,
						run_num);
		ret2 = host_destroy_realm(&realm);

		if (!ret1 || !ret2) {
			ERROR("%s(): enter=%d destroy=%d\n",
			__func__, ret1, ret2);
			return TEST_RESULT_FAIL;
		}
	}

	return host_cmp_result();
}

/*
 * @Test_Aim@ Test realm payload creation and execution
 */
test_result_t host_test_realm_rsi_version(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag0 = 0U;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_GET_RSI_VERSION, RMI_EXIT_HOST_CALL, 0U);
	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

/*
 * @Test_Aim@ Create realm with multiple rec
 * Test PAuth registers are preserved for each rec
 */
test_result_t host_realm_enable_pauth(void)
{
#if ENABLE_PAUTH == 0
	return TEST_RESULT_SKIPPED;
#else
	bool ret1, ret2;
	u_register_t rec_flag[MAX_REC_COUNT];
	struct realm realm;
	u_register_t feature_flag0 = 0U;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	pauth_test_lib_fill_regs_and_template(pauth_keys_before);
	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, MAX_REC_COUNT, 0U)) {
		return TEST_RESULT_FAIL;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		ret1 = host_enter_realm_execute(&realm, REALM_PAUTH_SET_CMD,
				RMI_EXIT_HOST_CALL, i);

		if (!ret1) {
			ERROR("Pauth set cmd failed\n");
			break;
		}
		/* Re-enter Realm to compare PAuth registers. */
		ret1 = host_enter_realm_execute(&realm, REALM_PAUTH_CHECK_CMD,
			RMI_EXIT_HOST_CALL, i);
		if (!ret1) {
			ERROR("Pauth check cmd failed\n");
			break;
		}
	}

	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
				__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	/* Check if PAuth keys are preserved. */
	if (!pauth_test_lib_compare_template(pauth_keys_before, pauth_keys_after)) {
		ERROR("%s(): NS PAuth keys not preserved\n",
				__func__);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
#endif
}

/*
 * @Test_Aim@ Test PAuth fault in Realm
 */
test_result_t host_realm_pauth_fault(void)
{
#if ENABLE_PAUTH == 0
	return TEST_RESULT_SKIPPED;
#else
	bool ret1, ret2;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag0 = 0U;
	long sl = RTT_MIN_LEVEL;

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_PAUTH_FAULT, RMI_EXIT_HOST_CALL, 0U);
	ret2 = host_destroy_realm(&realm);

	if (!ret1) {
		ERROR("%s(): enter=%d destroy=%d\n",
				__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
#endif
}

/*
 * This function is called on REC exit due to IRQ.
 * By checking Realm PMU state in RecExit object this finction
 * detects if the exit was caused by PMU interrupt. In that
 * case it disables physical PMU interrupt and sets virtual
 * PMU interrupt pending by writing to gicv3_lrs attribute
 * of RecEntry object and re-enters the Realm.
 *
 * @return true in case of PMU interrupt, false otherwise.
 */
static bool host_realm_handle_irq_exit(struct realm *realm_ptr,
		unsigned int rec_num)
{
	struct rmi_rec_run *run = (struct rmi_rec_run *)realm_ptr->run[rec_num];

	/* Check PMU overflow status */
	if (run->exit.pmu_ovf_status == RMI_PMU_OVERFLOW_ACTIVE) {
		unsigned int host_call_result;
		u_register_t exit_reason, retrmm;
		int ret;

		tftf_irq_disable(PMU_PPI);
		ret = tftf_irq_unregister_handler(PMU_PPI);
		if (ret != 0) {
			ERROR("Failed to %sregister IRQ handler\n", "un");
			return false;
		}

		/* Inject PMU virtual interrupt */
		run->entry.gicv3_lrs[0] =
			ICH_LRn_EL2_STATE_Pending | ICH_LRn_EL2_Group_1 |
			(PMU_VIRQ << ICH_LRn_EL2_vINTID_SHIFT);

		/* Re-enter Realm */
		INFO("Re-entering Realm with vIRQ %lu pending\n", PMU_VIRQ);

		retrmm = host_realm_rec_enter(realm_ptr, &exit_reason,
						&host_call_result, rec_num);
		if ((retrmm == REALM_SUCCESS) &&
		    (exit_reason == RMI_EXIT_HOST_CALL) &&
		    (host_call_result == TEST_RESULT_SUCCESS)) {
			return true;
		}

		ERROR("%s() failed, ret=%lx host_call_result %u\n",
			"host_realm_rec_enter", retrmm, host_call_result);
	}
	return false;
}

/*
 * @Test_Aim@ Test realm PMU
 *
 * This function tests PMU functionality in Realm
 *
 * @cmd: PMU test number
 * @return test result
 */
static test_result_t host_test_realm_pmuv3(uint8_t cmd)
{
	struct realm realm;
	u_register_t feature_flag0, rmm_feat_reg0;
	unsigned int num_cnts;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	bool ret1, ret2;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/* Get Max PMU counter implemented through RMI_FEATURES */
	if (host_rmi_features(0UL, &rmm_feat_reg0) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return TEST_RESULT_FAIL;
	}

	num_cnts = EXTRACT(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS, rmm_feat_reg0);
	host_set_pmu_state(&pmu_state);

	feature_flag0 = RMI_FEATURE_REGISTER_0_PMU_EN |
			INPLACE(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS, num_cnts);

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 |= RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, cmd,
					((cmd == REALM_PMU_CYCLE_INTERRUPT) ||
					 (cmd == REALM_PMU_EVENT_INTERRUPT)) ?
					RMI_EXIT_IRQ : RMI_EXIT_HOST_CALL, 0U);
	if (!ret1 || ((cmd != REALM_PMU_CYCLE_INTERRUPT) &&
		      (cmd != REALM_PMU_EVENT_INTERRUPT))) {
		goto test_exit;
	}

	ret1 = host_realm_handle_irq_exit(&realm, 0U);

test_exit:
	ret2 = host_destroy_realm(&realm);
	if (!ret1 || !ret2) {
		ERROR("%s() enter=%u destroy=%u\n", __func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	if (!host_check_pmu_state(&pmu_state)) {
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

/*
 * Test if the cycle counter works in Realm with NOPs execution
 */
test_result_t host_realm_pmuv3_cycle_works(void)
{
	return host_test_realm_pmuv3(REALM_PMU_CYCLE);
}

/*
 * Test if the event counter works in Realm with NOPs execution
 */
test_result_t host_realm_pmuv3_event_works(void)
{
	if (GET_PMU_CNT == 0) {
		tftf_testcase_printf("No event counters implemented\n");
		return TEST_RESULT_SKIPPED;
	}

	return host_test_realm_pmuv3(REALM_PMU_EVENT);
}

/*
 * Test if Realm entering/exiting RMM preserves PMU state
 */
test_result_t host_realm_pmuv3_rmm_preserves(void)
{
	return host_test_realm_pmuv3(REALM_PMU_PRESERVE);
}

/*
 * IRQ handler for PMU_PPI #23.
 * This handler should not be called, as RMM handles IRQs.
 */
static int host_overflow_interrupt(void *data)
{
	(void)data;

	assert(false);
	return -1;
}

static test_result_t host_realm_pmuv3_overflow_interrupt(uint8_t cmd)
{
	test_result_t ret;

	/* Register PMU IRQ handler */
	if (tftf_irq_register_handler(PMU_PPI, host_overflow_interrupt) != 0) {
		tftf_testcase_printf("Failed to %sregister IRQ handler\n", "");
		return TEST_RESULT_FAIL;
	}

	tftf_irq_enable(PMU_PPI, GIC_HIGHEST_NS_PRIORITY);

	ret = host_test_realm_pmuv3(cmd);
	if (ret != TEST_RESULT_SUCCESS) {
		tftf_irq_disable(PMU_PPI);
		if (tftf_irq_unregister_handler(PMU_PPI) != 0) {
			ERROR("Failed to %sregister IRQ handler\n", "un");
			return TEST_RESULT_FAIL;
		}
		return ret;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test PMU cycle counter interrupt functionality in Realm
 */
test_result_t host_realm_pmuv3_cycle_overflow_interrupt(void)
{
	return host_realm_pmuv3_overflow_interrupt(REALM_PMU_CYCLE_INTERRUPT);
}

/*
 * Test PMU event counter interrupt functionality in Realm
 */
test_result_t host_realm_pmuv3_event_overflow_interrupt(void)
{
	if (GET_PMU_CNT == 0) {
		tftf_testcase_printf("No event counters implemented\n");
		return TEST_RESULT_SKIPPED;
	}

	return host_realm_pmuv3_overflow_interrupt(REALM_PMU_EVENT_INTERRUPT);
}

/*
 * Test aim to create, enter and destroy MAX_REALM_COUNT realms
 * Host created MAX_REALM_COUNT realms with MAX_REC_COUNT rec each
 * Host enters all recs sequentially, starting from the random rec
 * Verifies all realms returned success
 * Destroys all realms
 */
test_result_t host_test_multiple_realm_create_enter(void)
{
	bool ret;
	u_register_t rec_flag[MAX_REC_COUNT];
	u_register_t feature_flag0 = 0U;
	long sl = RTT_MIN_LEVEL;
	unsigned int run_rec[MAX_REALM_COUNT], num;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	for (num = 0U; num < MAX_REALM_COUNT; num++) {
		/* Generate random REC start number */
		run_rec[num] = (unsigned int)rand() % MAX_REC_COUNT;

		ret = host_create_activate_realm_payload(&realm[num],
							(u_register_t)REALM_IMAGE_BASE,
							feature_flag0, 0U, sl, rec_flag,
							MAX_REC_COUNT, 0U);
		if (!ret) {
			goto destroy_realms;
		}
	}

	for (unsigned int j = 0U; j < MAX_REC_COUNT; j++) {
		for (unsigned int i = 0U; i < MAX_REALM_COUNT; i++) {
			host_shared_data_set_host_val(&realm[i], PRIMARY_PLANE_ID, run_rec[i],
					HOST_ARG1_INDEX, SLEEP_TIME_MS);

			ret = host_enter_realm_execute(&realm[i], REALM_SLEEP_CMD,
							RMI_EXIT_HOST_CALL, run_rec[i]);
			if (!ret) {
				goto destroy_realms;
			}

			/* Increment REC number */
			if (++run_rec[i] == MAX_REC_COUNT) {
				run_rec[i] = 0U;
			}
		}
	}

destroy_realms:
	for (unsigned int i = 0U; i < num; i++) {
		if (!host_destroy_realm(&realm[i])) {
			ERROR("Realm #%u destroy failed\n", i);
			ret = false;
		}
	}
	return (ret ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL);
}

/*
 * Test set_ripas functionality in Realm
 * Test allocates 3 PAGES and passes to Realm
 * Realm: verifies that initial RIPAS of these pages is EMPTY
 * Realm: requests RIPAS Change to RAM
 * Host: attempt to change RIPAS outside requested range, verifies error generated by RMM
 * Host: changes RIPAS of first PAGE and re-enters Realm
 * Realm: tracks progress and requests RIPAS Change to RAM till all pages are complete
 * Host: changes RIPAS of requested PAGE and re-enters Realm
 * Realm: verifies all PAGES are set to RIPAS=RAM
 */
test_result_t host_realm_set_ripas(void)
{
	bool ret1, ret2;
	u_register_t ret, base, new_base, exit_reason;
	unsigned int host_call_result = TEST_RESULT_FAIL;
	struct realm realm;
	struct rmi_rec_run *run;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	u_register_t test_page_num = 3U;
	u_register_t feature_flag0 = 0U;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG1_INDEX, 10U);
	ret1 = host_enter_realm_execute(&realm, REALM_SLEEP_CMD, RMI_EXIT_HOST_CALL, 0U);
	base = (u_register_t)page_alloc(PAGE_SIZE * test_page_num);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG2_INDEX,
			base + (PAGE_SIZE * test_page_num));

	for (unsigned int i = 0U; i < test_page_num; i++) {
		ret = host_realm_delegate_map_protected_data(true, &realm,
				base + (PAGE_SIZE * i), PAGE_SIZE,
				base + (PAGE_SIZE * i));
		if (ret != REALM_SUCCESS) {
			ERROR("host_realm_delegate_map_protected_data failed\n");
			goto destroy_realm;
		}
	}
	ret1 = host_enter_realm_execute(&realm, REALM_SET_RIPAS_CMD,
			RMI_EXIT_RIPAS_CHANGE, 0U);
	if (!ret1) {
		ERROR("Rec enter failed\n");
		goto destroy_realm;
	}
	run = (struct rmi_rec_run *)realm.run[0U];

	/* Attemp to set ripas for IPA out of requested range, expect error */
	ret = host_rmi_rtt_set_ripas(realm.rd,
				     realm.rec[0U],
				     run->exit.ripas_base - PAGE_SIZE,
				     run->exit.ripas_base,
				     &new_base);
	if (ret != RMI_ERROR_INPUT || new_base != 0U) {
		ERROR("host_rmi_rtt_set_ripas should have failed ret = 0x%lx\n", ret);
		goto destroy_realm;
	}

	while (run->exit.ripas_base <= base + (PAGE_SIZE * test_page_num)) {
		INFO("host_rmi_rtt_set_ripas ripas_base=0x%lx\n",
				run->exit.ripas_base);
		ret = host_rmi_rtt_set_ripas(realm.rd,
					     realm.rec[0U],
					     run->exit.ripas_base,
					     run->exit.ripas_base + PAGE_SIZE,
					     &new_base);
		if (ret != RMI_SUCCESS) {
			ERROR("host_rmi_rtt_set_ripas failed ret = 0x%lx\n", ret);
			goto destroy_realm;
		}
		ret = host_realm_rec_enter(&realm,
			&exit_reason, &host_call_result, 0U);
		if (ret != RMI_SUCCESS || exit_reason != RMI_EXIT_RIPAS_CHANGE) {
			goto destroy_realm;
		}
	}
destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	page_free(base);
	return host_call_result;
}

/*
 * Test set_ripas reject functionality in Realm
 * Test allocates PAGE and passes to Realm
 * Realm: verifies that initial RIPAS of page is EMPTY
 * Realm: requests RIPAS Change to RAM
 * Host: changes rejects RIPAS change and enters Realm
 * Realm: verifies REJECT response
 * Realm: verifies PAGE  has RIPAS=EMPTY
 */

test_result_t host_realm_reject_set_ripas(void)
{
	bool ret1, ret2;
	u_register_t ret, exit_reason;
	unsigned int host_call_result = TEST_RESULT_FAIL;
	struct realm realm;
	struct rmi_rec_run *run;
	u_register_t rec_flag[1] = {RMI_RUNNABLE}, base;
	u_register_t feature_flag0 = 0U;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	base = (u_register_t)page_alloc(PAGE_SIZE);

	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, base);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failede\n");
		goto destroy_realm;
	}
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG1_INDEX, base);
	ret1 = host_enter_realm_execute(&realm, REALM_REJECT_SET_RIPAS_CMD,
			RMI_EXIT_RIPAS_CHANGE, 0U);

	if (!ret1) {
		ERROR("Rec did not request RIPAS change\n");
		goto destroy_realm;
	}
	run = (struct rmi_rec_run *)realm.run[0];
	if (run->exit.ripas_base != base) {
		ERROR("Rec requested wrong exit.ripas_base\n");
		goto destroy_realm;
	}
	run->entry.flags = REC_ENTRY_FLAG_RIPAS_RESPONSE_REJECT;
	ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
	if (ret != RMI_SUCCESS || exit_reason != RMI_EXIT_HOST_CALL) {
		ERROR("Re-enter rec failed exit_reason=0x%lx", exit_reason);
	}

destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_call_result;
}

/*
 * Test aims to generate REALM Exit due to abort
 * when access page with RIPAS=DESTOYED HIPAS=ASSIGNED
 * Host maps a protected page (calls data_create) when realm is in new state
 * Initial state of PAGE is RIPAS=RAM HIPAS=ASSIGNED
 * Host calls data_destroy, new state HIPAS=UNASSIGNED RIPAS=DESTROYED
 * Enter Realm, Rec0 executes from page, and Rec1 reads the page
 * Realm should trigger an Instr/Data abort, and will exit to Host.
 * The Host verifies exit reason is Instr/Data abort.
 * Repeat the test for Plane N.
 */
test_result_t host_realm_abort_unassigned_destroyed(void)
{
	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, data, top, num_aux_planes = 0UL;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	u_register_t feature_flag0 = 0UL, feature_flag1 = 0UL;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE}, base;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	/* This test is skipped if S2POE is not supported to keep test simple */
	if (are_planes_supported() && is_single_rtt_supported()) {
		num_aux_planes = 1UL;

		/* use single RTT for all planes */
		feature_flag0 |= INPLACE(RMI_FEATURE_REGISTER_0_PLANE_RTT,
			RMI_PLANE_RTT_SINGLE);

		feature_flag1 = RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;
	}

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, feature_flag1, sl, rec_flag, 4U, num_aux_planes)) {
		return TEST_RESULT_FAIL;
	}

	base = (u_register_t)page_alloc(PAGE_SIZE);

	run = (struct rmi_rec_run *)realm.run[0];

	/* DATA_CREATE
	 * Copies content of TFTF_BASE in newly created page, any PA can be used for dummy copy
	 * maps 1:1 IPA:PA
	 */
	ret = host_realm_delegate_map_protected_data(false, &realm, base, PAGE_SIZE, TFTF_BASE);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}
	INFO("Initial state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG3_INDEX, base);

	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	if (ret != RMI_SUCCESS || data != base) {
		ERROR("host_rmi_data_destroy failed\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED) {
		ERROR("Wrong state after host_rmi_data_destroy\n");
		goto undelegate_destroy;
	}

	INFO("New state4 base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);

	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto undelegate_destroy;
	}

	/* Realm0 expect rec exit due to Instr Abort unassigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
		RMI_EXIT_SYNC, 0U);

	/* ESR.EC == 0b100000 Instruction Abort from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
			|| (EC_BITS(run->exit.esr) != EC_IABORT_LOWER_EL)
			|| ((run->exit.esr & ISS_IFSC_MASK) < FSC_L0_TRANS_FAULT)
			|| ((run->exit.esr & ISS_IFSC_MASK) > FSC_L3_TRANS_FAULT)
			|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault ESR=0x%lx\n", run->exit.esr);
		goto undelegate_destroy;
	}
	INFO("IA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);

	run = (struct rmi_rec_run *)realm.run[1];

	/* Realm1 expect rec exit due to Data Abort unassigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_SYNC, 1U);

	/* ESR.EC == 0b100100 Data Abort exception from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault\n");
		goto undelegate_destroy;
	}
	INFO("DA FAR=0x%lx, HPFAR=0x%lx ESR= 0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);

	if (num_aux_planes == 0U) {
		res = TEST_RESULT_SUCCESS;
		goto undelegate_destroy;
	}

	INFO("Running test on Plane 1\n");

	/*
	 * Arg used by Plane 1 Rec 2/3
	 * Plane 1 will fetch base, access it and get an abort causing rec exit
	 */
	host_shared_data_set_host_val(&realm, 1U, 2U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, 3U, HOST_ARG3_INDEX, base);

	/* Arg for Plane0 instruction to enter Plane1 on Rec 2,3 */
	host_realm_set_aux_plane_args(&realm, 1U, 2U);
	host_realm_set_aux_plane_args(&realm, 1U, 3U);

	/* Test cmd for Plane 1 Rec 2/3 */
	host_shared_data_set_realm_cmd(&realm, REALM_INSTR_FETCH_CMD, 1U, 2U);
	host_shared_data_set_realm_cmd(&realm, REALM_DATA_ACCESS_CMD, 1U, 3U);


	run = (struct rmi_rec_run *)realm.run[2];

	/* Rec2 expect rec exit due to Instr Abort unassigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
		RMI_EXIT_SYNC, 2U);

	/* ESR.EC == 0b100000 Instruction Abort from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
			|| (EC_BITS(run->exit.esr) != EC_IABORT_LOWER_EL)
			|| ((run->exit.esr & ISS_IFSC_MASK) < FSC_L0_TRANS_FAULT)
			|| ((run->exit.esr & ISS_IFSC_MASK) > FSC_L3_TRANS_FAULT)
			|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Plane1 Rec2 did not fault ESR=0x%lx\n", run->exit.esr);
		goto undelegate_destroy;
	}
	INFO("Plane1 IA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);

	run = (struct rmi_rec_run *)realm.run[3];

	/* Rec3 expect rec exit due to Data Abort unassigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
			RMI_EXIT_SYNC, 3U);

	/* ESR.EC == 0b100100 Data Abort exception from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Plane1 Rec3 did not fault\n");
		goto undelegate_destroy;
	}
	INFO("Plane1 DA FAR=0x%lx, HPFAR=0x%lx ESR= 0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);
	res = TEST_RESULT_SUCCESS;

undelegate_destroy:
	ret = host_rmi_granule_undelegate(base);
destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to generate REALM Exit due to Abort
 * when access page with RIPAS=RAM HIPAS=UNASSIGNED
 * Host allocates a PAGE, calls init_ripas when realm is in new state
 * Initial state of PAGE is RIPAS=RAM HIPAS=UNASSIGNED
 * Enter Realm, REC0 executes from page, and REC1 reads the page
 * Realm should trigger an Instr/Data abort, and will exit to Host.
 * Host verifies exit reason is Instr/Data abort.
 * Repeat the test for Plane N.
 */
test_result_t host_realm_abort_unassigned_ram(void)
{
	bool ret1, ret2;
	u_register_t ret, top, num_aux_planes = 0UL;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	u_register_t feature_flag0 = 0UL, feature_flag1 = 0UL;
	long sl = RTT_MIN_LEVEL;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE}, base;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	/* Test is skipped if S2POE is not supported to keep test simple */
	if (are_planes_supported() && is_single_rtt_supported()) {
		num_aux_planes = 1UL;

		/* use single RTT for all planes */
		feature_flag0 |= INPLACE(RMI_FEATURE_REGISTER_0_PLANE_RTT,
			RMI_PLANE_RTT_SINGLE);

		feature_flag1 = RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;
	}

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, feature_flag1, sl, rec_flag, 4U, num_aux_planes)) {
		return TEST_RESULT_FAIL;
	}

	/* This is dummy allocation to get a base address */
	base = (u_register_t)page_alloc(PAGE_SIZE);

	run = (struct rmi_rec_run *)realm.run[0];

	/* Set RIPAS of PAGE to RAM */
	ret = host_rmi_rtt_init_ripas(realm.rd, base, base + PAGE_SIZE, &top);
	if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
		/* Create missing RTTs till L3 and retry */
		int8_t level = RMI_RETURN_INDEX(ret);

		ret = host_rmi_create_rtt_levels(&realm, base,
				(u_register_t)level, 3U);
		if (ret != RMI_SUCCESS) {
			ERROR("host_rmi_create_rtt_levels failed\n");
			goto destroy_realm;
		}

		ret = host_rmi_rtt_init_ripas(realm.rd, base, base + PAGE_SIZE, &top);
	}

	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx line=%u\n",
			"host_rmi_rtt_init_ripas", ret, __LINE__);
		goto destroy_realm;
	}
	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto destroy_realm;
	}

	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}
	INFO("Initial state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG3_INDEX, base);

	/* Rec0 expect rec exit due to Instr Abort unassigned ram page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
			RMI_EXIT_SYNC, 0U);

	/* ESR.EC == 0b100000 Instruction Abort from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
			|| (EC_BITS(run->exit.esr) != EC_IABORT_LOWER_EL)
			|| ((run->exit.esr & ISS_IFSC_MASK) < FSC_L0_TRANS_FAULT)
			|| ((run->exit.esr & ISS_IFSC_MASK) > FSC_L3_TRANS_FAULT)
			|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_realm;
	}
	INFO("IA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);
	run = (struct rmi_rec_run *)realm.run[1];

	/* Rec1 expect rec exit due to Data Abort unassigned ram page */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_SYNC, 1U);

	/* ESR.EC == 0b100100 Data Abort exception from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_realm;
	}
	INFO("DA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);

	if (num_aux_planes == 0U) {
		res = TEST_RESULT_SUCCESS;
		goto destroy_realm;
	}

	INFO("Running test on Plane 1\n");

	/*
	 * Arg used by Plane 1 Rec 2/3
	 * Plane 1 will access the base, causing rec exit due to abort
	 */
	host_shared_data_set_host_val(&realm, 1U, 2U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, 3U, HOST_ARG3_INDEX, base);

	/* Arg for Plane0 to enter Plane1 on Rec 2,3 */
	host_realm_set_aux_plane_args(&realm, 1U, 2U);
	host_realm_set_aux_plane_args(&realm, 1U, 3U);

	/* Test cmd for Plane 1 Rec 2/3 */
	host_shared_data_set_realm_cmd(&realm, REALM_INSTR_FETCH_CMD, 1U, 2U);
	host_shared_data_set_realm_cmd(&realm, REALM_DATA_ACCESS_CMD, 1U, 3U);

	/* Rec2 expect rec exit due to Instr Abort unassigned ram page */
	ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
			RMI_EXIT_SYNC, 2U);

	run = (struct rmi_rec_run *)realm.run[2];

	/* ESR.EC == 0b100000 Instruction Abort from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
			|| (EC_BITS(run->exit.esr) != EC_IABORT_LOWER_EL)
			|| ((run->exit.esr & ISS_IFSC_MASK) < FSC_L0_TRANS_FAULT)
			|| ((run->exit.esr & ISS_IFSC_MASK) > FSC_L3_TRANS_FAULT)
			|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		INFO("Plane1 Rec2 did not fault FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n",
				run->exit.far, run->exit.hpfar,	run->exit.esr);
		goto destroy_realm;
	}
	INFO("Plane1 IA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);

	run = (struct rmi_rec_run *)realm.run[3];

	/* Rec3 expect rec exit due to Data Abort unassigned ram page */
	ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
			RMI_EXIT_SYNC, 3U);

	/* ESR.EC == 0b100100 Data Abort exception from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Plane1 Rec3 did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_realm;
	}
	INFO("Plane1 DA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
			run->exit.esr);
	res = TEST_RESULT_SUCCESS;

destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to generate REALM Exit due to Abort
 * when access page with RIPAS=DESTOYED HIPAS=Assigned
 * Host maps a protected page (calls data_create) when realm is in new state
 * initial state of PAGE is RIPAS=RAM HIPAS=ASSIGNED
 * Host calls data_destroy, new state HIPAS=UNASSIGNED RIPAS=DESTROYED
 * Host calls data_create_unknown, new state HIPAS=ASSIGNED RIPAS=DESTROYED
 * Enter Realm, REC0 executes from page, and REC1 reads the page
 * Realm should trigger an Instr/Data abort, and will exit to Host.
 * The Host verifies exit reason is Instr/Data abort.
 * Repeat the test for Plane N.
 */
test_result_t host_realm_abort_assigned_destroyed(void)
{
	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, top, data;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	u_register_t feature_flag0 = 0UL, num_aux_planes = 0UL;
	u_register_t feature_flag1 = 0UL;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE}, base;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	/* Test is skipped if S2POE is not supported to keep test simple */
	if (are_planes_supported() && is_single_rtt_supported()) {
		num_aux_planes = 1UL;

		/* use single RTT for all planes */
		feature_flag0 |= INPLACE(RMI_FEATURE_REGISTER_0_PLANE_RTT,
			RMI_PLANE_RTT_SINGLE);

		feature_flag1 = RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;
	}

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, feature_flag1, sl, rec_flag, 4U, num_aux_planes)) {
		return TEST_RESULT_FAIL;
	}

	base = (u_register_t)page_alloc(PAGE_SIZE);
	run = (struct rmi_rec_run *)realm.run[0];

	/* DATA_CREATE */
	/* Copied content of TFTF_BASE to new page, can use any adr, maps 1:1 IPA:PA */
	ret = host_realm_delegate_map_protected_data(false, &realm, base, PAGE_SIZE, TFTF_BASE);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after data create\n");
		goto destroy_realm;
	}
	INFO("Initial state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG3_INDEX, base);

	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto destroy_realm;
	}

	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	if (ret != RMI_SUCCESS || data != base) {
		ERROR("host_rmi_data_destroy failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED) {
		ERROR("Wrong state after host_rmi_data_destroy\n");
		goto destroy_realm;
	}
	ret = host_rmi_granule_undelegate(base);

	/* DATA_CREATE_UNKNOWN */
	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failede\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_DESTROYED)) {
		ERROR("wrong state after data create unknown\n");
		goto destroy_data;
	}

	/* Rec0, expect rec exit due to Instr Abort assigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
		RMI_EXIT_SYNC, 0U);

	/* ESR.EC == 0b100000 Instruction Abort from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_IABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_IFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_IFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_data;
	}
	INFO("IA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
				run->exit.esr);
	run = (struct rmi_rec_run *)realm.run[1];

	/* Rec1  expect rec exit due to Data Abort  assigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_SYNC, 1U);

	/* ESR.EC == 0b100100 Data Abort exception from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_data;
	}
	INFO("DA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
				run->exit.esr);

	if (num_aux_planes == 0U) {
		res = TEST_RESULT_SUCCESS;
		goto destroy_data;
	}

	INFO("Running test on Plane 1\n");

	/*
	 * Args used by Plane 1, Rec 2/3
	 * Plane1 accesses base, causes rec exit due to abort
	 */
	host_shared_data_set_host_val(&realm, 1U, 2U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, 3U, HOST_ARG3_INDEX, base);

	/* Arg for Plane0 to enter Plane1 on Rec 2,3 */
	host_realm_set_aux_plane_args(&realm, 1U, 2U);
	host_realm_set_aux_plane_args(&realm, 1U, 3U);

	/* Test cmd for Plane 1, Rec 2/3 */
	host_shared_data_set_realm_cmd(&realm, REALM_INSTR_FETCH_CMD, 1U, 2U);
	host_shared_data_set_realm_cmd(&realm, REALM_DATA_ACCESS_CMD, 1U, 3U);

	run = (struct rmi_rec_run *)realm.run[2];

	/* Rec2, expect rec exit due to Instr Abort assigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
		RMI_EXIT_SYNC, 2U);

	/* ESR.EC == 0b100000 Instruction Abort from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_IABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_IFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_IFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Plane1 Rec2 did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_data;
	}
	INFO("Plane1 IA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
				run->exit.esr);
	run = (struct rmi_rec_run *)realm.run[3];

	/* Rec3  expect rec exit due to Data Abort  assigned destroyed page */
	ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
			RMI_EXIT_SYNC, 3U);

	/* ESR.EC == 0b100100 Data Abort exception from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Plane1 Rec3 did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_data;
	}
	INFO("Plane1 DA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
				run->exit.esr);
	res = TEST_RESULT_SUCCESS;

destroy_data:
	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	ret = host_rmi_granule_undelegate(base);
destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to generate SEA in Realm by accessing
 * PAGE with HIPAS=assigned/unassigned and RIPAS=EMPTY
 * Host creates and executes 4 recs to generate SEA
 * Rec exception handler runs and returns back ESR to Host
 * Host validates ESR
 * Rec0 generated IA unassigned empty
 * Rec1 generated DA unassigned empty
 * Rec2 generated IA for assigned empty
 * Rec3 generated DA for assigned empty
 * Repeat the test for Plane1.
 * Accessing RIPAS=EMPTY causes plane exit to P0
 */
test_result_t host_realm_sea_empty(void)
{
	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base, esr, num_aux_planes = 0UL, far;
	u_register_t feature_flag1 = 0UL;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
				   RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	/* Test are skipped if S2POE is not supported to keep test simple */
	if (are_planes_supported() && is_single_rtt_supported()) {
		num_aux_planes = 1UL;

		/* use single RTT for all planes */
		feature_flag0 |= INPLACE(RMI_FEATURE_REGISTER_0_PLANE_RTT,
			RMI_PLANE_RTT_SINGLE);

		feature_flag1 = RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, feature_flag1, sl, rec_flag, 8U, num_aux_planes)) {
		return TEST_RESULT_FAIL;
	}

	base = (u_register_t)page_alloc(PAGE_SIZE * 2U);

	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (rtt.walk_level != 3U) {
		/* Create L3 RTT */
		host_rmi_create_rtt_levels(&realm, base, rtt.walk_level, 3U);
	}

	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY) ||
			(rtt.walk_level != 3U)) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}

	ret = host_rmi_rtt_readentry(realm.rd, base + PAGE_SIZE, 3L, &rtt);
	if (rtt.walk_level != 3U) {
		/* Create L3 RTT */
		host_rmi_create_rtt_levels(&realm, base + PAGE_SIZE, rtt.walk_level, 3U);
	}

	ret = host_rmi_rtt_readentry(realm.rd, base + PAGE_SIZE, 3L, &rtt);
	if (rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY) ||
			(rtt.walk_level != 3U)) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}

	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 2U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 3U, HOST_ARG3_INDEX, base);

	/* Rec0 expect IA due to SEA unassigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
			RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("Rec0 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, 0U, 0U, HOST_ARG2_INDEX);
	if (((esr & ISS_IFSC_MASK) != IFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_IABORT_CUR_EL)) {
		ERROR("Rec0 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec0 ESR=0x%lx\n", esr);

	/* Rec1 expect DA due to SEA unassigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_HOST_CALL, 1U);
	if (!ret1) {
		ERROR("Rec1 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, 0U, 1U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_DABORT_CUR_EL)) {
		ERROR("Rec1 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec1 ESR=0x%lx\n", esr);

	/* DATA_CREATE_UNKNOWN */
	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_EMPTY)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}
	INFO("state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);

	/* Rec2 expect IA due to SEA assigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
		RMI_EXIT_HOST_CALL, 2U);

	if (!ret1) {
		ERROR("Rec2 did not fault\n");
		goto undelegate_destroy;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, 0U, 2U, HOST_ARG2_INDEX);
	if (((esr & ISS_IFSC_MASK) != IFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_IABORT_CUR_EL)) {
		ERROR("Rec2 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec2 ESR=0x%lx\n", esr);

	/* Rec3 expect DA due to SEA unassigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_HOST_CALL, 3U);
	if (!ret1) {
		ERROR("Rec3 did not fault\n");
		goto undelegate_destroy;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, 0U, 3U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_DABORT_CUR_EL)) {
		ERROR("Rec3 incorrect ESR=0x%lx\n", esr);
	}
	INFO("Rec3 ESR=0x%lx\n", esr);

	if (num_aux_planes == 0U) {
		res = TEST_RESULT_SUCCESS;
		goto undelegate_destroy;
	}

	host_rmi_granule_undelegate(base);

	INFO("Running test on Plane 1\n");
	base += PAGE_SIZE;

	/*
	 * Args used by Plane 1, Rec 4/5/6/7
	 * Plane1 will access base, causing Plane exit to P0
	 * P0 will return ESR?FAR back to Host
	 */
	host_shared_data_set_host_val(&realm, 1U, 4U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, 5U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, 6U, HOST_ARG3_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, 7U, HOST_ARG3_INDEX, base);

	/* Arg for Plane0 to enter Plane1 on Rec4,5,6,7 */
	host_realm_set_aux_plane_args(&realm, 1U, 4U);
	host_realm_set_aux_plane_args(&realm, 1U, 5U);
	host_realm_set_aux_plane_args(&realm, 1U, 6U);
	host_realm_set_aux_plane_args(&realm, 1U, 7U);

	/* Test cmd for Plane 1, Rec 4/5/6/7 */
	host_shared_data_set_realm_cmd(&realm, REALM_INSTR_FETCH_CMD, 1U, 4U);
	host_shared_data_set_realm_cmd(&realm, REALM_DATA_ACCESS_CMD, 1U, 5U);
	host_shared_data_set_realm_cmd(&realm, REALM_INSTR_FETCH_CMD, 1U, 6U);
	host_shared_data_set_realm_cmd(&realm, REALM_DATA_ACCESS_CMD, 1U, 7U);

	/* Rec4 expect IA due to unassigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_PLANE_N_EXCEPTION_CMD,
			RMI_EXIT_HOST_CALL, 4U);
	if (!ret1) {
		ERROR("Rec4 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR FAR set by P0 */
	esr = host_shared_data_get_realm_val(&realm, 0U, 4U, HOST_ARG2_INDEX);
	far = host_shared_data_get_realm_val(&realm, 0U, 4U, HOST_ARG3_INDEX);
	if ((EC_BITS(esr) != EC_IABORT_LOWER_EL) || (far != base)) {
		ERROR("Rec4 incorrect ESR=0x%lx FAR=0x%lx\n", esr, far);
		goto destroy_realm;
	}
	INFO("Rec4 ESR=0x%lx\n", esr);

	/* Rec5 expect DA due to unassigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_PLANE_N_EXCEPTION_CMD,
			RMI_EXIT_HOST_CALL, 5U);
	if (!ret1) {
		ERROR("Rec5 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR FAR set by P0 */
	esr = host_shared_data_get_realm_val(&realm, 0U, 5U, HOST_ARG2_INDEX);
	far = host_shared_data_get_realm_val(&realm, 0U, 5U, HOST_ARG3_INDEX);
	if ((EC_BITS(esr) != EC_DABORT_LOWER_EL) || (far != base)) {
		ERROR("Rec5 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec5 ESR=0x%lx\n", esr);

	/* DATA_CREATE_UNKNOWN */
	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_EMPTY)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}
	INFO("final state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx level=0x%lx\n",
			base, rtt.state, rtt.ripas, rtt.walk_level);

	/* Rec6 expect IA due to assigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_PLANE_N_EXCEPTION_CMD,
		RMI_EXIT_HOST_CALL, 6U);

	if (!ret1) {
		ERROR("Rec6 did not fault\n");
		goto undelegate_destroy;
	}

	/* get ESR FAR set by P0 */
	esr = host_shared_data_get_realm_val(&realm, 0U, 6U, HOST_ARG2_INDEX);
	far = host_shared_data_get_realm_val(&realm, 0U, 6U, HOST_ARG3_INDEX);

	if ((EC_BITS(esr) != EC_IABORT_LOWER_EL) || (far != base)) {
		ERROR("Rec6 incorrect ESR=0x%lx\n", esr);
		goto undelegate_destroy;
	}
	INFO("Rec6 ESR=0x%lx\n", esr);

	/* Rec7 expect DA due to unassigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_PLANE_N_EXCEPTION_CMD,
			RMI_EXIT_HOST_CALL, 7U);
	if (!ret1) {
		ERROR("Rec7 did not fault\n");
		goto undelegate_destroy;
	}

	/* get ESR FAR set by P0 */
	esr = host_shared_data_get_realm_val(&realm, 0U, 7U, HOST_ARG2_INDEX);
	far = host_shared_data_get_realm_val(&realm, 0U, 7U, HOST_ARG3_INDEX);

	if ((EC_BITS(esr) != EC_DABORT_LOWER_EL) || (far != base)) {
		ERROR("Rec7 incorrect ESR=0x%lx\n", esr);
		goto undelegate_destroy;
	}
	INFO("Rec7 ESR=0x%lx\n", esr);
	res = TEST_RESULT_SUCCESS;

undelegate_destroy:
	ret = host_rmi_granule_undelegate(base);
destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to generate SEA in Realm by
 * executing instructions in unprotected IPA - Rec0
 * In Rec 1 , when HIPAS=UNASSIGNED_NS, we expect to get a Data abort.
 * Then Host will inject SEA to realm.
 * Realm exception handler runs and returns ESR back to Host
 * Host validates ESR
 * Repeat the test for Plane 1
 * executing instructions in unprotected IPA causes plane exit to P0
 * data access in unprotected IPA causes rec exit
 * Host injects SEA to Plane N
 */
test_result_t host_realm_sea_unprotected(void)
{

	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base, base_ipa, esr, far;
	unsigned int host_call_result;
	u_register_t exit_reason;
	struct realm realm;
	struct rtt_entry rtt;
	struct rmi_rec_run *run;
	u_register_t feature_flag0 = 0UL, feature_flag1 = 0UL, num_aux_planes = 0U;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	/* Test are skipped if S2POE is not supported to keep test simple */
	if (are_planes_supported() && is_single_rtt_supported()) {
		num_aux_planes = 1UL;

		/* use single RTT for all planes */
		feature_flag0 |= INPLACE(RMI_FEATURE_REGISTER_0_PLANE_RTT,
			RMI_PLANE_RTT_SINGLE);

		feature_flag1 = RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, feature_flag1, sl, rec_flag, 4U, num_aux_planes)) {
		return TEST_RESULT_FAIL;
	}

	/* Can choose any unprotected IPA adr, TFTF_BASE chosen for convenience */
	base = TFTF_BASE;
	base_ipa = base | (1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
					realm.rmm_feat_reg0) - 1U));


	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (rtt.state != RMI_UNASSIGNED) {
		ERROR("wrong state\n");
		goto destroy_realm;
	}

	run = (struct rmi_rec_run *)realm.run[0];
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG3_INDEX, base_ipa);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG3_INDEX, base_ipa);

	/* Rec0 expect SEA in realm due to IA unprotected IPA page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
			RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("Rec0 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG2_INDEX);
	if (((esr & ISS_IFSC_MASK) != IFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_IABORT_CUR_EL)) {
		ERROR("Rec0 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec0 ESR=0x%lx\n", esr);

	run = (struct rmi_rec_run *)realm.run[1U];

	/* Rec1 expect rec exit due to DA unprotected IPA page when HIPAS is UNASSIGNED_NS */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_SYNC, 1U);

	if (!ret1 || (run->exit.hpfar >> 4U) != (base_ipa >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U)) {
		ERROR("Rec1 did not fault exit=0x%lx ret1=%d HPFAR=0x%lx esr=0x%lx\n",
				run->exit.exit_reason, ret1, run->exit.hpfar, run->exit.esr);
		goto destroy_realm;
	}
	INFO("Host DA FAR=0x%lx, HPFAR=0x%lx\n", run->exit.far, run->exit.hpfar);
	INFO("Injecting SEA to Realm\n");

	/* Inject SEA back to Realm */
	run->entry.flags = REC_ENTRY_FLAG_INJECT_SEA;

	/* Rec1 re-entry expect exception handler to run, return ESR */
	ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 1U);
	if (ret != RMI_SUCCESS || exit_reason != RMI_EXIT_HOST_CALL) {
		ERROR("rec1 failed ret=0x%lx exit_reason=0x%lx", ret, run->exit.exit_reason);
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_DABORT_CUR_EL)) {
		ERROR("Rec1 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec1 ESR=0x%lx\n", esr);
	res = host_call_result;

	if (num_aux_planes == 0U) {
		goto destroy_realm;
	}

	INFO("Running test on Plane 1\n");

	run = (struct rmi_rec_run *)realm.run[2U];

	/*
	 * Arg for Plane0 instruction to enter Plane1 on Rec 2,3
	 */
	host_realm_set_aux_plane_args(&realm, 1U, 2U);
	host_realm_set_aux_plane_args(&realm, 1U, 3U);


	/* Test cmd for Plane 1 Rec 2/3 */
	host_shared_data_set_realm_cmd(&realm, REALM_INSTR_FETCH_CMD, 1U, 2U);
	host_shared_data_set_realm_cmd(&realm, REALM_DATA_ACCESS_CMD, 1U, 3U);

	/*
	 * Args for Plane1, Rec 2/3
	 * Executing base_ipa from plane 1 rec 2, causes plane exit to P0
	 * Data access from plane 1 rec 3, causes rec exit, host injects SEA
	 */
	host_shared_data_set_host_val(&realm, 1U, 2U, HOST_ARG3_INDEX, base_ipa);
	host_shared_data_set_host_val(&realm, 1U, 3U, HOST_ARG3_INDEX, base_ipa);

	/* Rec2 expect plane exit to P0 due to IA unprotected IPA page */
	ret1 = host_enter_realm_execute(&realm, REALM_PLANE_N_EXCEPTION_CMD,
			RMI_EXIT_HOST_CALL, 2U);
	if (!ret1) {
		ERROR("Rec2 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR/FAR set by P0 */
	esr = host_shared_data_get_realm_val(&realm, 0U, 2U, HOST_ARG2_INDEX);
	far = host_shared_data_get_realm_val(&realm, 0U, 2U, HOST_ARG3_INDEX);

	if (((EC_BITS(esr) != EC_IABORT_LOWER_EL) || (far != base_ipa))) {
		ERROR("Rec2 incorrect ESR=0x%lx far=0x%lx\n", esr, far);
		goto destroy_realm;
	}
	INFO("Rec2 ESR=0x%lx\n", esr);

	run = (struct rmi_rec_run *)realm.run[3U];

	/* Rec3 expect rec exit due to DA unprotected IPA page when HIPAS is UNASSIGNED_NS */
	ret1 = host_enter_realm_execute(&realm, REALM_ENTER_PLANE_N_CMD,
			RMI_EXIT_SYNC, 3U);

	if (!ret1 || (run->exit.hpfar >> 4U) != (base_ipa >> PAGE_SIZE_SHIFT)
		|| (EC_BITS(run->exit.esr) != EC_DABORT_LOWER_EL)
		|| ((run->exit.esr & ISS_DFSC_MASK) < FSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > FSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U)) {
		ERROR("Rec3 did not fault exit=0x%lx ret1=%d HPFAR=0x%lx esr=0x%lx\n",
				run->exit.exit_reason, ret1, run->exit.hpfar, run->exit.esr);
		goto destroy_realm;
	}
	INFO("Host DA FAR=0x%lx, HPFAR=0x%lx\n", run->exit.far, run->exit.hpfar);
	INFO("Injecting SEA to Realm PN Rec3\n");

	/* Inject SEA back to Realm P1 Rec3 */
	run->entry.flags = REC_ENTRY_FLAG_INJECT_SEA;

	/* Rec1 re-entry expect exception handler to run, return ESR */
	ret = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 3U);
	if (ret != RMI_SUCCESS || exit_reason != RMI_EXIT_HOST_CALL) {
		ERROR("rec3 failed ret=0x%lx exit_reason=0x%lx", ret, run->exit.exit_reason);
		goto destroy_realm;
	}

	/* get ESR/FAR set by Realm PN exception handler */
	esr = host_shared_data_get_realm_val(&realm, 1U, 3U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_DABORT_CUR_EL)) {
		ERROR("Rec3 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec3 ESR=0x%lx\n", esr);
	res = host_call_result;

destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * @Test_Aim@ Test to check if DIT bit is preserved across NS/RL switch
 */
test_result_t host_realm_enable_dit(void)
{
	bool ret1, ret2;
	struct realm realm;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[MAX_REC_COUNT], dit;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		rec_flag[i] = RMI_RUNNABLE;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, MAX_REC_COUNT, 0U)) {
		return TEST_RESULT_FAIL;
	}

	/* Enable FEAT_DIT on Host */
	write_dit(DIT_BIT);
	for (unsigned int i = 0; i < MAX_REC_COUNT; i++) {
		host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, i, HOST_ARG1_INDEX, 10U);
		ret1 = host_enter_realm_execute(&realm, REALM_DIT_CHECK_CMD,
				RMI_EXIT_HOST_CALL, i);
		if (!ret1) {
			break;
		}
	}

	ret2 = host_destroy_realm(&realm);

	dit = read_dit();
	if (dit != DIT_BIT) {
		ERROR("Host DIT bit not preserved\n");
		return TEST_RESULT_FAIL;
	}

	write_dit(0U);
	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Helper to test whether RTT_DESTROY will change state of
 * Unassigned ram page to Unassigned Destroyed
 * Realm is in new state, activate realm before RTT_DESTROY if requested
 */
static test_result_t test_rtt_destroy_ram(struct realm *realm, bool activate)
{
	u_register_t ret, top, out_rtt, base;
	struct rtt_entry rtt;

	/* Find an address not mapped in L3 */
	base = ALIGN_DOWN(PAGE_POOL_BASE, RTT_MAP_SIZE(2U));
	while (true) {
		ret = host_rmi_rtt_readentry(realm->rd, base, 3L, &rtt);
		if (ret != RMI_SUCCESS || rtt.walk_level != 2U || rtt.state != RMI_UNASSIGNED
				|| (rtt.ripas != RMI_EMPTY)) {
			base += RTT_MAP_SIZE(2U);
			continue;
		}
		break;
	}

	INFO("base = 0x%lx\n", base);

	/* Create L3 RTT entries */
	ret = host_rmi_create_rtt_levels(realm, base, rtt.walk_level, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3L, &rtt);
	if (rtt.walk_level != 3L) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	ret = host_rmi_rtt_init_ripas(realm->rd, base, base + PAGE_SIZE, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx line=%u\n",
			"host_rmi_rtt_init_ripas", ret, __LINE__);
		return TEST_RESULT_FAIL;
	}

	/* INIT_RIPAS should move state to unassigned ram */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after INIT_RIPAS\n");
		return TEST_RESULT_FAIL;
	}

	if (activate) {
		/* Activate Realm */
		if (host_realm_activate(realm) != REALM_SUCCESS) {
			ERROR("%s() failed\n", "host_realm_activate");
			return TEST_RESULT_FAIL;
		}
	}

	/* Destroy newly created rtt, for protected IPA there should be no live L3 entry */
	ret = host_rmi_rtt_destroy(realm->rd, base, 3L, &out_rtt, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_rtt_destroy failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}
	ret = host_rmi_granule_undelegate(out_rtt);

	/* Walk should terminate at L2 after RTT_DESTROY */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED || rtt.walk_level != 2L) {
		ERROR("Wrong state after host_rmi_rtt_destroy\n");
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
}

/*
 * Helper to test whether RTT_DESTROY will change state of
 * Unassigned empty page to Unassigned Destroyed
 * Realm can be in new or active state
 */
static test_result_t test_rtt_destroy_empty(struct realm *realm)
{
	u_register_t base, ret, top, out_rtt;
	struct rtt_entry rtt;

	/* Find an address not mapped in L3 */
	base = ALIGN_DOWN(PAGE_POOL_BASE, RTT_MAP_SIZE(2U));
	while (true) {
		ret = host_rmi_rtt_readentry(realm->rd, base, 3L, &rtt);
		if (ret != RMI_SUCCESS || rtt.walk_level != 2L || rtt.state != RMI_UNASSIGNED
				|| (rtt.ripas != RMI_EMPTY)) {
			base += RTT_MAP_SIZE(2U);
			continue;
		}
		break;
	}

	INFO("base = 0x%lx\n", base);

	/* Create L3 RTT entries */
	ret = host_rmi_create_rtt_levels(realm, base, rtt.walk_level, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3L, &rtt);
	if (rtt.walk_level != 3L) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* Destroy newly created rtt, for protected IPA there should be no live L3 entry */
	ret = host_rmi_rtt_destroy(realm->rd, base, 3L, &out_rtt, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_rtt_destroy failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}
	ret = host_rmi_granule_undelegate(out_rtt);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_granule_undelegate RTT failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* Walk should terminate at L2 after RTT_DESTROY */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED || rtt.walk_level != 2L) {
		ERROR("Wrong state after host_rmi_rtt_destroy\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test aims to test PAS transitions
 * when realm is in new state
 * 1. Test initial state of PAGE is Unassigned Empty
 * 2. Test DATA_CREATE moves initial state to Assigned Ram
 *    a. Test DATA_DESTROY moves state to Unassigned Destroyed
 *    b. Test DATA_CREATE_UNKNOWN moves state to Assigned Destroyed
 *    c. Test DATA_DESTROY moves state to Unassigned Destroyed
 * 3. Test DATA_CREATE_UNKNOWN moves initial state (new page) to Assigned Empty
 *    Test DATA_DESTROY moves state to Unassigned Empty
 * 4. Test INIT_RIPAS moves initial state (new page) to Unassigned RAM
 *    a. Test DATA_CREATE_UNKNOWN moves state to Assigned Ram
 * 5. Test RTT_DESTROY moves initial state (new page) to Unassigned Destroyed
 * 6. Test RTT_DESTROY moves state (new page) unassigned ram to Unassigned Destroyed
 * Transition
 *
 *     +------------------+            +-------------------+            +-------------------+
 *     |  Assigned Empty  |            |   Assigned Dest   |            | Assigned RAM      |
 *     +------------------+            +--+---^------------+            +-------^---+-----^-+
 *          ^          |                  |   ^                                 ^   |     ^
 *          |          |                  |   |                                 |   |     |
 *          |          |                  |   |                 2a              |   |     |
 *          |          |                  |   |      +--------------------------+---+     |
 *          |          |                  |   | 2b   |                          |         |4a
 *          |          |3a                |   +---------+                       |         |
 *        3 |          |                2c|          |  |                       |         |
 *          |          |                  |          |  |                       |         |
 *          |    +-----+--------2---------+----------+--+-----------------------+         |
 *          |    |     |                  |          |  |                                 |
 *          |    |     V                  V          V  |                                 |
 *     +----+----+-----v---+           |--V----------V--+---|         |------------------+--|
 * --->|  Unassigned Empty |---------->|Unassigned Dest     |<--------|  Unassigned RAM     |
 * 1   +--------------+----+     5     +--------------------+    6    +---------^-----------+
 *                    |			                                        ^
 *                    |                                                         |
 *                    +---------------------------------------------------------+
 *                                                      4
 */
test_result_t host_realm_pas_validation_new(void)
{
	bool ret1;
	test_result_t test_result = TEST_RESULT_FAIL;
	u_register_t ret, data, top;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t rec_flag[2U] = {RMI_RUNNABLE, RMI_RUNNABLE}, base;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 2U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	INFO("Test 1\n");
	base = (u_register_t)page_alloc(PAGE_SIZE);

	/* Create level 3 RTT */
	ret = host_rmi_create_rtt_levels(&realm, base, 3L, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed\n");
		goto destroy_realm;
	}

	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY) || rtt.walk_level != 3L) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}

	/* 2. DATA_CREATE copy TFTF_BASE, chosen for convenience, can be any adr */
	INFO("Test 2\n");
	ret = host_realm_delegate_map_protected_data(false, &realm, base, PAGE_SIZE, TFTF_BASE);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}

	/* 2a DATA_DESTROY */
	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	if (ret != RMI_SUCCESS || data != base) {
		ERROR("host_rmi_data_destroy failed\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED) {
		ERROR("Wrong state after host_rmi_data_destroy\n");
		goto undelegate_destroy;
	}

	/* Undelegated to use helper function host_realm_delegate_map_protected_data */
	host_rmi_granule_undelegate(base);

	/*2b DATA_CREATE_UNKNOWN */
	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_DESTROYED)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}

	/* 2c DATA_DESTROY */
	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	if (ret != RMI_SUCCESS || data != base) {
		ERROR("host_rmi_data_destroy failed\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED) {
		ERROR("Wrong state after host_rmi_data_destroy\n");
		goto undelegate_destroy;
	}

	host_rmi_granule_undelegate(base);

	/* 3. start with new page */
	INFO("Test 3\n");
	base = (u_register_t)page_alloc(PAGE_SIZE);
	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_EMPTY)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	if (ret != RMI_SUCCESS || data != base) {
		ERROR("host_rmi_data_destroy failed\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_EMPTY) {
		ERROR("Wrong state after host_rmi_data_destroy\n");
		goto undelegate_destroy;
	}
	host_rmi_granule_undelegate(base);

	/* 4. start with new page */
	INFO("Test 4\n");
	base = (u_register_t)page_alloc(PAGE_SIZE);
	ret = host_rmi_rtt_init_ripas(realm.rd, base, base + PAGE_SIZE, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx line=%u\n",
			"host_rmi_rtt_init_ripas", ret, __LINE__);
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after INIT_RIPAS\n");
		goto undelegate_destroy;
	}
	/* 4a. DATA_CREATE_UNKNOWN */
	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, 0U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}
	host_rmi_granule_undelegate(base);

	/* 5. */
	INFO("Test 5\n");
	test_result = test_rtt_destroy_empty(&realm);
	if (test_result != TEST_RESULT_SUCCESS) {
		ERROR("Test 5 failed\n");
		goto destroy_realm;
	}

	/* 6. */
	INFO("Test 6\n");
	test_result = test_rtt_destroy_ram(&realm, false);
	goto destroy_realm;

undelegate_destroy:
	ret = host_rmi_granule_undelegate(base);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_granule_undelegate failed base=0x%lx ret=0x%lx\n", base, ret);
	}
destroy_realm:
	ret1 = host_destroy_realm(&realm);
	if (!ret1) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret1);
		return TEST_RESULT_FAIL;
	}

	return test_result;
}

/*
 * Test aim is to test RTT_DESTROY for active realm
 * Test initial state of page is unassigned empty
 * After RTT_DESTROY verify state is unassigned destroyed
 */
test_result_t host_realm_pas_validation_active(void)
{
	bool ret;
	test_result_t test_result = TEST_RESULT_FAIL;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		goto destroy_realm;
	}

	test_result = test_rtt_destroy_ram(&realm, true);
	if (test_result != TEST_RESULT_SUCCESS) {
		ERROR("test_rtt_destroy_ram failed\n");
		goto destroy_realm;
	}

	test_result = test_rtt_destroy_empty(&realm);

destroy_realm:
	ret = host_destroy_realm(&realm);

	if (!ret) {
		ERROR("%s(): destroy=%d\n", __func__, ret);
		return TEST_RESULT_FAIL;
	}
	return test_result;
}

/*
 * Test aims to generate SEA in Realm by accessing
 * PAGE with IPA outside realm IPA space and
 * Generate Data abort by accessing
 * PAGE with IPA outside max PA supported
 * Rec0 and Rec2 tries to create Data Abort to realm
 * Rec1 and Rec3 tries to create Instruction Abort to realm
 * Realm exception handler runs and returns ESR
 * Host validates ESR
 */
test_result_t host_realm_sea_adr_fault(void)
{
	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t base_ipa, esr, feature_flag0, base;
	struct realm realm;
	u_register_t rec_flag[4U] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE};
	struct rmi_rec_run *run;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	feature_flag0 = INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 0x2CU);

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, RTT_MIN_LEVEL, rec_flag, 4U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	/* Any Adr */
	base = TFTF_BASE;
	/* IPA outside Realm space */
	base_ipa = base | (1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
					realm.rmm_feat_reg0) + 1U));
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG3_INDEX, base_ipa);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG3_INDEX, base_ipa);

	INFO("base_ipa=0x%lx\n", base_ipa);

	run = (struct rmi_rec_run *)realm.run[0];

	/* Rec0 expect SEA in realm due to Data access to address outside Realm IPA size */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("Rec0 did not fault exit=0x%lx ret1=%d HPFAR=0x%lx esr=0x%lx\n",
				run->exit.exit_reason, ret1, run->exit.hpfar, run->exit.esr);
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_NO_WALK_SEA)
			|| (EC_BITS(esr) != EC_DABORT_CUR_EL)
			|| ((esr & (1UL << ESR_ISS_EABORT_EA_BIT)) == 0U)) {
		ERROR("Rec0 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec0 ESR=0x%lx\n", esr);

	run = (struct rmi_rec_run *)realm.run[1];

	/* Rec1 expect SEA in realm due to Instruction access to address outside Realm IPA size */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
		RMI_EXIT_HOST_CALL, 1U);
	if (!ret1) {
		ERROR("Rec1 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, PRIMARY_PLANE_ID, 1U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != IFSC_NO_WALK_SEA)
			|| (EC_BITS(esr) != EC_IABORT_CUR_EL)
			|| ((esr & (1UL << ESR_ISS_EABORT_EA_BIT)) == 0U)) {
		ERROR("Rec1 did not fault exit=0x%lx ret1=%d HPFAR=0x%lx esr=0x%lx\n",
				run->exit.exit_reason, ret1, run->exit.hpfar, run->exit.esr);
		goto destroy_realm;
	}
	INFO("Rec1 ESR=0x%lx\n", esr);

	/* IPA outside max PA supported */
	base_ipa |= (1UL << 53U);

	INFO("base_ipa=0x%lx\n", base_ipa);

	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 2U, HOST_ARG3_INDEX, base_ipa);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 3U, HOST_ARG3_INDEX, base_ipa);

	run = (struct rmi_rec_run *)realm.run[2];

	/* Rec2 expect SEA in realm due to Data access to address outside Realm IPA size */
	ret1 = host_enter_realm_execute(&realm, REALM_DATA_ACCESS_CMD,
			RMI_EXIT_HOST_CALL, 2U);
	if (!ret1) {
		ERROR("Rec2 did not fault exit=0x%lx ret1=%d HPFAR=0x%lx esr=0x%lx\n",
				run->exit.exit_reason, ret1, run->exit.hpfar, run->exit.esr);
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, PRIMARY_PLANE_ID, 2U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != FSC_L0_ADR_SIZE_FAULT)
			|| (EC_BITS(esr) != EC_DABORT_CUR_EL)
			|| ((esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U)) {
		ERROR("Rec2 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec2 ESR=0x%lx\n", esr);

	run = (struct rmi_rec_run *)realm.run[3];

	/* Rec3 expect SEA in realm due to Instruction access to address outside Realm IPA size */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
		RMI_EXIT_HOST_CALL, 3U);
	if (!ret1) {
		ERROR("Rec3 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, PRIMARY_PLANE_ID, 3U, HOST_ARG2_INDEX);
	if (((esr & ISS_IFSC_MASK) != FSC_L0_ADR_SIZE_FAULT)
			|| (EC_BITS(esr) != EC_IABORT_CUR_EL)
			|| ((esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U)) {
		ERROR("Rec3 did not fault exit=0x%lx ret1=%d HPFAR=0x%lx esr=0x%lx\n",
				run->exit.exit_reason, ret1, run->exit.hpfar, run->exit.esr);
		goto destroy_realm;
	}
	INFO("Rec3 ESR=0x%lx\n", esr);
	res = TEST_RESULT_SUCCESS;

destroy_realm:
	ret2 = host_destroy_realm(&realm);

	if (!ret2) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret2);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to test RMI_RTT_FOLD for Unassinged Empty entries
 * Find a adr not mapped in L1 RTT of Stage 2 tables in RMM
 * and mapped at L0 with HIPAS=unassigned RIPAS=empty
 * Host creates L3 RTT for this adr
 * Host folds RTT till L0
 * Host recreates L3, which should cause unfold operation
 */
test_result_t host_test_rtt_fold_unfold_unassigned_empty(void)
{

	bool ret1;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		ERROR("Realm creation failed\n");
		goto destroy_realm;
	}

	/* Find an address not mapped at L1 and mapped at L0 as unassigned empty */
	base = ALIGN_DOWN(TFTF_BASE, RTT_MAP_SIZE(1U));
	while (true) {
		ret = host_rmi_rtt_readentry(realm.rd, base, 1L, &rtt);
		if (ret != RMI_SUCCESS || rtt.walk_level > 0U || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_EMPTY)) {
			base += RTT_MAP_SIZE(1U);
			if (host_ipa_is_ns(base, realm.rmm_feat_reg0)) {
				ERROR("could not find unmapped adr range\n");
				goto destroy_realm;
			}
			continue;
		}
		break;
	}

	INFO("base=0x%lx\n", base);

	/* Create RTT entries */
	ret = host_rmi_create_rtt_levels(&realm, base, 0L, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* L3 entry is expected now */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 3L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_EMPTY)) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* Walk should terminate at L2 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 2L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_EMPTY)) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 2L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* Walk should terminate at L1 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 1L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_EMPTY)) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 1L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* Walk should terminate at L0 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 0L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_EMPTY)) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* Create RTT entries will cause unfold operation */
	ret = host_rmi_create_rtt_levels(&realm, base, rtt.walk_level, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 3L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_EMPTY)) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	res = TEST_RESULT_SUCCESS;

destroy_realm:
	ret1 = host_destroy_realm(&realm);

	if (!ret1) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret1);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to test RMI_RTT_FOLD for Unassigned RAM entries
 * Find a adr not mapped in L1 RTT of Stage 2 tables in RMM
 * and mapped at L0 with HIPAS=unassigned RIPAS=empty
 * Change RIPAS to RAM by calling RMI_RTT_INIT_RIPAS
 * Host creates L3 RTT for this adr
 * Host folds RTT till L0
 * Host recreates L3, which should cause unfold operation
 */
test_result_t host_test_rtt_fold_unfold_unassigned_ram(void)
{

	bool ret1;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base, top;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		ERROR("Realm creation failed\n");
		goto destroy_realm;
	}

	/* Find an address not mapped at L1 and mapped at L0 as unassigned empty */
	base = ALIGN_DOWN(TFTF_BASE, RTT_MAP_SIZE(1U));
	while (true) {
		ret = host_rmi_rtt_readentry(realm.rd, base, 1L, &rtt);
		if (ret != RMI_SUCCESS || rtt.walk_level > 0L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_EMPTY)) {
			base += RTT_MAP_SIZE(1U);
			if (host_ipa_is_ns(base, realm.rmm_feat_reg0)) {
				ERROR("could not find unmapped adr range\n");
				goto destroy_realm;
			}
			continue;
		}
		break;
	}

	/* Set RIPAS of PAGE to RAM */
	ret = host_rmi_rtt_init_ripas(realm.rd, base, base + RTT_MAP_SIZE(0U), &top);
	if (ret != RMI_SUCCESS || top != (base + RTT_MAP_SIZE(0))) {
		ERROR("%s() failed, ret=0x%lx line=%u\n",
			"host_rmi_rtt_init_ripas", ret, __LINE__);
		goto destroy_realm;
	}
	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto destroy_realm;
	}

	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_RAM) || rtt.walk_level != 0L) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}

	INFO("base=0x%lx\n", base);

	/* Create RTT entries */
	ret = host_rmi_create_rtt_levels(&realm, base, 0L, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 3L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_RAM)) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* Walk should terminate at L2 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 2L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_RAM)) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 2L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* Walk should terminate at L1 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 1L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_RAM)) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 1L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* Walk should terminate at L0 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 0L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_RAM)) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* Create RTT entries will cause unfold operation */
	ret = host_rmi_create_rtt_levels(&realm, base, rtt.walk_level, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 3L || rtt.state != RMI_UNASSIGNED
			|| (rtt.ripas != RMI_RAM)) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	res = TEST_RESULT_SUCCESS;

destroy_realm:
	ret1 = host_destroy_realm(&realm);

	if (!ret1) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret1);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to test RMI_RTT_FOLD for Unprotected IPA range
 * Host creates L1 1 GB block entry for unprotected IPA
 * Host creates L2 entry for the same, this causes unfold of all 512 entries
 * Host verifies walk_level is 2
 * Host folds RTT back to L1
 * Host verifies walk_level is 1
 */
test_result_t host_test_rtt_fold_unfold_assigned_ns(void)
{

	bool ret1;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, ns_ipa, base_pa, top;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		ERROR("Realm creation failed\n");
		goto destroy_realm;
	}

	/*
	 * Pick an address which is to be mapped to unprotected IPA space.
	 * We use TFTF Base as the PA to map in unprotected IPA space of RMM.
	 * Note that as long as Realm does not explicitly touch this unprotected IPA space,
	 * it will not hit a fault.
	 * Hence it does not matter even if the same pages are delegated
	 * or even if it is invalid memory.
	 */
	base_pa = ALIGN(TFTF_BASE, RTT_L1_BLOCK_SIZE);
	ns_ipa = base_pa | (1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
					realm.rmm_feat_reg0) - 1U));

	INFO("base=0x%lx\n", ns_ipa);
	ret = host_realm_map_unprotected(&realm, base_pa, RTT_L1_BLOCK_SIZE);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_map_unprotected fail base=0x%lx ret=0x%lx\n", ns_ipa, ret);
		goto destroy_realm;
	}

	ret = host_rmi_rtt_readentry(realm.rd, ns_ipa, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 1L || rtt.state != RMI_ASSIGNED) {
		INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
		rtt.state, rtt.walk_level, rtt.out_addr);

	/* Unfold Create L2 RTT entries */
	ret = host_rmi_create_rtt_levels(&realm, ns_ipa, 1L, 2L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
	}

	ret = host_rmi_rtt_readentry(realm.rd, ns_ipa, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 2L || rtt.state != RMI_ASSIGNED) {
		ERROR("Initial realm table creation changed\n");
		INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
		rtt.state, rtt.walk_level, rtt.out_addr);

	INFO("Fold L2\n");
	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, ns_ipa, 2L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* Walk should terminate at L1 */
	ret = host_rmi_rtt_readentry(realm.rd, ns_ipa, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 1L || rtt.state != RMI_ASSIGNED) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
			rtt.state, rtt.walk_level, rtt.out_addr);

	res = TEST_RESULT_SUCCESS;
	INFO("unmap\n\n");

	/* unmap */
	ret = host_rmi_rtt_unmap_unprotected(realm.rd, ns_ipa, 1L, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_rtt_mapunprotected failed ret=0x%lx\n", ret);
	}

destroy_realm:
	ret1 = host_destroy_realm(&realm);

	if (!ret1) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret1);
		return TEST_RESULT_FAIL;
	}

	return res;
}

/*
 * Test aims to test RMI_RTT_FOLD for assigned empty entries
 * Host calls DATA_CREATE_UNKNOWN on 2 mb range
 * Host verifies walk_level is 3
 * Host folds RTT_FOLD Level 2
 * Host verifies walk_level is 2
 */
test_result_t host_test_rtt_fold_unfold_assigned_empty(void)
{
	bool ret1;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		ERROR("Realm creation failed\n");
		goto destroy_realm;
	}

	/*
	 * Any 2 MB range not mapped in RTT and which can be delegated,
	 * using heap for second realm here.
	 */
	base = ALIGN_DOWN(PAGE_POOL_BASE + (PAGE_POOL_MAX_SIZE - RTT_L2_BLOCK_SIZE),
			RTT_L2_BLOCK_SIZE);

	for (unsigned int i = 0U; i < 512; i++) {
		ret = host_realm_delegate_map_protected_data(true, &realm, base + (PAGE_SIZE * i),
				PAGE_SIZE, 0U);
		if (ret != RMI_SUCCESS) {
			ERROR("host_realm_delegate_map_protected_data failed\n");
			goto undelegate_destroy;
		}
	}

	INFO("base=0x%lx\n", base);
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_EMPTY) || rtt.walk_level != 3L) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 3U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto undelegate_destroy;
	}

	/* Walk should terminate at L2 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 2L || rtt.state != RMI_ASSIGNED ||
			rtt.ripas != RMI_EMPTY) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
				" rtt.walk_level=0x%lx rtt.out_addr=0x%llx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr);
		goto undelegate_destroy;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* Create L3 RTT entries */
	ret = host_rmi_create_rtt_levels(&realm, base, 2L, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 3L || rtt.state != RMI_ASSIGNED ||
			rtt.ripas != RMI_EMPTY) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	res = TEST_RESULT_SUCCESS;

undelegate_destroy:
	if (res != TEST_RESULT_SUCCESS) {
		for (unsigned int i = 0U; i < 512; i++) {
			ret = host_rmi_granule_undelegate(base + (PAGE_SIZE * i));
			if (ret != RMI_SUCCESS) {
				ERROR("host_rmi_granule_undelegate failed base=0x%lx ret=0x%lx\n",
					base, ret);
			}
		}
	}
destroy_realm:
	ret1 = host_destroy_realm(&realm);
	if (!ret1) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret1);
		return TEST_RESULT_FAIL;
	}
	return res;
}

/*
 * Test aims to test RMI_RTT_FOLD for assigned ram entries
 * Host calls DATA_CREATE on 2 mb range
 * Host verifies walk_level is 3
 * Host folds RTT_FOLD Level 2
 * Host verifies walk_level is 2 and RIPAS is RAM
 */
test_result_t host_test_rtt_fold_unfold_assigned_ram(void)
{
	bool ret1;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		ERROR("Realm creation failed\n");
		goto destroy_realm;
	}

	/*
	 * Any 2 MB range not mapped in RTT and which can be delegated,
	 * using heap for second realm here.
	 */
	base = ALIGN_DOWN(PAGE_POOL_BASE + (PAGE_POOL_MAX_SIZE - RTT_L2_BLOCK_SIZE),
			RTT_L2_BLOCK_SIZE);

	INFO("base=0x%lx\n", base);
	for (unsigned int i = 0U; i < 512; i++) {
		ret = host_realm_delegate_map_protected_data(false, &realm, base + (PAGE_SIZE * i),
				PAGE_SIZE, REALM_IMAGE_BASE);
		if (ret != RMI_SUCCESS) {
			ERROR("host_realm_delegate_map_protected_data failed base=0x%lx\n",
					base + (PAGE_SIZE * i));
			goto undelegate_destroy;
		}
	}

	/* INIT_RIPAS should move state to unassigned ram */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after INIT_RIPAS\n");
		goto undelegate_destroy;
	}
	host_realm_activate(&realm);

	/* RTT Fold */
	ret = host_realm_fold_rtt(realm.rd, base, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_fold_rtt failed ret=0x%lx\n", ret);
		goto undelegate_destroy;
	}

	/* Walk should terminate at L2 */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 2L || rtt.state != RMI_ASSIGNED ||
			rtt.ripas != RMI_RAM) {
		ERROR("host_rmi_rtt_readentry failed ret=0x%lx rtt.state=0x%lx"
			" rtt.walk_level=0x%lx rtt.out_addr=0x%llx ripas=0x%lx\n",
				ret, rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);
		goto undelegate_destroy;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx rtt.ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	/* Create L3 RTT entries */
	ret = host_rmi_create_rtt_levels(&realm, base, 2L, 3L);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (ret != RMI_SUCCESS || rtt.walk_level != 3L || rtt.state != RMI_ASSIGNED ||
			rtt.ripas != RMI_RAM) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		goto destroy_realm;
	}
	INFO("rtt.state=0x%lx rtt.walk_level=0x%lx rtt.out_addr=0x%llx ripas=0x%lx\n",
			rtt.state, rtt.walk_level, rtt.out_addr, rtt.ripas);

	res = TEST_RESULT_SUCCESS;

undelegate_destroy:
	if (res != TEST_RESULT_SUCCESS) {
		for (unsigned int i = 0U; i < 512; i++) {
			ret = host_rmi_granule_undelegate(base + (PAGE_SIZE * i));
			if (ret != RMI_SUCCESS) {
				ERROR("host_rmi_granule_undelegate failed base=0x%lx ret=0x%lx\n",
					base + (PAGE_SIZE * i), ret);
			}
		}
	}
destroy_realm:
	ret1 = host_destroy_realm(&realm);
	if (!ret1) {
		ERROR("%s(): destroy=%d\n",
		__func__, ret1);
		return TEST_RESULT_FAIL;
	}
	return res;
}

/*
 * Test aims to test that TF-RMM takes SCTLR2_EL1.EASE bit into account
 * when injecting a SEA (Feat_DoubleFault2).
 */
test_result_t host_test_feat_doublefault2(void)
{
	bool ret;
	u_register_t rec_flag;
	u_register_t base;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	SKIP_TEST_IF_DOUBLE_FAULT2_NOT_SUPPORTED();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	rec_flag = RMI_RUNNABLE;

	if (!host_create_activate_realm_payload(&realm,
					(u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, &rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Allocate a page so we pass its address as first argument of
	 * the test command. The test will attempt an instruction fetch
	 * from that address, which will fail as the address will not
	 * be mapped into the Realm.
	 */
	base = (u_register_t)page_alloc(PAGE_SIZE);

	(void)host_rmi_rtt_readentry(realm.rd, base, 3L, &rtt);
	if (rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY)) {
		ERROR("wrong initial state\n");
		host_destroy_realm(&realm);
		return TEST_RESULT_FAIL;
	}

	host_shared_data_set_host_val(&realm, 0U, 0U, HOST_ARG3_INDEX, base);

	for (unsigned int i = 0U; i < 2U; i++) {
		host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U, HOST_ARG2_INDEX,
					      (unsigned long)i);

		/* Rec0 expect IA due to SEA unassigned empty page */
		ret = host_enter_realm_execute(&realm, REALM_FEAT_DOUBLEFAULT2_TEST,
							RMI_EXIT_HOST_CALL, 0U);

		if (!ret) {
			host_destroy_realm(&realm);
			return TEST_RESULT_FAIL;
		}
	}

	host_destroy_realm(&realm);
	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Create realm with a single REC
 * Test attestation process for REC
 */
test_result_t host_realm_test_attestation(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_ATTESTATION,
				RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("Realm attestation test failed\n");
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
 * @Test_Aim@ Create realm with a single REC
 * Test attestation fault for REC
 */
test_result_t host_realm_test_attestation_fault(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_ATTESTATION_FAULT,
				RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("Realm attestation fault test failed\n");
	}

	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
				__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
