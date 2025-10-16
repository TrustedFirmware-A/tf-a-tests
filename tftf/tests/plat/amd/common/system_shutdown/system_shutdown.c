/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

struct sys_shutdown system_shutdown[] = {
	{
		.shutdown_type = PM_SHUTDOWN_TYPE_RESET,
		.shutdown_subtype = PM_SHUTDOWN_SUBTYPE_RST_SUBSYSTEM,
	},
};

/*
 * This function can be used by a subsystem to shutdown self or restart
 * self, Ps or system.
 */
test_result_t test_system_shutdown(void)
{
	int32_t status, i;

	for (i = 0; i < ARRAY_SIZE(system_shutdown); i++) {
		uint32_t type = system_shutdown[i].shutdown_type;
		uint32_t subtype = system_shutdown[i].shutdown_subtype;

		status = xpm_system_shutdown(type, subtype);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR shutting down system, "
					     "Status: 0x%x\n", __func__, status);
			return TEST_RESULT_FAIL;
		}
		tftf_testcase_printf("Success for type = %d and subtype = %d\n",
				     type, subtype);
	}

	return TEST_RESULT_SUCCESS;
}
