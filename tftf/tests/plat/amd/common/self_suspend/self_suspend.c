/*
 * Copyright (c) 2025-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_nodeid_plat.h"

/*
 * On Versal Gen 2 the primary APU core is PM_DEV_ACPU_0_0 (cluster 0, core 0).
 * The legacy PM_DEV_ACPU_0 (0x1810C003) is not registered in the Versal Gen 2
 * device table; PM_DEV_ACPU_CORE selects the correct ID per platform.
 */
#define PROC_DEV_ID             PM_DEV_ACPU_CORE

/* Extern Variable */
extern void  __attribute__((weak)) *_vector_table;

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
