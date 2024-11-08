/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/context_mgmt/context_el1.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#define TSP_MODIFY_WITH_DUMMY_VALUE	1
#define TSP_RESTORE_WITH_DEFAULT	0

#ifdef	__aarch64__
/**
 * Global place holders for the entire test
 */
static el1_ctx_regs_t el1_ctx_original = {0};
static el1_ctx_regs_t el1_ctx_before_smc = {0};
static el1_ctx_regs_t el1_ctx_after_eret = {0};
#endif

/**
 * Public Function: test_tsp_el1_ctx_regs_ctx_mgmt.
 *
 * @Test_Aim: This Test aims to validate EL1 context registers are restored to
 * correct values upon returning from context switch triggered by an Standard
 * SMC to TSP. This will ensure the context-management library at EL3 precisely
 * saves and restores the appropriate world specific context during world switch
 * and prevents data leakage in all cases.
 */
test_result_t test_tsp_el1_regs_ctx_mgmt(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	smc_args tsp_svc_params;
	smc_ret_values tsp_result = {0};
	test_result_t ret_value;
	bool result;

	SKIP_TEST_IF_TSP_NOT_PRESENT();

	/**
	 * 1. Read the values of EL1 registers and save them into the
	 * global buffer before the world switch.
	 * This will be safe copy of registers which will be restored
	 * back at the end of the test.
	 */
	save_el1_sysregs_context(&el1_ctx_original);

	/**
	 * 2. Modify/Corrupt the EL1 registers with some dummy values.
	 * This will be essential for the test to ensure registers are
	 * accessed and modified in both the security states, which will
	 * be an ideal usecase in realtime.
	 */
	modify_el1_context_sysregs(&el1_ctx_original, NS_CORRUPT_EL1_REGS);

	/**
	 * 3. Read the values of EL1 registers again after modification
	 * and save them into an other buffer before the world switch.
	 * This will be used for comparison at the later stage on ERET.
	 */
	save_el1_sysregs_context(&el1_ctx_before_smc);

	/* 4. Standard SMC to TSP, to modify EL1 registers in secure world */
	tsp_svc_params.fid = TSP_STD_FID(TSP_MODIFY_EL1_CTX);
	tsp_svc_params.arg1 = TSP_MODIFY_WITH_DUMMY_VALUE;
	tsp_result = tftf_smc(&tsp_svc_params);

	/* Check the result of the TSP_MODIFY_EL1_CTX STD_SMC call */
	if (tsp_result.ret0 != 0) {
		ERROR("TSP_MODIFY_EL1_CTX returned wrong result: "
				     "got %d expected: 0 \n",
				     (unsigned int)tsp_result.ret0);
		return TEST_RESULT_FAIL;
	}

	/**
	 * 5. Read the values of EL1 registers and save them into
	 * the global buffer after the ERET.
	 */
	save_el1_sysregs_context(&el1_ctx_after_eret);

	/* 6. Validate the EL1 contexts */
	result = compare_el1_contexts(&el1_ctx_before_smc, &el1_ctx_after_eret);

	if (result) {
		INFO("EL1 context is safely handled by the "
					"EL3 Ctx-management Library\n");
		ret_value = TEST_RESULT_SUCCESS;
	} else {
		ERROR("EL1 context is corrupted during world switch\n");
		ret_value = TEST_RESULT_FAIL;

		/* Print the saved EL1 context before world switch. */
		INFO("EL1 context registers list before SMC in NWd:\n");
		print_el1_sysregs_context(&el1_ctx_before_smc);

		/* Print the saved EL1 context after ERET */
		INFO("EL1 context registers list after ERET in NWd:\n");
		print_el1_sysregs_context(&el1_ctx_after_eret);
	}

	/**
	 * 7. Restore the EL1 registers to their original value in order
	 * to avoid any unintended crash for next tests.
	 */
	modify_el1_context_sysregs(&el1_ctx_original, NS_RESTORE_EL1_REGS);

	/* 8. Standard SMC to TSP, to restore EL1 regs in secure world */
	tsp_svc_params.fid = TSP_STD_FID(TSP_MODIFY_EL1_CTX);
	tsp_svc_params.arg1 = TSP_RESTORE_WITH_DEFAULT;
	tsp_result = tftf_smc(&tsp_svc_params);

	/* Check the result of the TSP_MODIFY_EL1_CTX STD_SMC call */
	if (tsp_result.ret0 != 0) {
		ERROR("TSP_MODIFY_EL1_CTX returned wrong result: "
				     "got %d expected: 0 \n",
				     (unsigned int)tsp_result.ret0);
		return TEST_RESULT_FAIL;
	}
	return ret_value;

#endif /* __aarch64__ */
}
