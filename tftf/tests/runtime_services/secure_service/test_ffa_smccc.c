/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>

#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <ffa_svc.h>
#include <runtime_services/spm_test_helpers.h>
#include <spm_common.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <xlat_tables_defs.h>

#define expect_eq(expr, value)							\
	do {									\
		if ((expr) != (value)) {					\
			ERROR("expect failed %s:%u\n", __FILE__, __LINE__);	\
			return TEST_RESULT_FAIL;				\
		}								\
	} while (0);

static const struct ffa_uuid sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}, {IVY_UUID}
	};

struct ffa_value8 {
	u_register_t fid;
	u_register_t arg1;
	u_register_t arg2;
	u_register_t arg3;
	u_register_t arg4;
	u_register_t arg5;
	u_register_t arg6;
	u_register_t arg7;
};

/* Declared in test_ffa_smccc_asm.S. */
uint32_t test_ffa_smc(struct ffa_value8 *);
uint32_t test_ffa_smc_ext(struct ffa_value *);

/**
 * FF-A service calls are emitted at the NS physical FF-A instance.
 * Such services do not return results in registers beyond x7.
 * Check callee(s) preserves GP registers beyond x7 per SMCCCv1.2.
 */
test_result_t test_smccc_callee_preserved(void)
{
	struct ffa_value8 args;
	struct mailbox_buffers mb;

	/*
	 * Permit running the test on configurations running
	 * the S-EL2 SPMC where 4 test partitions are deployed.
	 */
	CHECK_SPMC_TESTING_SETUP(1, 1, sp_uuids);
	reset_tftf_mailbox();

	/* Declare RX/TX buffers locally to the test. */
	CONFIGURE_MAILBOX(mb, PAGE_SIZE);

	memset(&args, 0, sizeof(struct ffa_value8));
	args.fid  = FFA_VERSION;
	args.arg1 = 0x10001;
	expect_eq(test_ffa_smc(&args), 0);
	expect_eq(args.fid, 0x10001);
	expect_eq(args.arg1, 0);
	expect_eq(args.arg2, 0);
	expect_eq(args.arg3, 0);
	expect_eq(args.arg4, 0);
	expect_eq(args.arg5, 0);
	expect_eq(args.arg6, 0);
	expect_eq(args.arg7, 0);

	memset(&args, 0, sizeof(struct ffa_value8));
	args.fid  = FFA_ID_GET;
	expect_eq(test_ffa_smc(&args), 0);
	expect_eq(args.fid, FFA_SUCCESS_SMC32);
	expect_eq(args.arg1, 0);
	expect_eq(args.arg2, 0);
	expect_eq(args.arg3, 0);
	expect_eq(args.arg4, 0);
	expect_eq(args.arg5, 0);
	expect_eq(args.arg6, 0);
	expect_eq(args.arg7, 0);

	memset(&args, 0, sizeof(struct ffa_value8));
	args.fid  = FFA_RXTX_MAP_SMC64;
	args.arg1 = (uintptr_t)mb.send;
	args.arg2 = (uintptr_t)mb.recv;
	args.arg3 = 1;
	expect_eq(test_ffa_smc(&args), 0);
	expect_eq(args.fid, FFA_SUCCESS_SMC32);
	expect_eq(args.arg1, 0);
	expect_eq(args.arg2, 0);
	expect_eq(args.arg3, 0);
	expect_eq(args.arg4, 0);
	expect_eq(args.arg5, 0);
	expect_eq(args.arg6, 0);
	expect_eq(args.arg7, 0);

	memset(&args, 0, sizeof(struct ffa_value8));
	args.fid = FFA_PARTITION_INFO_GET;
	expect_eq(test_ffa_smc(&args), 0);
	expect_eq(args.fid, FFA_SUCCESS_SMC32);
	expect_eq(args.arg1, 0);
	expect_eq(args.arg2, ARRAY_SIZE(sp_uuids));
	expect_eq(args.arg3, sizeof(struct ffa_partition_info));
	expect_eq(args.arg4, 0);
	expect_eq(args.arg5, 0);
	expect_eq(args.arg6, 0);
	expect_eq(args.arg7, 0);

	memset(&args, 0, sizeof(struct ffa_value8));
	args.fid = FFA_RX_RELEASE;
	expect_eq(test_ffa_smc(&args), 0);
	expect_eq(args.fid, FFA_SUCCESS_SMC32);
	expect_eq(args.arg1, 0);
	expect_eq(args.arg2, 0);
	expect_eq(args.arg3, 0);
	expect_eq(args.arg4, 0);
	expect_eq(args.arg5, 0);
	expect_eq(args.arg6, 0);
	expect_eq(args.arg7, 0);

	memset(&args, 0, sizeof(struct ffa_value8));
	args.fid = FFA_RXTX_UNMAP;
	expect_eq(test_ffa_smc(&args), 0);
	expect_eq(args.fid, FFA_SUCCESS_SMC32);
	expect_eq(args.arg1, 0);
	expect_eq(args.arg2, 0);
	expect_eq(args.arg3, 0);
	expect_eq(args.arg4, 0);
	expect_eq(args.arg5, 0);
	expect_eq(args.arg6, 0);
	expect_eq(args.arg7, 0);

	return TEST_RESULT_SUCCESS;
}

/**
 * An FF-A service call is emitted at the NS physical FF-A instance.
 * The service returns results in x0-x17 registers.
 * Check callee(s) preserve GP registers beyond x17 per SMCCCv1.2.
 */
test_result_t test_smccc_ext_callee_preserved(void)
{
	struct ffa_value args_ext;

	CHECK_SPMC_TESTING_SETUP(1, 1, sp_uuids);

	/* Test the SMCCC extended registers range. */
	memset(&args_ext, 0, sizeof(struct ffa_value));
	args_ext.fid  = FFA_PARTITION_INFO_GET_REGS_SMC64;
	expect_eq(test_ffa_smc_ext(&args_ext), 0);
	expect_eq(args_ext.fid, FFA_SUCCESS_SMC64);
	expect_eq(args_ext.arg1, 0);
	expect_eq(args_ext.arg2 >> 48, sizeof(struct ffa_partition_info));
	expect_eq(args_ext.arg2 & 0xffff, ARRAY_SIZE(sp_uuids) - 1);

	return TEST_RESULT_SUCCESS;
}
