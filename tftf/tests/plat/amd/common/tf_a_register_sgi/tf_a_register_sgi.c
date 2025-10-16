/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

struct register_sgi sgi_register_list[] = {
	{
		.sgi_num = XLNX_EVENT_SGI_NUM,
		.reset = 0,
	},
};

/*
 * This function register the IPI interrupt.
 */
test_result_t test_tf_a_register_sgi(void)
{
	int32_t status, i;

	for (i = 0; i < ARRAY_SIZE(sgi_register_list); i++) {
		uint32_t sgi_number = sgi_register_list[i].sgi_num;
		uint32_t reset = sgi_register_list[i].reset;

		status = tf_a_pm_register_sgi(sgi_number, reset);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR registering sgi, "
					     "Status: 0x%x\n", __func__, status);
			return TEST_RESULT_FAIL;
		}
		tftf_testcase_printf("Registered SGI num : %d\n", sgi_number);
	}

	return TEST_RESULT_SUCCESS;
}
