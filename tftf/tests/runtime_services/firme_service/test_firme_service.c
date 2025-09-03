/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <firme.h>
#include <test_helpers.h>
#include <tftf_lib.h>

static uint64_t expected_feature_reg[2] = { 0, 0 };

test_result_t test_firme_base_version(void)
{
	uint16_t version_major;
	uint16_t version_minor;
	int32_t res;

	/* First check base service version */
	res = firme_version(FIRME_BASE_SERVICE_ID);
	if (res == FIRME_NOT_SUPPORTED) {
		tftf_testcase_printf("FIRME base service not supported!\n");
		return TEST_RESULT_FAIL;
	} else if (res < 0) {
		tftf_testcase_printf(
			"FIRME_SERVICE_VERSION returned unexpected result: %d\n",
			res);
		return TEST_RESULT_FAIL;
	}

	/* Extract version fields. */
	version_major = (res >> FIRME_VERSION_MAJOR_SHIFT) &
			FIRME_VERSION_MAJOR_MASK;
	version_minor = (res >> FIRME_VERSION_MINOR_SHIFT) &
			FIRME_VERSION_MINOR_MASK;

	/* We expect 1.0 */
	if (version_major != 1 || version_minor != 0) {
		tftf_testcase_printf(
			"Error: unexpected FIRME base service version: %u.%u!\n",
			version_major, version_minor);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_firme_granule_mgmt_version(void)
{
	uint64_t feat_reg = 0U;
	uint16_t version_major;
	uint16_t version_minor;
	int32_t res;

	res = firme_features(FIRME_BASE_SERVICE_ID, 1, &feat_reg);
	if ((feat_reg & FIRME_BASE_SERVICE_GRANULE_MGMT_BIT) == 0) {
		tftf_testcase_printf("FIRME GM service not supported!\n");
		return TEST_RESULT_SKIPPED;
	}

	/* First check granule management service version */
	res = firme_version(FIRME_GM_SERVICE_ID);
	if (res == FIRME_NOT_SUPPORTED) {
		tftf_testcase_printf("FIRME GM service not supported!\n");
		return TEST_RESULT_FAIL;
	}

	/* Extract version fields. */
	version_major = (res >> FIRME_VERSION_MAJOR_SHIFT) &
			FIRME_VERSION_MAJOR_MASK;
	version_minor = (res >> FIRME_VERSION_MINOR_SHIFT) &
			FIRME_VERSION_MINOR_MASK;

	/* We expect 1.0 */
	if (version_major != 1 || version_minor != 0) {
		tftf_testcase_printf(
			"Error: unexpected FIRME granule management service version: %u.%u!\n",
			version_major, version_minor);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_firme_base_features(void)
{
	uint64_t feat_reg = 0xDEADBEEFDEADBEEF;
	uint64_t feat_reg_0_expected = 0x3;
	int32_t res;

	tftf_testcase_printf("Checking base service feature register...\n");

	/* Ensure we can get feature reg 0; feat reg 1 depends on what
	 * services are enabled so is tested separately.
	 */
	res = firme_features(FIRME_BASE_SERVICE_ID, 0, &feat_reg);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf("Error: SMC call returned %d\n", res);
		return TEST_RESULT_FAIL;
	}

	/* Check feature reg against expected value. */
	if (feat_reg != feat_reg_0_expected) {
		tftf_testcase_printf(
			"Error: received reg 0x%llx, expected 0x%llx\n",
			feat_reg, feat_reg_0_expected);
		return TEST_RESULT_FAIL;
	}

	/* Try again with invalid reg index 2. */
	res = firme_features(FIRME_BASE_SERVICE_ID, 2, &feat_reg);
	if (res != FIRME_NOT_SUPPORTED) {
		tftf_testcase_printf("Error: OOR feature reg bad return: %d\n",
				     res);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_firme_granule_mgmt_features()
{
	uint64_t feat_reg;
	int32_t res;

	res = firme_features(FIRME_BASE_SERVICE_ID, 1, &feat_reg);
	if ((feat_reg & FIRME_BASE_SERVICE_GRANULE_MGMT_BIT) == 0) {
		tftf_testcase_printf("FIRME GM service not supported!\n");
		return TEST_RESULT_SKIPPED;
	}

	tftf_testcase_printf("Checking GM service feature registers...\n");
	expected_feature_reg[0] = 0x1;
	expected_feature_reg[1] = 0x80;
	for (uint32_t i = 0; i < 2; i++) {
		feat_reg = 0xDEADBEEFDEADBEEF;

		/* Make sure we can get both feature regs. */
		res = firme_features(FIRME_GM_SERVICE_ID, i, &feat_reg);
		if (res != FIRME_SUCCESS) {
			tftf_testcase_printf("Error: SMC call returned %d\n",
					     res);
			return TEST_RESULT_FAIL;
		}

		/* Check feature reg against expected value. */
		if (feat_reg != expected_feature_reg[i]) {
			tftf_testcase_printf(
				"Error: received reg 0x%llx, expected 0x%llx\n",
				feat_reg, expected_feature_reg[i]);
			return TEST_RESULT_FAIL;
		}
	}

	/* Try again with invalid reg index 2. */
	res = firme_features(FIRME_GM_SERVICE_ID, 2, &feat_reg);
	if (res != FIRME_NOT_SUPPORTED) {
		tftf_testcase_printf("Error: OOR feature reg bad return: %d\n",
				     res);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}