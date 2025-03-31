/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <sync.h>
#include <realm_helpers.h>
#include <utils_def.h>

/* Check if Realm gets undefined abort when it access MPAM registers */
bool test_realm_mpam_undef_abort(void)
{
	void (*write_reg[])(u_register_t) = {write_mpam0_el1,
					     write_mpam1_el1,
					     write_mpamsm_el1,
					     write_mpamidr_el1};
	unsigned int n_access = ARRAY_SIZE(write_reg);

	realm_reset_undef_abort_count();

	/* Install exception handler to catch undefined abort */
	register_custom_sync_exception_handler(realm_sync_exception_handler);

	for (unsigned int i = 0U; i < n_access; i++) {
		write_reg[i](0UL);
	}

	unregister_custom_sync_exception_handler();

	return (realm_get_undef_abort_count() == (unsigned long)n_access);
}
