/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <tftf_lib.h>
#include <rdaspen_cpu_ras_helpers.h>

test_result_t test_check_erxctlr(void)
{
	test_result_t result = TEST_RESULT_FAIL;
	uint64_t erxctrl_el1;
	uint64_t expected;

	/* Select Error Record 1, Error record 0 is for the DSU */
	write_errselr_el1(1);

	expected = ERXCTLR_CFI_BIT | ERXCTLR_FI_BIT |
		   ERXCTLR_ED_BIT | ERXCTLR_TFPEN_BIT;
	erxctrl_el1 = read_erxctlr_el1();
	/* Check that the expected ERXCTLR_EL1 control bits are set. */
	if ((erxctrl_el1 & expected) == expected)
		result = TEST_RESULT_SUCCESS;

	return result;
}
