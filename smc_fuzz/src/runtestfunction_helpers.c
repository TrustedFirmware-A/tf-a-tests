/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sdei_fuzz_helper.h>
#include <tsp_fuzz_helper.h>


/*
 * Invoke the SMC call based on the function name specified.
 */
void runtestfunction(char *funcstr)
{
	run_sdei_fuzz(funcstr);
	run_tsp_fuzz(funcstr);
}
