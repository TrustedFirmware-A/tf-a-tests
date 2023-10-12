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

static fpu_reg_state_t fpu_temp_rl;
/*
 * This function reads sleep time in ms from shared buffer and spins PE
 * in a loop for that time period.
 */
static void realm_sleep_cmd(void)
{
	uint64_t sleep = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);

	realm_printf("Realm: going to sleep for %llums\n", sleep);
	waitms(sleep);
}

/*
 * This function requests RSI/ABI version from RMM.
 */
static void realm_get_rsi_version(void)
{
	u_register_t version;

	version = rsi_get_version();
	if (version == (u_register_t)SMC_UNKNOWN) {
		realm_printf("SMC_RSI_ABI_VERSION failed (%ld)", (long)version);
		return;
	}

	realm_printf("RSI ABI version %u.%u (expected: %u.%u)",
	RSI_ABI_VERSION_GET_MAJOR(version),
	RSI_ABI_VERSION_GET_MINOR(version),
	RSI_ABI_VERSION_GET_MAJOR(RSI_ABI_VERSION),
	RSI_ABI_VERSION_GET_MINOR(RSI_ABI_VERSION));
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
			realm_get_rsi_version();
			test_succeed = true;
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
			fpu_state_fill_regs_and_template(&fpu_temp_rl);
			test_succeed = true;
			break;
		case REALM_REQ_FPU_CMP_CMD:
			test_succeed = fpu_state_compare_template(&fpu_temp_rl);
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
