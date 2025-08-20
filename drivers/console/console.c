/*
 * Copyright (c) 2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/pl011.h>

int console_putc(int c)
{
	return console_pl011_putc(c);
}

#ifdef SMC_FUZZ_VARIABLE_COVERAGE
int console_putc_fuzzer(int c)
{
	return console_pl011_putc_fuzzer(c);
}

void tftf_switch_console_state(int state)
{
	tftf_console_state = state;
}

int tftf_get_console_state(void)
{
	return tftf_console_state;
}

#endif
