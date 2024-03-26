/*
 * Copyright (c) 2021-2024, Arm Limited. All rights reserved.
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
#include <pauth.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_pmu.h>
#include <host_shared_data.h>

#define SLEEP_TIME_MS	20U

extern const char *rmi_exit[];

#if ENABLE_PAUTH
static uint128_t pauth_keys_before[NUM_KEYS];
static uint128_t pauth_keys_after[NUM_KEYS];
#endif

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
		if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				(u_register_t)PAGE_POOL_BASE,
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

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
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
 * @Test_Aim@ Create realm with multiple rec
 * Test PAuth registers are preserved for each rec
 */
test_result_t host_realm_enable_pauth(void)
{
#if ENABLE_PAUTH == 0
	return TEST_RESULT_SKIPPED;
#else
	bool ret1, ret2;
	u_register_t rec_flag[MAX_REC_COUNT] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
		RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,};
	struct realm realm;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	pauth_test_lib_fill_regs_and_template(pauth_keys_before);
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

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
				(u_register_t)PAGE_POOL_BASE,
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

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
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

	if (!host_create_activate_realm_payload(&realm1, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}


	if (!host_create_activate_realm_payload(&realm2, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE + PAGE_POOL_MAX_SIZE,
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

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, 10U);
	ret1 = host_enter_realm_execute(&realm, REALM_SLEEP_CMD, RMI_EXIT_HOST_CALL, 0U);
	base = (u_register_t)page_alloc(PAGE_SIZE * test_page_num);
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG2_INDEX,
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

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	base = (u_register_t)page_alloc(PAGE_SIZE);

	ret = host_realm_delegate_map_protected_data(true, &realm, base, PAGE_SIZE, base);
	if (ret != RMI_SUCCESS) {
		ERROR("host_realm_delegate_map_protected_data failede\n");
		goto destroy_realm;
	}
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base);
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
 * The Host verifies exit reason is Instr/Data abort
 */
test_result_t host_realm_abort_unassigned_destroyed(void)
{
	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, data, top;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	u_register_t rec_flag[2U] = {RMI_RUNNABLE, RMI_RUNNABLE}, base;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 2U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after DATA_CRATE_UNKNOWN\n");
		goto undelegate_destroy;
	}
	INFO("Initial state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, HOST_ARG1_INDEX, base);

	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	if (ret != RMI_SUCCESS || data != base) {
		ERROR("host_rmi_data_destroy failed\n");
		goto undelegate_destroy;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
			|| ((run->exit.esr & ISS_IFSC_MASK) < IFSC_L0_TRANS_FAULT)
			|| ((run->exit.esr & ISS_IFSC_MASK) > IFSC_L3_TRANS_FAULT)
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
		|| ((run->exit.esr & ISS_DFSC_MASK) < DFSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > DFSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault\n");
		goto undelegate_destroy;
	}
	INFO("DA FAR=0x%lx, HPFAR=0x%lx ESR= 0x%lx\n", run->exit.far, run->exit.hpfar,
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
 */
test_result_t host_realm_abort_unassigned_ram(void)
{
	bool ret1, ret2;
	u_register_t ret, top;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t rec_flag[2U] = {RMI_RUNNABLE, RMI_RUNNABLE}, base;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 2U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
	}

	/* This is dummy allocation to get a base address */
	base = (u_register_t)page_alloc(PAGE_SIZE);

	run = (struct rmi_rec_run *)realm.run[0];

	/* Set RIPAS of PAGE to RAM */
	ret = host_rmi_rtt_init_ripas(realm.rd, base, base + PAGE_SIZE, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx line=%u\n",
			"host_rmi_rtt_init_ripas", ret, __LINE__);
		goto destroy_realm;
	}
	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto destroy_realm;
	}

	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}
	INFO("Initial state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, HOST_ARG1_INDEX, base);

	/* Rec0 expect rec exit due to Instr Abort unassigned ram page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
			RMI_EXIT_SYNC, 0U);

	/* ESR.EC == 0b100000 Instruction Abort from a lower Exception level */
	if (!ret1 || ((run->exit.hpfar >> 4U) != (base >> PAGE_SIZE_SHIFT)
			|| (EC_BITS(run->exit.esr) != EC_IABORT_LOWER_EL)
			|| ((run->exit.esr & ISS_IFSC_MASK) < IFSC_L0_TRANS_FAULT)
			|| ((run->exit.esr & ISS_IFSC_MASK) > IFSC_L3_TRANS_FAULT)
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
		|| ((run->exit.esr & ISS_DFSC_MASK) < DFSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > DFSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_realm;
	}
	INFO("DA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
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
 * The Host verifies exit reason is Instr/Data abort
 */
test_result_t host_realm_abort_assigned_destroyed(void)
{
	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, top, data;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	u_register_t rec_flag[2U] = {RMI_RUNNABLE, RMI_RUNNABLE}, base;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 2U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_ASSIGNED ||
			(rtt.ripas != RMI_RAM)) {
		ERROR("wrong state after data create\n");
		goto destroy_realm;
	}
	INFO("Initial state base = 0x%lx rtt.state=0x%lx rtt.ripas=0x%lx\n",
			base, rtt.state, rtt.ripas);
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, HOST_ARG1_INDEX, base);

	if (host_realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_activate");
		goto destroy_realm;
	}

	ret = host_rmi_data_destroy(realm.rd, base, &data, &top);
	if (ret != RMI_SUCCESS || data != base) {
		ERROR("host_rmi_data_destroy failed\n");
		goto destroy_realm;
	}
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
		|| ((run->exit.esr & ISS_IFSC_MASK) < IFSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_IFSC_MASK) > IFSC_L3_TRANS_FAULT)
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
		|| ((run->exit.esr & ISS_DFSC_MASK) < DFSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > DFSC_L3_TRANS_FAULT)
		|| ((run->exit.esr & (1UL << ESR_ISS_EABORT_EA_BIT)) != 0U))) {
		ERROR("Rec did not fault ESR=0x%lx\n", run->exit.esr);
		goto destroy_data;
	}
	INFO("DA FAR=0x%lx, HPFAR=0x%lx ESR=0x%lx\n", run->exit.far, run->exit.hpfar,
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
 */
test_result_t host_realm_sea_empty(void)
{
	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base, esr;
	struct realm realm;
	struct rtt_entry rtt;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 4U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
	}

	base = (u_register_t)page_alloc(PAGE_SIZE);

	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
	if (rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY)) {
		ERROR("wrong initial state\n");
		goto destroy_realm;
	}
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, 1U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, 2U, HOST_ARG1_INDEX, base);
	host_shared_data_set_host_val(&realm, 3U, HOST_ARG1_INDEX, base);

	/* Rec0 expect IA due to SEA unassigned empty page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
			RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("Rec0 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, 0U, HOST_ARG2_INDEX);
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
	esr = host_shared_data_get_realm_val(&realm, 1U, HOST_ARG2_INDEX);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	esr = host_shared_data_get_realm_val(&realm, 2U, HOST_ARG2_INDEX);
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
	esr = host_shared_data_get_realm_val(&realm, 3U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_DABORT_CUR_EL)) {
		ERROR("Rec3 incorrect ESR=0x%lx\n", esr);
	}
	INFO("Rec3 ESR=0x%lx\n", esr);
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
 */
