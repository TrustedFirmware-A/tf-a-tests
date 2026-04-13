/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal Gen 2 platform-specific EEMI SMC dispatch.
 *
 * Versal Gen 2 TF-A routes SMCs differently from Versal:
 *
 *  - TF-A-only commands (api_id >> PM_API_MODULE_SHIFT == TF_A_CMD_MODULE_ID,
 *    e.g. TF_A_FEATURE_CHECK = 0xa00):
 *    Use the direct format PM_SIP_SVC | api_id, with arguments in x1-x7.
 *
 *  - Standard PM APIs (everything else):
 *    Use the pass-through FID PM_PASSTHROUGH_FID.  TF-A's EXTRACT_ARGS macro
 *    reconstructs pm_arg[i] from alternating lower/upper 32-bit halves of
 *    x1-x7.  The dispatch target (PM_MODULE_PREFIX | api_id) occupies the
 *    low 32 bits of x1; the remaining pm_args are interleaved as:
 *      x1[31: 0] = PM_MODULE_PREFIX | api_id   (dispatch target)
 *      x1[63:32] = pm_arg[0]
 *      x2[31: 0] = pm_arg[1]
 *      x2[63:32] = pm_arg[2]
 *      x3[31: 0] = pm_arg[3]
 *      x3[63:32] = pm_arg[4]
 *      ...and so on through x7.
 *    Callers pass pre-packed 64-bit pairs; this function re-interleaves them
 *    using PACK_PM_PAIR().  A PM_FEATURE_CHECK call (also via pass-through)
 *    precedes the actual call.
 */

#include <stdint.h>

#include "eemi_api.h"
#include "eemi_smc.h"
#include "xpm_defs.h"
#include "xpm_defs_plat.h"

#include <tftf_lib.h>

static int do_feature_check(const uint32_t api_id)
{
	smc_args args = { 0 };
	smc_ret_values ret;

	/*
	 * Pass-through format for PM_FEATURE_CHECK:
	 *   x1[31: 0] = PM_MODULE_PREFIX | PM_FEATURE_CHECK  (dispatch target)
	 *   x1[63:32] = api_id                               (pm_arg[0]: API to check)
	 */
	args.fid = PM_PASSTHROUGH_FID;
	args.arg1 = PACK_PM_PAIR(api_id, PM_MODULE_PREFIX | PM_FEATURE_CHECK);

	ret = tftf_smc(&args);

	return lower_32_bits(ret.ret0);
}

int eemi_call(const uint32_t arg0,
	      const uint64_t arg1, const uint64_t arg2,
	      const uint64_t arg3, const uint64_t arg4,
	      const uint64_t arg5, const uint64_t arg6,
	      const uint64_t arg7,
	      uint32_t *const ret_payload)
{
	smc_args args = { 0 };
	smc_ret_values ret;
	int32_t status = PM_RET_SUCCESS;

	if ((arg0 >> PM_API_MODULE_SHIFT) == TF_A_CMD_MODULE_ID) {
		/* TF-A-only commands: direct SMC, no PLM feature-check */
		args.fid = PM_SIP_SVC | arg0;
		args.arg1 = arg1;
		args.arg2 = arg2;
		args.arg3 = arg3;
		args.arg4 = arg4;
		args.arg5 = arg5;
		args.arg6 = arg6;
		args.arg7 = arg7;
	} else {
		/*
		 * Standard PM APIs via pass-through (PM_PASSTHROUGH_FID).
		 * Re-interleave the caller's pre-packed 64-bit pairs so that
		 * EXTRACT_ARGS can reconstruct the original pm_arg[0..4].
		 * arg5/arg6/arg7 are unused on this path: EXTRACT_ARGS
		 * reconstructs all pm_args from x1-x4 only, and they remain
		 * zero from the smc_args initializer above.
		 */
		args.fid = PM_PASSTHROUGH_FID;
		args.arg1 = PACK_PM_PAIR(arg1, PM_MODULE_PREFIX | arg0);
		args.arg2 = PACK_PM_PAIR(arg2, upper_32_bits(arg1));
		args.arg3 = PACK_PM_PAIR(arg3, upper_32_bits(arg2));
		args.arg4 = PACK_PM_PAIR(arg4, upper_32_bits(arg3));

		status = do_feature_check(arg0);
		if (status != PM_RET_SUCCESS) {
			tftf_testcase_printf("%s ERROR Status:0x%x, Feature Check Failed for API Id:0x%x\n",
					     __func__, status, arg0);
		}
	}

	if (status == PM_RET_SUCCESS) {
		ret = tftf_smc(&args);

		if (ret_payload != NULL) {
			ret_payload[0] = lower_32_bits(ret.ret0);
			ret_payload[1] = upper_32_bits(ret.ret0);
			ret_payload[2] = lower_32_bits(ret.ret1);
			ret_payload[3] = upper_32_bits(ret.ret1);
			ret_payload[4] = lower_32_bits(ret.ret2);
			ret_payload[5] = upper_32_bits(ret.ret2);
			ret_payload[6] = lower_32_bits(ret.ret3);
		}

		status = lower_32_bits(ret.ret0);
	}

	return status;
}
