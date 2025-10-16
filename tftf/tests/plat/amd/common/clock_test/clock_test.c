/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

#define PM_CLOCK_ENABLE		1U
#define PM_CLOCK_DISABLE	0U

struct test_clocks {
	uint32_t clock_id;
	uint32_t device_id;
};

struct test_clocks test_clock_list[] = {
	{
		.clock_id = PM_CLK_QSPI_REF,
		.device_id = PM_DEV_QSPI,
	},
	{
		.clock_id = PM_CLK_GEM0_REF,
		.device_id = PM_DEV_GEM_0,
	},
};

/*
 * This function will test the clock apis without requesting the device.
 * function should return an error when enabling the clock of a node
 * that is not requested.
 */
test_result_t test_clock_control_without_device_request(void)
{
	int32_t test_clock_num = ARRAY_SIZE(test_clock_list);
	uint32_t result = 0U;
	int32_t status, i;

	for (i = 0; i < test_clock_num; i++) {
		uint32_t clock_id  = test_clock_list[i].clock_id;
		uint32_t device_id = test_clock_list[i].device_id;

		status = xpm_clock_get_status(clock_id, &result);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the clock status of 0x%x clockid, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		if (result == PM_CLOCK_ENABLE) {
			status = xpm_clock_disable(clock_id);
			if (status != PM_RET_SUCCESS) {
				tftf_testcase_printf("%s ERROR to disable 0x%x clock, "
						     "Status: 0x%x\n", __func__, clock_id, status);
				return TEST_RESULT_FAIL;
			}

			status = xpm_release_node(device_id);
			if (status != PM_RET_SUCCESS) {
				tftf_testcase_printf("%s ERROR to release 0x%x node, "
						     "Status: 0x%x\n", __func__, device_id, status);
				return TEST_RESULT_FAIL;
			}
		}

		status = xpm_clock_enable(clock_id);
		if (status == PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR clock 0x%x enabled without device request, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function will test clock apis after requesting the device.
 * First it will request a device later it will test all controls of
 * clock api like enabling, disabling, set parent, get parent,
 * set divider, get divider etc., after performing all clock apis it
 * will disable the particular clock.
 */
test_result_t test_clock_control_with_device_request(void)
{
	int32_t test_clock_num = ARRAY_SIZE(test_clock_list);
	int32_t parent_id = 1U;
	int32_t divider = 10U;
	uint32_t result = 0U;
	int32_t status, i;

	for (i = 0; i < test_clock_num; i++) {
		uint32_t clock_id  = test_clock_list[i].clock_id;
		uint32_t device_id = test_clock_list[i].device_id;

		status = xpm_clock_get_status(clock_id, &result);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the clock status of 0x%x clockid, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_request_node(device_id, PM_CAP_ACCESS, 0, 0);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, device_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_enable(clock_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to enable 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_disable(clock_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to disable 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_get_status(clock_id, &result);
		if (status != PM_RET_SUCCESS || result != PM_CLOCK_DISABLE) {
			tftf_testcase_printf("%s ERROR to get the clock status of 0x%x clockid, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_enable(clock_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to enable 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_get_status(clock_id, &result);
		if (status != PM_RET_SUCCESS || result != PM_CLOCK_ENABLE) {
			tftf_testcase_printf("%s ERROR to get the clock status of 0x%x clockid, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_set_parent(clock_id, parent_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set the parent for 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_get_parent(clock_id, &result);
		if (status != PM_RET_SUCCESS || parent_id != result) {
			tftf_testcase_printf("%s ERROR to get the parent for 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_set_divider(clock_id, divider);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set the divider for 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_get_divider(clock_id, &result);
		if (status != PM_RET_SUCCESS || divider != result) {
			tftf_testcase_printf("%s ERROR to set the divider for 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_clock_disable(clock_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to disable 0x%x clock, "
					     "Status: 0x%x\n", __func__, clock_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_release_node(device_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release 0x%x node, "
					     "Status: 0x%x\n", __func__, device_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}
