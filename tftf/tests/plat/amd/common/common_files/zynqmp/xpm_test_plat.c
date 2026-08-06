/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ZynqMP test data and validation helpers. USB0 backs both tables: the APU
 * can request its node and USB is idle during a bare-metal run.
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_defs_plat.h"
#include "xpm_nodeid.h"
#include "xpm_nodeid_plat.h"
#include "xpm_test.h"

const struct test_clocks test_clock_list[] = {
	{
		.clock_id = CLK_USB0_BUS_REF,
		.device_id = NODE_USB_0,
	},
};

const uint32_t test_clock_list_size = ARRAY_SIZE(test_clock_list);

const struct test_pins test_pin_list[] = {
	{
		.node_id = NODE_USB_0,
		.pin_id = ZYNQMP_PIN_USB0,
		.function_id = ZYNQMP_PINCTRL_FUNC_USB0,
		.reset_id = ZYNQMP_RST_USB0,
	},
};

const uint32_t test_pin_list_size = ARRAY_SIZE(test_pin_list);

const struct test_pll_api test_pll_list[] = {
	{
		.clock_id = NODE_APLL,
	},
};

const uint32_t test_pll_list_size = ARRAY_SIZE(test_pll_list);

bool pm_is_valid_repeat_request_status(int32_t status)
{
	/* The firmware rejects a repeat request. */
	return status != PM_RET_SUCCESS;
}

bool pm_clock_control_is_refused(uint32_t clock_id)
{
	/* TF-A filters the gate error, so the refusal shows on set parent. */
	return xpm_clock_set_parent(clock_id, PM_CLK_TEST_PARENT) != PM_RET_SUCCESS;
}
