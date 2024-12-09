/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdio.h>
#include <lib/hob/hob.h>
#include <lib/hob/hob_guid.h>
#include <lib/hob/mmram.h>
#include <lib/utils_def.h>

#define ALIGN_UP(x, a)		((x + (a - 1)) & ~(a - 1))

void tftf_dump_hob_generic_header(struct efi_hob_generic_header h)
{
	printf("Hob Type: 0x%x\n", h.hob_type);
	printf("Hob Length: 0x%x\n", h.hob_length);
}
