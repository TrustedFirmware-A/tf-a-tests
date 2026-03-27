/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdlib.h>

#include <arch_features.h>
#include <debug.h>
#include <host_realm_helper.h>
#include <host_realm_lfa.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <test_helpers.h>

test_result_t host_test_realm_rsi_version_with_lfa(void)
{
	test_result_t lfa_ret;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	bool lpa2 = false;
	u_register_t s2sz = MAX_IPA_BITS;
	long sl = RTT_MIN_LEVEL;
	bool ret1, ret2;
	struct test_realm_params params = {0};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	lfa_ret = test_lfa_version();
	if (lfa_ret != TEST_RESULT_SUCCESS) {
		ERROR("%s(): LFA is not supported=%d\n", __func__, lfa_ret);
		return TEST_RESULT_SKIPPED;
	}

	if (is_feat_52b_on_4k_2_supported()) {
		lpa2 = true;
		s2sz = MAX_IPA_BITS_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	params.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE;
	params.lpa2 = lpa2;
	params.s2sz = s2sz;
	params.sl = sl;
	params.rec_flag = rec_flag;
	params.rec_count = 1U;

	if (!host_create_activate_realm_payload(&realm, &params)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_GET_RSI_VERSION, RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("%s(): Initial enter failed\n", __func__);
		goto cleanup;
	}

	/* Activate new RMM and then request RSI version again */
	lfa_ret = test_lfa_activate_rmm();
	if (lfa_ret != TEST_RESULT_SUCCESS) {
		ERROR("%s(): LFA of RMM failed=%d\n", __func__, lfa_ret);
		goto cleanup;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_GET_RSI_VERSION, RMI_EXIT_HOST_CALL, 0U);
	if (!ret1) {
		ERROR("%s(): Enter Realm with newly activated RMM failed\n", __func__);
	}

cleanup:
	ret2 = host_destroy_realm(&realm);
	if (!ret2) {
		ERROR("%s(): Realm destroy failed\n", __func__);
	}

	return (ret1 && ret2) ? host_cmp_result() : TEST_RESULT_FAIL;
}
