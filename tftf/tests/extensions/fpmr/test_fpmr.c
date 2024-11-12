/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>
#include <tftf_lib.h>
#include <debug.h>
#include <arch_helpers.h>

test_result_t test_fpmr_enabled(void)
{
	SKIP_TEST_IF_AARCH32();

#if __aarch64__
	SKIP_TEST_IF_FPMR_NOT_SUPPORTED();

	read_fpmr();
#endif
	return TEST_RESULT_SUCCESS;
}
