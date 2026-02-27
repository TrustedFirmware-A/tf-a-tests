/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <fuzz_helper.h>
 #include "smcmalloc.h"

 #include <tftf_lib.h>

test_result_t run_psci_fuzz(int funcid, struct memmod *mmod);
