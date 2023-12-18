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

void tftf_test_tsp_smc(uint64_t tsp_id, char *);
void run_tsp_fuzz(char *);
