/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_nodeid.h"

/*
 * This function is called to notify the power management controller
 * about the completed power management initialization.
 */
test_result_t test_init_finalize(void)
{
	int32_t status;

	status = xpm_init_finalize();
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR Power management initialization, "
				     "Status: 0x%x\n", __func__, status);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
