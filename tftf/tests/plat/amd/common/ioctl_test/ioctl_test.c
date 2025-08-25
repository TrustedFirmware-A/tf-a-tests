/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

struct test_ioctl test_ioctl_list[] = {
	{
		.node_id = PM_DEV_RPU0_0,
		.ioctl_id = IOCTL_GET_RPU_OPER_MODE,
		.ioctl_arg1 = 0U,
		.ioctl_arg2 = 0U,
	},
	{
		.node_id = PM_DEV_RPU0_0,
		.ioctl_id = IOCTL_SET_RPU_OPER_MODE,
		.ioctl_arg1 = XPM_RPU_MODE_SPLIT,
		.ioctl_arg2 = 0U,
	},
	{
		.node_id = PM_DEV_RPU0_0,
		.ioctl_id = IOCTL_GET_RPU_OPER_MODE,
		.ioctl_arg1 = 0U,
		.ioctl_arg2 = 0U,
	},
};

/*
 * This function calls IOCTL to firmware for device control and
 * configuration.
 */
test_result_t test_ioctl_api(void)
{
	uint32_t ioctl_response = 0U;
	int32_t status, i;

	for (i = 0; i < (int32_t)ARRAY_SIZE(test_ioctl_list); i++) {
		status = xpm_ioctl(test_ioctl_list[i].node_id,
				   test_ioctl_list[i].ioctl_id,
				   test_ioctl_list[i].ioctl_arg1,
				   test_ioctl_list[i].ioctl_arg2,
				   &ioctl_response);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR Fails for IOCTL Id: %u, Status: 0x%x",
					     __func__, test_ioctl_list[i].ioctl_id, status);
			return TEST_RESULT_FAIL;
		}

		if (test_ioctl_list[i].ioctl_id == IOCTL_GET_RPU_OPER_MODE) {
			tftf_testcase_printf("%s RPU_OPER_MODE: 0x%x\n", __func__,
					     ioctl_response);
		} else {
			tftf_testcase_printf("%s Mode Set done successfully\n",
					     __func__);
		}
	}

	return TEST_RESULT_SUCCESS;
}
