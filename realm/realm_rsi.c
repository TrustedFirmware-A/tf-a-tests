
/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <host_realm_rmi.h>
#include <lib/aarch64/arch_features.h>
#include <realm_rsi.h>
#include <smccc.h>

/* This function return RSI_ABI_VERSION */
u_register_t rsi_get_version(u_register_t req_ver)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{RSI_VERSION, req_ver, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL});

	if (res.ret0 == SMC_UNKNOWN) {
		return SMC_UNKNOWN;
	}
	/* Return lower version. */
	return res.ret1;
}

/* This function will call the Host to request IPA of the NS shared buffer */
u_register_t rsi_get_ns_buffer(void)
{
	smc_ret_values res = {};
	struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));

	host_cal.imm = HOST_CALL_GET_SHARED_BUFF_CMD;
	res = tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
	if (res.ret0 != RSI_SUCCESS) {
		return 0U;
	}
	return host_cal.gprs[0];
}

/* This function call Host and request to exit Realm with proper exit code */
void rsi_exit_to_host(enum host_call_cmd exit_code)
{
	struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));

	host_cal.imm = exit_code;
	host_cal.gprs[0] = read_mpidr_el1() & MPID_MASK;
	tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
}
