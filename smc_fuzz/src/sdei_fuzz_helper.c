/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <arg_struct_def.h>
#include "constraint.h"
#include <fuzz_names.h>
#include <sdei_fuzz_helper.h>

int stbev = 0;

#define MIN_PPI_ID		U(16)
#define MAX_PPI_ID		U(31)
#define MIN_SPI_ID		U(32)
#define MAX_SPI_ID		U(255)
#define EV_COOKIE 0xDEADBEEF
#define PPI_RANGE ((uint64_t[2]) {MIN_PPI_ID, MAX_PPI_ID})
#define SPI_RANGE ((uint64_t[2]) {MIN_SPI_ID, MAX_SPI_ID})
#define BIND_SLOTS_MASK			0xffffU
#define FEATURES_SHARED_SLOTS_SHIFT	16U
#define FEATURES_PRIVATE_SLOTS_SHIFT	0U

extern sdei_handler_t sdei_entrypoint;
extern sdei_handler_t sdei_entrypoint_resume;

#define HANDLER_SVALUE ((uint64_t[1]) { (uint64_t)(uintptr_t)sdei_entrypoint_resume })
#define PE_SVALUE ((uint64_t[1]) { read_mpidr_el1() })
#define ZERO_EVENT_SVALUE ((uint64_t[1]) { 0 })
#define EV_COOKIE_SVALUE ((uint64_t[1]) { 0xDEADBEEF })
#define VENDOR_EVENTS_RANGE ((uint64_t[2]) { 0x40000000, 0x40FFFFFF })
#define STD_EVENTS_RANGE ((uint64_t[2]) { 0x00000001, 0x00FFFFFF })
#define ANY_ROUTING ((uint64_t[1]) { SDEI_REGF_RM_ANY })
#define PE_ROUTING ((uint64_t[1]) { SDEI_REGF_RM_PE })
#define ROUTING_MODES ((uint64_t[2]) { SDEI_REGF_RM_ANY, SDEI_REGF_RM_PE })
#define OUT_OF_RESOURCE_ERR -10
#define MAX_BIND_SLOTS (1 << 16)

#define register_handler() setconstraint(FUZZER_CONSTRAINT_SVALUE, HANDLER_SVALUE, 1, \
SDEI_EVENT_REGISTER_CALL_ARG2_ADDR, mmod, FUZZER_CONSTRAINT_ACCMODE)

char *return_str(int64_t return_val)
{
	switch (return_val) {
	case -1:
		return "NOT_SUPPORTED";
	case -2:
		return "INVALID_PARAMETERS";
	case -3:
		return "DENIED";
	case -5:
		return "PENDING";
	case -10:
		return "OUT_OF_RESOURCE";
	default:
		return "UNKNOWN ERROR CODE";
	}
}

void print_return(char *funcstr, int64_t ret)
{
	if (ret < 0) {
		printf("%s failed: 0x%llx. %s\n", funcstr, ret, return_str(ret));
	} else {
		printf("%s successful return: %llx\n", funcstr, ret);
	}
}

void set_event_constraints(int sdei_arg_name, struct memmod *mmod)
{
	setconstraint(FUZZER_CONSTRAINT_SVALUE, ZERO_EVENT_SVALUE, 1, sdei_arg_name, mmod, FUZZER_CONSTRAINT_ACCMODE);
	setconstraint(FUZZER_CONSTRAINT_SVALUE, STD_EVENTS_RANGE, 2, sdei_arg_name, mmod, FUZZER_CONSTRAINT_ACCMODE);
	setconstraint(FUZZER_CONSTRAINT_SVALUE, VENDOR_EVENTS_RANGE, 2, sdei_arg_name, mmod, FUZZER_CONSTRAINT_ACCMODE);
}

/*
 * SDEI function that has no arguments
 */
int64_t tftf_test_sdei_noarg(int64_t (*sdei_func)(void), char *funcstr)
{
		int64_t ret = (*sdei_func)();

		if (ret < 0) {
			tftf_testcase_printf("%s failed: 0x%llx\n", funcstr, ret);
		}

		printf("%s return: %llx\n", funcstr, ret);

		return ret;
}

