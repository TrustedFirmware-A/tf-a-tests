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

bool pm_is_valid_repeat_request_status(int32_t status)
{
	/* The firmware accepts a repeat request. */
	return status == PM_RET_SUCCESS;
}
