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
	if (custom_serror_handler == NULL) {
		return false;
	}

	return custom_serror_handler();
}