/*
 * SDEI function that has single argument
 */
void tftf_test_sdei_singlearg(int64_t (*sdei_func)(uint64_t), char *funcstr)
{
		struct sdei_intr_ctx intr_ctx;
		int bev;

		bev = sdei_interrupt_bind(tftf_get_timer_irq(), &intr_ctx);
		int64_t ret = (*sdei_func)(bev);

		if (ret < 0) {
			tftf_testcase_printf("%s failed: 0x%llx\n", funcstr, ret);
		}
}

uint64_t *bound_shared_inums;
uint64_t *bound_private_inums;
uint64_t *bound_shared_evnums;
uint64_t *bound_private_evnums;
int64_t private_slots_count;
int64_t shared_slots_count;
static int64_t private_slots_len;
static int64_t shared_slots_len;


void bound_event_constraints(int fieldname, struct memmod *mmod)
{
	setconstraint(FUZZER_CONSTRAINT_VECTOR, bound_shared_evnums, shared_slots_len, fieldname, mmod, FUZZER_CONSTRAINT_ACCMODE);
	setconstraint(FUZZER_CONSTRAINT_VECTOR, bound_private_evnums, private_slots_len, fieldname, mmod, FUZZER_CONSTRAINT_ACCMODE);
}

void release_shared_slots(struct memmod *mmod, int slots, bool release)
{

	if ((slots < 0) || (slots > shared_slots_len)) {
		return;
	}

	struct inputparameters inp;
	int64_t ret;
	struct sdei_intr_ctx intr_ctx;

	if (release) {
		for (int k = 0; k < slots; k++) {
			uint64_t release_enum[1] = {bound_shared_evnums[shared_slots_len - 1 - k]};


			setconstraint(FUZZER_CONSTRAINT_SVALUE, release_enum, 1, SDEI_EVENT_UNREGISTER_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
			inp = generate_args(SDEI_EVENT_UNREGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);
			ret = sdei_event_unregister(inp.x1);
			print_return("unregister", ret);


			setconstraint(FUZZER_CONSTRAINT_SVALUE, release_enum, 1, SDEI_INTERRUPT_RELEASE_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
			inp = generate_args(SDEI_INTERRUPT_RELEASE_CALL, SANITY_LEVEL_3);
			ret = sdei_interrupt_release(inp.x1, &intr_ctx);
			print_return("release", ret);
		}
	}

	for (int i = shared_slots_len; i > 0; i--) {
		bound_shared_inums[i] = bound_shared_inums[i-slots];
	}
	for (int i = 0; i < slots; i++) {
		bound_shared_inums[i] = 0;
	}

	for (int i = shared_slots_len; i > 0; i--) {
		bound_shared_evnums[i] = bound_shared_evnums[i-slots];
	}
	for (int i = 0; i < slots; i++) {
		bound_shared_evnums[i] = 0;
	}
	shared_slots_count = (shared_slots_count + slots < shared_slots_len ? shared_slots_count + slots : shared_slots_len);
}

void release_private_slots(struct memmod *mmod, int slots, bool release)
{
	if ((slots < 0) || (slots > private_slots_len)) {
		return;
	}

	struct inputparameters inp;
	int64_t ret;
	struct sdei_intr_ctx intr_ctx;

	if (release) {
		for (int k = 0; k < slots; k++) {
			uint64_t release_enum[1] = {bound_private_evnums[private_slots_len-1-k]};

			setconstraint(FUZZER_CONSTRAINT_SVALUE, release_enum, 1, SDEI_EVENT_UNREGISTER_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
			inp = generate_args(SDEI_EVENT_UNREGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);
			ret = sdei_event_unregister(inp.x1);
			print_return("unregister", ret);

			setconstraint(FUZZER_CONSTRAINT_SVALUE, release_enum, 1, SDEI_INTERRUPT_RELEASE_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
			inp = generate_args(SDEI_INTERRUPT_RELEASE_CALL, SANITY_LEVEL_3);
			ret = sdei_interrupt_release(inp.x1, &intr_ctx);
			print_return("release", ret);
		}
	}

	for (int i = private_slots_len; i > 0; i--) {
		bound_private_inums[i] = bound_private_inums[i-slots];
	}
	for (int i = 0; i < slots; i++) {
		bound_private_inums[i] = 0;
	}

	for (int i = private_slots_len; i > 0; i--) {
		bound_private_evnums[i] = bound_private_evnums[i-slots];
	}
	for (int i = 0; i < slots; i++) {
		bound_private_evnums[i] = 0;
	}

	private_slots_count = (private_slots_count + slots < private_slots_len ? private_slots_count + slots : private_slots_len);
}

void initalize_interrupt_slots(struct memmod *mmod)
{
	uint64_t bind_slots[1] = {0};

	setconstraint(FUZZER_CONSTRAINT_SVALUE, bind_slots, 1, SDEI_FEATURES_CALL_ARG1_FEAT, mmod, FUZZER_CONSTRAINT_ACCMODE);
	struct inputparameters inp = generate_args(SDEI_FEATURES_CALL, SMC_FUZZ_SANITY_LEVEL);
	int64_t slots = sdei_features(inp.x1);

	shared_slots_count = slots & 0xffffU;
	private_slots_count = (slots >> 16U) & 0xfffU;
	shared_slots_len = shared_slots_count;
	private_slots_len = private_slots_count;
	static uint64_t tmp[MAX_BIND_SLOTS];
	static uint64_t tmp1[MAX_BIND_SLOTS];
	static uint64_t tmp2[MAX_BIND_SLOTS];
	static uint64_t tmp3[MAX_BIND_SLOTS];

	bound_shared_inums = tmp;
	bound_shared_evnums = tmp1;

	bound_private_inums = tmp2;
	bound_private_evnums = tmp3;
	for (int i = 0; i < shared_slots_count; i++) {
		bound_shared_inums[i] = 0;
		bound_shared_evnums[i] = 0;
	}
	for (int i = 0; i < private_slots_count; i++) {
		bound_private_inums[i] = 0;
		bound_private_evnums[i] = 0;
	}
}

void release_full_slots(struct inputparameters inp, struct memmod *mmod)
{
	if ((!shared_slots_count)) {
			if (inp.x1 >= MIN_SPI_ID && inp.x1 <= MAX_SPI_ID) {
				release_shared_slots(mmod, 1, true);
			}
		}

	if ((!private_slots_count)) {
		if (inp.x1 >= MIN_PPI_ID && inp.x1 <= MAX_PPI_ID) {
			release_private_slots(mmod, 1, true);
		}
	}
}

/*
 * SDEI function called from fuzzer
 */
void run_sdei_fuzz(int funcid, struct memmod *mmod, bool inrange, int cntid)
{
	#ifdef SMC_FUZZER_DEBUG
	if (inrange) {
		printf("%d\n", cntid);
	}
	#endif

	if (cntid == 0 && CONSTRAIN_EVENTS) {
		initalize_interrupt_slots(mmod);
	}

	#ifdef SMC_FUZZER_DEBUG
	if (CONSTRAIN_EVENTS) {
		printf("Bound priv inums: %llu, %llu, %llu\n", bound_private_inums[0], bound_private_inums[1], bound_private_inums[2]);
		printf("Bound priv evnums: %llu, %llu, %llu\n", bound_private_evnums[0], bound_private_evnums[1], bound_private_evnums[2]);
		printf("Bound shared inums: %llu, %llu, %llu\n", bound_shared_inums[0], bound_shared_inums[1], bound_shared_inums[2]);
		printf("Bound shared evnums: %llu, %llu, %llu\n", bound_shared_evnums[0], bound_shared_evnums[1], bound_shared_evnums[2]);
		printf("Shared slots left: %lld\n", shared_slots_count);
		printf("Private slots left: %lld\n\n", private_slots_count);
	}
	#endif
	if (funcid == sdei_version_funcid) {
		long long ret;

		if (inrange) {
			ret = sdei_version();

			if (ret != MAKE_SDEI_VERSION(1, 0, 0)) {
				tftf_testcase_printf("Unexpected SDEI version: 0x%llx\n", ret);
			}
		}
	} else if (funcid == sdei_pe_unmask_funcid) {
		if (inrange) {
			tftf_test_sdei_noarg(sdei_pe_unmask, "sdei_pe_unmask");
		}
	} else if (funcid == sdei_pe_mask_funcid) {
		if (inrange) {
			tftf_test_sdei_noarg(sdei_pe_mask, "sdei_pe_mask");
		}
	} else if (funcid == sdei_interrupt_bind_funcid) {
		struct sdei_intr_ctx intr_ctx;

		setconstraint(FUZZER_CONSTRAINT_RANGE, SPI_RANGE, 2,
		SDEI_INTERRUPT_BIND_CALL_ARG1_INUM, mmod, FUZZER_CONSTRAINT_ACCMODE);

		if (INTR_ASSERT) {
			setconstraint(FUZZER_CONSTRAINT_RANGE, PPI_RANGE, 2,
			SDEI_INTERRUPT_BIND_CALL_ARG1_INUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		}

		struct inputparameters inp = generate_args(SDEI_INTERRUPT_BIND_CALL, SANITY_LEVEL_3);

		release_full_slots(inp, mmod);

		if (inrange) {
			stbev = sdei_interrupt_bind(inp.x1, &intr_ctx);
			if (stbev < 0) {
				tftf_testcase_printf("sdei_interrupt_bind failed: 0x%llx %d\n", inp.x1, stbev);
			} else if (CONSTRAIN_EVENTS) {
				bool duplicate = false;

				if (inp.x1 >= MIN_SPI_ID && inp.x1 <= MAX_SPI_ID) {

					for (int i = 0; i < shared_slots_len; i++) {
						if (bound_shared_inums[i] == inp.x1) {
							duplicate = true;
						}
					}
					if (!duplicate) {
						shared_slots_count--;
						bound_shared_inums[shared_slots_count] = inp.x1;
						bound_shared_evnums[shared_slots_count] = stbev;
					}

				} else if (inp.x1 >= MIN_PPI_ID && inp.x1 <= MAX_PPI_ID) {
					for (int i = 0; i < private_slots_len; i++) {
						if (bound_private_inums[i] == inp.x1) {
							duplicate = true;
						}
					}
					if (!duplicate) {
						private_slots_count--;
						bound_private_inums[private_slots_count] = inp.x1;
						bound_private_evnums[private_slots_count] = stbev;
					}
				}
			}

			#ifdef SMC_FUZZER_DEBUG
			printf("stbev is %d and interrupt number is %lld\n", stbev, inp.x1);
			#endif
		}
	} else if (funcid == sdei_event_status_funcid) {
		struct inputparameters inp;

		if (CONSTRAIN_EVENTS) {
			bound_event_constraints(SDEI_EVENT_STATUS_CALL_ARG1_BEV, mmod);
			set_event_constraints(SDEI_EVENT_STATUS_CALL_ARG1_BEV, mmod);
		}
		inp = generate_args(SDEI_EVENT_STATUS_CALL, SMC_FUZZ_SANITY_LEVEL);
		int64_t ret;

		if (inrange) {
			ret = sdei_event_status(inp.x1);
			if (ret < 0) {

				tftf_testcase_printf("sdei_event_status failed: 0x%llx %d\n", ret, stbev);
			} else {

			}
		}

	} else if (funcid == sdei_event_signal_funcid) {

		setconstraint(FUZZER_CONSTRAINT_SVALUE, PE_SVALUE, 1,
		SDEI_EVENT_SIGNAL_CALL_ARG2_PE, mmod, FUZZER_CONSTRAINT_ACCMODE);
		struct inputparameters inp = generate_args(SDEI_EVENT_SIGNAL_CALL, SMC_FUZZ_SANITY_LEVEL);
		int64_t ret;

		if (inrange) {
			ret = sdei_event_signal(inp.x2);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_signal failed: %lld\n", ret);
			}
		}
	} else if (funcid == sdei_private_reset_funcid) {
		if (inrange) {
			tftf_test_sdei_noarg(sdei_private_reset, "sdei_private_reset");
		}
	} else if (funcid == sdei_shared_reset_funcid) {
		int64_t ret = -1;

		if (inrange) {
			ret = tftf_test_sdei_noarg(sdei_shared_reset, "sdei_shared_reset");
		}
		if (ret == 0) {
			release_shared_slots(mmod, private_slots_len, false);
			release_private_slots(mmod, shared_slots_len, false);
		}

	} else if (funcid == sdei_event_register_funcid) {

		if (CONSTRAIN_EVENTS) {
			set_event_constraints(SDEI_EVENT_REGISTER_CALL_ARG1_ENUM, mmod);
			bound_event_constraints(SDEI_EVENT_REGISTER_CALL_ARG1_ENUM, mmod);
		}

		register_handler();

		uint64_t routing_modes[2] = {SDEI_REGF_RM_ANY, SDEI_REGF_RM_PE};

		setconstraint(FUZZER_CONSTRAINT_RANGE, routing_modes, 2, SDEI_EVENT_REGISTER_CALL_ARG4_ROUTING, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, EV_COOKIE_SVALUE, 1, SDEI_EVENT_REGISTER_CALL_ARG3_ARG, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, PE_SVALUE, 1, SDEI_EVENT_REGISTER_CALL_ARG5_AFF, mmod, FUZZER_CONSTRAINT_ACCMODE);

		struct inputparameters inp = generate_args(SDEI_EVENT_REGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_register(inp.x1, (int (*)(int,  uint64_t))(uintptr_t)inp.x2, inp.x3, inp.x4, inp.x5);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_register failed: 0x%llx\n", ret);
			}
		}


	} else if (funcid == sdei_event_enable_funcid) {
		struct inputparameters inp;


		if (CONSTRAIN_EVENTS) {
			set_event_constraints(SDEI_EVENT_ENABLE_CALL_ARG1_ENUM, mmod);
			bound_event_constraints(SDEI_EVENT_ENABLE_CALL_ARG1_ENUM, mmod);
		}

		inp = generate_args(SDEI_EVENT_ENABLE_CALL, SMC_FUZZ_SANITY_LEVEL);
		int64_t ret;

		if (inrange) {
			ret = sdei_event_enable(inp.x1);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_enable failed: 0x%llx\n", ret);
			}
		}

	} else if (funcid == sdei_event_disable_funcid) {
		struct inputparameters inp;

		if (CONSTRAIN_EVENTS) {
			set_event_constraints(SDEI_EVENT_DISABLE_CALL_ARG1_ENUM, mmod);
			bound_event_constraints(SDEI_EVENT_DISABLE_CALL_ARG1_ENUM, mmod);
		}

		inp = generate_args(SDEI_EVENT_DISABLE_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_disable(inp.x1);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_disable failed: 0x%llx\n", ret);
			}
		}

	} else if (funcid == sdei_features_funcid) {
		uint64_t feature_values[2] = {0, 1};

		setconstraint(FUZZER_CONSTRAINT_RANGE, feature_values, 2, SDEI_FEATURES_CALL_ARG1_FEAT, mmod, FUZZER_CONSTRAINT_ACCMODE);
		struct inputparameters inp = generate_args(SDEI_FEATURES_CALL, SANITY_LEVEL_3);

		int64_t ret;

		if (inrange) {
			ret = sdei_features(inp.x1);
			if (ret < 0) {
				tftf_testcase_printf("sdei_features failed: 0x%llx\n", ret);
			} else if ((ret >> 32) == 0) {
				#ifdef SMC_FUZZER_DEBUG
				printf("SUCCESS: sdei_features expected [63:32]\n");
				printf("private event slots: %llx\n", (ret & 0xffffU));
				printf("shared event slots: %llx\n", ((ret >> 16U) & 0xfffU));
				#endif
			}
		}
	} else if (funcid == sdei_event_unregister_funcid) {
		struct inputparameters inp;

		if (CONSTRAIN_EVENTS) {
			set_event_constraints(SDEI_EVENT_UNREGISTER_CALL_ARG1_ENUM, mmod);
			bound_event_constraints(SDEI_EVENT_UNREGISTER_CALL_ARG1_ENUM, mmod);
		}

		inp = generate_args(SDEI_EVENT_UNREGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_unregister(inp.x1);

			if (ret < 0) {
				tftf_testcase_printf("sdei_event_unregister failed: 0x%llx\n", ret);
			}
		}

	} else if (funcid == sdei_event_context_funcid) {

		uint64_t register_range[2] = {0, 17};

		setconstraint(FUZZER_CONSTRAINT_RANGE, register_range, 2, SDEI_EVENT_CONTEXT_CALL_ARG1_PARAM, mmod, FUZZER_CONSTRAINT_ACCMODE);
		struct inputparameters inp = generate_args(SDEI_EVENT_CONTEXT_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_context(inp.x1);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_context failed: 0x%llx\n", ret);
			}
		}

	} else if (funcid == sdei_event_complete_funcid) {

		uint64_t status_codes[2] = {0, 1};

		setconstraint(FUZZER_CONSTRAINT_RANGE, status_codes, 2, SDEI_EVENT_COMPLETE_CALL_ARG1_STAT, mmod, FUZZER_CONSTRAINT_ACCMODE);

		struct inputparameters inp = generate_args(SDEI_EVENT_COMPLETE_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_complete(inp.x1);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_complete failed: 0x%llx\n", ret);
			}
		}

	} else if (funcid == sdei_event_complete_and_resume_funcid) {
		struct inputparameters inp = generate_args(SDEI_EVENT_COMPLETE_AND_RESUME_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_complete_and_resume(inp.x1);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_complete_and_resume failed: 0x%llx\n", ret);
			}
		}

	} else if (funcid == sdei_event_get_info_funcid) {
		struct inputparameters inp;
		uint64_t info_values[2] = {0, 4};

		setconstraint(FUZZER_CONSTRAINT_RANGE, info_values, 2, SDEI_EVENT_GET_INFO_CALL_ARG2_INFO, mmod, FUZZER_CONSTRAINT_ACCMODE);


		if (CONSTRAIN_EVENTS) {
			set_event_constraints(SDEI_EVENT_GET_INFO_CALL_ARG1_ENUM, mmod);
			bound_event_constraints(SDEI_EVENT_GET_INFO_CALL_ARG1_ENUM, mmod);
		}

		inp = generate_args(SDEI_EVENT_GET_INFO_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_get_info(inp.x1, inp.x2);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_get_info failed: 0x%llx\n", ret);
			}
		}
	} else if (funcid == sdei_event_routing_set_funcid) {
		uint64_t routing_modes[2] = {SDEI_REGF_RM_ANY, SDEI_REGF_RM_PE};

		setconstraint(FUZZER_CONSTRAINT_RANGE, routing_modes, 2, SDEI_EVENT_ROUTING_SET_CALL_ARG2_ROUTING, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, PE_SVALUE, 1, SDEI_EVENT_ROUTING_SET_CALL_ARG3_AFF, mmod, FUZZER_CONSTRAINT_ACCMODE);
		struct inputparameters inp;

		if (CONSTRAIN_EVENTS) {
			set_event_constraints(SDEI_EVENT_ROUTING_SET_CALL_ARG1_ENUM, mmod);
			bound_event_constraints(SDEI_EVENT_ROUTING_SET_CALL_ARG1_ENUM, mmod);
		}

		inp = generate_args(SDEI_EVENT_ROUTING_SET_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_event_routing_set(inp.x1, inp.x2);
			if (ret < 0) {
				tftf_testcase_printf("sdei_event_routing_set failed: 0x%llx\n", ret);
			}
		}

	} else if (funcid == sdei_interrupt_release_funcid) {
		struct sdei_intr_ctx intr_ctx;

		setconstraint(FUZZER_CONSTRAINT_RANGE, PPI_RANGE, 2,
		SDEI_INTERRUPT_RELEASE_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_RANGE, SPI_RANGE, 2,
		SDEI_INTERRUPT_RELEASE_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_ACCMODE);

		struct inputparameters inp = generate_args(SDEI_INTERRUPT_RELEASE_CALL, SMC_FUZZ_SANITY_LEVEL);

		int64_t ret;

		if (inrange) {
			ret = sdei_interrupt_release(inp.x1, &intr_ctx);
			if (ret < 0) {

				tftf_testcase_printf("sdei_interrupt_release failed: 0x%llx\n", ret);
			} else {
				if (inp.x1 >= MIN_SPI_ID && inp.x1 <= MAX_SPI_ID) {
					release_shared_slots(mmod, 1, false);
				} else if (inp.x1 >= MIN_PPI_ID && inp.x1 <= MAX_PPI_ID) {
					release_private_slots(mmod, 1, false);
				}
			}
		}

	} else if (funcid == sdei_routing_set_coverage_funcid) {
		int64_t ret;
		struct inputparameters inp;

		uint64_t bind_slots[1] = {0};

		setconstraint(FUZZER_CONSTRAINT_SVALUE, bind_slots, 1, SDEI_FEATURES_CALL_ARG1_FEAT, mmod, FUZZER_CONSTRAINT_ACCMODE);
		inp = generate_args(SDEI_FEATURES_CALL, SMC_FUZZ_SANITY_LEVEL);
		int64_t slots = sdei_features(inp.x1);

		print_return("features", slots);

		// bind shared interrupt to create shared event
		struct sdei_intr_ctx intr_ctx;
		uint64_t inum_range[2] = { MIN_SPI_ID, U(255) };

		setconstraint(FUZZER_CONSTRAINT_RANGE, inum_range, 2, SDEI_INTERRUPT_BIND_CALL_ARG1_INUM, mmod, FUZZER_CONSTRAINT_ACCMODE);
		inp = generate_args(SDEI_INTERRUPT_BIND_CALL, SANITY_LEVEL_3);

		release_full_slots(inp, mmod);


		ret = sdei_interrupt_bind(inp.x1, &intr_ctx);
		if (ret < 0) {
			return;
		}

		// register shared event
		uint64_t evnum[1] = {ret};

		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_EVENT_REGISTER_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, HANDLER_SVALUE, 1, SDEI_EVENT_REGISTER_CALL_ARG2_ADDR, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, PE_SVALUE, 1, SDEI_EVENT_REGISTER_CALL_ARG5_AFF, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, ANY_ROUTING, 1, SDEI_EVENT_REGISTER_CALL_ARG4_ROUTING, mmod, FUZZER_CONSTRAINT_ACCMODE);
		inp = generate_args(SDEI_EVENT_REGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_register(inp.x1, (int (*)(int,  uint64_t))(uintptr_t)inp.x2, inp.x3, inp.x4, inp.x5);
		print_return("register", ret);

		uint64_t signal_info[1] = {0};

		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_EVENT_GET_INFO_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, signal_info, 1, SDEI_EVENT_GET_INFO_CALL_ARG2_INFO, mmod, FUZZER_CONSTRAINT_ACCMODE);
		inp = generate_args(SDEI_EVENT_GET_INFO_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_get_info(inp.x1, inp.x2);
		print_return("get_info", ret);


		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_EVENT_ROUTING_SET_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, ANY_ROUTING, 1, SDEI_EVENT_ROUTING_SET_CALL_ARG2_ROUTING, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, PE_SVALUE, 1, SDEI_EVENT_ROUTING_SET_CALL_ARG3_AFF, mmod, FUZZER_CONSTRAINT_ACCMODE);
		inp = generate_args(SDEI_EVENT_ROUTING_SET_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_routing_set(inp.x1, inp.x2);
		print_return("routing_set", ret);

		// unregister
		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_EVENT_UNREGISTER_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_EVENT_UNREGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_unregister(inp.x1);
		print_return("unregister", ret);

		// release
		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_INTERRUPT_RELEASE_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_INTERRUPT_RELEASE_CALL, SMC_FUZZ_SANITY_LEVEL);
		sdei_interrupt_release(inp.x1, &intr_ctx);
		print_return("release", ret);

		if (inp.x1 >= MIN_SPI_ID && inp.x1 <= MAX_SPI_ID) {
			release_shared_slots(mmod, 1, false);
		} else if (inp.x1 >= MIN_PPI_ID && inp.x1 <= MAX_PPI_ID) {
			release_private_slots(mmod, 1, false);
		}

	} else if (funcid == sdei_event_get_info_coverage_funcid) {
		int64_t ret;
		struct inputparameters inp;
		uint64_t info[1];

		// bind shared interrupt to create shared event
		struct sdei_intr_ctx intr_ctx;
		uint64_t inum_range[2] = { MIN_SPI_ID, U(255)};

		setconstraint(FUZZER_CONSTRAINT_RANGE, inum_range, 2, SDEI_INTERRUPT_BIND_CALL_ARG1_INUM, mmod, FUZZER_CONSTRAINT_ACCMODE);
		inp = generate_args(SDEI_INTERRUPT_BIND_CALL, SANITY_LEVEL_3);

		release_full_slots(inp, mmod);

		ret = sdei_interrupt_bind(inp.x1, &intr_ctx);

		if (ret < 0) {
			return;
		}

		uint64_t evnum[1] = {ret};

		// event type
		info[0] = 0;
		setconstraint(FUZZER_CONSTRAINT_SVALUE, info, 1, SDEI_EVENT_GET_INFO_CALL_ARG2_INFO, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_EVENT_GET_INFO_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_EVENT_GET_INFO_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_get_info(inp.x1, inp.x2);
		print_return("get info", ret);

		// event signalable
		info[0] = 1;
		setconstraint(FUZZER_CONSTRAINT_SVALUE, info, 1, SDEI_EVENT_GET_INFO_CALL_ARG2_INFO, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_EVENT_GET_INFO_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_get_info(inp.x1, inp.x2);
		print_return("get info", ret);

		// event priority
		printf("priority\n");
		info[0] = 2;
		setconstraint(FUZZER_CONSTRAINT_SVALUE, info, 1, SDEI_EVENT_GET_INFO_CALL_ARG2_INFO, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_EVENT_GET_INFO_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_get_info(inp.x1, inp.x2);
		print_return("get info", ret);

		// register event
		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_EVENT_REGISTER_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, HANDLER_SVALUE, 1, SDEI_EVENT_REGISTER_CALL_ARG2_ADDR, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, PE_SVALUE, 1, SDEI_EVENT_REGISTER_CALL_ARG5_AFF, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_SVALUE, PE_ROUTING, 1, SDEI_EVENT_REGISTER_CALL_ARG4_ROUTING, mmod, FUZZER_CONSTRAINT_ACCMODE);
		inp = generate_args(SDEI_EVENT_REGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_register(inp.x1, (int (*)(int,  uint64_t))(uintptr_t)inp.x2, inp.x3, inp.x4, inp.x5);
		print_return("register", ret);

		// event routing mode
		info[0] = 3;
		setconstraint(FUZZER_CONSTRAINT_SVALUE, info, 1, SDEI_EVENT_GET_INFO_CALL_ARG2_INFO, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_EVENT_GET_INFO_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_get_info(inp.x1, inp.x2);
		print_return("get info", ret);

		// event affinity
		info[0] = 4;
		setconstraint(FUZZER_CONSTRAINT_SVALUE, info, 1, SDEI_EVENT_GET_INFO_CALL_ARG2_INFO, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_EVENT_GET_INFO_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_get_info(inp.x1, inp.x2);
		print_return("get info", ret);

		// unregister
		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_EVENT_UNREGISTER_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_EVENT_UNREGISTER_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_event_unregister(inp.x1);
		print_return("unregister", ret);

		// release
		setconstraint(FUZZER_CONSTRAINT_SVALUE, evnum, 1, SDEI_INTERRUPT_RELEASE_CALL_ARG1_ENUM, mmod, FUZZER_CONSTRAINT_EXCMODE);
		inp = generate_args(SDEI_INTERRUPT_RELEASE_CALL, SMC_FUZZ_SANITY_LEVEL);
		ret = sdei_interrupt_release(inp.x1, &intr_ctx);
		print_return("release", ret);

		if (inp.x1 >= MIN_SPI_ID && inp.x1 <= MAX_SPI_ID) {
			release_shared_slots(mmod, 1, false);
		} else if (inp.x1 >= MIN_PPI_ID && inp.x1 <= MAX_PPI_ID) {
			release_private_slots(mmod, 1, false);
		}
	}
}
