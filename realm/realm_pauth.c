/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <arch_features.h>
#include <debug.h>
#include <pauth.h>
#include <realm_rsi.h>
#include <sync.h>

static volatile bool set_cmd_done;

static bool exception_handler(void)
{
	u_register_t lr = read_elr_el1();

	/* Disable PAuth to avoid further PAuth faults. */
	pauth_disable();

	/* Check for PAuth exception. */
	/* Note- PAuth decode instruction clobbers PAC Fields[63:56] in case of error. */
	if (lr & (0xFFULL << 56U)) {
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	}

	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);

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
	if (!is_armv8_3_pauth_present()) {
		return false;
	}
	pauth_test_lib_test_intrs();
	pauth_test_lib_fill_regs_and_template();
	set_cmd_done = true;
	return true;
}

bool test_realm_pauth_check_cmd(void)
{
	if (!is_armv8_3_pauth_present() || !set_cmd_done) {
		return false;
	}
	return pauth_test_lib_compare_template();
}
