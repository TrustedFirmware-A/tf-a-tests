/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fuzz_helper.h>
#include <power_management.h>
#include <sdei.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <timer.h>

void tftf_test_sdei_noarg(int64_t (*sdei_func)(void), char *);
void tftf_test_sdei_singlearg(int64_t (*sdei_func)(uint64_t), char *funcstr);
void run_sdei_fuzz(char *);
