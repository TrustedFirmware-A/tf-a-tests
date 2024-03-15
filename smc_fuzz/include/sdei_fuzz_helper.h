/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fuzz_helper.h>
#include "smcmalloc.h"

#include <power_management.h>
#include <sdei.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <timer.h>

#ifndef sdei_version_funcid
#define sdei_version_funcid 0
#endif
#ifndef sdei_pe_unmask_funcid
#define sdei_pe_unmask_funcid 0
#endif
#ifndef sdei_pe_mask_funcid
#define sdei_pe_mask_funcid 0
#endif
#ifndef sdei_event_status_funcid
#define sdei_event_status_funcid 0
#endif
#ifndef sdei_event_signal_funcid
#define sdei_event_signal_funcid 0
#endif
#ifndef sdei_private_reset_funcid
#define sdei_private_reset_funcid 0
#endif
#ifndef sdei_shared_reset_funcid
#define sdei_shared_reset_funcid 0
#endif


void tftf_test_sdei_noarg(int64_t (*sdei_func)(void), char *funcstr);
void tftf_test_sdei_singlearg(int64_t (*sdei_func)(uint64_t), char *funcstr);
void run_sdei_fuzz(int funcid, struct memmod *mmod);
