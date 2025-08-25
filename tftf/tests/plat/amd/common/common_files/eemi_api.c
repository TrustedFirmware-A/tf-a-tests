/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "eemi_api.h"
#include "xpm_defs.h"

static int do_feature_check(const uint32_t api_id)
{
	smc_args args;
	smc_ret_values ret;

	args.fid = (PM_SIP_SVC | PM_FEATURE_CHECK);
	args.arg1 = api_id;

	ret = tftf_smc(&args);

	return lower_32_bits(ret.ret0);
}

static int eemi_call(const uint32_t arg0, const uint64_t arg1, const uint64_t arg2,
		     const uint64_t arg3, const uint64_t arg4, const uint64_t arg5,
		     const uint64_t arg6, const uint64_t arg7,
		     uint32_t *const ret_payload)
{
	smc_args args;
	smc_ret_values ret;
	int32_t status;

	args.fid = (PM_SIP_SVC | arg0);
	args.arg1 = arg1;
	args.arg2 = arg2;
	args.arg3 = arg3;
	args.arg4 = arg4;
	args.arg5 = arg5;
	args.arg6 = arg6;
	args.arg7 = arg7;

	/*
	 * 'arg0' represents the API ID. This check ensures that the API is supported
	 * by TF-A/PLM before making the actual API call.
	 */
	status = do_feature_check(arg0);
	if (status != PM_RET_SUCCESS) {
		tftf_testcase_printf("%s ERROR Status:0x%x, Feature Check Failed for "
				     "API Id:0x%x\n", __func__, status, arg0);
		return status;
	}

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

int xpm_get_api_version(uint32_t *version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_GET_API_VERSION, 0, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*version = ret_payload[1];

	return ret;
}

int xpm_get_chip_id(uint32_t *id_code, uint32_t *version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_GET_CHIPID, 0, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS) {
		*id_code = ret_payload[1];
		*version = ret_payload[2];
	}

	return ret;
}

int xpm_feature_check(const uint32_t api_id, uint32_t *const version)
{
	uint32_t ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	ret = eemi_call(PM_FEATURE_CHECK, api_id, 0, 0, 0, 0, 0, 0, ret_payload);
	if (ret == PM_RET_SUCCESS)
		*version = ret_payload[1];

	return ret;
}
