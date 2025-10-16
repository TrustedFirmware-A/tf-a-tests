/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

int api_id_list[] = {
	PM_GET_CALLBACK_DATA,
	PM_GET_TRUSTZONE_VERSION,
	TF_A_PM_REGISTER_SGI,
	TF_A_FEATURE_CHECK,
};

/*
 * This function tests the behavior of the tf_a_feature_check() function, which
 * retrieves the supported API Version.
 */
test_result_t test_tf_a_feature_check(void)
{
	uint32_t version = 0U;
	int32_t status, index;

	for (index = 0; index < ARRAY_SIZE(api_id_list); index++) {
		status = tf_a_feature_check(api_id_list[index], &version);

		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR fetching the supported version for %x, "
					     "Status: 0x%x\n", __func__, api_id_list[index],
					     status);
			return TEST_RESULT_FAIL;
		}

		tftf_testcase_printf("Api Id: 0x%x Version: %u Status: %s\n\n",
				     api_id_list[index], version,
				     ((status == 0U) ? "Supported" : "Not Supported"));
	}

	return TEST_RESULT_SUCCESS;
}
