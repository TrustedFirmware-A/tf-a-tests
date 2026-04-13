/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal Gen 2 platform-specific PM definitions.
 */

#ifndef XPM_DEFS_PLAT_H_
#define XPM_DEFS_PLAT_H_

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

#endif /* XPM_DEFS_PLAT_H_ */
