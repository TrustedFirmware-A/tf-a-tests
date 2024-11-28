/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "doe_helpers.h"
#include <pcie_doe.h>
#include <test_helpers.h>

test_result_t doe_discovery_test(void)
{
	uint32_t bdf, doe_cap_base;
	int ret;

	SKIP_TEST_IF_DOE_NOT_SUPPORTED(bdf, doe_cap_base);

	ret = doe_discovery(bdf, doe_cap_base);
	if (ret != 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t spdm_version_test(void)
{
	uint32_t bdf, doe_cap_base;
	int ret;

	SKIP_TEST_IF_DOE_NOT_SUPPORTED(bdf, doe_cap_base);

	ret = get_spdm_version(bdf, doe_cap_base);
	if (ret != 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
