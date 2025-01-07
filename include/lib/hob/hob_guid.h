/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOB_GUID_H
#define HOB_GUID_H

#include <stdint.h>
#include <lib/hob/efi_types.h>

/**
 * Guid used for creating StandaloneMm related information.
 */

#define MM_PEI_MMRAM_MEMORY_RESERVE_GUID                                        \
{                                                                               \
	0x0703f912, 0xbf8d, 0x4e2a, {0xbe, 0x07, 0xab, 0x27, 0x25, 0x25, 0xc5, 0x92 } \
}

#define MM_NS_BUFFER_GUID                                                       \
{                                                                               \
	0xf00497e3, 0xbfa2, 0x41a1, {0x9d, 0x29, 0x54, 0xc2, 0xe9, 0x37, 0x21, 0xc5 } \
}

struct mm_comm_buffer_desc {
	efi_physical_address_t physical_start;
	uint64_t number_of_pages;
};

#endif /* HOB_GUID_H */
