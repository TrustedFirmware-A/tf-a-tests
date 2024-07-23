/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <time.h>

#include <fuzz_helper.h>
#include "smcmalloc.h"

#include <power_management.h>
#include <sdei.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <timer.h>

#ifndef ven_el3_svc_uuid_funcid
#define ven_el3_svc_uuid_funcid 0
#endif
#ifndef ven_el3_svc_count_funcid
#define ven_el3_svc_count_funcid 0
#endif
#ifndef ven_el3_svc_version_funcid
#define ven_el3_svc_version_funcid 0
#endif

void run_ven_el3_fuzz(int funcid, struct memmod *mmod);
