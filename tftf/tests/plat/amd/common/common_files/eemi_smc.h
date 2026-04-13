/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EEMI_SMC_H_
#define EEMI_SMC_H_

#include <stdint.h>

/*
 * Platform-specific SMC interface for EEMI calls.
 *
 * This function packs the given arguments into SMC registers and dispatches
 * the call. Implementations live in versal/eemi_smc.c and versal2/eemi_smc.c;
 * the build system selects the correct one for each platform.
 *
 * @arg0:        PM API ID
 * @arg1..arg7:  PM API arguments (64-bit pairs for Versal Gen 2 pass-through)
 * @ret_payload: Optional output buffer (7 x uint32_t); may be NULL
 * Returns:      PM return status (lower 32 bits of ret.ret0)
 */
int eemi_call(const uint32_t arg0,
	      const uint64_t arg1, const uint64_t arg2,
	      const uint64_t arg3, const uint64_t arg4,
	      const uint64_t arg5, const uint64_t arg6,
	      const uint64_t arg7,
	      uint32_t *const ret_payload);

#endif /* EEMI_SMC_H_ */
