/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>
#include <tftf_lib.h>
#include <debug.h>
#include <arch_helpers.h>

test_result_t test_debugv8p9_enabled(void)
{
	SKIP_TEST_IF_AARCH32();

#if __aarch64__
	SKIP_TEST_IF_DEBUGV8P9_NOT_SUPPORTED();

	if (!is_feat_debugv8p9_ebwe_supported()) {
		return TEST_RESULT_SKIPPED;
	}

	/* Is RES0 if NUM_BREAKPOINTS <= 16 */
	read_mdselr_el1();

	return TEST_RESULT_SUCCESS;
#endif
}
