/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>

#include <firme.h>
#include <smccc.h>
#include <tftf_lib.h>

int32_t firme_version(uint8_t service_id)
{
	smc_args args = { FIRME_SERVICE_VERSION_FID, (uint64_t)service_id };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	return ret.ret0;
}

int32_t firme_features(uint8_t service_id, uint8_t reg_index, uint64_t *reg)
{
	smc_args args = { FIRME_SERVICE_FEATURES_FID, (uint64_t)service_id,
			  (uint64_t)reg_index };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	if (ret.ret0 == FIRME_SUCCESS) {
		*reg = (uint64_t)ret.ret1;
	}

	return ret.ret0;
}
