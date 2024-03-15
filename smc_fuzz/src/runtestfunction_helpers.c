/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sdei_fuzz_helper.h>
#include "smcmalloc.h"
#include <tsp_fuzz_helper.h>

/*
 * Invoke the SMC call based on the function name specified.
 */
void runtestfunction(int funcid, struct memmod *mmod)
{
	run_sdei_fuzz(funcid, mmod);
	run_tsp_fuzz(funcid);
}
