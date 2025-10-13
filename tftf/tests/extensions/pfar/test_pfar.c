/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

#define TEST_PATTERN_1_48BIT	0x123456789abc
#define TEST_PATTERN_2_48BIT	0x456789abcdef

/*
 * @brief Test FEAT_PFAR support when the extension is enabled.
 *
 * Write to the PFAR_EL2 and PFAR_EL1 system registers, perform a dummy SMC
 * call, then read the registers back.
 *
 * The test ensures that accesses to these registers do not trap
 * to EL3 as well as that their values are preserved correctly.
 *
 * @return test_result_t
 */
test_result_t test_pfar_support(void)
{
#ifdef __aarch64__
	smc_args tsp_svc_params;
	uint64_t reg_read, pa_mask;

	SKIP_TEST_IF_PFAR_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;

	write_pfar_el1(GENMASK(47, 0));
	pa_mask = read_pfar_el1();
	if (!IS_POWER_OF_TWO(pa_mask + 1)) {
		NOTICE("PFAR_EL1 PA bits masked incorrectly: 0x%llx\n",
			pa_mask);
		return TEST_RESULT_FAIL;
	}

	if (IS_IN_EL2()) {
		write_pfar_el2(TEST_PATTERN_2_48BIT & pa_mask);
	}
	write_pfar_el1(TEST_PATTERN_1_48BIT & pa_mask);
	/* return value is irrelevant, just trigger a context switch */
	tftf_smc(&tsp_svc_params);

	if (IS_IN_EL2()) {
		reg_read = read_pfar_el2();
		if (reg_read != (TEST_PATTERN_2_48BIT & pa_mask)) {
			NOTICE("PFAR_EL2 unexpected value after context switch: 0x%llx\n",
				reg_read);
			return TEST_RESULT_FAIL;
		}
	}
	reg_read = read_pfar_el1();
	if (reg_read != (TEST_PATTERN_1_48BIT & pa_mask)) {
		NOTICE("PFAR_EL1 unexpected value after context switch: 0x%llx\n",
				reg_read);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
