/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <lfa.h>
#include <lfa_test_helpers.h>
#include <test_helpers.h>
#include <tftf_lib.h>

static uint64_t num_components;
static struct lfa_test_inventory_entry rmm_component;
static struct lfa_test_inventory_entry bl31_component;

static test_result_t test_lfa_component_cmd(const struct lfa_test_target *target,
					    const struct lfa_test_inventory_entry *component,
					    uint32_t fid,
					    const char *operation)
{
	smc_args args;
	smc_ret_values ret;

	if (!component->present) {
		return TEST_RESULT_SKIPPED;
	}

	args = lfa_test_init_fw_args(fid, component->fw_id);
	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: Unexpected error for %s %s: 0x%08lx\n",
				     __func__, target->name, operation, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Simple surface tests for Live Firmware Activation.
 *
 * This test checks the version number. It runs on the lead CPU.
 */
test_result_t test_lfa_version(void)
{
	smc_args args = { LFA_VERSION };
	uint32_t major, minor;
	smc_ret_values ret = tftf_smc(&args);

	major = (uint32_t)((ret.ret0 >> LFA_VERSION_MAJOR_SHIFT) & LFA_VERSION_MAJOR_MASK);
	minor = (uint32_t)((ret.ret0 >> LFA_VERSION_MINOR_SHIFT) & LFA_VERSION_MINOR_MASK);

	VERBOSE("%s LFA API Version : %d.%d\n", __func__, major, minor);

	if ((major != LFA_VERSION_MAJOR) || (minor != LFA_VERSION_MINOR)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_FEATURES.
 *
 * This test checks LFA_FEATURES for the two extreme FID values and then does
 * one negative test.
 */
test_result_t test_lfa_features(void)
{
	smc_args args = { .fid = LFA_FEATURES, .arg1 = LFA_VERSION };
	smc_ret_values ret = tftf_smc(&args);

	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: Features for FID=0x%08lx returned %08lx\n",
				     __func__, args.arg1, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	args.arg1 = LFA_CANCEL;
	ret = tftf_smc(&args);
	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: Features for FID=0x%08lx returned %08lx\n",
				     __func__, args.arg1, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	args.arg1 = LFA_INVALID;
	ret = tftf_smc(&args);
	if (ret.ret0 == SMC_OK) {
		tftf_testcase_printf("%s: Features for FID=0x%08lx returned %08lx\n",
				     __func__, args.arg1, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_GET_INFO.
 *
 * This test checks that LFA_GET_INFO returns successfully and the proper
 * error if an invalid lfa_selector is provided.
 */
test_result_t test_lfa_get_info(void)
{
	smc_args args = { .fid = LFA_GET_INFO, .arg1 = 0U };
	smc_ret_values ret = tftf_smc(&args);

	if (ret.ret0 != SMC_OK) {
		tftf_testcase_printf("%s: Unexpected error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	num_components = ret.ret1;

	/* Try giving invalid argument and expect Failure from SMC */
	args.arg1 = 1U;
	ret = tftf_smc(&args);
	if (ret.ret0 == SMC_OK) {
		tftf_testcase_printf("%s: Unexpected success\n", __func__);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_GET_INVENTORY.
 */
test_result_t test_lfa_get_inventory(void)
{
	smc_args args = { .fid = LFA_GET_INVENTORY, .arg1 = 0U };
	smc_ret_values ret;

	for (uint64_t i = 0U; i < num_components; i++) {
		args.arg1 = i;

		ret = tftf_smc(&args);
		if (ret.ret0 != SMC_OK) {
			tftf_testcase_printf("%s: Unexpected error: 0x%08lx\n",
					     __func__, ret.ret0);
			return TEST_RESULT_FAIL;
		}

		lfa_test_record_inventory_entry(&lfa_test_rmm, i, &ret, &rmm_component);
		lfa_test_record_inventory_entry(&lfa_test_bl31, i, &ret, &bl31_component);
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_PRIME for RMM.
 */
test_result_t test_lfa_prime_rmm(void)
{
	return test_lfa_component_cmd(&lfa_test_rmm, &rmm_component, LFA_PRIME,
				      "PRIME");
}

/*
 * @Test_Aim@ Test for LFA_PRIME for BL31.
 */
test_result_t test_lfa_prime_bl31(void)
{
	return test_lfa_component_cmd(&lfa_test_bl31, &bl31_component, LFA_PRIME,
				      "PRIME");
}

/*
 * @Test_Aim@ Test for LFA_ACTIVATE for RMM.
 */
test_result_t test_lfa_activate_rmm(void)
{
	return test_lfa_component_cmd(&lfa_test_rmm, &rmm_component,
				      LFA_ACTIVATE, "ACTIVATE");
}

/*
 * @Test_Aim@ Test for LFA_CANCEL for RMM.
 */
test_result_t test_lfa_cancel_rmm(void)
{
	return test_lfa_component_cmd(&lfa_test_rmm, &rmm_component, LFA_CANCEL,
				      "CANCEL");
}

/*
 * @Test_Aim@ Test for LFA_CANCEL for BL31.
 */
test_result_t test_lfa_cancel_bl31(void)
{
	return test_lfa_component_cmd(&lfa_test_bl31, &bl31_component, LFA_CANCEL,
				      "CANCEL");
}
