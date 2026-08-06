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

/* Clock exercised by the clock tests, with the device that owns it. */
struct test_clocks {
	uint32_t clock_id;              /**< Clock ID */
	uint32_t device_id;             /**< Device that owns the clock */
};

extern const struct test_clocks test_clock_list[];
extern const uint32_t test_clock_list_size;

/* Whether the status is correct for re-requesting an already owned node. */
bool pm_is_valid_repeat_request_status(int32_t status);

/* Whether clock control is refused for a device this master does not own. */
bool pm_clock_control_is_refused(uint32_t clock_id);

#endif /* XPM_TEST_H_ */
