/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal platform-specific EEMI SMC dispatch.
 *
 * On Versal, every PM API is called via PM_SIP_SVC | api_id (direct SMC
 * format).  TF-A's pm_smc_handler extracts pm_arg[] directly from the lower
 * 32 bits of x1-x7.  A PM_FEATURE_CHECK call precedes each API invocation to
 * verify that the PLM supports the requested API.
 */

#include <stdint.h>

#include "eemi_api.h"
#include "eemi_smc.h"
#include "xpm_defs.h"

#include <tftf_lib.h>

static int do_feature_check(const uint32_t api_id)
{
	smc_args args = { 0 };
	smc_ret_values ret;

	args.fid = PM_SIP_SVC | PM_FEATURE_CHECK;
	args.arg1 = api_id;

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
	int32_t status;

	/* All PM APIs use the direct SMC format on Versal */
	args.fid = PM_SIP_SVC | arg0;
	args.arg1 = arg1;
	args.arg2 = arg2;
	args.arg3 = arg3;
	args.arg4 = arg4;
	args.arg5 = arg5;
	args.arg6 = arg6;
	args.arg7 = arg7;

	status = do_feature_check(arg0);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR Status:0x%x, Feature Check Failed for API Id:0x%x\n",
				     __func__, status, arg0);
	} else {
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
