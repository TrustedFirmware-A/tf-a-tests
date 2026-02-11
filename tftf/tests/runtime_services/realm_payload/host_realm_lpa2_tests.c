/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>

static struct realm realm;

/*
 * @Test_Aim@ Test realm creation with no LPA2 and -1 RTT starting level
 */
test_result_t host_test_realm_no_lpa2_invalid_sl(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct test_realm_params params = {0};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.sl = RTT_MIN_LEVEL_LPA2;
	params.rec_flag = rec_flag;
	params.rec_count = 1U;

	if (!host_create_activate_realm_payload(&realm, &params)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Test realm creation with no LPA2 and S2SZ > 48 bits
 */
test_result_t host_test_realm_no_lpa2_invalid_s2sz(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct test_realm_params params = {0};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.s2sz = 50UL;
	params.sl = RTT_MIN_LEVEL;
	params.rec_flag = rec_flag;
	params.rec_count = 1U;

	if (!host_create_activate_realm_payload(&realm, &params)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Test creating a Realm with LPA2 disabled but FEAT_LPA2 present
 * on the platform.
 * The Realm creation should succeed.
 */
test_result_t host_test_non_lpa2_realm_on_lpa2plat(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	struct test_realm_params params = {0};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported() == false) {
		return TEST_RESULT_SKIPPED;
	}

	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.s2sz = 48UL;
	params.sl = RTT_MIN_LEVEL;
	params.rec_flag = rec_flag;
	params.rec_count = 1U;

	if (!host_create_activate_realm_payload(&realm, &params)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_destroy_realm(&realm)) {
		ERROR("%s(): failed to destroy realm\n", __func__);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Create a Realm with LPA2 disabled but FEAT_LPA2 present
 * on the platform.
 * Test that RMI_RTT_MAP_UNPROTECTED and RMI_RTT_CREATE commands fails if PA >= 48 bits
 */
test_result_t host_test_data_bound_non_lpa2_realm_on_lpa2plat(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t ret, base, base_ipa;
	test_result_t result = TEST_RESULT_FAIL;
	struct test_realm_params params = {0};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported() == false) {
		return TEST_RESULT_SKIPPED;
	}

	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.s2sz = 48UL;
	params.sl = RTT_MIN_LEVEL;
	params.rec_flag = rec_flag;
	params.rec_count = 1U;

	if (!host_create_activate_realm_payload(&realm, &params)) {
		return TEST_RESULT_FAIL;
	}

	/* Can choose any unprotected IPA adr, TFTF_BASE chosen for convenience */
	base = TFTF_BASE;
	base = base | (1UL << 48UL);

	ret = host_realm_map_unprotected(&realm, base, PAGE_SIZE);

	if (ret == REALM_SUCCESS) {
		ERROR("host_realm_map_unprotected should have failed\n");
		goto destroy_realm;
	}

	base_ipa = base | (1UL << (realm.s2sz - 1UL));

	ret = host_rmi_create_rtt_levels(&realm, base_ipa, RTT_MIN_LEVEL, 3);

	if (ret == REALM_SUCCESS) {
		ERROR("host_rmi_create_rtt_levels should have failed\n");
		goto destroy_realm;
	}

	result = TEST_RESULT_SUCCESS;

destroy_realm:
	if (!host_destroy_realm(&realm)) {
		ERROR("%s(): failed to destroy realm\n", __func__);
		return TEST_RESULT_FAIL;
	}

	return result;
}

/*
 * @Test_Aim@ Test creating a Realm payload with LPA2 enabled on a platform
 * which does not implement FEAT_LPA2.
 * Realm creation must fail.
 */
test_result_t host_test_lpa2_realm_on_non_lpa2plat(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	bool lpa2 = false;
	u_register_t s2sz = MAX_IPA_BITS;
	struct test_realm_params params = {0};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported() == true) {
		return TEST_RESULT_SKIPPED;
	} else {
		lpa2 = true;
	}

	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.lpa2 = lpa2;
	params.s2sz = s2sz;
	params.sl = RTT_MIN_LEVEL;
	params.rec_flag = rec_flag;
	params.rec_count = 1U;

	if (!host_create_activate_realm_payload(&realm, &params)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);

	return TEST_RESULT_FAIL;
}

