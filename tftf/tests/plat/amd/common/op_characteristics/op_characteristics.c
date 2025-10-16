/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

/*
 * This function will request the power management controller to
 * return information about an operating characteristic of a component.
 */
test_result_t test_op_characteristics(void)
{
	int32_t status;
	uint32_t result;
	uint32_t type = PM_OPCHAR_TYPE_TEMP;

	status = xpm_op_characteristics(PM_DEV_SOC, type, &result);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR getting op characteristics, type = %x, "
				     "Status: 0x%x\n", __func__, type, status);
		return TEST_RESULT_FAIL;
	}
	tftf_testcase_printf("Temp = %x\n", result);

	return TEST_RESULT_SUCCESS;
}

/*
 * This function will request the power management controller to
 * return information about an operating characteristic of a component,
 * but with an invalid parameter to test error handling.
 */
test_result_t test_op_characteristics_invalid_param(void)
{
	int32_t status;
	uint32_t result;
	uint32_t type = PM_OPCHAR_TYPE_TEMP;

	status = xpm_op_characteristics(PM_DEV_USB_0, type, &result);
	if (status == PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR getting op characteristics, type = %x, "
				     "Status: 0x%x\n", __func__, type, status);
		return TEST_RESULT_FAIL;
	}
	tftf_testcase_printf("Temp = %x\n", result);

	return TEST_RESULT_SUCCESS;
}
