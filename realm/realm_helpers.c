/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

/* Generate 64-bit random number */
unsigned long long realm_rand64(void)
{
	return ((unsigned long long)rand() << 32) | rand();
}

