/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fuzz_helper.h>
#include <power_management.h>
#include <sdei.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <timer.h>

#ifndef tsp_add_op_funcid
#define tsp_add_op_funcid 0
#endif
#ifndef tsp_sub_op_funcid
#define tsp_sub_op_funcid 0
#endif
#ifndef tsp_mul_op_funcid
#define tsp_mul_op_funcid 0
#endif
#ifndef tsp_div_op_funcid
#define tsp_div_op_funcid 0
#endif

void tftf_test_tsp_smc(uint64_t tsp_id, char *funcstr);
void run_tsp_fuzz(int funcid);
