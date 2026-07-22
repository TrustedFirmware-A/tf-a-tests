/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>
#include <tftf_lib.h>
#include <tftf.h>

/*
 * FEAT_RAS introduces EL1 system registers to query error records, which can be
 * accessed independently of any FFH/KFH handling setup or any system specific
 * RAS implementation.  Accessing these registers will check that they don't
 * cause an UNDEF via a trap to EL3.
 */
test_result_t test_ras_reg_access(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	SKIP_TEST_IF_RAS_NOT_SUPPORTED();

	/* ERRIDR_EL1 is a proxy to SCR_EL3.TERR but it also guards the others */
	if (EXTRACT(ERRIDR_EL1_NUM, read_erridr_el1()) != 0) {
		/* proxy to SCR_EL3.FIEN */
		read_errselr_el1();
		/* proxy to SCR_EL3.TWERR */
		write_errselr_el1(0);
	}

	return TEST_RESULT_SUCCESS;
#endif
}
