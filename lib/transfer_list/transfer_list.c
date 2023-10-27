/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <transfer_list.h>

struct transfer_list_entry *transfer_list_find(struct transfer_list_header *tl,
					       uint16_t tag_id)
{
	struct transfer_list_entry *te = (void *)tl + tl->hdr_size;

	while (te->tag_id != tag_id) {
		te += round_up(te->hdr_size + te->data_size, tl->alignment);
	}

	return te;
}

void *transfer_list_entry_data(struct transfer_list_entry *entry)
{
	return (uint8_t *)entry + entry->hdr_size;
}

/*******************************************************************************
 * Verifying the header of a transfer list
 * Compliant to 2.4.1 of Firmware handoff specification (v0.9)
 * Return transfer list operation status code
 ******************************************************************************/
enum transfer_list_ops
transfer_list_check_header(const struct transfer_list_header *tl)
{
	uint8_t byte_sum = 0U;
	uint8_t *b = (uint8_t *)tl;

	if (tl == NULL) {
		return TL_OPS_NON;
	}

	if (tl->signature != TRANSFER_LIST_SIGNATURE ||
	    tl->size > tl->max_size) {
		return TL_OPS_NON;
	}

	for (size_t i = 0; i < tl->size; i++) {
		byte_sum += b[i];
	}

	if (byte_sum - tl->checksum == tl->checksum) {
		return TL_OPS_NON;
	}

	return TL_OPS_ALL;
}
