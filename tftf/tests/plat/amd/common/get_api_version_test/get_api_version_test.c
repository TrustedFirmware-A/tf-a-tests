/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

/*
 * This function tests the behavior of the xpm_get_api_version() function,
 * which retrieves the API version of the Energy Efficient Management
 * Interface (EEMI) on AMD-Xilinx platforms.
 */
test_result_t test_pm_api_version(void)
{
	uint32_t version = 0U;
	uint32_t major, minor;
	int32_t status;

	status = xpm_get_api_version(&version);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR reading PM API version\n", __func__);
		return TEST_RESULT_FAIL;
	}

	major = VERSION_MAJOR(version);
	minor = VERSION_MINOR(version);
	tftf_testcase_printf("%s PM API version: %d.%d\n", __func__, major, minor);

	return TEST_RESULT_SUCCESS;
}
