
/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
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
	/* Return lower version */
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
	host_cal.gprs[0] = read_mpidr_el1();
	tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
}

/* This function will exit to the Host to request RIPAS CHANGE of IPA range */
u_register_t rsi_ipa_state_set(u_register_t base,
			       u_register_t top,
			       rsi_ripas_type ripas,
			       u_register_t flag,
			       u_register_t *new_base,
			       rsi_ripas_respose_type *response)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{RSI_IPA_STATE_SET, base, top, ripas, flag});
	if (res.ret0 == RSI_SUCCESS) {
		*new_base = res.ret1;
		*response = res.ret2;
	}
	return res.ret0;
}

/* This function will return RIPAS of IPA range */
u_register_t rsi_ipa_state_get(u_register_t base,
				u_register_t top,
				u_register_t *out_top,
				rsi_ripas_type *ripas)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{RSI_IPA_STATE_GET, base, top});
	if (res.ret0 == RSI_SUCCESS) {
		*out_top = res.ret1;
		*ripas = res.ret2;
	}
	return res.ret0;
}

/* This function will initialize the attestation context */
u_register_t rsi_attest_token_init(u_register_t challenge_0,
				   u_register_t challenge_1,
				   u_register_t challenge_2,
				   u_register_t challenge_3,
				   u_register_t challenge_4,
				   u_register_t challenge_5,
				   u_register_t challenge_6,
				   u_register_t challenge_7,
				   u_register_t *out_token_upper_bound)
{
	smc_ret_values_ext res = {};

	tftf_smc_no_retval_x8(&(smc_args_ext) {
		RSI_ATTEST_TOKEN_INIT,
		challenge_0,
		challenge_1,
		challenge_2,
		challenge_3,
		challenge_4,
		challenge_5,
		challenge_6,
		challenge_7
	},
	&res);

	if (res.ret0 == RSI_SUCCESS) {
		*out_token_upper_bound = res.ret1;
	}

	return res.ret0;
}

/* This function will retrieve the (or part of) attestation token */
u_register_t rsi_attest_token_continue(u_register_t buffer_addr,
					u_register_t offset,
					u_register_t buffer_size,
					u_register_t *bytes_copied)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args) {
		RSI_ATTEST_TOKEN_CONTINUE,
		buffer_addr,
		offset,
		buffer_size
	});

	if ((res.ret0 == RSI_SUCCESS) || (res.ret0 == RSI_INCOMPLETE)) {
		*bytes_copied = res.ret1;
	}
	return res.ret0;
}
