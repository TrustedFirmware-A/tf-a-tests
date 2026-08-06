/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ZynqMP test targets and platform values other than node IDs.
 */

#ifndef XPM_DEFS_PLAT_H_
#define XPM_DEFS_PLAT_H_

#include "xpm_nodeid_plat.h"

/* A blocking acknowledge is needed for the firmware status to be returned. */
#define PM_REQ_ACK_BLOCKING		2U
#define PM_REQ_ACK_DEFAULT		PM_REQ_ACK_BLOCKING

/* Setting a divider needs a sentinel selecting DIV0; reading returns both. */
#define CLK_USB0_BUS_REF		32U

#define PM_CLK_DIV0_SELECT		0xFFFF0000U
#define PM_CLK_DIVIDER_MASK		0xFFFFU

/* Parent selected by the clock tests. */
#define PM_CLK_TEST_PARENT		2U

#define PM_CLK_SET_DIVIDER(val)		(PM_CLK_DIV0_SELECT | \
					 ((val) & PM_CLK_DIVIDER_MASK))

#endif /* XPM_DEFS_PLAT_H_ */
