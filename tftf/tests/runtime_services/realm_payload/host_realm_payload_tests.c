/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
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
#include <pauth.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_pmu.h>
#include <host_shared_data.h>

#define SLEEP_TIME_MS	20U

extern const char *rmi_exit[];

/*
 * @Test_Aim@ Test realm payload creation, execution and destruction  iteratively
 */
test_result_t host_test_realm_create_enter(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	for (unsigned int i = 0U; i < 5U; i++) {
		if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				(u_register_t)PAGE_POOL_BASE,
				(u_register_t)(PAGE_POOL_MAX_SIZE +
				NS_REALM_SHARED_MEM_SIZE),
				(u_register_t)PAGE_POOL_MAX_SIZE,
				0UL, rec_flag, 1U)) {
			return TEST_RESULT_FAIL;
		}
		if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
				NS_REALM_SHARED_MEM_SIZE)) {
			return TEST_RESULT_FAIL;
		}

		host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, SLEEP_TIME_MS);
		ret1 = host_enter_realm_execute(&realm, REALM_SLEEP_CMD, RMI_EXIT_HOST_CALL, 0U);
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

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
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
 * @Test_Aim@ Test PAuth in realm
 */
test_result_t host_realm_enable_pauth(void)
{
#if ENABLE_PAUTH == 0
	return TEST_RESULT_SKIPPED;
#else
	bool ret1, ret2;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	pauth_test_lib_fill_regs_and_template();
	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				(u_register_t)PAGE_POOL_BASE,
				(u_register_t)(PAGE_POOL_MAX_SIZE +
					NS_REALM_SHARED_MEM_SIZE),
				(u_register_t)PAGE_POOL_MAX_SIZE,
				0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
				NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_PAUTH_SET_CMD, RMI_EXIT_HOST_CALL, 0U);

	if (ret1) {
		/* Re-enter Realm to compare PAuth registers. */
		ret1 = host_enter_realm_execute(&realm, REALM_PAUTH_CHECK_CMD,
				RMI_EXIT_HOST_CALL, 0U);
	}

	ret2 = host_destroy_realm(&realm);

	if (!ret1) {
		ERROR("%s(): enter=%d destroy=%d\n",
				__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	/* Check if PAuth keys are preserved. */
	if (!pauth_test_lib_compare_template()) {
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

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				(u_register_t)PAGE_POOL_BASE,
				(u_register_t)(PAGE_POOL_MAX_SIZE +
					NS_REALM_SHARED_MEM_SIZE),
				(u_register_t)PAGE_POOL_MAX_SIZE,
				0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
				NS_REALM_SHARED_MEM_SIZE)) {
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
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	bool ret1, ret2;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	host_set_pmu_state();

	feature_flag = RMI_FEATURE_REGISTER_0_PMU_EN |
			INPLACE(FEATURE_PMU_NUM_CTRS, (unsigned long long)(-1));

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			feature_flag, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, cmd,
					(cmd == REALM_PMU_INTERRUPT) ?
					RMI_EXIT_IRQ : RMI_EXIT_HOST_CALL, 0U);
	if (!ret1 || (cmd != REALM_PMU_INTERRUPT)) {
		goto test_exit;
	}

	ret1 = host_realm_handle_irq_exit(&realm, 0U);

test_exit:
	ret2 = host_destroy_realm(&realm);
	if (!ret1 || !ret2) {
		ERROR("%s() enter=%u destroy=%u\n", __func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	if (!host_check_pmu_state()) {
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

/*
 * Test PMU interrupt functionality in Realm
 */
test_result_t host_realm_pmuv3_overflow_interrupt(void)
{
	/* Register PMU IRQ handler */
	int ret = tftf_irq_register_handler(PMU_PPI, host_overflow_interrupt);

	if (ret != 0) {
		tftf_testcase_printf("Failed to %sregister IRQ handler\n",
					"");
		return TEST_RESULT_FAIL;
	}

	tftf_irq_enable(PMU_PPI, GIC_HIGHEST_NS_PRIORITY);
	return host_test_realm_pmuv3(REALM_PMU_INTERRUPT);
}

/*
 * Test aim to create, enter and destroy 2 realms
 * Host created 2 realms with 1 rec each
 * Host enters both rec sequentially
 * Verifies both realm returned success
 * Destroys both realms
 */
test_result_t host_test_multiple_realm_create_enter(void)
{
	bool ret1, ret2, ret3;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm1, realm2;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload(&realm1, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}


	if (!host_create_realm_payload(&realm2, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE + PAGE_POOL_MAX_SIZE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		ret2 = host_destroy_realm(&realm1);
		return TEST_RESULT_FAIL;
	}

	if (!host_create_shared_mem(&realm1, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		ret1 = false;
		goto destroy_realms;
	}

	if (!host_create_shared_mem(&realm2, NS_REALM_SHARED_MEM_BASE +
				NS_REALM_SHARED_MEM_SIZE, NS_REALM_SHARED_MEM_SIZE)) {
		ret1 = false;
		goto destroy_realms;
	}

	host_shared_data_set_host_val(&realm1, 0U, HOST_ARG1_INDEX, SLEEP_TIME_MS);
	ret1 = host_enter_realm_execute(&realm1, REALM_SLEEP_CMD, RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		goto destroy_realms;
	}
	host_shared_data_set_host_val(&realm2, 0U, HOST_ARG1_INDEX, SLEEP_TIME_MS);
	ret1 = host_enter_realm_execute(&realm2, REALM_SLEEP_CMD, RMI_EXIT_HOST_CALL, 0U);

destroy_realms:
	ret2 = host_destroy_realm(&realm1);
	ret3 = host_destroy_realm(&realm2);

	if (!ret3 || !ret2) {
		ERROR("destroy failed\n");
		return TEST_RESULT_FAIL;
	}

	if (!ret1) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
