/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
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
#include <realm_helpers.h>
#include <realm_psi.h>
#include <realm_rsi.h>
#include <realm_tests.h>
#include <serror.h>
#include <sync.h>
#include <tftf_lib.h>

static fpu_state_t rl_fpu_state_write;
static fpu_state_t rl_fpu_state_read;
static rsi_plane_run run __aligned(PAGE_SIZE);

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

static bool test_realm_enter_plane_n(void)
{
	u_register_t base, plane_index, perm_index, flags = 0U;
	bool ret1;

	plane_index = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	base = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);
	perm_index = plane_index + 1U;

	ret1 = plane_common_init(plane_index, perm_index, base, &run);
	if (!ret1) {
		return ret1;
	}

	realm_printf("Entering plane %ld, ep=0x%lx run=0x%lx\n", plane_index, base, &run);
	return realm_plane_enter(plane_index, perm_index, base, flags, &run);
}

static bool test_realm_enter_plane_n_reg_rw(void)
{
	u_register_t base, plane_index, perm_index, flags = 0U;
	u_register_t reg1, reg2, reg3, reg4, ret;
	bool ret1;

	if (realm_is_plane0()) {
		plane_index = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
		base = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);
		perm_index = plane_index + 1U;

		ret1 = plane_common_init(plane_index, perm_index, base, &run);
		if (!ret1) {
			return ret1;
		}

		realm_printf("Entering plane %ld, ep=0x%lx run=0x%lx\n", plane_index, base, &run);
		ret = realm_plane_enter(plane_index, perm_index, base, flags, &run);
		if (ret) {
			/* get return value from plane1 */
			reg1 = realm_shared_data_get_plane_n_val(plane_index,
					REC_IDX(read_mpidr_el1()), HOST_ARG1_INDEX);

			reg2 = realm_shared_data_get_plane_n_val(plane_index,
					REC_IDX(read_mpidr_el1()), HOST_ARG2_INDEX);

			realm_printf("P0 read 0x%lx 0x%lx\n", reg1, reg2);

			/* read pauth register for plane1 */
			ret = rsi_plane_reg_read(plane_index, SYSREG_ID_apiakeylo_el1, &reg3);
			if ((ret != RSI_SUCCESS) || (reg1 != reg3)) {
				realm_printf("pauth register mismatch 0x%lx 0x%lx\n", reg1, reg3);
				return false;
			}

			/* read sctlr register for plane1 */
			ret = rsi_plane_reg_read(plane_index, SYSREG_ID_sctlr_el1, &reg4);
			if ((ret != RSI_SUCCESS) || (reg2 != reg4)) {
				realm_printf("sctlr register mismatch 0x%lx 0x%lx\n", reg2, reg4);
				return false;
			}

			/* write pauth register and verify it is same after exiting plane n */
			ret = rsi_plane_reg_write(plane_index, SYSREG_ID_apibkeylo_el1, 0xABCD);
			if (ret != RSI_SUCCESS) {
				realm_printf("pauth register write failed\n");
				return false;
			}

			/* enter plane n */
			ret = realm_plane_enter(plane_index, perm_index, base, flags, &run);
			if (ret) {
				/* read pauth register for plane1 */
				ret = rsi_plane_reg_read(plane_index, SYSREG_ID_apibkeylo_el1,
						&reg3);

				if ((ret != RSI_SUCCESS) || (reg3 != 0xABCD)) {
					realm_printf("reg mismatch after write 0x%lx\n", reg3);
					return false;
				}
			}

			/* read sysreg not supported by rmm, expect error */
			ret = rsi_plane_reg_read(plane_index, SYSREG_ID_mpamidr_el1, &reg3);
			if (ret == RSI_SUCCESS) {
				realm_printf("reg read should have failed\n");
				return false;
			}
			return true;
		}
		return false;
	} else {
		realm_printf("PN set 0x%lx 0x%lx\n", read_apiakeylo_el1(), read_sctlr_el1());

		/* return pauth and sctlr back to p0 */
		realm_shared_data_set_my_realm_val(HOST_ARG1_INDEX, read_apiakeylo_el1());
		realm_shared_data_set_my_realm_val(HOST_ARG2_INDEX, read_sctlr_el1());
		return true;
	}
}

