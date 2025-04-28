/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <transfer_list.h>

/*******************************************************************************
 * Search for an existing transfer entry with the specified tag id from a
 * transfer list
 * Return pointer to the found transfer entry or NULL on error
 ******************************************************************************/
struct transfer_list_entry *transfer_list_find(struct transfer_list_header *tl,
					       uint16_t tag_id)
{
	struct transfer_list_entry *te = NULL;

	do {
		te = transfer_list_next(tl, te);
	} while (te && (te->tag_id != tag_id));

	return te;
}

/*******************************************************************************
 * Enumerate the next transfer entry
 * Return pointer to the next transfer entry or NULL on error
 ******************************************************************************/
struct transfer_list_entry *transfer_list_next(struct transfer_list_header *tl,
					       struct transfer_list_entry *last)
{
	struct transfer_list_entry *te = NULL;
	uintptr_t tl_ev = 0;
	uintptr_t va = 0;
	uintptr_t ev = 0;
	size_t sz = 0;

	if (!tl) {
		return NULL;
	}

	tl_ev = (uintptr_t)tl + tl->size;

	if (last) {
		va = (uintptr_t)last;
		/* check if the total size overflow */
		if (add_overflow(last->hdr_size, last->data_size, &sz)) {
			return NULL;
		}
		/* roundup to the next entry */
		if (add_with_round_up_overflow(va, sz, TRANSFER_LIST_GRANULE,
					       &va)) {
			return NULL;
		}
	} else {
		va = (uintptr_t)tl + tl->hdr_size;
	}

	te = (struct transfer_list_entry *)va;

	if (va + sizeof(*te) > tl_ev || te->hdr_size < sizeof(*te) ||
	    add_overflow(te->hdr_size, te->data_size, &sz) ||
	    add_overflow(va, sz, &ev) || ev > tl_ev) {
		return NULL;
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
