/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/extensions/sysreg128.h>
#include <test_helpers.h>

#include "./test_d128.h"

/*
 * @brief Test FEAT_D128 feature support when the extension is enabled.
 *
 * The test ensures that access to common system registers PAR_EL1, TTBR0_EL1, TTBR1_EL1,
 * TTBR0_EL2, VTTBR_EL2 and the VHE register TTBR1_EL2 do not trap to EL3.
 *
 * Additonally this test verifies that EL1 registers are properly context switched by
 * making dummy SMC call to TSPD (running at S-EL1).
 *
 * TODO: EL2 registers context switching.
 *
 * @return test_result_t
 */
test_result_t test_d128_support(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_D128_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	smc_args tsp_svc_params;

	/* Access 128 bit variant of system registers */
	read128_par_el1();
	read128_ttbr0_el1();
	read128_ttbr1_el1();
	read128_ttbr0_el2();
	read128_ttbr1_el2();
	if (is_armv8_1_vhe_present()) {
		read128_vttbr_el2();
	}

	/*
	 * Check that EL1 registers are properly contexted by EL3. Write to the registers
	 * make dummy SMC call TSP and ensure that values are prorperly contexted.
	 * NOTE: Ideal way would be to modify these registers in TSP in SMC handler
	 */
	write128_par_el1(PAR_EL1_MASK_FULL);
	write128_ttbr0_el1(TTBR_REG_MASK_FULL);
	write128_ttbr1_el1(TTBR_REG_MASK_FULL);

	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;
	tftf_smc(&tsp_svc_params);

	if ((read128_par_el1() != PAR_EL1_MASK_FULL) ||
	    (read128_ttbr0_el1() != TTBR_REG_MASK_FULL) ||
	    (read128_ttbr1_el1() != TTBR_REG_MASK_FULL)) {
		NOTICE("Unexpected value of registers after context switch\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