static bool test_realm_s2poe_undef_abort(void)
{
	realm_reset_undef_abort_count();

	/* Install exception handler to catch undefined abort */
	register_custom_sync_exception_handler(realm_sync_exception_handler);
	write_s2por_el1(0UL);
	unregister_custom_sync_exception_handler();

	return (realm_get_undef_abort_count() != 0UL);
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

bool test_realm_set_ripas(void)
{
	u_register_t ret, base, new_base, top, new_top;
	rsi_ripas_respose_type response;
	rsi_ripas_type ripas;

	base = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	top = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);
	realm_printf("base=0x%lx top=0x%lx\n", base, top);
	ret = rsi_ipa_state_get(base, top, &new_top, &ripas);
	if (ripas != RSI_EMPTY) {
		return false;
	}

	ret = rsi_ipa_state_set(base, top, RSI_RAM,
				RSI_NO_CHANGE_DESTROYED, &new_base, &response);
	if ((ret != RSI_SUCCESS) || (response != RSI_ACCEPT)) {
		return false;
	}

	while (new_base < top) {
		realm_printf("new_base=0x%lx top=0x%lx\n", new_base, top);
		ret = rsi_ipa_state_set(new_base, top, RSI_RAM,
				RSI_NO_CHANGE_DESTROYED, &new_base, &response);
		if ((ret != RSI_SUCCESS) || (response != RSI_ACCEPT)) {
			realm_printf("rsi_ipa_state_set failed\n");
			return false;
		}
	}

	/* Verify that RIAS has changed for range base-top */
	ret = rsi_ipa_state_get(base, top, &new_top, &ripas);
	if ((ret != RSI_SUCCESS) || (ripas != RSI_RAM) || (new_top != top)) {
		realm_printf("rsi_ipa_state_get failed base=0x%lx top=0x%lx",
				"new_top=0x%lx ripas=%u ret=0x%lx\n",
				base, top, ripas);
		return false;
	}

	return true;
}

bool test_realm_reject_set_ripas(void)
{
	u_register_t ret, base, top, new_base, new_top;
	rsi_ripas_respose_type response;
	rsi_ripas_type ripas;

	base = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	top = base + PAGE_SIZE;
	ret = rsi_ipa_state_get(base, top, &new_top, &ripas);
	if ((ret != RSI_SUCCESS) || (ripas != RSI_EMPTY)) {
		realm_printf("Wrong initial ripas=%u\n", ripas);
		return false;
	}
	ret = rsi_ipa_state_set(base, top, RSI_RAM,
				RSI_NO_CHANGE_DESTROYED, &new_base, &response);
	if ((ret == RSI_SUCCESS) && (response == RSI_REJECT)) {
		realm_printf("rsi_ipa_state_set passed response=%u\n", response);
		ret = rsi_ipa_state_get(base, top, &new_top, &ripas);
		if ((ret == RSI_SUCCESS) && (ripas == RSI_EMPTY) &&
						(new_top == top)) {
			return true;
		} else {
			realm_printf("rsi_ipa_state_get failed top=0x%lx",
					"new_top=0x%lx ripas=%u ret=0x%lx\n",
					top, new_top, ripas, ret);
			return false;
		}
	}
	realm_printf("rsi_ipa_state_set failed ret=0x%lx response=%u\n",
			ret, response);
	return false;
}

bool test_realm_dit_check_cmd(void)
{
	if (is_armv8_4_dit_present()) {
		write_dit(DIT_BIT);
		realm_printf("Testing DIT=0x%lx\n", read_dit());
		/* Test if DIT is preserved after HOST_CALL */
		if (read_dit() == DIT_BIT) {
			return true;
		}
	}
	return false;
}

