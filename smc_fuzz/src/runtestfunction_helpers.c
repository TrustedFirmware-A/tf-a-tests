/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arg_struct_def.h>
#include <ffa_fuzz_helper.h>
#include <sdei_fuzz_helper.h>
#include "smcmalloc.h"
#include <tsp_fuzz_helper.h>
#include <tftf_lib.h>

int cntid;
#include <vendor_fuzz_helper.h>

/*
 * Invoke the SMC call based on the function name specified.
 */
test_result_t runtestfunction(int funcid, struct memmod *mmod)
{
	test_result_t res = TEST_RESULT_SUCCESS;
	bool inrange = (cntid >= SMC_FUZZ_CALL_START) && (cntid < SMC_FUZZ_CALL_END);
	inrange = inrange && (funcid != EXCLUDE_FUNCID);
#ifdef SDEI_INCLUDE
	res = run_sdei_fuzz(funcid, mmod, inrange, cntid);
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}
#endif
#ifdef FFA_INCLUDE
	run_ffa_fuzz(funcid, mmod);
#endif
	run_tsp_fuzz(funcid);
#ifdef VEN_INCLUDE
	run_ven_el3_fuzz(funcid, mmod);
#endif

	cntid++;

	return res;
}
