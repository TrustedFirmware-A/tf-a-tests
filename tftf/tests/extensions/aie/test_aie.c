/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

#define TEST_PATTERN_1		0x123456789abcdef0
#define TEST_PATTERN_2		0xf0edcba987654321

#ifdef __aarch64__
static bool check_reg(uint64_t reg_val, uint64_t expected, const char *reg_name)
{
	if (reg_val == expected) {
		return true;
	}

	NOTICE("%s unexpected value after context switch: 0x%llx\n",
	       reg_name, reg_val);

	return false;
}
#endif

/*
 * @brief Test FEAT_AIE support when the extension is enabled.
 *
 * Write to the [A]MAIR2_ELx system registers, perform a dummy SMC call, then
 * read the registers back.
 *
 * The test ensures that accesses to these registers do not trap
 * to EL3 as well as that their values are preserved correctly.
 *
 * @return test_result_t
 */
test_result_t test_aie_support(void)
{
#ifdef __aarch64__
	smc_args tsp_svc_params;

	SKIP_TEST_IF_AIE_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;

	if (IS_IN_EL2()) {
		write_mair2_el2(TEST_PATTERN_2);
		write_amair2_el2(TEST_PATTERN_2);
	}
	write_mair2_el1(TEST_PATTERN_1);
	write_amair2_el1(TEST_PATTERN_1);
	/* return value is irrelevant, just trigger a context switch */
	tftf_smc(&tsp_svc_params);

	if (IS_IN_EL2()) {
		if (!check_reg(read_mair2_el2(), TEST_PATTERN_2, "MAIR2_EL2")) {
			return TEST_RESULT_FAIL;
		}
		if (!check_reg(read_amair2_el2(), TEST_PATTERN_2,
			       "AMAIR2_EL2")) {
			return TEST_RESULT_FAIL;
		}
	}

	if (!check_reg(read_mair2_el1(), TEST_PATTERN_1, "MAIR2_EL1")) {
		return TEST_RESULT_FAIL;
	}
	if (!check_reg(read_amair2_el1(), TEST_PATTERN_1, "AMAIR2_EL1")) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
