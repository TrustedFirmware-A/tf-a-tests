/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

/*
 * @brief Test FEAT_SRMASK support when the extension is enabled.
 *
 * Read sctlrmask_el1 to check it doesn't trap, i.e. the register exists.
 *
 * @return test_result_t
 */
test_result_t test_srmask_support(void)
{
	/* SRMASK is an AArch64-only feature.*/
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	SKIP_TEST_IF_SRMASK_NOT_SUPPORTED();

	read_sctlrmask_el1();

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}
