/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_nodeid_plat.h"

/* Device ID of the primary APU core, portable across platforms */
#define PROC_DEV_ID		PM_DEV_ACPU_CORE

/* enable argument: 1 = enable wakeup source, 0 = disable it */
#define ENABLE_WAKEUP_SOURCE	1U

/*
 * This function is used by a CPU to set wakeup source.
 */
test_result_t test_set_wakeup_source(void)
{
	int32_t status;

	status = xpm_set_wakeup_source(PROC_DEV_ID, PM_DEV_TTC_0, ENABLE_WAKEUP_SOURCE);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR Set WakeUp Source: 0x%x, Status: 0x%x\n",
				     __func__, PM_DEV_TTC_0, status);

		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
