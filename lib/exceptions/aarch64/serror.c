/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdbool.h>

#include <arch_helpers.h>
#include <debug.h>
#include <serror.h>

static exception_handler_t custom_serror_handler;

void register_custom_serror_handler(exception_handler_t handler)
{
	custom_serror_handler = handler;
}

void unregister_custom_serror_handler(void)
{
	custom_serror_handler = NULL;
}

bool tftf_serror_handler(void)
{
	uint64_t elr_elx = IS_IN_EL2() ? read_elr_el2() : read_elr_el1();
	bool resume = false;

	if (custom_serror_handler == NULL) {
		return false;
	}

	resume = custom_serror_handler();

	/*
	 * TODO: if there is a test exepecting an Aync EA and expects to resume,
	 * then there needs to be additional info from test handler as to whether
	 * elr can be incremented or not.
	 */
	if (resume) {
		/* Move ELR to next instruction to allow tftf to continue */
		if (IS_IN_EL2()) {
			write_elr_el2(elr_elx + 4U);
		} else {
			write_elr_el1(elr_elx + 4U);
		}
	}

	return resume;
}
