/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <fuzz_names.h>
#include <tsp_fuzz_helper.h>

/*
 * Generic TSP based function call for math operations
 */
void tftf_test_tsp_smc(uint64_t tsp_id, char *funcstr)
{
		uint64_t fn_identifier = TSP_FAST_FID(tsp_id);
		uint64_t arg1 = 4;
		uint64_t arg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, arg1, arg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
}

/*
 * TSP function called from fuzzer
 */
void run_tsp_fuzz(int funcid)
{
	if (funcid == tsp_add_op_funcid) {
		tftf_test_tsp_smc(TSP_ADD, "tsp_add_op");
	} else if (funcid == tsp_sub_op_funcid) {
		tftf_test_tsp_smc(TSP_SUB, "tsp_sub_op");
	} else if (funcid == tsp_mul_op_funcid) {
		tftf_test_tsp_smc(TSP_MUL, "tsp_mul_op");
	} else if (funcid == tsp_div_op_funcid) {
		tftf_test_tsp_smc(TSP_DIV, "tsp_div_op");
	}
}
