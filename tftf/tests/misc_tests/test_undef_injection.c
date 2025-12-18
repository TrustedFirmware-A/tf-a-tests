/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <assert.h>
#include <debug.h>
#include <smccc.h>
#include <sync.h>
#include <tftf_lib.h>
#include <platform_def.h>

static volatile bool undef_injection_triggered;

static bool undef_injection_handler(void)
{
	uint64_t esr_el2 = read_esr_el2();
	if (EC_BITS(esr_el2) == EC_UNKNOWN) {
		VERBOSE("UNDEF injection from EL3\n");
		undef_injection_triggered = true;
		return true;
	}

	return false;
}

/*
 * Test to verify UNDEF injection support in TF-A
 *
 * This test tries to access GCS EL2 registers which traps to EL3 and then
 * the error is injected back from EL3 to TFTF to ensure that injection
 * logic in TF-A is working, it also ensures that EL3 is still functional
 * after UNDEF injection.
 *
 * To trap GCS register access to EL3, we run this test on a model with
 * FEAT_GCS present but the traps from EL3 are not disabled by setting
 * ENABLE_FEAT_GCS = 0
 */
test_result_t test_undef_injection(void)
{
	undef_injection_triggered = false;

	register_custom_sync_exception_handler(undef_injection_handler);

	/* Try to access a register which traps to EL3 */
	read_gcscr_el2();

	unregister_custom_sync_exception_handler();

	/* Ensure that EL3 still functional */
	smc_args args;
	smc_ret_values smc_ret;
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	smc_ret = tftf_smc(&args);

	tftf_testcase_printf("SMCCC Version = %d.%d\n",
		(int)((smc_ret.ret0 >> SMCCC_VERSION_MAJOR_SHIFT) & SMCCC_VERSION_MAJOR_MASK),
		(int)((smc_ret.ret0 >> SMCCC_VERSION_MINOR_SHIFT) & SMCCC_VERSION_MINOR_MASK));

	if (undef_injection_triggered == false) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
