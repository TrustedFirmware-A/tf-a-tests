/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

/*
 * @brief Test FEAT_SCTLR2 support when the extension is enabled.
 *
 * Ensure that reading of SCTLR2_EL1 and SCTLR2_EL2 system registers does not
 * trap to EL3. Also, verify that SCTLR2_EL1 register value is preserved
 * correctly during world switch to TSP.
 *
 * TODO: Test context switching of SCTLR2_EL2
 *
 * @return test_result_t
 */
test_result_t test_sctlr2_support(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_SCTLR2_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	smc_args tsp_svc_params;

	/* Access the registers */
	read_sctlr2_el1();
	read_sctlr2_el2();

	/* Check that SCTLR2_EL1 is properly contexted by EL3. Write to the registers
	 * make dummy SMC call TSP and ensure that values are prorperly contexted.
	 * Set EnIDCP128_BIT which is guaranteed to be present if SCTLR2 is present.
	 */
	write_sctlr2_el1(read_sctlr2_el1() | SCTLR2_EnIDCP128_BIT);

	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;
	tftf_smc(&tsp_svc_params);

	if ((read_sctlr2_el1() & SCTLR2_EnIDCP128_BIT) == 0) {
		ERROR("SCTLR2_EL1 unexpected value after context switch\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
