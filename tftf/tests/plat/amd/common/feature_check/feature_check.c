/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

/*
 * This function tests the behavior of the xpm_feature_check() function, which
 * queries information about the feature version.
 */

int api_id[] = {
	PM_GET_API_VERSION,
	PM_GET_NODE_STATUS,
	PM_FEATURE_CHECK
};

test_result_t test_feature_check(void)
{
	uint32_t version = 0U;
	int32_t status, index;

	for (index = 0; index < ARRAY_SIZE(api_id); index++) {
		status = xpm_feature_check(api_id[index], &version);

		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR querying feature version for API Id:0x%x, "
					     "Status: 0x%x\n", __func__, api_id[index], status);
			return TEST_RESULT_FAIL;
		}

		tftf_testcase_printf("Api Id: 0x%x Version: %u\n\n", api_id[index], version);
	}

	return TEST_RESULT_SUCCESS;
}
