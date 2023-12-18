/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <tsp_fuzz_helper.h>

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

void run_tsp_fuzz(char *funcstr)
{
	if (strcmp(funcstr, "tsp_add_op") == CMP_SUCCESS) {
		tftf_test_tsp_smc(TSP_ADD, "tsp_add_op");
	} else if (strcmp(funcstr, "tsp_sub_op") == CMP_SUCCESS) {
		tftf_test_tsp_smc(TSP_SUB, "tsp_sub_op");
	} else if (strcmp(funcstr, "tsp_mul_op") == CMP_SUCCESS) {
		tftf_test_tsp_smc(TSP_MUL, "tsp_mul_op");
	} else if (strcmp(funcstr, "tsp_div_op") == CMP_SUCCESS) {
		tftf_test_tsp_smc(TSP_DIV, "tsp_div_op");
	}
}
