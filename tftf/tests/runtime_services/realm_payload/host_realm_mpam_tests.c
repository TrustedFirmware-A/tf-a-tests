/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <debug.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <test_helpers.h>

static test_result_t realm_test_feat_mpam(enum realm_cmd cmd)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag0 = 0UL;
	long sl = RTT_MIN_LEVEL;

	assert((cmd >= REALM_MPAM_ACCESS) && (cmd <= REALM_MPAM_PRESENT));

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();
	SKIP_TEST_IF_FEAT_MPAM_NOT_SUPPORTED();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag0 = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag0, 0UL, sl, rec_flag, 1U, 0U, get_test_mecid())) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, cmd, RMI_EXIT_HOST_CALL, 0U);
	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%u destroy=%u\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return true;
}

/*
 * @Test_Aim@ Test that FEAT_MPAM is hidden to the realm
 */
test_result_t host_realm_hide_feat_mpam(void)
{
	return realm_test_feat_mpam(REALM_MPAM_PRESENT);
}

/*
 * @Test_Aim@ Test that access to MPAM registers triggers an undefined abort
 * taken into the realm.
 */
test_result_t host_realm_mpam_undef_abort(void)
{
	return realm_test_feat_mpam(REALM_MPAM_ACCESS);
}
