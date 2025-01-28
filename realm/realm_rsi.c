
/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <host_realm_rmi.h>
#include <lib/aarch64/arch_features.h>
#include <realm_helpers.h>
#include <realm_rsi.h>
#include <smccc.h>

/* This function return RSI_ABI_VERSION */
u_register_t rsi_get_version(u_register_t req_ver)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_VERSION, req_ver, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL});

	if (res.ret0 == SMC_UNKNOWN) {
		return SMC_UNKNOWN;
	}

	if (res.ret0 == RSI_ERROR_STATE) {
		return RSI_ERROR_STATE;
	}

	/* Return lower version */
	return res.ret1;
}

/* This function returns RSI feature register at 'feature_reg_index' */
u_register_t rsi_features(u_register_t feature_reg_index,
			  u_register_t *feature_reg_value_ret)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args) {SMC_RSI_FEATURES, feature_reg_index,
				    0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
	/* This command always returns RSI_SUCCESS */
	if (res.ret0 == RSI_SUCCESS) {
		*feature_reg_value_ret = res.ret1;
	}

	return res.ret0;
}

/* This function call Host and request to exit Realm with proper exit code */
u_register_t rsi_exit_to_host(enum host_call_cmd exit_code)
{
	struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));
	smc_ret_values res = {};

	host_cal.imm = exit_code;
	host_cal.gprs[0] = realm_get_my_plane_num();
	host_cal.gprs[1] = read_mpidr_el1();
	res = tftf_smc(&(smc_args) {SMC_RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
	return res.ret0;
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
			{SMC_RSI_IPA_STATE_SET, base, top, ripas, flag});
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
			{SMC_RSI_IPA_STATE_GET, base, top});
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
		SMC_RSI_ATTEST_TOKEN_INIT,
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
		SMC_RSI_ATTEST_TOKEN_CONTINUE,
		buffer_addr,
		offset,
		buffer_size
	});

	if ((res.ret0 == RSI_SUCCESS) || (res.ret0 == RSI_INCOMPLETE)) {
		*bytes_copied = res.ret1;
	}
	return res.ret0;
}

u_register_t rsi_realm_config(struct rsi_realm_config *s)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{SMC_RSI_REALM_CONFIG, (u_register_t)s});
	return res.ret0;
}

u_register_t rsi_mem_get_perm_value(u_register_t plane_index,
				    u_register_t perm_index,
				    u_register_t *perm)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{SMC_RSI_MEM_GET_PERM_VALUE, plane_index, perm_index});
	if (res.ret0 == RSI_SUCCESS) {
		*perm = res.ret1;
	}
	return res.ret0;
}

u_register_t rsi_mem_set_perm_value(u_register_t plane_index,
				    u_register_t perm_index,
				    u_register_t perm)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{SMC_RSI_MEM_SET_PERM_VALUE, plane_index, perm_index,
			 perm});
	return res.ret0;
}

u_register_t rsi_mem_set_perm_index(u_register_t base,
				    u_register_t top,
				    u_register_t perm_index,
				    u_register_t cookie,
				    u_register_t *new_base,
				    u_register_t *response,
				    u_register_t *new_cookie)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{SMC_RSI_MEM_SET_PERM_INDEX, base, top, perm_index,
			 cookie});
	if (res.ret0 == RSI_SUCCESS) {
		*new_base = res.ret1;
		*response = res.ret2;
		*new_cookie = res.ret3;
	}
	return res.ret0;
}

u_register_t rsi_plane_enter(u_register_t plane_index,
			     u_register_t plane_run)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{SMC_RSI_PLANE_ENTER, plane_index, plane_run});
	return res.ret0;
}

u_register_t rsi_plane_reg_read(u_register_t plane_index,
				u_register_t register_encoding,
				u_register_t *value)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{SMC_RSI_PLANE_REG_READ, plane_index,
			 register_encoding});
	if (res.ret0 == RSI_SUCCESS) {
		*value = res.ret1;
	}
	return res.ret0;
}

