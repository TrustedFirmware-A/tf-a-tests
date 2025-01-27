/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <stdlib.h>

#include <realm_helpers.h>
#include <realm_psi.h>
#include <realm_rsi.h>
#include <smccc.h>

static unsigned int volatile realm_got_undef_abort;

/* Generate 64-bit random number */
unsigned long long realm_rand64(void)
{
	return ((unsigned long long)rand() << 32) | rand();
}

/* This function will call the Host to request IPA of the NS shared buffer */
u_register_t realm_get_ns_buffer(void)
{
	smc_ret_values res = {};
	struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));

	host_cal.imm = HOST_CALL_GET_SHARED_BUFF_CMD;
	res = tftf_smc(&(smc_args) {SMC_RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});

	if (res.ret0 != RSI_SUCCESS) {
		/* retry with PSI */
		hvc_ret_values ret = tftf_hvc(&(hvc_args) {PSI_CALL_GET_SHARED_BUFF_CMD, 0UL, 0UL,
			0UL, 0UL, 0UL, 0UL, 0UL});

		if (ret.ret0 != RSI_SUCCESS) {
			return 0U;
		}
		return ret.ret1;
	}

	return host_cal.gprs[0];
}

bool realm_sync_exception_handler(void)
{
	uint64_t esr_el1 = read_esr_el1();

	if (EC_BITS(esr_el1) == EC_UNKNOWN) {
		realm_printf("received undefined abort. "
			     "ESR_EL1: 0x%llx ELR_EL1: 0x%llx\n",
			     esr_el1, read_elr_el1());
		realm_got_undef_abort++;
	}
	return true;
}

void realm_reset_undef_abort_count(void)
{
	realm_got_undef_abort = 0U;
}

unsigned int realm_get_undef_abort_count(void)
{
	return realm_got_undef_abort;
}
