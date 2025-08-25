/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

uint32_t test_node_list[] = {
	PM_DEV_USB_0,
	PM_DEV_RTC
};

/*
 * This function iterates over a list of test nodes, requesting each
 * node, releasing it, and then attempting to release the same node
 * again to verify the expected behavior when a node is already
 * released. It checks the success of each operation to ensure proper
 * functionality of the API.
 */
test_result_t test_release_already_released_node(void)
{
	int32_t test_node_num = ARRAY_SIZE(test_node_list);
	uint32_t capabilities = PM_CAP_ACCESS;
	xpm_node_status node_status = {0U};
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_node_num; i++) {
		uint32_t node_id = test_node_list[i];

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, Status: 0x%x\n",
					     __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_release_node(node_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release 0x%x node, Status: 0x%x\n",
					     __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_release_node(node_id);
		if (status == PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release already released 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function iterates over a list of test nodes, first requesting
 * each node, then attempting to request the same node again to verify
 * the expected behavior when a node is already in use. It checks the
 * success of each operation to ensure proper functionality of the API.
 */
test_result_t test_request_already_requested_node(void)
{
	int32_t test_node_num = ARRAY_SIZE(test_node_list);
	uint32_t capabilities = PM_CAP_ACCESS;
	xpm_node_status node_status = {0U};
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_node_num; i++) {
		uint32_t node_id = test_node_list[i];

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request already request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_release_node(node_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function iterates over a list of test nodes, requesting each
 * node, setting the maximum latency for each node, and
 * verifying the success of each operation to ensure the proper
 * functionality of the API.
 */
test_result_t test_set_max_latency(void)
{
	int32_t test_node_num = ARRAY_SIZE(test_node_list);
	uint32_t capabilities = PM_CAP_ACCESS;
	uint32_t max_latency = XPM_MAX_LATENCY;
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_node_num; i++) {
		uint32_t node_id = test_node_list[i];

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_set_max_latency(node_id, max_latency);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set the max latency for 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_release_node(node_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function iterates over a list of test nodes, requesting each
 * node, setting requirements with specific capabilities, and
 * verifying the status before and after each operation to ensure
 * proper functionality of the API.
 */
test_result_t test_set_requirement(void)
{
	int32_t test_node_num = ARRAY_SIZE(test_node_list);
	uint32_t capabilities = PM_CAP_ACCESS;
	uint32_t capabilities2 = PM_CAP_CONTEXT;
	xpm_node_status node_status = {0U};
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_node_num; i++) {
		uint32_t node_id = test_node_list[i];

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_set_requirement(node_id, capabilities2, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set requirement for 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_release_node(node_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function iterates over a list of test nodes, verifying the
 * status of each node before and after requesting and releasing
 * access, ensuring that each API call succeeds and behaves as expected.
 */
test_result_t test_get_node_status(void)
{
	int32_t test_node_num = ARRAY_SIZE(test_node_list);
	xpm_node_status node_status = {0U};
	uint32_t capabilities = PM_CAP_ACCESS;
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_node_num; i++) {
		uint32_t node_id = test_node_list[i];

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_release_node(node_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_get_node_status(node_id, &node_status);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the status of 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}