static bool test_plane_exception_cmd(void)
{
	u_register_t base, plane_index, perm_index, flags = 0U;
	bool ret1;

	plane_index = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	base = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);
	perm_index = plane_index + 1U;

	ret1 = plane_common_init(plane_index, perm_index, base, &run);
	if (!ret1) {
		return ret1;
	}

	realm_printf("Entering plane %ld, ep=0x%lx run=0x%lx\n", plane_index, base, &run);
	ret1 = realm_plane_enter(plane_index, perm_index, base, flags, &run);

	realm_printf("Plane exit reason=0x%lx\n", run.exit.exit_reason);

	/*
	 * @TODO- P0 can inject SEA back to PN,
	 * if capability is added in future.
	 */
	if (ret1 && (run.exit.exit_reason == RSI_EXIT_SYNC)) {
		u_register_t far, esr, elr;

		far = run.exit.far;
		esr = run.exit.esr;
		elr = run.exit.elr;

		/* Return ESR FAR to Host */
		realm_shared_data_set_my_realm_val(HOST_ARG2_INDEX, esr);
		realm_shared_data_set_my_realm_val(HOST_ARG3_INDEX, far);
		realm_printf("Plane exit FAR=0x%lx ESR=0x%lx ELR=0x%lx\n",
			far, esr, elr);
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	}
	return false;
}

static bool test_realm_instr_fetch_cmd(void)
{
	u_register_t base, new_top;
	void (*func_ptr)(void);
	rsi_ripas_type ripas;

	base = realm_shared_data_get_my_host_val(HOST_ARG3_INDEX);
	rsi_ipa_state_get(base, base + PAGE_SIZE, &new_top, &ripas);
	realm_printf("Initial ripas=%u\n", ripas);
	/* Causes instruction abort */
	realm_printf("Generate Instruction Abort base=0x%lx\n", base);
	func_ptr = (void (*)(void))base;
	func_ptr();
	/* Should not return */
	return false;
}

static bool test_realm_data_access_cmd(void)
{
	u_register_t base, new_top;
	rsi_ripas_type ripas;
	base = realm_shared_data_get_my_host_val(HOST_ARG3_INDEX);
	rsi_ipa_state_get(base, base + PAGE_SIZE, &new_top, &ripas);
	realm_printf("Initial ripas=%u\n", ripas);
	/* Causes data abort */
	realm_printf("Generate Data Abort base=0x%lx\n", base);
	*((volatile uint64_t *)base);

	return false;
}

static bool realm_exception_handler(void)
{
	u_register_t base, far, esr, elr;

	base = realm_shared_data_get_my_host_val(HOST_ARG3_INDEX);
	far = read_far_el1();
	esr = read_esr_el1();
	elr = read_elr_el1();

	if (far == base) {
		/* Return ESR to Host */
		realm_shared_data_set_my_realm_val(HOST_ARG2_INDEX, esr);
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	}
	realm_printf("Realm Abort fail incorrect FAR=0x%lx ESR=0x%lx ELR=0x%lx\n",
			far, esr, elr);
	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);

	/* Should not return */
	return false;
}

static bool realm_serror_handler_doublefault(bool *incr_elr_elx)
{
	*incr_elr_elx = false;

	if ((read_sctlr2_el1() & SCTLR2_EASE_BIT) != 0UL) {
		/* The serror exception should have been routed here */
		*incr_elr_elx = true;

		return true;
	}

	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);

	/* Should have never get here */
	return false;
}

static bool realm_sync_handler_doublefault(void)
{
	if ((read_sctlr2_el1() & SCTLR2_EASE_BIT) == 0UL) {
		/* The sync exception should have been routed here */
		return true;
	}

	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);

	/* Should have never get here */
	return false;
}

