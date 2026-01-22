/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <test_helpers.h>

/*
 * @brief Test FEAT_HACDBS support when the extension is enabled.
 *
 * Read hacdbsbr_el2 to check it doesn't trap.
 *
 * @return test_result_t
 */
test_result_t test_hacdbs_support(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	SKIP_TEST_IF_HACDBS_NOT_SUPPORTED();

	if (IS_IN_EL2()) {
                read_hacdbsbr_el2();

                return TEST_RESULT_SUCCESS;
        }

        return TEST_RESULT_SKIPPED;
#endif
}
