/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ZynqMP test data and validation helpers. USB0 backs both tables: the APU
 * can request its node and USB is idle during a bare-metal run.
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_defs_plat.h"
#include "xpm_nodeid.h"
#include "xpm_nodeid_plat.h"
#include "xpm_test.h"

bool pm_is_valid_repeat_request_status(int32_t status)
{
	/* The firmware rejects a repeat request. */
	return status != PM_RET_SUCCESS;
}
