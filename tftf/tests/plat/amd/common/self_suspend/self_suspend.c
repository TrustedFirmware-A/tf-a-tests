/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_nodeid.h"

#define PROC_DEV_ID             PM_DEV_ACPU_0

/* Extern Variable */
extern void  __attribute__((weak)) *_vector_table;

/*
 * This function is used by a CPU to set wakeup source.
 */
test_result_t test_set_wakeup_source(void)
{
	int32_t status;

	status = xpm_set_wakeup_source(PROC_DEV_ID, PM_DEV_TTC_0, 1);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR Set WakeUp Source: 0x%x, Status: 0x%x\n",
				     __func__, PM_DEV_TTC_0, status);

		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function is used by a CPU to declare that it is about to
 * suspend itself.
 */
test_result_t test_self_suspend(void)
{
	int32_t status;

	status = xpm_self_suspend(PROC_DEV_ID, 0xFFFFFFFF,
				  PM_SUSPEND_STATE_SUSPEND_TO_RAM,
				  (uint64_t)&_vector_table);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR Self-suspend, Status: 0x%x\n", __func__, status);

		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
