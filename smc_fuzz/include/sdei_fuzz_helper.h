/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <time.h>

#include <fuzz_helper.h>
#include "smcmalloc.h"

#include <power_management.h>
#include <sdei.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <timer.h>

int64_t tftf_test_sdei_noarg(int64_t (*sdei_func)(void), char *funcstr);
void tftf_test_sdei_singlearg(int64_t (*sdei_func)(uint64_t), char *funcstr);
void run_sdei_fuzz(int funcid, struct memmod *mmod, bool inrange, int cntid);
char *return_str(int64_t ret);
void print_ret(char *funcstr, int64_t ret);
