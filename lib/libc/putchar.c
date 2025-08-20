/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include <drivers/console.h>

int putchar(int c)
{
	int res;
#ifdef SMC_FUZZ_VARIABLE_COVERAGE
	if(tftf_get_console_state() == CONSOLE_FLAG_SMC_FUZZER) {
		if (console_putc_fuzzer((unsigned char)c) >= 0)
			res = c;
		else
			res = EOF;
	}
	else {
		if (console_putc((unsigned char)c) >= 0)
			res = c;
		else
			res = EOF;
	}
#else
	if (console_putc((unsigned char)c) >= 0)
		res = c;
	else
		res = EOF;
#endif
	return res;
}
