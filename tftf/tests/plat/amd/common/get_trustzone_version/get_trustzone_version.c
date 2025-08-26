/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

/*
 * This function tests the behavior of the get_trustzone_version() function,
 * which retrieves the trustzone version.
 */
test_result_t test_trustzone_version(void)
{
	uint32_t tz_version = 0U;
	uint32_t major, minor;
	int32_t status;

	status = get_trustzone_version(&tz_version);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR reading trustzone version, "
				     "Status: 0x%x\n", __func__, status);
		return TEST_RESULT_FAIL;
	}

	major = VERSION_MAJOR(tz_version);
	minor = VERSION_MINOR(tz_version);
	tftf_testcase_printf("%s Trustzone version: %d.%d\n", __func__, major, minor);

	return TEST_RESULT_SUCCESS;
}
