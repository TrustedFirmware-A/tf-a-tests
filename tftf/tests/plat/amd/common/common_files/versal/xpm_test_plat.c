/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal test data and validation helpers.
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_defs_plat.h"
#include "xpm_nodeid.h"
#include "xpm_nodeid_plat.h"
#include "xpm_test.h"

const struct test_clocks test_clock_list[] = {
	{
		.clock_id = PM_CLK_QSPI_REF,
		.device_id = PM_DEV_QSPI,
	},
	{
		.clock_id = PM_CLK_GEM0_REF,
		.device_id = PM_DEV_GEM_0,
	},
};

const uint32_t test_clock_list_size = ARRAY_SIZE(test_clock_list);

bool pm_is_valid_repeat_request_status(int32_t status)
{
	/* The firmware accepts a repeat request. */
	return status == PM_RET_SUCCESS;
}

bool pm_clock_control_is_refused(uint32_t clock_id)
{
	/* The gate call reports the refusal. */
	return xpm_clock_enable(clock_id) != PM_RET_SUCCESS;
}
