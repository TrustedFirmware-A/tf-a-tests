/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

/*
 * This function tests the behavior of the xpm_get_chip_id() function, which
 * retrieves the Chip ID and version information from the Energy Efficient
 * Management Interface (EEMI) on AMD-Xilinx platforms.
 */
test_result_t test_get_chip_id(void)
{
	uint32_t id_code = 0U;
	uint32_t version = 0U;
	int32_t status;

	status = xpm_get_chip_id(&id_code, &version);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR reading chip id, Status: 0x%x\n", __func__, status);
		return TEST_RESULT_FAIL;
	}

	tftf_testcase_printf("%s id_code = 0x%08x, version = 0x%08x\n", __func__,
			     id_code, version);

	return TEST_RESULT_SUCCESS;
}
