/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <arch_helpers.h>
#include <host_shared_data.h>
#include <realm_rsi.h>

/*
 * A printf formatted function used in the Realm world to log messages
 * in the shared buffer.
 * Locate the shared logging buffer and print its content
 */
void realm_printf(const char *fmt, ...)
{
	host_shared_data_t *guest_shared_data = realm_get_my_shared_structure();
	char *log_buffer = (char *)guest_shared_data->log_buffer;
	va_list args;

	va_start(args, fmt);
	if (strnlen((const char *)log_buffer, MAX_BUF_SIZE) == MAX_BUF_SIZE) {
		(void)memset((char *)log_buffer, 0, MAX_BUF_SIZE);
	}
	(void)vsnprintf((char *)log_buffer +
			strnlen((const char *)log_buffer, MAX_BUF_SIZE),
			MAX_BUF_SIZE, fmt, args);
	va_end(args);
	rsi_exit_to_host(HOST_CALL_EXIT_PRINT_CMD);
}

void __attribute__((__noreturn__)) do_panic(const char *file, int line)
{
	realm_printf("PANIC in file: %s line: %d\n", file, line);
	while (true) {
		continue;
	}
}

/* This is used from printf() when crash dump is reached */
int console_putc(int c)
{
	host_shared_data_t *guest_shared_data = realm_get_my_shared_structure();
	char *log_buffer = (char *)guest_shared_data->log_buffer;

	if ((c < 0) || (c > 127)) {
		return -1;
	}
	if (strnlen((const char *)log_buffer, MAX_BUF_SIZE) == MAX_BUF_SIZE) {
		(void)memset((char *)log_buffer, 0, MAX_BUF_SIZE);
	}
	*((char *)log_buffer + strnlen((const char *)log_buffer, MAX_BUF_SIZE)) = c;

	return c;
}
