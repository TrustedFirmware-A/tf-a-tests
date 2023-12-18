/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include <arch_features.h>
#include <debug.h>
#include <fpu.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <pauth.h>
#include "realm_def.h"
#include <realm_rsi.h>
#include <realm_tests.h>
#include <tftf_lib.h>

static fpu_state_t rl_fpu_state_write;
static fpu_state_t rl_fpu_state_read;

/*
 * This function reads sleep time in ms from shared buffer and spins PE
 * in a loop for that time period.
 */
static void realm_sleep_cmd(void)
{
	uint64_t sleep = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);

	realm_printf("going to sleep for %llums\n", sleep);
	waitms(sleep);
}

static void realm_loop_cmd(void)
{
	while (true) {
		waitms(500);
	}
}

/*
 * This function requests RSI/ABI version from RMM.
 */
static bool realm_get_rsi_version(void)
{
	u_register_t version = 0U;

	version = rsi_get_version(RSI_ABI_VERSION_VAL);
	if (version == (u_register_t)SMC_UNKNOWN) {
		realm_printf("SMC_RSI_ABI_VERSION failed\n");
		return false;
	}

	realm_printf("RSI ABI version %u.%u (expected: %u.%u)\n",
	RSI_ABI_VERSION_GET_MAJOR(version),
	RSI_ABI_VERSION_GET_MINOR(version),
	RSI_ABI_VERSION_GET_MAJOR(RSI_ABI_VERSION_VAL),
	RSI_ABI_VERSION_GET_MINOR(RSI_ABI_VERSION_VAL));
	return true;
}

/*
 * This is the entry function for Realm payload, it first requests the shared buffer
 * IPA address from Host using HOST_CALL/RSI, it reads the command to be executed,
 * performs the request, and returns to Host with the execution state SUCCESS/FAILED
 *
 * Host in NS world requests Realm to execute certain operations using command
 * depending on the test case the Host wants to perform.
 */
void realm_payload_main(void)
{
	bool test_succeed = false;

	realm_set_shared_structure((host_shared_data_t *)rsi_get_ns_buffer());
	if (realm_get_my_shared_structure() != NULL) {
		uint8_t cmd = realm_shared_data_get_my_realm_cmd();

		switch (cmd) {
		case REALM_SLEEP_CMD:
			realm_sleep_cmd();
			test_succeed = true;
			break;
		case REALM_LOOP_CMD:
			realm_loop_cmd();
			test_succeed = true;
			break;
		case REALM_MULTIPLE_REC_PSCI_DENIED_CMD:
			test_succeed = test_realm_multiple_rec_psci_denied_cmd();
			break;
		case REALM_PAUTH_SET_CMD:
			test_succeed = test_realm_pauth_set_cmd();
			break;
		case REALM_PAUTH_CHECK_CMD:
			test_succeed = test_realm_pauth_check_cmd();
			break;
		case REALM_PAUTH_FAULT:
			test_succeed = test_realm_pauth_fault();
			break;
		case REALM_GET_RSI_VERSION:
			test_succeed = realm_get_rsi_version();
			break;
		case REALM_PMU_CYCLE:
			test_succeed = test_pmuv3_cycle_works_realm();
			break;
		case REALM_PMU_EVENT:
			test_succeed = test_pmuv3_event_works_realm();
			break;
		case REALM_PMU_PRESERVE:
			test_succeed = test_pmuv3_rmm_preserves();
			break;
		case REALM_PMU_INTERRUPT:
			test_succeed = test_pmuv3_overflow_interrupt();
			break;
		case REALM_REQ_FPU_FILL_CMD:
			fpu_state_write_rand(&rl_fpu_state_write);
			test_succeed = true;
			break;
		case REALM_REQ_FPU_CMP_CMD:
			fpu_state_read(&rl_fpu_state_read);
			test_succeed = !fpu_state_compare(&rl_fpu_state_write,
							  &rl_fpu_state_read);
			break;
		case REALM_SVE_RDVL:
			test_succeed = test_realm_sve_rdvl();
			break;
		case REALM_SVE_ID_REGISTERS:
			test_succeed = test_realm_sve_read_id_registers();
			break;
		case REALM_SVE_PROBE_VL:
			test_succeed = test_realm_sve_probe_vl();
			break;
		case REALM_SVE_OPS:
			test_succeed = test_realm_sve_ops();
			break;
		case REALM_SVE_FILL_REGS:
			test_succeed = test_realm_sve_fill_regs();
			break;
		case REALM_SVE_CMP_REGS:
			test_succeed = test_realm_sve_cmp_regs();
			break;
		case REALM_SVE_UNDEF_ABORT:
			test_succeed = test_realm_sve_undef_abort();
			break;
		case REALM_SME_ID_REGISTERS:
			test_succeed = test_realm_sme_read_id_registers();
			break;
		case REALM_SME_UNDEF_ABORT:
			test_succeed = test_realm_sme_undef_abort();
			break;
		default:
			realm_printf("%s() invalid cmd %u\n", __func__, cmd);
			break;
		}
	}

	if (test_succeed) {
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	} else {
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}
}
