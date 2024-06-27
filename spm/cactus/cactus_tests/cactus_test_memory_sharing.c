/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sp_def.h>
#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include <debug.h>
#include <ffa_helpers.h>
#include <sp_helpers.h>
#include "sp_tests.h"
#include "spm_common.h"
#include "stdint.h"
#include <xlat_tables_defs.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <sync.h>

static volatile uint32_t data_abort_gpf_triggered;

static bool data_abort_gpf_handler(void)
{
	uint64_t esr_el1 = read_esr_el1();

	VERBOSE("%s count %u esr_el1 %llx elr_el1 %lx\n",
		__func__, data_abort_gpf_triggered, esr_el1,
		read_elr_el1());

	/* Expect a data abort because of a GPF. */
	if ((EC_BITS(esr_el1) == EC_DABORT_CUR_EL) &&
	    ((ISS_BITS(esr_el1) & ISS_DFSC_MASK) == DFSC_GPF_DABORT)) {
		data_abort_gpf_triggered++;
		return true;
	}

	return false;
}

/**
 * Each Cactus SP has a memory region dedicated to memory sharing tests
 * described in their partition manifest.
 * This function returns the expected base address depending on the
 * SP ID (should be the same as the manifest).
 */
static void *share_page(ffa_id_t cactus_sp_id)
{
	switch (cactus_sp_id) {
	case SP_ID(1):
		return (void *)CACTUS_SP1_MEM_SHARE_BASE;
	case SP_ID(2):
		return (void *)CACTUS_SP2_MEM_SHARE_BASE;
	case SP_ID(3):
		return (void *)CACTUS_SP3_MEM_SHARE_BASE;
	default:
		ERROR("Helper function expecting a valid Cactus SP ID!\n");
		panic();
	}
}

static void *share_page_non_secure(ffa_id_t cactus_sp_id)
{
	if (cactus_sp_id != SP_ID(3)) {
		ERROR("Helper function expecting a valid Cactus SP ID!\n");
		panic();
	}

	return (void *)CACTUS_SP3_NS_MEM_SHARE_BASE;
}

static bool cactus_mem_unmap_and_relinquish(
		struct ffa_composite_memory_region *composite,
		void *send, ffa_memory_handle_t handle, ffa_id_t vm_id)
{
	int ret;

	for (uint32_t i = 0; i < composite->constituent_count; i++) {
		uint64_t base_address = (uint64_t)composite->constituents[i]
								.address;
		size_t size = composite->constituents[i].page_count * PAGE_SIZE;

		ret = mmap_remove_dynamic_region(
			(uint64_t)composite->constituents[i].address,
			composite->constituents[i].page_count * PAGE_SIZE);

		if (ret != 0) {
			ERROR("Failed to unmap received memory region %llx "
			      "size: %lu (error:%d)\n",
			      base_address, size, ret);
			return false;
		}
	}

	if (!memory_relinquish((struct ffa_mem_relinquish *)send,
				handle, vm_id)) {
		return false;
	}

	return true;
}