static void test_realm_feat_doublefault2(void)
{
	u_register_t ease_bit = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);

	unregister_custom_sync_exception_handler();
	register_custom_sync_exception_handler(realm_sync_handler_doublefault);
	register_custom_serror_handler(realm_serror_handler_doublefault);

	if (ease_bit != 0UL) {
		write_sctlr2_el1(read_sctlr2_el1() | SCTLR2_EASE_BIT);
	} else {
		write_sctlr2_el1(read_sctlr2_el1() & ~SCTLR2_EASE_BIT);
	}

	(void)test_realm_data_access_cmd();
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

	register_custom_sync_exception_handler(realm_exception_handler);

	/* No serror handler registered by default */
	unregister_custom_serror_handler();

	realm_set_shared_structure(realm_get_ns_buffer());

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
		case REALM_ENTER_PLANE_N_CMD:
			test_succeed = test_realm_enter_plane_n();
			break;
		case REALM_PLANE_N_REG_RW_CMD:
			test_succeed = test_realm_enter_plane_n_reg_rw();
			break;
		case REALM_S2POE_ACCESS:
			test_succeed = test_realm_s2poe_undef_abort();
			break;
		case REALM_MPAM_ACCESS:
			test_succeed = test_realm_mpam_undef_abort();
			break;
		case REALM_MPAM_PRESENT:
			/* FEAT_MPAM must be hidden to the Realm */
			test_succeed = !is_feat_mpam_supported();
			break;
		case REALM_MULTIPLE_REC_PSCI_DENIED_CMD:
			test_succeed = test_realm_multiple_rec_psci_denied_cmd();
			break;
		case REALM_MULTIPLE_REC_MULTIPLE_CPU_CMD:
			test_succeed = test_realm_multiple_rec_multiple_cpu_cmd();
			break;
		case REALM_PLANES_MULTIPLE_REC_MULTIPLE_CPU_CMD:
			test_succeed = test_realm_multiple_plane_multiple_rec_multiple_cpu_cmd();
			break;
		case REALM_FEAT_DOUBLEFAULT2_TEST:
			test_realm_feat_doublefault2();
			test_succeed = true;
			break;
		case REALM_INSTR_FETCH_CMD:
			test_succeed = test_realm_instr_fetch_cmd();
			break;
		case REALM_DATA_ACCESS_CMD:
			test_succeed = test_realm_data_access_cmd();
			break;
		case REALM_PLANE_N_EXCEPTION_CMD:
			test_succeed = test_plane_exception_cmd();
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
		case REALM_DIT_CHECK_CMD:
			test_succeed = test_realm_dit_check_cmd();
			break;
		case REALM_GET_RSI_VERSION:
			test_succeed = realm_get_rsi_version();
			break;
		case REALM_PMU_CYCLE:
			test_succeed = test_pmuv3_cycle_works_realm();
			break;
		case REALM_PMU_COUNTER:
			test_succeed = test_pmuv3_counter();
			break;
		case REALM_PMU_EVENT:
			test_succeed = test_pmuv3_event_works_realm();
			break;
		case REALM_PMU_PRESERVE:
			test_succeed = test_pmuv3_rmm_preserves();
			break;
		case REALM_PMU_CYCLE_INTERRUPT:
			test_succeed = test_pmuv3_overflow_interrupt(true);
			break;
		case REALM_PMU_EVENT_INTERRUPT:
			test_succeed = test_pmuv3_overflow_interrupt(false);
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
		case REALM_REJECT_SET_RIPAS_CMD:
			test_succeed = test_realm_reject_set_ripas();
			break;
		case REALM_SET_RIPAS_CMD:
			test_succeed = test_realm_set_ripas();
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
		case REALM_ATTESTATION:
			test_succeed = test_realm_attestation();
			break;
		case REALM_ATTESTATION_FAULT:
			test_succeed = test_realm_attestation_fault();
			break;
		case REALM_WRITE_BRBCR_EL1:
			test_succeed = test_realm_write_brbcr_el1_reg();
			break;
		default:
			realm_printf("%s() invalid cmd %u\n", __func__, cmd);
			break;
		}
	}

	if (test_succeed) {
		if (realm_is_plane0()) {
			rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
		} else {
			psi_exit_to_plane0(PSI_CALL_EXIT_SUCCESS_CMD,
					0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL);
		}
	} else {
		if (realm_is_plane0()) {
			rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
		} else {
			psi_exit_to_plane0(PSI_CALL_EXIT_FAILED_CMD,
					0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL);
		}
	}
}
