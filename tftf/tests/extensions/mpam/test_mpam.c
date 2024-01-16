/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>
#include <tftf_lib.h>
#include <tftf.h>

/* EL3 is expected to allow access to MPAM system registers from EL2.
 * Reading these registers will trap to EL3 and crash when EL3 has not
 * allowed access.
 */

test_result_t test_mpam_reg_access(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	SKIP_TEST_IF_MPAM_NOT_SUPPORTED();

	read_mpamidr_el1();
	read_mpam2_el2();

	return TEST_RESULT_SUCCESS;
#endif
}
