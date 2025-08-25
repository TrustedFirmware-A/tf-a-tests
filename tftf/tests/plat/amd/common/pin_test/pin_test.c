/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

struct test_pins {
	uint32_t node_id;
	uint32_t pin_id;
	uint32_t function_id;
};

struct test_pins test_pin_list[] = {
	{
		.node_id = PM_DEV_GEM_0,
		.pin_id = PM_STMIC_LMIO_0,
		.function_id = PIN_FUNC_GEM0,
	},
};

/*
 * This function iterates through a list of test pins, requests the associated
 * device node, attempts to set both the pin function and a pin configuration
 * parameter (slew rate in this case) without prior pin request, and verifies
 * that both operations fail as expected. It releases the device node after each
 * pin operation and logs errors if any step fails.
 */
test_result_t test_set_pin_parameter_of_unrequested_pin(void)
{
	int32_t test_pin_num = ARRAY_SIZE(test_pin_list);
	uint32_t capabilities = PM_CAP_ACCESS;
	uint32_t param_id = PINCTRL_CONFIG_SLEW_RATE;
	uint32_t set_param_val = 0U;
	uint32_t get_param_val = 0U;
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_pin_num; i++) {
		uint32_t node_id = test_pin_list[i].node_id;
		uint32_t pin_id = test_pin_list[i].pin_id;
		uint32_t set_function_id = test_pin_list[i].function_id;

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_set_function(pin_id, set_function_id);
		if (status == PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR set pinctrl func w/o requesting 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_parameter(pin_id, param_id, &get_param_val);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the parameter of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		if (get_param_val != PINCTRL_SLEW_RATE_SLOW)
			set_param_val = PINCTRL_SLEW_RATE_SLOW;
		else
			set_param_val = PINCTRL_SLEW_RATE_FAST;

		status = xpm_pinctrl_set_parameter(pin_id, param_id, set_param_val);
		if (status == PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR set pin parameter w/o requesting 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
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
 * This function iterates through a list of test pins, requests the associated
 * device node, attempts to retrieve and set the pin function without prior pin
 * request, and verifies that the set operation fails as expected. It releases
 * the device node after each pin operation and logs errors if any step fails.
 */
test_result_t test_set_pin_function_of_unrequested_pin(void)
{
	int32_t test_pin_num = ARRAY_SIZE(test_pin_list);
	uint32_t capabilities = PM_CAP_ACCESS;
	uint32_t get_function_id = 0U;
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_pin_num; i++) {
		uint32_t node_id = test_pin_list[i].node_id;
		uint32_t pin_id = test_pin_list[i].pin_id;
		uint32_t set_function_id = test_pin_list[i].function_id;

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_function(pin_id, &get_function_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the function of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_set_function(pin_id, set_function_id);
		if (status == PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR set pinctrl func w/o requesting 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_function(pin_id, &get_function_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the function of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
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
 * This function iterates through a list of test pins, requests the associated
 * device and pin, sets a specified pin function, sets and verifies a pin
 * configuration parameter (slew rate in this case), and releases both the pin
 * and device after each operation. It performs checks for successful operations
 * and logs errors if any step fails.
 */
test_result_t test_set_pin_config_param(void)
{
	int32_t test_pin_num = ARRAY_SIZE(test_pin_list);
	uint32_t param_id = PINCTRL_CONFIG_SLEW_RATE;
	uint32_t capabilities = PM_CAP_ACCESS;
	uint32_t set_param_val = 0U;
	uint32_t get_param_val = 0U;
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_pin_num; i++) {
		uint32_t node_id = test_pin_list[i].node_id;
		uint32_t pin_id = test_pin_list[i].pin_id;
		uint32_t set_function_id = test_pin_list[i].function_id;

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_request(pin_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_reset_assert(PM_RST_GEM_0, PM_RESET_ACTION_PULSE);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to reset assert\n", __func__);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_set_function(pin_id, set_function_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set the function of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_parameter(pin_id, param_id, &get_param_val);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the parameter of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		if (get_param_val != PINCTRL_SLEW_RATE_SLOW)
			set_param_val = PINCTRL_SLEW_RATE_SLOW;
		else
			set_param_val = PINCTRL_SLEW_RATE_FAST;

		status = xpm_pinctrl_set_parameter(pin_id, param_id, set_param_val);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set the parameter of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_parameter(pin_id, param_id, &get_param_val);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the parameter of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		if (set_param_val != get_param_val) {
			tftf_testcase_printf("%s ERROR pin parameter do not match\n",
					     __func__);
			return TEST_RESULT_FAIL;
		}

		if (get_param_val != PINCTRL_SLEW_RATE_SLOW)
			set_param_val = PINCTRL_SLEW_RATE_SLOW;
		else
			set_param_val = PINCTRL_SLEW_RATE_FAST;

		status = xpm_pinctrl_set_parameter(pin_id, param_id, set_param_val);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set the parameter of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_parameter(pin_id, param_id, &get_param_val);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the parameter of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		if (set_param_val != get_param_val) {
			tftf_testcase_printf("%s ERROR pin parameter do not match\n",
					     __func__);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_release(pin_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
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
 * This function iterates through a list of test pins, requests the associated
 * device and pin, sets a specified pin function, verifies the function setting,
 * and releases both the pin and device after each operation. It performs checks
 * for successful operations and logs errors if any step fails.
 */
test_result_t test_set_pin_function(void)
{
	int32_t test_pin_num = ARRAY_SIZE(test_pin_list);
	uint32_t capabilities = PM_CAP_ACCESS;
	uint32_t get_function_id = 0U;
	uint32_t ack = 0U;
	int32_t status, i;

	for (i = 0; i < test_pin_num; i++) {
		uint32_t node_id = test_pin_list[i].node_id;
		uint32_t pin_id = test_pin_list[i].pin_id;
		uint32_t set_function_id = test_pin_list[i].function_id;

		status = xpm_request_node(node_id, capabilities, PM_MAX_QOS, ack);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x node, "
					     "Status: 0x%x\n", __func__, node_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_request(pin_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to request 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_reset_assert(PM_RST_GEM_0, PM_RESET_ACTION_PULSE);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to reset assert\n", __func__);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_function(pin_id, &get_function_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the function of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_set_function(pin_id, set_function_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to set the function of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_get_function(pin_id, &get_function_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to get the function of 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
			return TEST_RESULT_FAIL;
		}

		if (set_function_id != get_function_id) {
			tftf_testcase_printf("%s ERROR function ids do not match\n",
					     __func__);
			return TEST_RESULT_FAIL;
		}

		status = xpm_pinctrl_release(pin_id);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR to release 0x%x pin, "
					     "Status: 0x%x\n", __func__, pin_id, status);
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
