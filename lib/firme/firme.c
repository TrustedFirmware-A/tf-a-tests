/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <debug.h>
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

int32_t firme_ide_km_keyset_prog(uint64_t ecam_address, uint64_t flags,
				 uint64_t keyset_id, uint64_t KeyQW0,
				 uint64_t KeyQW1, uint64_t KeyQW2,
				 uint64_t KeyQW3, uint64_t handle)
{
	smc_args_ext args = {};
	smc_ret_values_ext ret = {};

	args.fid = FIRME_SERVICE_IDE_KEYSET_PROG_FID;
	args.arg1 = ecam_address;
	args.arg2 = flags;
	args.arg3 = keyset_id;
	args.arg4 = KeyQW0;
	args.arg5 = KeyQW1;
	args.arg6 = KeyQW2;
	args.arg7 = KeyQW3;
	args.arg8 = handle;

	tftf_smc_no_retval_x8(&args, &ret);

	return ret.ret0;
}

int32_t firme_ide_km_keyset_go(uint64_t ecam_address, uint64_t flags,
			       uint64_t keyset_id, uint64_t handle)
{
	smc_args args = { FIRME_SERVICE_IDE_KEYSET_GO_FID, ecam_address,
			  flags, keyset_id, handle };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	return ret.ret0;
}

int32_t firme_ide_km_keyset_stop(uint64_t ecam_address, uint64_t flags,
				 uint64_t keyset_id, uint64_t handle)
{
	smc_args args = { FIRME_SERVICE_IDE_KEYSET_STOP_FID, ecam_address,
			  flags, keyset_id, handle };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	return ret.ret0;
}

int32_t firme_ide_km_keyset_poll(uint64_t ecam_address, uint64_t keyset_id,
				 uint64_t *handle_ret)
{
	smc_args args = { FIRME_SERVICE_IDE_KEYSET_POLL_FID, ecam_address, 0,
			  keyset_id };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	if (ret.ret0 == FIRME_SUCCESS) {
		*handle_ret = (uint64_t)ret.ret1;
	}

	return ret.ret0;

}

int32_t firme_ide_km_poll(uint64_t ecam_address, uint64_t *handle_ret,
			  uint64_t *keyset_id_ret)
{
	smc_args args = { FIRME_SERVICE_IDE_KEYSET_POLL_FID, ecam_address, 1 };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	if (ret.ret0 == FIRME_SUCCESS) {
		*handle_ret = (uint64_t)ret.ret1;
		*keyset_id_ret = (uint64_t)ret.ret2;
	}

	return ret.ret0;

}
