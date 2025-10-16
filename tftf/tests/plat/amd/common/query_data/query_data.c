/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

struct test_query test_query_list[] = {
	{
		.query_id = XPM_QID_CLOCK_GET_NUM_CLOCKS,
	},
	{
		.query_id = XPM_QID_PINCTRL_GET_NUM_PINS,
	},
	{
		.query_id = XPM_QID_PINCTRL_GET_NUM_FUNCTIONS,
	},
	{
		.query_id = XPM_QID_PINCTRL_GET_NUM_FUNCTION_GROUPS,
		.query_arg1 = PIN_FUNC_I2C0,
	},
};

/**
 * This function queries information about the platform resources.
 */
int test_query_data(void)
{
	int32_t status;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(test_query_list); i++) {
		uint32_t query_data[4] = {0};

		status = xpm_query_data(test_query_list[i].query_id,
					test_query_list[i].query_arg1,
					test_query_list[i].query_arg2,
					test_query_list[i].query_arg3,
					query_data);

		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("ERROR: Failed XPm_Query for qid 0x%x, Status: 0x%x\n",
					     test_query_list[i].query_id, status);
			return TEST_RESULT_FAIL;
		}

		tftf_testcase_printf("Query Id = %x Output = %x\n\r",
				     test_query_list[i].query_id, query_data[0]);
	}

	return TEST_RESULT_SUCCESS;
}
