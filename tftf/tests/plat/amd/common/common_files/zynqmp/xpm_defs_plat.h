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

/* Out-of-range values, rejected before they can reconfigure a live PLL. */
#define PM_PLL_TEST_SET_MODE		0xFFU
#define PM_PLL_TEST_SET_PARAM		0xFFU
#define PM_PLL_TEST_SET_PARAM_VALUE	0U

/* Setting a divider needs a sentinel selecting DIV0; reading returns both. */
#define CLK_USB0_BUS_REF		32U

#define PM_CLK_DIV0_SELECT		0xFFFF0000U
#define PM_CLK_DIVIDER_MASK		0xFFFFU

/* Parent selected by the clock tests. */
#define PM_CLK_TEST_PARENT		2U

#define PM_CLK_SET_DIVIDER(val)		(PM_CLK_DIV0_SELECT | \
					 ((val) & PM_CLK_DIVIDER_MASK))

/* MIO52 in the USB0 group, whose node the APU can request. */
#define ZYNQMP_PIN_USB0			52U	/* MIO52, USB0 group */
#define ZYNQMP_PINCTRL_FUNC_USB0	43U

/* Core reset of the USB controller that owns the pin. */
#define ZYNQMP_RST_USB0			1059U

#endif /* XPM_DEFS_PLAT_H_ */
