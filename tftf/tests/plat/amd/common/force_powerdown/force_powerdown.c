/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

#define PM_SUBSYS_RPU0_0	0x1C000005
#define TARGET_SUBSYSTEM	PM_SUBSYS_RPU0_0
#define TARGET_DEVICEID		PM_DEV_RPU0_0
#define WAKEUP_ADDR		0xFFE00000U

/*
 * This function is used to force power down the subsystem if the
 * subsystem is unresponsive and by calling this API all the resources of
 * that subsystem will be automatically released.
 *
 * This function is used to force power down the subsystem if the
 * subsystem is unresponsive. By calling this API, all the resources of
 * that subsystem will be automatically released.
 *
 * Force power down support for individual processors and power domains has been
 * deprecated. As a result, the only available option now is to force power down
 * the entire subsystem.
 *
 * To support this, another subsystem (RPU) needs to be present. For example,
 * the RPU subsystem can be added with only a NOP CDO (which contains only a
 * single "nop" instruction). This allows the force power down feature to be
 * tested without requiring an actual executable partition for the RPU.
 */
test_result_t test_force_powerdown(void)
{
	int32_t status;

	status = xpm_force_powerdown(TARGET_SUBSYSTEM, 1);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR force powering down system: 0x%x, "
				     "Status: 0x%x\n", __func__, TARGET_SUBSYSTEM, status);
		return TEST_RESULT_FAIL;
	}

	tftf_testcase_printf("Waiting for 10 seconds before waking up the target\n\r");
	waitms(10000);

	return TEST_RESULT_SUCCESS;
}

/*
 * This function can be used to request power up of a CPU node
 * within the same PU, or to power up another PU.
 */
test_result_t test_request_wake_up(void)
{
	int32_t status;

	status = xpm_request_wakeup(TARGET_SUBSYSTEM, 1, WAKEUP_ADDR, 0);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR requesting wake-up for %x, "
				     "Status: 0x%x\n", __func__, TARGET_SUBSYSTEM, status);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
