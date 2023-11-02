/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <psci.h>
#include <sdei.h>
#include <smccc.h>
#include <tftf_lib.h>

#ifdef __aarch64__
static volatile uint64_t sdei_event_received;
extern void inject_unrecoverable_ras_error(void);
extern int serror_sdei_event_handler(int ev, uint64_t arg);

int sdei_handler(int ev, uint64_t arg)
{
	sdei_event_received = 1;
	tftf_testcase_printf("SError SDEI event received.\n");
	return 0;
}

/*
 * Test to verify nested exception handling of SErrors in EL3.
 *
 * This test exercises the path of EL3 nested exception handling of SErrors
 * during SMC exception handling. In SMC exception handling vector path
 * during synchronization of errors, a pending async EA is detected which
 * gets handled in EL3 (as it is in FFH mode) as a nested exception. Original
 * SMC call is handled after async EA is handled.
 *
 * This test works in conjunction with "ras_ffh_nested.patch"
 * present in CI repository.
 *
 * Test steps:
 *  1. TF-A is build for Firmware first handling for RAS errors.
 *  2. Register/enable SDEI event notification for RAS error.
 *  3. Make an SMC call to get the SMCCC version which will be used for
 *     comparing later on, along with that it also changes SCR_EL3.EA=0 to
 *     route SError to TFTF. This allow SError to be pended when next SMC
 *     call is made.
 *  4. Disable SError (PSTATE.A = 1)
 *  5. Inject RAS error and give time for it to trigger.
 *  6. At this point SError is pended (ISR_EL1 = 0x100)
 *  7. Make SMC call to get the version
 *  8. On entering EL3, sync_exception_vector entry, will find that SError is
 *     pending.
 *  9. Based on FFH routing model EL3 will call "handle_pending_async_ea" to
 *     handle nested exception SError first.
 *  10.RAS error will be handled by platform handler and be notified to TFTF
 *     through SDEI handler.
 *  12.Once the control returns back to vector entry of SMC, EL3 will continue
 *     with original SMC request.
 *
 * Checks:
 *  1. Ensure that we did recieve SDEI notification
 *  2. Ensure that second SMC request was successful.
 *
 */
test_result_t test_ras_ffh_nested(void)
{
	int64_t ret;
	const int event_id = 5000;
	smc_args args;
	smc_ret_values smc_ret;
	u_register_t expected_ver;

        /* Register SDEI handler */
        ret = sdei_event_register(event_id, serror_sdei_event_handler, 0,
                        SDEI_REGF_RM_PE, read_mpidr_el1());
        if (ret < 0) {
                tftf_testcase_printf("SDEI event register failed: 0x%llx\n",
                        ret);
                return TEST_RESULT_FAIL;
        }

        ret = sdei_event_enable(event_id);
        if (ret < 0) {
                tftf_testcase_printf("SDEI event enable failed: 0x%llx\n", ret);
                return TEST_RESULT_FAIL;
        }

        ret = sdei_pe_unmask();
        if (ret < 0) {
                tftf_testcase_printf("SDEI pe unmask failed: 0x%llx\n", ret);
                return TEST_RESULT_FAIL;
        }

	/* Get the version to compare against */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	smc_ret = tftf_smc(&args);
	expected_ver = smc_ret.ret0;
	smc_ret.ret0 = 0;

	disable_serror();

        inject_unrecoverable_ras_error();

	waitms(50);

	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;

	/* Ensure that we are testing reflection path, SMC before SError */
	if (sdei_event_received == true) {
		tftf_testcase_printf("SError was triggered before SMC\n");
		return TEST_RESULT_FAIL;
	}

	smc_ret = tftf_smc(&args);

	tftf_testcase_printf("SMCCC Version = %d.%d\n",
		(int)((smc_ret.ret0 >> SMCCC_VERSION_MAJOR_SHIFT) & SMCCC_VERSION_MAJOR_MASK),
		(int)((smc_ret.ret0 >> SMCCC_VERSION_MINOR_SHIFT) & SMCCC_VERSION_MINOR_MASK));

	if ((int32_t)smc_ret.ret0 != expected_ver) {
		printf("Unexpected SMCCC version: 0x%x\n", (int)smc_ret.ret0);
		return TEST_RESULT_FAIL;
        }

	if (sdei_event_received == false) {
		tftf_testcase_printf("SError is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
#else
test_result_t test_ras_ffh_nested(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}
#endif
