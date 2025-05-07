/*
 * Copyright (c) 2021-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdlib.h>

#include <arch_features.h>
#include <debug.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <test_helpers.h>

/*
 * @Test_Aim@ Test MECID assignment to Realms
 * Test whether two realms accept different MECIDs
 */
test_result_t host_realm_test_mecid(void)
{
	bool ret1 = false, ret2 = false, fail = false;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm1, realm2;
	u_register_t feature_flag0 = 0UL;
	unsigned long feat_reg1;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!is_feat_mec_supported()) {
		return TEST_RESULT_SKIPPED;
	}

	/* Only test when RMM v1.1 is supported */
	if ((host_rmi_features(1UL, &feat_reg1) != 0UL) || (feat_reg1 == 0UL)) {
		return TEST_RESULT_SKIPPED;
	}

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm1, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, 1U, 0U, TEST_MECID1)) {
		fail = true;
	}

	if (!host_create_activate_realm_payload(&realm2, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, 1U, 0U, TEST_MECID2)) {
		fail = true;
		goto destroy_realm1;
	}

	ret2 = host_destroy_realm(&realm2);
destroy_realm1:
	ret1 = host_destroy_realm(&realm1);

	if (fail || !ret1 || !ret2) {
		ERROR("%s(): fail=%d destroy1=%d destroy2=%d\n",
				__func__, fail, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test MECID assignment to Realms
 * Test whether two realms accept the same MECID
 */
test_result_t host_realm_test_mecid_fault(void)
{
	bool ret1 = false, ret2 = false, fail = false;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm1, realm2;
	u_register_t feature_flag0 = 0UL;
	unsigned long feat_reg1;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!is_feat_mec_supported()) {
		return TEST_RESULT_SKIPPED;
	}

	/* Only test when RMM v1.1 is supported */
	if ((host_rmi_features(1UL, &feat_reg1) != 0UL) || (feat_reg1 == 0UL)) {
		return TEST_RESULT_SKIPPED;
	}

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm1, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, 1U, 0U, TEST_MECID1)) {
		fail = true;
	}

	if (!host_create_activate_realm_payload(&realm2, (u_register_t)REALM_IMAGE_BASE,
				feature_flag0, 0U, sl, rec_flag, 1U, 0U, TEST_MECID1)) {
		/*
		 * Creation should fail as there should not be two Realms with the same
		 * MECID.
		 */
		ret2 = true;

		goto destroy_realm1;
	}
	fail = true;

	ret2 = host_destroy_realm(&realm2);
destroy_realm1:
	ret1 = host_destroy_realm(&realm1);

	if (fail || !ret1 || !ret2) {
		ERROR("%s(): fail=%d destroy1=%d destroy2=%d\n",
				__func__, fail, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
