/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>
#include <tftf_lib.h>
#include <tftf.h>

/*
 * FEAT_RAS introduces EL1 system registers to query error records, those
 * CPU specific parts of the RAS extension can be accessed independently of
 * any FFH/KFH handling setup or any system specific RAS implementation.
 * Reading these registers will trap to EL3 and crash when EL3 has not
 * allowed access, which is controlled by the SCR_EL3.TERR bit.
 */

test_result_t test_ras_reg_access(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	SKIP_TEST_IF_RAS_NOT_SUPPORTED();

	read_erridr_el1();

	return TEST_RESULT_SUCCESS;
#endif
}
