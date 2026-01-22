/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <test_helpers.h>

/*
 * @brief Test FEAT_STEP2 support when the extension is enabled.
 *
 * Read mdstepop_el1 to check it doesn't trap.
 *
 * @return test_result_t
 */
test_result_t test_step2_support(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	SKIP_TEST_IF_STEP2_NOT_SUPPORTED();

	read_mdstepop_el1();

	return TEST_RESULT_SUCCESS;
#endif
}
