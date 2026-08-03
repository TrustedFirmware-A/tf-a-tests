/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>

#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <spm_common.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#define SPM_VERSION FFA_VERSION_COMPILED

/*
 * Exercise the FF-A v1.2 version negotiation rules implemented by the EL3
 * SPMC. Keep the state transitions in one test: FF-A version negotiation is
 * endpoint state which cannot be reset between TFTF testcases.
 */
test_result_t test_el3_spmc_nwd_ffa_version(void)
{
	struct ffa_value ret;
	test_result_t result;

	/* An incompatible version should return the current negotiated version. */
	result = expect_ffa_version(SPM_VERSION + 1, SPM_VERSION);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	/* An incompatible version should return the current negotiated version. */
	result = expect_ffa_version(0, SPM_VERSION);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	/* Negotiate the version used by the normal-world endpoint. */
	result = expect_ffa_version(SPM_VERSION, SPM_VERSION);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	/* The first non-version ABI completes version negotiation. */
	ret = ffa_spm_id_get();
	if (is_ffa_call_error(ret) || ffa_endpoint_id(ret) != SPMC_ID) {
		ERROR("FFA_SPM_ID_GET failed or returned an unexpected ID.\n");
		return TEST_RESULT_FAIL;
	}

	/* A later compatible request must not change the negotiated version. */
	result = expect_ffa_version(FFA_VERSION_1_1, SPM_VERSION);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	return TEST_RESULT_SUCCESS;
}
