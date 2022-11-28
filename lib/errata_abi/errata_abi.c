/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tftf.h>
#include <arch_helpers.h>
#include <debug.h>
#include <errata_abi.h>
#include <platform.h>
#include <tftf_lib.h>

const em_function_t em_functions[TOTAL_ABI_CALLS] = {
	DEFINE_EM_FUNC(VERSION, true),
	DEFINE_EM_FUNC(FEATURES, true),
	DEFINE_EM_FUNC(CPU_ERRATUM_FEATURES, true),
};

int32_t tftf_em_abi_version(void)
{
	smc_args args = { EM_VERSION };
	smc_ret_values ret_vals;

	ret_vals = tftf_smc(&args);
	return ret_vals.ret0;
}

bool tftf_em_abi_feature_implemented(uint32_t id)
{
	smc_args args = {
		EM_FEATURES,
		id,
	};
	smc_ret_values ret_vals;

	ret_vals = tftf_smc(&args);
	if (ret_vals.ret0 == EM_SUCCESS) {
		return true;
	} else {
		return false;
	}
}

smc_ret_values tftf_em_abi_cpu_feature_implemented(uint32_t cpu_erratum,
						uint32_t forward_flag)
{
	smc_args args = {
		EM_CPU_ERRATUM_FEATURES,
		cpu_erratum,
		forward_flag
	};
	return tftf_smc(&args);
}