test_result_t host_realm_sea_unprotected(void)
{

	bool ret1, ret2;
	test_result_t res = TEST_RESULT_FAIL;
	u_register_t ret, base, base_ipa, esr;
	unsigned int host_call_result;
	u_register_t exit_reason;
	struct realm realm;
	struct rtt_entry rtt;
	struct rmi_rec_run *run;
	u_register_t rec_flag[2U] = {RMI_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 2U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
	}

	/* Can choose any unprotected IPA adr, TFTF_BASE chosen for convenience */
	base = TFTF_BASE;
	base_ipa = base | (1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
					realm.rmm_feat_reg0) - 1U));


	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
	if (rtt.state != RMI_UNASSIGNED) {
		ERROR("wrong state\n");
		goto destroy_realm;
	}

	run = (struct rmi_rec_run *)realm.run[0];
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base_ipa);
	host_shared_data_set_host_val(&realm, 1U, HOST_ARG1_INDEX, base_ipa);

	/* Rec0 expect SEA in realm due to IA unprotected IPA page */
	ret1 = host_enter_realm_execute(&realm, REALM_INSTR_FETCH_CMD,
			RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("Rec0 did not fault\n");
		goto destroy_realm;
	}

	/* get ESR set by Realm exception handler */
	esr = host_shared_data_get_realm_val(&realm, 0U, HOST_ARG2_INDEX);
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
		|| ((run->exit.esr & ISS_DFSC_MASK) < DFSC_L0_TRANS_FAULT)
		|| ((run->exit.esr & ISS_DFSC_MASK) > DFSC_L3_TRANS_FAULT)
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
	esr = host_shared_data_get_realm_val(&realm, 1U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_NO_WALK_SEA) || (EC_BITS(esr) != EC_DABORT_CUR_EL)) {
		ERROR("Rec1 incorrect ESR=0x%lx\n", esr);
		goto destroy_realm;
	}
	INFO("Rec1 ESR=0x%lx\n", esr);
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
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
	RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE}, dit;

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

	/* Enable FEAT_DIT on Host */
	write_dit(DIT_BIT);
	for (unsigned int i = 0; i < MAX_REC_COUNT; i++) {
		host_shared_data_set_host_val(&realm, i, HOST_ARG1_INDEX, 10U);
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
		ret = host_rmi_rtt_readentry(realm->rd, base, 3U, &rtt);
		if (ret != RMI_SUCCESS || rtt.walk_level != 2U || rtt.state != RMI_UNASSIGNED
				|| (rtt.ripas != RMI_EMPTY)) {
			base += RTT_MAP_SIZE(2U);
			continue;
		}
		break;
	}

	INFO("base = 0x%lx\n", base);

	/* Create L3 RTT entries */
	ret = host_rmi_create_rtt_levels(realm, base, rtt.walk_level, 3U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3U, &rtt);
	if (rtt.walk_level != 3U) {
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
	ret = host_rmi_rtt_readentry(realm->rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_destroy(realm->rd, base, 3U, &out_rtt, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_rtt_destroy failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}
	ret = host_rmi_granule_undelegate(out_rtt);

	/* Walk should terminate at L2 after RTT_DESTROY */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3U, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED || rtt.walk_level != 2U) {
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
		ret = host_rmi_rtt_readentry(realm->rd, base, 3U, &rtt);
		if (ret != RMI_SUCCESS || rtt.walk_level != 2U || rtt.state != RMI_UNASSIGNED
				|| (rtt.ripas != RMI_EMPTY)) {
			base += RTT_MAP_SIZE(2U);
			continue;
		}
		break;
	}

	INFO("base = 0x%lx\n", base);

	/* Create L3 RTT entries */
	ret = host_rmi_create_rtt_levels(realm, base, rtt.walk_level, 3U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* L3 entry should be created */
	ret = host_rmi_rtt_readentry(realm->rd, base, 3U, &rtt);
	if (rtt.walk_level != 3U) {
		ERROR("host_rmi_create_rtt_levels failed ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* Destroy newly created rtt, for protected IPA there should be no live L3 entry */
	ret = host_rmi_rtt_destroy(realm->rd, base, 3U, &out_rtt, &top);
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
	ret = host_rmi_rtt_readentry(realm->rd, base, 3U, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			rtt.ripas != RMI_DESTROYED || rtt.walk_level != 2U) {
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

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 2U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
	}

	INFO("Test 1\n");
	base = (u_register_t)page_alloc(PAGE_SIZE);

	/* Create level 3 RTT */
	ret = host_rmi_create_rtt_levels(&realm, base, 3U, 3U);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels failed\n");
		goto destroy_realm;
	}

	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
	if (ret != RMI_SUCCESS || rtt.state != RMI_UNASSIGNED ||
			(rtt.ripas != RMI_EMPTY) || rtt.walk_level != 3U) {
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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
	ret = host_rmi_rtt_readentry(realm.rd, base, 3U, &rtt);
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

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 1U)) {
		goto destroy_realm;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
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
	u_register_t base_ipa, esr, feature_flag, base;
	struct realm realm;
	u_register_t rec_flag[4U] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE};
	struct rmi_rec_run *run;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 0x2CU);
	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			feature_flag, rec_flag, 4U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(&realm, NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		goto destroy_realm;
	}

	/* Any Adr */
	base = TFTF_BASE;
	/* IPA outside Realm space */
	base_ipa = base | (1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
					realm.rmm_feat_reg0) + 1U));
	host_shared_data_set_host_val(&realm, 0U, HOST_ARG1_INDEX, base_ipa);
	host_shared_data_set_host_val(&realm, 1U, HOST_ARG1_INDEX, base_ipa);

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
	esr = host_shared_data_get_realm_val(&realm, 0U, HOST_ARG2_INDEX);
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
	esr = host_shared_data_get_realm_val(&realm, 1U, HOST_ARG2_INDEX);
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

	host_shared_data_set_host_val(&realm, 2U, HOST_ARG1_INDEX, base_ipa);
	host_shared_data_set_host_val(&realm, 3U, HOST_ARG1_INDEX, base_ipa);

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
	esr = host_shared_data_get_realm_val(&realm, 2U, HOST_ARG2_INDEX);
	if (((esr & ISS_DFSC_MASK) != DFSC_L0_ADR_SIZE_FAULT)
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
	esr = host_shared_data_get_realm_val(&realm, 3U, HOST_ARG2_INDEX);
	if (((esr & ISS_IFSC_MASK) != IFSC_L0_ADR_SIZE_FAULT)
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