CACTUS_CMD_HANDLER(mem_send_cmd, CACTUS_MEM_SEND_CMD)
{
	struct ffa_memory_region *m;
	struct ffa_composite_memory_region *composite;
	int ret = -1;
	unsigned int mem_attrs;
	uint32_t *ptr;
	ffa_id_t source = ffa_dir_msg_source(*args);
	ffa_id_t vm_id = ffa_dir_msg_dest(*args);
	uint32_t mem_func = cactus_req_mem_send_get_mem_func(*args);
	uint64_t handle = cactus_mem_send_get_handle(*args);
	ffa_memory_region_flags_t retrv_flags =
					 cactus_mem_send_get_retrv_flags(*args);
	uint32_t words_to_write = cactus_mem_send_words_to_write(*args);
	bool expect_exception = cactus_mem_send_expect_exception(*args);

	struct ffa_memory_access receiver = ffa_memory_access_init(
		vm_id, FFA_DATA_ACCESS_RW,
		(mem_func == FFA_MEM_SHARE_SMC64)
			? FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED
			: FFA_INSTRUCTION_ACCESS_NX,
		0, NULL);

	EXPECT(memory_retrieve(mb, &m, handle, source, &receiver, 1,
			       retrv_flags),
	       true);

	composite = ffa_memory_region_get_composite(m, 0);

	/* This test is only concerned with RW permissions. */
	if (m->receivers[0].receiver_permissions.permissions.data_access !=
	    FFA_DATA_ACCESS_RW) {
		ERROR("Permissions not expected!\n");
		return cactus_error_resp(vm_id, source, CACTUS_ERROR_TEST);
	}

	mem_attrs = MT_RW_DATA | MT_EXECUTE_NEVER;

	if (m->attributes.security == FFA_MEMORY_SECURITY_NON_SECURE) {
		mem_attrs |= MT_NS;
	}

	for (uint32_t i = 0; i < composite->constituent_count; i++) {
		uint64_t base_address = (uint64_t)composite->constituents[i]
								.address;
		size_t size = composite->constituents[i].page_count * PAGE_SIZE;

		ret = mmap_add_dynamic_region(
			base_address, base_address, size, mem_attrs);

		if (ret != 0) {
			ERROR("Failed to map received memory region %llx "
			      "size: %lu (error:%d)\n",
			      base_address, size, ret);
			return cactus_error_resp(vm_id,
						 source,
						 CACTUS_ERROR_TEST);
		}
	}

	VERBOSE("Memory has been mapped\n");

	for (uint32_t i = 0; i < composite->constituent_count; i++) {
		ptr = (uint32_t *) composite->constituents[i].address;

		for (uint32_t j = 0; j < words_to_write; j++) {

			/**
			 * Check that memory has been cleared by the SPMC
			 * before using it.
			 */
			if ((retrv_flags & FFA_MEMORY_REGION_FLAG_CLEAR) != 0U) {
				VERBOSE("Check if memory has been cleared.\n");
				if (ptr[j] != 0) {
					/*
					 * If it hasn't been cleared, shouldn't
					 * be used.
					 */
					ERROR("Memory NOT cleared!\n");
					cactus_mem_unmap_and_relinquish(composite,
							      mb->send,
							      handle, vm_id);
					ffa_rx_release();
					return cactus_error_resp(
						vm_id, source,
						CACTUS_ERROR_TEST);
				}
			} else {
				/*
				 * In RME enabled systems, the memory is expected
				 * to be scrubbed on PAS updates from S to NS.
				 * As well, it is likely that the memory
				 * addresses are shadowed, and the contents are
				 * not visible accross updates from the
				 * different address spaces. As such, the SP
				 * shall not rely on memory content to be
				 * in any form. FFA_MEM_LEND/FFA_MEM_DONATE are
				 * thus considered for memory allocation
				 * purposes.
				 *
				 * Expect valid data if:
				 * - Operation between SPs.
				 * - Memory sharing from NWd to SP.
				 */
				if ((mem_func != FFA_MEM_SHARE_SMC64 &&
				    !IS_SP_ID(m->sender)) ||
				    expect_exception) {
					continue;
				}

				VERBOSE("Check memory contents. Expect %u "
					"words of %x\n", words_to_write,
					mem_func + 0xFFA);

				/* SPs writing `mem_func` + 0xFFA. */
				if (ptr[i] != mem_func + 0xFFA) {
					ERROR("Memory content NOT as expected!\n");
					cactus_mem_unmap_and_relinquish(
						composite, mb->send, handle,
						vm_id);
					ffa_rx_release();
					return cactus_error_resp(
						vm_id, source,
						CACTUS_ERROR_TEST);
				}
			}
		}
	}

	data_abort_gpf_triggered = 0;
	register_custom_sync_exception_handler(data_abort_gpf_handler);

	/* Write mem_func to retrieved memory region for validation purposes. */
	for (uint32_t i = 0; i < composite->constituent_count; i++) {
		ptr = (uint32_t *) composite->constituents[i].address;
		for (uint32_t j = 0; j < words_to_write; j++) {
			ptr[j] = mem_func + 0xFFA;
		}
	}

	unregister_custom_sync_exception_handler();

	/*
	 * A FFA_MEM_DONATE changes the ownership of the page, as such no
	 * relinquish is needed.
	 */
	if (mem_func != FFA_MEM_DONATE_SMC64 &&
	    !cactus_mem_unmap_and_relinquish(composite, mb->send, handle,
					     vm_id)) {
		return cactus_error_resp(vm_id, source,
					 CACTUS_ERROR_TEST);
	}

	if (ffa_func_id(ffa_rx_release()) != FFA_SUCCESS_SMC32) {
		ERROR("Failed to release buffer!\n");
		return cactus_error_resp(vm_id, source,
					 CACTUS_ERROR_FFA_CALL);
	}

	return cactus_success_resp(vm_id,
				   source, data_abort_gpf_triggered);
}

