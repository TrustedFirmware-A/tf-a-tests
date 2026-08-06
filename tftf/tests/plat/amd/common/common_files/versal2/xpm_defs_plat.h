/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal Gen 2 platform-specific PM definitions.
 */

#ifndef XPM_DEFS_PLAT_H_
#define XPM_DEFS_PLAT_H_

#include "xpm_nodeid_plat.h"

/*
 * Versal Gen 2 SMC dispatch constants.
 *
 * On Versal Gen 2, standard PM APIs are routed via the pass-through FID
 * with the PM module prefix encoded in the lower 32 bits of x1, while
 * TF-A-only commands are identified by a dedicated module ID in the
 * upper byte of the API ID.
 */
#define PM_PASSTHROUGH_FID	0xC2000FFFU	/* Pass-through SMC FID */
#define PM_MODULE_PREFIX	0x200U		/* module 0x2 shifted into bits[15:8] (0x2 << 8 = 0x200) */
#define PM_API_MODULE_SHIFT	8U		/* api_id[15:8] = module ID */
#define TF_A_CMD_MODULE_ID	0xAU		/* (api_id >> PM_API_MODULE_SHIFT) for TF-A-only commands */

/*
 * Pack two 32-bit values into one 64-bit SMC argument word: the lower
 * 32 bits of @hi occupy bits[63:32] and @lo occupies bits[31:0].  The
 * Versal Gen 2 pass-through path interleaves the PM arguments this way
 * so TF-A's EXTRACT_ARGS macro can reconstruct pm_arg[].
 */
#define PACK_PM_PAIR(hi, lo) \
	(((uint64_t)(uint32_t)(hi) << 32U) | (uint64_t)(uint32_t)(lo))

/* The firmware returns the status directly, so no acknowledge is asked for. */
#define PM_REQ_ACK_DEFAULT      0U

/* Values the PLL set cases pass, rejected without touching live hardware. */
#define PM_PLL_TEST_SET_MODE            PM_PLL_MODE_RESET
#define PM_PLL_TEST_SET_PARAM           PM_PLL_PARAM_ID_FBDIV
#define PM_PLL_TEST_SET_PARAM_VALUE     10U

/* The divider is a plain value, returned unchanged. */
#define PM_CLK_TEST_PARENT              1U

#define PM_CLK_SET_DIVIDER(val)         (val)
#define PM_CLK_DIVIDER_MASK             0xFFFFFFFFU

#endif /* XPM_DEFS_PLAT_H_ */
