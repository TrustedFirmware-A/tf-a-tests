/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"

/*
 * This function reads the device reset state.
 */
test_result_t test_reset_get_status(void)
{
	int32_t status;
	uint32_t result;
	uint32_t reset_id = PM_RST_GEM_0;

	status = xpm_reset_get_status(reset_id, &result);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR getting reset status for reset_id: 0x%x, "
				     "Status: 0x%x\n", __func__, reset_id, status);
		return TEST_RESULT_FAIL;
	}

	tftf_testcase_printf("State = %x\n", result);

	return TEST_RESULT_SUCCESS;
}
