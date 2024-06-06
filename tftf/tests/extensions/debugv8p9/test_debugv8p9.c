/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
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

	read_mdselr_el1();
#endif
	return TEST_RESULT_SUCCESS;
}
