/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sdei_fuzz_helper.h>
#include "smcmalloc.h"
#include <tsp_fuzz_helper.h>

int cntid = 0;

#include <vendor_fuzz_helper.h>

/*
 * Invoke the SMC call based on the function name specified.
 */
void runtestfunction(int funcid, struct memmod *mmod)
{
	bool inrange = (cntid >= SMC_FUZZ_CALL_START) && (cntid < SMC_FUZZ_CALL_END);
	inrange = inrange && (funcid != EXCLUDE_FUNCID);

	run_sdei_fuzz(funcid, mmod, inrange, cntid);
	run_tsp_fuzz(funcid);
	run_ven_el3_fuzz(funcid, mmod);

	cntid++;
}
