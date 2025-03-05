/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <lib/hob/hob.h>
#include <lib/hob/hob_guid.h>
#include <lib/hob/mmram.h>
#include <lib/utils/uuid_utils.h>
#include <lib/utils_def.h>

#define ALIGN_UP(x, a)		((x + (a - 1)) & ~(a - 1))

void dump_hob_generic_header(struct efi_hob_generic_header *h)
{
	assert(h != NULL);
	INFO("Hob Type: 0x%x\n", h->hob_type);
	INFO("Hob Length: 0x%x\n", h->hob_length);
}

void dump_efi_mmram_descriptor(struct efi_mmram_descriptor *m)
{
	INFO("        Physical start: 0x%llx\n", m->physical_start);
	INFO("        CPU start: 0x%llx\n", m->cpu_start);
	INFO("        Physical size: 0x%llx\n", m->physical_size);
	INFO("        Region state: 0x%llx\n", m->region_state);
}

void dump_efi_hob_firmware_volume(struct efi_hob_firmware_volume *fv)
{
	dump_hob_generic_header(&fv->header);
	INFO("    Base_address: 0x%llx\n", fv->base_address);
	INFO("    Length: 0x%llx\n", fv->length);
}

static void dump_efi_guid(struct efi_guid guid)
{
	INFO("    Time low: 0x%x\n", guid.time_low);
	INFO("    Time mid: 0x%x\n", guid.time_mid);
	INFO("    Time hi and version: 0x%x\n", guid.time_hi_and_version);
	INFO("    Clock_seq_and_node: [0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n",
		guid.clock_seq_and_node[0],
		guid.clock_seq_and_node[1],
		guid.clock_seq_and_node[2],
		guid.clock_seq_and_node[3],
		guid.clock_seq_and_node[4],
		guid.clock_seq_and_node[5],
		guid.clock_seq_and_node[6],
		guid.clock_seq_and_node[7]);
}

static void dump_guid_hob_data(struct efi_hob_guid_type *guid_hob)
{
	union uuid_helper_t uuid_name = {.efi_guid = guid_hob->name};
	union uuid_helper_t mmram_mem_resv_guid = {
		.efi_guid = (struct efi_guid)MM_PEI_MMRAM_MEMORY_RESERVE_GUID};
	union uuid_helper_t ns_buffer_guid = {
		.efi_guid = (struct efi_guid)MM_NS_BUFFER_GUID};
	uintptr_t guid_data = (uintptr_t)(&guid_hob->name + 1);

	/* Dump GUID HOB data according to GUID type. */
	if (uuid_equal(&uuid_name.uuid_struct,
		       &mmram_mem_resv_guid.uuid_struct)) {
		struct efi_mmram_hob_descriptor_block *mmram_desc_block =
			(struct efi_mmram_hob_descriptor_block *)guid_data;
		INFO("    MM_PEI_MMRAM_MEMORY_RESERVE_GUID with %u regions\n",
		     mmram_desc_block->number_of_mm_reserved_regions);
		for (uint32_t i = 0;
		     i < mmram_desc_block->number_of_mm_reserved_regions; i++) {
			INFO("    MMRAM_DESC[%u]:\n", i);
			dump_efi_mmram_descriptor(
				&mmram_desc_block->descriptor[i]);
		}
	} else if (uuid_equal(&uuid_name.uuid_struct,
			      &ns_buffer_guid.uuid_struct)) {
		INFO("    MM_NS_BUFFER_GUID\n");
		dump_efi_mmram_descriptor(
			(struct efi_mmram_descriptor *)guid_data);
	}
}

static void dump_guid_hob(struct efi_hob_guid_type *guid_hob)
{
	dump_hob_generic_header(&guid_hob->header);
	dump_efi_guid(guid_hob->name);
	dump_guid_hob_data(guid_hob);
}

void dump_hob_list(struct efi_hob_handoff_info_table *hob_list)
{
	uintptr_t next_hob_addr;
	struct efi_hob_generic_header *next;

	assert(hob_list != NULL);
	dump_hob_generic_header(&hob_list->header);
	INFO("    Version: %u\n", hob_list->version);
	INFO("    Boot Mode: %u\n", hob_list->boot_mode);
	INFO("    EFI Memory Top: 0x%llx\n", hob_list->efi_memory_top);
	INFO("    EFI Memory Bottom: 0x%llx\n", hob_list->efi_memory_bottom);
	INFO("    EFI Free Memory Top: 0x%llx\n", hob_list->efi_free_memory_top);
	INFO("    EFI Free Memory Bottom: 0x%llx\n", hob_list->efi_free_memory_bottom);
	INFO("    EFI End of Hob List: 0x%llx\n", hob_list->efi_end_of_hob_list);

	next_hob_addr = (uintptr_t)(hob_list) + (uintptr_t)hob_list->header.hob_length;
	assert(next_hob_addr < (uintptr_t)hob_list->efi_end_of_hob_list);
	next = (struct efi_hob_generic_header *)next_hob_addr;

	while (next != NULL) {
		assert(next->hob_type != 0x0);
		switch (next->hob_type) {
		case EFI_HOB_TYPE_GUID_EXTENSION:
			dump_guid_hob((struct efi_hob_guid_type *)next);
			break;
		case EFI_HOB_TYPE_FV:
			dump_efi_hob_firmware_volume((struct
					efi_hob_firmware_volume *)next);
			break;
		default:
			dump_hob_generic_header(next);
		}

		if (next->hob_type == EFI_HOB_TYPE_END_OF_HOB_LIST) {
			break;
		}

		next_hob_addr = (uintptr_t)(next) + (uintptr_t)next->hob_length;

		if (next_hob_addr >= (uintptr_t) hob_list->efi_end_of_hob_list) {
			next = NULL;
		} else {
			assert(next_hob_addr < (uintptr_t)hob_list->efi_end_of_hob_list);
			next = (struct efi_hob_generic_header *)next_hob_addr;
		}
	}

}
