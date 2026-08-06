/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ZynqMP EEMI SMC dispatch.
 */

#include "eemi_api.h"
#include "eemi_smc.h"
#include "xpm_defs.h"

int eemi_call(const uint32_t arg0,
	      const uint64_t arg1, const uint64_t arg2,
	      const uint64_t arg3, const uint64_t arg4,
	      const uint64_t arg5, const uint64_t arg6,
	      const uint64_t arg7,
	      uint32_t *const ret_payload)
{
	smc_args args = { 0 };
	smc_ret_values ret;

	/* Direct SMC format: PM_SIP_SVC | api_id. */
	args.fid = (PM_SIP_SVC | arg0);
	args.arg1 = arg1;
	args.arg2 = arg2;
	args.arg3 = arg3;
	args.arg4 = arg4;
	args.arg5 = arg5;
	args.arg6 = arg6;
	args.arg7 = arg7;

	ret = tftf_smc(&args);

	if (ret_payload) {
		ret_payload[0] = lower_32_bits(ret.ret0);
		ret_payload[1] = upper_32_bits(ret.ret0);
		ret_payload[2] = lower_32_bits(ret.ret1);
		ret_payload[3] = upper_32_bits(ret.ret1);
		ret_payload[4] = lower_32_bits(ret.ret2);
		ret_payload[5] = upper_32_bits(ret.ret2);
		ret_payload[6] = lower_32_bits(ret.ret3);
	}

	return lower_32_bits(ret.ret0);
}
