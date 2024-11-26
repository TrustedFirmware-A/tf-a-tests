/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/extensions/sysreg128.h>
#include <test_helpers.h>

#include "./test_the.h"

/*
 * @brief Test FEAT_THE support when the extension is enabled.
 *
 * Write to the RCWMASK_EL1 and RCWSMASK_EL1 system registers with
 * a mask of allowed bits in 128-bit mode, if FEAT_D128 is supported,
 * otherwise in 64-bit mode, perform a dummy SMC call, then read the
 * registers back.
 *
 * The test ensures that accesses to these registers do not trap
 * to EL3 as well as that their values are preserved correctly.
 *
 * @return test_result_t
 */
test_result_t test_the_support(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_THE_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	smc_args tsp_svc_params;
	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;

	if (is_feat_d128_supported()) {
		uint128_t reg_read;

		write128_rcwmask_el1(RCWMASK_EL1_MASK_FULL);
		write128_rcwsmask_el1(RCWSMASK_EL1_MASK_FULL);
		tftf_smc(&tsp_svc_params);

		reg_read = read128_rcwmask_el1();
		if (reg_read != RCWMASK_EL1_MASK_FULL) {
			NOTICE("RCWMASK_EL1 unexpected value after context switch: 0x%llx:0x%llx\n",
				(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
			return TEST_RESULT_FAIL;
		}

		reg_read = read128_rcwsmask_el1();
		if (reg_read != RCWSMASK_EL1_MASK_FULL) {
			NOTICE("RCWSMASK_EL1 unexpected value after context switch: 0x%llx:0x%llx\n",
				(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
			return TEST_RESULT_FAIL;
		}

		return TEST_RESULT_SUCCESS;
	}

	uint64_t reg_read;

	write_rcwmask_el1(RCWMASK_EL1_MASK_LOW);
	write_rcwsmask_el1(RCWSMASK_EL1_MASK_LOW);
	tftf_smc(&tsp_svc_params);

	reg_read = read_rcwmask_el1();
	if (reg_read != RCWMASK_EL1_MASK_LOW) {
		NOTICE("RCWMASK_EL1 unexpected value after context switch: 0x%llx\n", reg_read);
		return TEST_RESULT_FAIL;
	}

	reg_read = read_rcwsmask_el1();
	if (reg_read != RCWSMASK_EL1_MASK_LOW) {
		NOTICE("RCWSMASK_EL1 unexpected value after context switch: 0x%llx\n", reg_read);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
