/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <debug.h>
#include <psci.h>
#include <realm_def.h>
#include <tftf_lib.h>

typedef void (*secondary_ep_t)(u_register_t);
static secondary_ep_t entrypoint[MAX_REC_COUNT];
static u_register_t context_id[MAX_REC_COUNT];
void realm_entrypoint(void);
void realm_payload_main(void);

void realm_cpu_off(void)
{
	smc_args args = { SMC_PSCI_CPU_OFF };

	tftf_smc(&args);
}

u_register_t realm_cpu_on(u_register_t mpidr, uintptr_t ep, u_register_t cxt_id)
{
	smc_args args;
	smc_ret_values ret_vals;


	if (mpidr > MAX_REC_COUNT) {
		return PSCI_E_INVALID_PARAMS;
	}

	if (entrypoint[mpidr] != NULL) {
		return PSCI_E_ALREADY_ON;
	}

	args.fid = SMC_PSCI_CPU_ON;
	args.arg1 = mpidr;
	args.arg2 = (u_register_t)realm_entrypoint;
	args.arg3 = cxt_id;
	entrypoint[mpidr] = (secondary_ep_t)ep;
	context_id[mpidr] = cxt_id;
	ret_vals = tftf_smc(&args);
	return ret_vals.ret0;
}

u_register_t realm_psci_affinity_info(u_register_t target_affinity,
		uint32_t lowest_affinity_level)
{
	smc_args args;
	smc_ret_values ret_vals;

	args.fid = SMC_PSCI_AFFINITY_INFO;
	args.arg1 = target_affinity;
	args.arg2 = lowest_affinity_level;
	ret_vals = tftf_smc(&args);
	return ret_vals.ret0;
}

u_register_t realm_psci_features(uint32_t psci_func_id)
{

	smc_args args;
	smc_ret_values ret_vals;

	args.fid = SMC_PSCI_FEATURES;
	args.arg1 = psci_func_id;
	ret_vals = tftf_smc(&args);
	return ret_vals.ret0;
}

void realm_secondary_entrypoint(u_register_t cxt_id)
{
	u_register_t my_mpidr, id;
	secondary_ep_t ep;

	my_mpidr = read_mpidr_el1() & MPID_MASK;
	ep = entrypoint[my_mpidr];
	id = context_id[my_mpidr];
	if (ep != NULL) {
		entrypoint[my_mpidr] = NULL;
		context_id[my_mpidr] = 0;
		(ep)(id);
	} else {
		/*
		 * Host can execute Rec directly without CPU_ON
		 * from Realm, if Rec is created RUNNABLE
		 * Jump to main in this case.
		 */
		while (true) {
			realm_payload_main();
		}
	}
	realm_cpu_off();
}
