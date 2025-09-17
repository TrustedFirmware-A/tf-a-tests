/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <drivers/measured_boot/event_log/tcg.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <transfer_list.h>

#include "event_print.h"

extern u_register_t hw_config_base;
extern u_register_t ns_tl;
extern u_register_t tl_signature;

#define DTB_PREAMBLE U(0xedfe0dd0)

test_result_t test_handoff_header(void)
{
	struct transfer_list_header *tl = (struct transfer_list_header *)ns_tl;

#if __aarch64__
	uint64_t signature = TRANSFER_LIST_HANDOFF_X1_VALUE(TRANSFER_LIST_VERSION);
#else
	uint32_t signature = TRANSFER_LIST_HANDOFF_R1_VALUE(TRANSFER_LIST_VERSION);
#endif /* __aarch64__ */

	if (signature != tl_signature) {
		return TEST_RESULT_FAIL;
	}

	if (transfer_list_check_header(tl) == TL_OPS_NON) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_handoff_dtb_payload(void)
{
	tftf_testcase_printf("Validating HW_CONFIG from transfer list.\n");
	struct transfer_list_header *tl = (struct transfer_list_header *)ns_tl;
	struct transfer_list_entry *te = (void *)tl + tl->hdr_size;
	uintptr_t dtb_ptr;

	te = transfer_list_find(tl, TL_TAG_FDT);

	if (te == NULL) {
		tftf_testcase_printf(
			"Failed to find HW CONFIG TE in transfer list!");
		return TEST_RESULT_FAIL;
	}

	dtb_ptr = (unsigned long)transfer_list_entry_data(te);

	if ((dtb_ptr != hw_config_base) &&
	    (*(uint32_t *)dtb_ptr != DTB_PREAMBLE)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_handoff_event_payload(void)
{

	struct transfer_list_header *tl = (struct transfer_list_header *)ns_tl;
	struct transfer_list_entry *te;
	uint8_t *log_addr;
	size_t log_size;

	tftf_testcase_printf(
		"Validate that a TPM event log can be successfully dumped when "
		"present in the transfer list.");

	te = transfer_list_find(tl, TL_TAG_TPM_EVLOG);
	if (te == NULL) {
		return TEST_RESULT_SKIPPED;
	}

	/* 4-bytes are reserved in TE data section. */
	log_addr = (uint8_t *)transfer_list_entry_data(te) + U(4);
	log_size = te->data_size - U(4);

	if (event_log_dump(log_addr, log_size) != 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
