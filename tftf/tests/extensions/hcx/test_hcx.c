/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tftf_lib.h>
#include <tftf.h>
#include <arch_helpers.h>
#include <arch_features.h>

/* This very simple test just ensures that HCRX_EL2 access does not trap. */
test_result_t test_feat_hcx_enabled(void)
{
#ifdef __aarch64__
	u_register_t hcrx_el2;

	/* Make sure FEAT_HCX is supported. */
	if (!get_feat_hcx_support()) {
		return TEST_RESULT_SKIPPED;
	}

	/* Attempt to read HCRX_EL2, if not enabled this should trap to EL3. */
	hcrx_el2 = read_hcrx_el2();

	/*
	 * If we make it this far, access to HCRX_EL2 was not trapped, and
	 * therefore FEAT_HCX is supported.
	 */
	if (hcrx_el2 == HCRX_EL2_INIT_VAL) {
		/*
		 * If the value of the register is the reset value, the test
		 * passed.
		 */
		return TEST_RESULT_SUCCESS;
	}
	/*
	 * Otherwise, the test fails, as the HCRX_EL2 register has
	 * not been initialized properly.
	 */
	return TEST_RESULT_FAIL;
#else
	/* Skip test if AArch32 */
	return TEST_RESULT_SKIPPED;
#endif
}
