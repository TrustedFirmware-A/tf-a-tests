/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "doe_helpers.h"

#include <test_helpers.h>

#define SKIP_TEST_IF_DOE_NOT_SUPPORTED()					\
	do {									\
		/* Test PCIe DOE only for RME */				\
		if (!get_armv9_2_feat_rme_support()) {				\
			tftf_testcase_printf("FEAT_RME not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
										\
		pcie_init();							\
		if (find_doe_device(&bdf, &doe_cap_base) != 0) {		\
			tftf_testcase_printf("PCIe DOE not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

test_result_t doe_discovery_test(void)
{
	uint32_t bdf, doe_cap_base;
	int ret;

	SKIP_TEST_IF_DOE_NOT_SUPPORTED();

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

	SKIP_TEST_IF_DOE_NOT_SUPPORTED();

	ret = get_spdm_version(bdf, doe_cap_base);
	if (ret != 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