CACTUS_CMD_HANDLER(req_mem_send_cmd, CACTUS_REQ_MEM_SEND_CMD)
{
	struct ffa_value ffa_ret;
	uint32_t mem_func = cactus_req_mem_send_get_mem_func(*args);
	ffa_id_t receiver_id = cactus_req_mem_send_get_receiver(*args);
	ffa_memory_handle_t handle;
	ffa_id_t vm_id = ffa_dir_msg_dest(*args);
	ffa_id_t source = ffa_dir_msg_source(*args);
	uint32_t *ptr;
	bool non_secure = cactus_req_mem_send_get_non_secure(*args);
	void *share_page_addr =
		non_secure ? share_page_non_secure(vm_id) : share_page(vm_id);
	unsigned int mem_attrs;
	int ret;
	const uint32_t words_to_write = 10;
	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(receiver_id,
								 mem_func);

	VERBOSE("%x requested to send memory to %x (func: %x), page: %llx\n",
		source, receiver_id, mem_func, (uint64_t)share_page_addr);

	const struct ffa_memory_region_constituent constituents[] = {
		{share_page_addr, 1, 0}
	};

	const uint32_t constituents_count = (sizeof(constituents) /
					     sizeof(constituents[0]));

	VERBOSE("Sharing at 0x%llx\n", (uint64_t)constituents[0].address);
	mem_attrs = MT_RW_DATA;

	if (non_secure) {
		mem_attrs |= MT_NS;
	}

	ret = mmap_add_dynamic_region(
		(uint64_t)constituents[0].address,
		(uint64_t)constituents[0].address,
		constituents[0].page_count * PAGE_SIZE,
		mem_attrs);

	if (ret != 0) {
		ERROR("Failed map share memory before sending (%d)!\n",
		      ret);
		return cactus_error_resp(vm_id, source,
					 CACTUS_ERROR_TEST);
	}

	/* Write to memory before sharing to SP. */
	if (IS_SP_ID(receiver_id)) {
		for (size_t i = 0; i < constituents_count; i++) {
			VERBOSE("Sharing Address: %p\n",
					constituents[i].address);
			ptr = (uint32_t *)constituents[i].address;
			for (size_t j = 0; j < words_to_write; j++) {
				ptr[j] = mem_func + 0xFFA;
			}
		}
	}

	handle = memory_init_and_send(mb->send, PAGE_SIZE, vm_id, &receiver, 1,
				       constituents, constituents_count,
				       mem_func, &ffa_ret);

	/*
	 * If returned an invalid handle, we should break the test.
	 */
	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		VERBOSE("Received an invalid FF-A memory Handle!\n");
		return cactus_error_resp(vm_id, source,
					 ffa_error_code(ffa_ret));
	}

	ffa_ret = cactus_mem_send_cmd(vm_id, receiver_id, mem_func, handle,
				      0, 10, false);

	if (!is_ffa_direct_response(ffa_ret)) {
		return cactus_error_resp(vm_id, source, CACTUS_ERROR_FFA_CALL);
	}

	/* If anything went bad on the receiver's end. */
	if (cactus_get_response(ffa_ret) == CACTUS_ERROR) {
		ERROR("Received error from receiver!\n");
		return cactus_error_resp(vm_id, source, CACTUS_ERROR_TEST);
	}

	if (mem_func != FFA_MEM_DONATE_SMC64) {
		/*
		 * Do a memory reclaim only if the mem_func regards to memory
		 * share or lend operations, as with a donate the owner is
		 * permanently given up access to the memory region.
		 */
		ffa_ret = ffa_mem_reclaim(handle, 0);
		if (is_ffa_call_error(ffa_ret)) {
			return cactus_error_resp(vm_id, source,
						 CACTUS_ERROR_TEST);
		}

		/**
		 * Read Content that has been written to memory to validate
		 * access to memory segment has been reestablished, and receiver
		 * made use of memory region.
		 */
		#if (LOG_LEVEL >= LOG_LEVEL_VERBOSE)
			uint32_t *ptr = (uint32_t *)constituents->address;

			VERBOSE("Memory contents after receiver SP's use:\n");
			for (unsigned int i = 0U; i < 5U; i++) {
				VERBOSE("      %u: %x\n", i, ptr[i]);
			}
		#endif
	}

	/* Always unmap the sent memory region, will be remapped by another
	 * test if needed. */
	ret = mmap_remove_dynamic_region(
		(uint64_t)constituents[0].address,
		constituents[0].page_count * PAGE_SIZE);

	if (ret != 0) {
		ERROR("Failed to unmap share memory region (%d)!\n", ret);
		return cactus_error_resp(vm_id, source,
					 CACTUS_ERROR_TEST);
	}

	return cactus_success_resp(vm_id, source, 0);
}