u_register_t rsi_plane_reg_write(u_register_t plane_index,
				 u_register_t register_encoding,
				 u_register_t value)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{SMC_RSI_PLANE_REG_WRITE, plane_index,
			 register_encoding, value});
	return res.ret0;
}

/* This function return instance ID of Realm device */
u_register_t rsi_rdev_get_instance_id(u_register_t rdev_id,
				      u_register_t *rdev_inst_id)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_RDEV_GET_INSTANCE_ID, rdev_id, 0UL, 0UL, 0UL, 0UL,
		 0UL, 0UL});

	if (res.ret0 == RSI_SUCCESS) {
		*rdev_inst_id = res.ret1;
	}

	return res.ret0;
}

/* This function return state of the Realm device */
u_register_t rsi_rdev_get_state(u_register_t rdev_id, u_register_t rdev_inst_id,
				u_register_t *rdev_rsi_state)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_RDEV_GET_STATE, rdev_id, rdev_inst_id, 0UL, 0UL, 0UL,
		 0UL, 0UL});

	if (res.ret0 == RSI_SUCCESS) {
		*rdev_rsi_state = res.ret1;
	}

	return res.ret0;
}

/* This function triggers RDEV interruptible operation to get_measurements */
u_register_t rsi_rdev_get_measurements(u_register_t rdev_id,
				       u_register_t rdev_inst_id,
				       u_register_t meas_params_ptr)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_RDEV_GET_MEASUREMENTS, rdev_id, rdev_inst_id,
		 meas_params_ptr, 0UL, 0UL, 0UL, 0UL});

	return res.ret0;
}

/* This function triggers RDEV interruptible operation to get_measurements */
u_register_t rsi_rdev_get_info(u_register_t rdev_id, u_register_t rdev_inst_id,
			       u_register_t rdev_info_ptr)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		       {SMC_RSI_RDEV_GET_INFO, rdev_id, rdev_inst_id,
			rdev_info_ptr, 0UL, 0UL, 0UL, 0UL});

	return res.ret0;
}

/* This function triggers RDEV interruptible operation to lock */
u_register_t rsi_rdev_lock(u_register_t rdev_id, u_register_t rdev_inst_id)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_RDEV_LOCK, rdev_id, rdev_inst_id,
		 0UL, 0UL, 0UL, 0UL, 0UL});

	return res.ret0;
}

/* This function triggers RDEV interruptible operation to get interface report */
u_register_t rsi_rdev_get_interface_report(u_register_t rdev_id,
					   u_register_t rdev_inst_id,
					   u_register_t tdisp_version_max)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_RDEV_GET_INTERFACE_REPORT, rdev_id, rdev_inst_id,
		 tdisp_version_max, 0UL, 0UL, 0UL, 0UL});

	return res.ret0;
}

/* This function triggers RDEV interruptible operation to start */
u_register_t rsi_rdev_start(u_register_t rdev_id, u_register_t rdev_inst_id)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_RDEV_START, rdev_id, rdev_inst_id,
		 0UL, 0UL, 0UL, 0UL, 0UL});

	return res.ret0;
}

/* This function triggers RDEV interruptible operation to stop the TDI */
u_register_t rsi_rdev_stop(u_register_t rdev_id, u_register_t rdev_inst_id)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{SMC_RSI_RDEV_STOP, rdev_id, rdev_inst_id,
		 0UL, 0UL, 0UL, 0UL, 0UL});

	return res.ret0;
}

/* This function exits the REC to do vdev communicate */
u_register_t rsi_rdev_continue(u_register_t rdev_id, u_register_t rdev_inst_id)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		       {SMC_RSI_RDEV_CONTINUE, rdev_id, rdev_inst_id, 0UL, 0UL,
			0UL, 0UL, 0UL});

	return res.ret0;
}
