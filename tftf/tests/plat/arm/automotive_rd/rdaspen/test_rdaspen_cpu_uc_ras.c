/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <tftf_lib.h>
#include <rdaspen_cpu_ras_helpers.h>

test_result_t test_cpu_ras_uc(void)
{
	uint64_t erx_status;

	/* Select Error Record 1, Error record 0 is for the DSU */
	write_errselr_el1(1);

	/* Clear PFG control register */
	write_cpu_erxpfgctl_el1(0U);

	/* Clear the status register , rewriting ERX_STATUS to itself clears it */
	erx_status = read_erxstatus_el1();
	write_erxstatus_el1(erx_status);

	/* Enable the PFG counter */
	enable_cpu_pfg_cdn_register();

	/* Writes the UC bit to Arm the PFG CTRL to inject UC */
	write_cpu_pfg_ctrl_register_uc();

	/* Returning here means the expected fatal error path did not trigger. */
	return TEST_RESULT_FAIL;
}
