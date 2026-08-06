/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Test data and validation helpers each platform defines in its own
 * common_files directory, keeping the shared tests free of conditionals.
 */

#ifndef XPM_TEST_H_
#define XPM_TEST_H_

#include <stdbool.h>
#include <stdint.h>

#include "eemi_api.h"

/* Whether the status is correct for re-requesting an already owned node. */
bool pm_is_valid_repeat_request_status(int32_t status);

#endif /* XPM_TEST_H_ */
