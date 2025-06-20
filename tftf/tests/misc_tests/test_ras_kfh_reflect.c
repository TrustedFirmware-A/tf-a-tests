/*
 * Copyright (c) 2023-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <drivers/arm/arm_gic.h>
#include <irq.h>
#include <platform.h>
#include <psci.h>
#include <serror.h>
#include <smccc.h>
#include <tftf_lib.h>

#ifdef __aarch64__
static volatile uint64_t serror_triggered;
static volatile uint64_t irq_triggered;
static u_register_t expected_ver;
extern void inject_unrecoverable_ras_error(void);

/*
 * Tests to verify reflection of lower EL SErrors in RAS KFH mode.
 *
 * These tests exercises the path of EL3 reflection of SError back to lower
 * EL, which gets triggered as part of error synchronization during EL3
 * entry. This test works in conjunction with "ras_kfh_reflection.patch"
 * present in CI repository.
 *
 * One test each to verify reflection in sync and async exception.
 *
 */
static bool serror_handler(bool *incr_elr_elx)
{
	serror_triggered = 1;
	*incr_elr_elx = false;
	tftf_testcase_printf("SError event received.\n");
	return true;
}

static int irq_handler(void *data)
{
	irq_triggered = 1;
	tftf_testcase_printf("IRQ received.\n");
	return true;
}

/*
 * Test Steps:
 *  1. Register a custom SError handler for tftf
 *  2. Make an SMC call to get the SMCCC version which will be used for
 *     comparing later on, along with that it also changes SCR_EL3.I = 1
 *     to route IRQ to EL3.
 *  3. Disable SError (PSTATE.A = 1)
 *  4. Inject RAS error and give time for it to trigger.
 *  5. Register an SGI handler and inject SGI.
 *  6. Becaue the IRQ is targeted to EL3 it will trap in EL3 irq_vector_entry
 *  7. On entering EL3 it will find that SError is pending, So it will call
 *     "reflect_pending_serror_to_lower_el" and eret.
 *  8. TF-A will eret back from EL3(without handling IRQ) and during ERET
 *     change SCR_EL3.I back to 0 along with unmasking SError for TFTF.
 *     SPSR.PSTATE.A = 0.
 *  9. At tftf entry it will see both IRQ and SError pending, so it can take
 *     either of exception first (based on priority of SError/IRQ). The fvp model
 *     on which it was tested, IRQ is taken first.
 *  10.First IRQ handler will be called and then SError handler will called.
 *
 */
test_result_t test_ras_kfh_reflect_irq(void)
{
	smc_args args;
	unsigned int mpid = read_mpidr_el1();
        unsigned int core_pos = platform_get_core_pos(mpid);
        const unsigned int sgi_id = IRQ_NS_SGI_0;
	smc_ret_values smc_ret;
        int ret;

	/* Get the SMCCC version to compare against */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	smc_ret	= tftf_smc(&args);
	expected_ver = smc_ret.ret0;

	register_custom_serror_handler(serror_handler);
	disable_serror();
	inject_unrecoverable_ras_error();

	waitms(50);

	ret = tftf_irq_register_handler(sgi_id, irq_handler);
	 if (ret != 0) {
                tftf_testcase_printf("Failed to register initial IRQ handler\n");
                return TEST_RESULT_FAIL;
        }
	tftf_irq_enable(sgi_id, GIC_HIGHEST_NS_PRIORITY);
	tftf_send_sgi(sgi_id, core_pos);

	if ((serror_triggered == false) || (irq_triggered == false)) {
		tftf_testcase_printf("SError or IRQ is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	ret = tftf_irq_unregister_handler(sgi_id);
	if (ret != 0) {
		tftf_testcase_printf("Failed to unregister IRQ handler\n");
		return TEST_RESULT_FAIL;
	}

	unregister_custom_serror_handler();
	return TEST_RESULT_SUCCESS;
}

/*
 * Test Steps:
 *  1. Register a custom SError handler for tftf
 *  3. Disable SError (PSTATE.A = 1)
 *  4. Inject RAS error and give time for it to trigger.
 *  5. Ensure SError is not triggered before making SMC call.
 *  7. On entering EL3 it will find that SError is pending, So it will call
 *     "reflect_pending_serror_to_lower_el" and eret.
 *  8. TF-A will eret back from EL3(without handling SMC) and during ERET
 *     unmask SError for TFTF (SPSR.PSTATE.A = 0).
 *  9 .At TFTF entry it will see an SError pending which will cause registered
 *     SError handler to be called.
 *  10.After retruning back from EL3 the original SMC request will be handled.
 */
test_result_t test_ras_kfh_reflect_sync(void)
{
	smc_args args;
	smc_ret_values ret;

	serror_triggered = 0;

	register_custom_serror_handler(serror_handler);
	disable_serror();
	inject_unrecoverable_ras_error();

	waitms(50);

	/* Ensure that we are testing reflection path, SMC before SError */
	if (serror_triggered == true) {
		tftf_testcase_printf("SError was triggered before SMC\n");
		return TEST_RESULT_FAIL;
	}

	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	ret = tftf_smc(&args);
	tftf_testcase_printf("SMCCC Version = %d.%d\n",
		(int)((ret.ret0 >> SMCCC_VERSION_MAJOR_SHIFT) & SMCCC_VERSION_MAJOR_MASK),
		(int)((ret.ret0 >> SMCCC_VERSION_MINOR_SHIFT) & SMCCC_VERSION_MINOR_MASK));

	if ((int32_t)ret.ret0 != expected_ver) {
		tftf_testcase_printf("Unexpected SMCCC version: 0x%x\n", (int)ret.ret0);
		return TEST_RESULT_FAIL;
        }

	unregister_custom_serror_handler();

	if (serror_triggered == false) {
		tftf_testcase_printf("SError is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
#else
test_result_t test_ras_kfh_reflect_irq(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}

test_result_t test_ras_kfh_reflect_sync(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}
#endif
