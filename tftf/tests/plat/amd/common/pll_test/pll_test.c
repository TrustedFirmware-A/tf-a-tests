/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_nodeid.h"

struct test_pll_api test_pll[] = {
	{
		.clock_id = PM_CLK_RPU_PLL,
	},
};

/*
 * This function is used to set the parameters for specified PLL clock.
 */
test_result_t test_pll_set_parameter(void)
{
	int32_t status, i;

	for (i = 0; i < ARRAY_SIZE(test_pll); i++) {
		uint32_t clock_id = test_pll[i].clock_id;

		status = xpm_pll_set_parameter(clock_id, PM_PLL_PARAM_ID_FBDIV, 10);
		if (status == PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR PLL Set Parameter for Clock ID: 0x%x, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function is used to get the parameter of specified PLL clock.
 */
test_result_t test_pll_get_parameter(void)
{
	int32_t status, i;
	uint32_t value;

	for (i = 0; i < ARRAY_SIZE(test_pll); i++) {
		uint32_t clock_id = test_pll[i].clock_id;

		status = xpm_pll_get_parameter(clock_id, PM_PLL_PARAM_ID_FBDIV, &value);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR PLL Get Parameter for Clock ID: 0x%x, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}
		tftf_testcase_printf("Value = %x\n\r", value);
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function is used to set the mode of specified PLL clock
 */
test_result_t test_pll_set_mode(void)
{
	int32_t status, i;

	for (i = 0; i < ARRAY_SIZE(test_pll); i++) {
		uint32_t clock_id = test_pll[i].clock_id;

		status = xpm_pll_set_mode(clock_id, PM_PLL_MODE_RESET);
		if (status == PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR PLL Set Mode for Clock ID: 0x%x, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function is used to get the mode of specified PLL clock
 */
test_result_t test_pll_get_mode(void)
{
	int32_t status, i;
	uint32_t value;

	for (i = 0; i < ARRAY_SIZE(test_pll); i++) {
		uint32_t clock_id = test_pll[i].clock_id;

		status = xpm_pll_get_mode(clock_id, &value);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR PLL Get Mode for Clock ID: 0x%x, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}
		tftf_testcase_printf("Mode = %x\n\r", value);
	}

	return TEST_RESULT_SUCCESS;
}
