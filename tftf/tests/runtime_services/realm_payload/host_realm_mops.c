/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <test_helpers.h>

/*
 * Test FEAT_MOPS enablement by RMM
 *
 * Launch a Realm and execute MOPS CPY instructions inside it.
 *
 * - If FEAT_MOPS is present, RMM must enable it for Realms: the CPY sequence
 *   should complete without trapping and the copied data must match.
 * - If FEAT_MOPS is absent, the CPY instructions encode as UNDEFINED and must
 *   trap to R-EL1 as EC_UNKNOWN (3 traps, one per CPY{F,M,E}P instruction).
 *
 * Both outcomes are verified on the Realm side; the host simply checks that
 * the Realm finished with HOST_CALL_EXIT_SUCCESS_CMD.
 */
test_result_t host_realm_feat_mops(void)
{
	unsigned long rec_flag[] = {RMI_RUNNABLE};
	unsigned long feature_flag = 0UL;
	unsigned long feature_flag1 = 0UL;
	struct realm realm;
	long sl = RTT_MIN_LEVEL;
	test_result_t test_result = TEST_RESULT_SUCCESS;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported()) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
						feature_flag, feature_flag1, sl,
						rec_flag, 1U, 0U,
						get_test_mecid())) {
		return TEST_RESULT_FAIL;
	}

	if (!host_enter_realm_execute(&realm, REALM_FEAT_MOPS,
				      RMI_EXIT_HOST_CALL, 0)) {
		test_result = TEST_RESULT_FAIL;
	}

	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return test_result;
}

