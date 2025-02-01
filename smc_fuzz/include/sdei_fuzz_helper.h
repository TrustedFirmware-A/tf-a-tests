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


#ifndef sdei_event_register_funcid
#define sdei_event_register_funcid 0
#endif
#ifndef sdei_event_enable_funcid
#define sdei_event_enable_funcid 0
#endif
#ifndef sdei_event_disable_funcid
#define sdei_event_disable_funcid 0
#endif
#ifndef sdei_event_context_funcid
#define sdei_event_context_funcid 0
#endif
#ifndef sdei_event_complete_funcid
#define sdei_event_complete_funcid 0
#endif
#ifndef sdei_event_complete_and_resume_funcid
#define sdei_event_complete_and_resume_funcid 0
#endif
#ifndef sdei_event_unregister_funcid
#define sdei_event_unregister_funcid 0
#endif
#ifndef sdei_event_get_info_funcid
#define sdei_event_get_info_funcid 0
#endif
#ifndef sdei_event_routing_set_funcid
#define sdei_event_routing_set_funcid 0
#endif
#ifndef sdei_interrupt_bind_funcid
#define sdei_interrupt_bind_funcid 0
#endif
#ifndef sdei_interrupt_release_funcid
#define sdei_interrupt_release_funcid 0
#endif
#ifndef sdei_features_funcid
#define sdei_features_funcid 0
#endif
#ifndef sdei_signal_hang_funcid
#define sdei_signal_hang_funcid 0
#endif
#ifndef experiment_funcid
#define experiment_funcid 0
#endif
#ifndef repeat_interrupt_bind_funcid
#define repeat_interrupt_bind_funcid 0
#endif
#ifndef sdei_event_get_info_coverage_funcid
#define sdei_event_get_info_coverage_funcid 0
#endif
#ifndef sdei_routing_set_coverage_funcid
#define sdei_routing_set_coverage_funcid 0
#endif
#ifndef test_funcid
#define test_funcid 0
#endif


int64_t tftf_test_sdei_noarg(int64_t (*sdei_func)(void), char *funcstr);
void tftf_test_sdei_singlearg(int64_t (*sdei_func)(uint64_t), char *funcstr);
void run_sdei_fuzz(int funcid, struct memmod *mmod, bool inrange, int cntid);
char *return_str(int64_t ret);
void print_ret(char *funcstr, int64_t ret);
