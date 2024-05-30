/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <arch_features.h>
#include <assert.h>
#include <debug.h>
#include <pauth.h>
#include <realm_rsi.h>
#include <sync.h>

static volatile bool set_cmd_done[MAX_REC_COUNT];
static uint128_t pauth_keys_before[MAX_REC_COUNT][NUM_KEYS];
static uint128_t pauth_keys_after[MAX_REC_COUNT][NUM_KEYS];

static bool exception_handler(void)
{
	/* Disable PAuth to avoid further PAuth faults. */
	pauth_disable();

	rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);

	/* Does not return. */
	return false;
}

void dummy_func(void)
{
	realm_printf("shouldn't reach here.\n");
	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
}

bool test_realm_pauth_fault(void)
{
	u_register_t ptr = (u_register_t)dummy_func;

	if (!is_armv8_3_pauth_present()) {
		return false;
	}

	register_custom_sync_exception_handler(exception_handler);
	realm_printf("overwrite LR to generate fault.\n");
	__asm__("mov	x17, x30;	"
		"mov	x30, %0;	"	/* overwite LR. */
		"isb;			"
		"autiasp;		"
		"ret;			"	/* fault on return.  */
		:
		: "r"(ptr));

	/* Does not return. */
	return false;
}

/*
 * TF-A is expected to allow access to key registers from lower EL's,
 * reading the keys excercises this, on failure this will trap to
 * EL3 and crash.
 */
bool test_realm_pauth_set_cmd(void)
{
	unsigned int rec = read_mpidr_el1() & MPID_MASK;

	if (!is_armv8_3_pauth_present()) {
		return false;
	}
	assert(rec < MAX_REC_COUNT);
	pauth_test_lib_test_intrs();
	pauth_test_lib_fill_regs_and_template(pauth_keys_before[rec]);
	set_cmd_done[rec] = true;
	return true;
}

bool test_realm_pauth_check_cmd(void)
{
	unsigned int rec = read_mpidr_el1() & MPID_MASK;
	bool ret;

	assert(rec < MAX_REC_COUNT);
	if (!is_armv8_3_pauth_present() || !set_cmd_done[rec]) {
		return false;
	}
	ret = pauth_test_lib_compare_template(pauth_keys_before[rec], pauth_keys_after[rec]);
	realm_printf("Pauth key comparison ret=%d\n", ret);
	return ret;
}
