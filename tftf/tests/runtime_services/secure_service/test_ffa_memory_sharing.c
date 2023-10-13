/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include "ffa_helpers.h"

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <spm_common.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <xlat_tables_defs.h>

#define MAILBOX_SIZE PAGE_SIZE

#define SENDER HYP_ID
#define RECEIVER SP_ID(1)

/*
 * A number of pages that is large enough that it must take two fragments to
 * share.
 */
#define FRAGMENTED_SHARE_PAGE_COUNT                                            \
	(sizeof(struct ffa_memory_region) /                                    \
	 sizeof(struct ffa_memory_region_constituent))

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
	};

/* Memory section to be used for memory share operations */
static __aligned(PAGE_SIZE) uint8_t
	share_page[PAGE_SIZE * FRAGMENTED_SHARE_PAGE_COUNT];
static __aligned(PAGE_SIZE) uint8_t consecutive_donate_page[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t four_share_pages[PAGE_SIZE * 4];

static bool check_written_words(uint32_t *ptr, uint32_t word, uint32_t wcount)
{
	VERBOSE("TFTF - Memory contents after SP use:\n");
	for (unsigned int i = 0U; i < wcount; i++) {
		VERBOSE("      %u: %x\n", i, ptr[i]);

		/* Verify content of memory is as expected. */
		if (ptr[i] != word) {
			return false;
		}
	}
	return true;
}

static bool test_memory_send_expect_denied(uint32_t mem_func,
					   void *mem_ptr,
					   ffa_id_t borrower)
{
	struct ffa_value ret;
	struct mailbox_buffers mb;
	struct ffa_memory_region_constituent constituents[] = {
						{(void *)mem_ptr, 1, 0}
					};
	ffa_memory_handle_t handle;

	const uint32_t constituents_count = sizeof(constituents) /
			sizeof(struct ffa_memory_region_constituent);

	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(borrower,
								 mem_func);

	GET_TFTF_MAILBOX(mb);

	handle = memory_init_and_send((struct ffa_memory_region *)mb.send,
					MAILBOX_SIZE, SENDER, &receiver, 1,
					constituents, constituents_count,
					mem_func, &ret);

	if (handle != FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Received a valid FF-A memory handle, and that isn't "
		      "expected.\n");
		return false;
	}

	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED)) {
		return false;
	}

	return true;
}

/**
 * Test invocation to FF-A memory sharing interfaces that should return in an
 * error.
 */
test_result_t test_share_forbidden_ranges(void)
{
	const uintptr_t forbidden_address[] = {
		/* Cactus SP memory. */
		(uintptr_t)0x7200000,
		/* SPMC Memory. */
		(uintptr_t)0x6000000,
		/* NS memory defined in cactus tertiary. */
		(uintptr_t)0x0000880080001000,
	};

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	for (unsigned i = 0; i < 3; i++) {
		if (!test_memory_send_expect_denied(
			FFA_MEM_SHARE_SMC32, (void *)forbidden_address[i],
			RECEIVER)) {
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Tests that it is possible to share memory with SWd from NWd.
 * After calling the respective memory send API, it will expect a reply from
 * cactus SP, at which point it will reclaim access to the memory region and
 * check the memory region has been used by receiver SP.
 *
 * Accessing memory before a memory reclaim operation should only be possible
 * in the context of a memory share operation.
 * According to the FF-A spec, the owner is temporarily relinquishing
 * access to the memory region on a memory lend operation, and on a
 * memory donate operation the access is relinquished permanently.
 * SPMC is positioned in S-EL2, and doesn't control stage-1 mapping for
 * EL2. Therefore, it is impossible to enforce the expected access
 * policy for a donate and lend operations within the SPMC.
 * Current SPMC implementation is under the assumption of trust that
 * Hypervisor (sitting in EL2) would relinquish access from EL1/EL0
 * FF-A endpoint at relevant moment.
 */
static test_result_t test_memory_send_sp(uint32_t mem_func, ffa_id_t borrower,
					 struct ffa_memory_region_constituent *constituents,
					 size_t constituents_count)
{
	struct ffa_value ret;
	ffa_memory_handle_t handle;
	uint32_t *ptr;
	struct mailbox_buffers mb;

	/* Arbitrarily write 5 words after using memory. */
	const uint32_t nr_words_to_write = 5;

	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(borrower,
								 mem_func);

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	if (constituents_count != 1) {
		WARN("Test expects constituents_count to be 1\n");
	}

	for (size_t i = 0; i < constituents_count; i++) {
		VERBOSE("TFTF - Address: %p\n", constituents[0].address);
	}

	handle = memory_init_and_send((struct ffa_memory_region *)mb.send,
					MAILBOX_SIZE, SENDER, &receiver, 1,
					constituents, constituents_count,
					mem_func, &ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		return TEST_RESULT_FAIL;
	}

	VERBOSE("TFTF - Handle: %llx\n", handle);

	ptr = (uint32_t *)constituents[0].address;

	ret = cactus_mem_send_cmd(SENDER, borrower, mem_func, handle, 0,
				  nr_words_to_write);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		ERROR("Failed memory send operation!\n");
		return TEST_RESULT_FAIL;
	}

	if (mem_func != FFA_MEM_DONATE_SMC32) {

		/* Reclaim memory entirely before checking its state. */
		if (is_ffa_call_error(ffa_mem_reclaim(handle, 0))) {
			tftf_testcase_printf("Couldn't reclaim memory\n");
			return TEST_RESULT_FAIL;
		}

		/*
		 * Check that borrower used the memory as expected for this
		 * test.
		 */
		if (!check_written_words(ptr, mem_func, nr_words_to_write)) {
			ERROR("Fail because of state of memory.\n");
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_mem_share_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	return test_memory_send_sp(FFA_MEM_SHARE_SMC32, RECEIVER, constituents,
				   constituents_count);
}

test_result_t test_mem_lend_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	return test_memory_send_sp(FFA_MEM_LEND_SMC32, RECEIVER, constituents,
				   constituents_count);
}

test_result_t test_mem_donate_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)share_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);
	return test_memory_send_sp(FFA_MEM_DONATE_SMC32, RECEIVER, constituents,
				   constituents_count);
}

test_result_t test_consecutive_donate(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)consecutive_donate_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	test_result_t ret = test_memory_send_sp(FFA_MEM_DONATE_SMC32, SP_ID(1),
						constituents,
						constituents_count);

	if (ret != TEST_RESULT_SUCCESS) {
		ERROR("Failed at first attempting of sharing.\n");
		return TEST_RESULT_FAIL;
	}

	if (!test_memory_send_expect_denied(FFA_MEM_DONATE_SMC32,
					    consecutive_donate_page,
					    SP_ID(1))) {
		ERROR("Memory was successfully donated again from the NWd, to "
		      "the same borrower.\n");
		return TEST_RESULT_FAIL;
	}

	if (!test_memory_send_expect_denied(FFA_MEM_DONATE_SMC32,
					    consecutive_donate_page,
					    SP_ID(2))) {
		ERROR("Memory was successfully donated again from the NWd, to "
		      "another borrower.\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test requests a memory send operation between cactus SPs.
 * Cactus SP should reply to TFTF on whether the test succeeded or not.
 */
static test_result_t test_req_mem_send_sp_to_sp(uint32_t mem_func,
						ffa_id_t sender_sp,
						ffa_id_t receiver_sp,
						bool non_secure)
{
	struct ffa_value ret;

	/***********************************************************************
	 * Check if SPMC's ffa_version and presence of expected FF-A endpoints.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	ret = cactus_req_mem_send_send_cmd(HYP_ID, sender_sp, mem_func,
					   receiver_sp, non_secure);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		ERROR("Failed sharing memory between SPs. Error code: %d\n",
			cactus_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test requests a memory send operation from SP to VM.
 * The tests expects cactus to reply CACTUS_ERROR, providing FF-A error code of
 * the last memory send FF-A call that cactus performed.
 */
static test_result_t test_req_mem_send_sp_to_vm(uint32_t mem_func,
						ffa_id_t sender_sp,
						ffa_id_t receiver_vm)
{
	struct ffa_value ret;

	/**********************************************************************
	 * Check if SPMC's ffa_version and presence of expected FF-A endpoints.
	 *********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	ret = cactus_req_mem_send_send_cmd(HYP_ID, sender_sp, mem_func,
					   receiver_vm, false);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR &&
	    cactus_error_code(ret) == FFA_ERROR_DENIED) {
		return TEST_RESULT_SUCCESS;
	}

	tftf_testcase_printf("Did not get the expected error, "
			     "mem send returned with %d\n",
			     cactus_get_response(ret));
	return TEST_RESULT_FAIL;
}

test_result_t test_req_mem_share_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_SHARE_SMC32, SP_ID(3),
					  SP_ID(2), false);
}

test_result_t test_req_ns_mem_share_sp_to_sp(void)
{
	/*
	 * Skip the test when RME is enabled (for test setup reasons).
	 * For RME tests, the model specifies 48b physical address size
	 * at the PE, but misses allocating RAM and increasing the PA at
	 * the interconnect level.
	 */
	if (get_armv9_2_feat_rme_support() != 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/* This test requires 48b physical address size capability. */
	SKIP_TEST_IF_PA_SIZE_LESS_THAN(48);

	return test_req_mem_send_sp_to_sp(FFA_MEM_SHARE_SMC32, SP_ID(3),
					  SP_ID(2), true);
}

test_result_t test_req_mem_lend_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_LEND_SMC32, SP_ID(3),
					  SP_ID(2), false);
}

test_result_t test_req_mem_donate_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_DONATE_SMC32, SP_ID(1),
					  SP_ID(3), false);
}

test_result_t test_req_mem_share_sp_to_vm(void)
{
	return test_req_mem_send_sp_to_vm(FFA_MEM_SHARE_SMC32, SP_ID(1),
					  HYP_ID);
}

test_result_t test_req_mem_lend_sp_to_vm(void)
{
	return test_req_mem_send_sp_to_vm(FFA_MEM_LEND_SMC32, SP_ID(2),
					  HYP_ID);
}

test_result_t test_mem_share_to_sp_clear_memory(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
			sizeof(struct ffa_memory_region_constituent);
	struct mailbox_buffers mb;
	uint32_t remaining_constituent_count;
	uint32_t total_length;
	uint32_t fragment_length;
	ffa_memory_handle_t handle;
	struct ffa_value ret;
	uint32_t *ptr;
	/* Arbitrarily write 10 words after using shared memory. */
	const uint32_t nr_words_to_write = 10U;

	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(
			RECEIVER, FFA_MEM_LEND_SMC32);

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, MAILBOX_SIZE, SENDER,
		&receiver, 1, constituents, constituents_count, 0,
		FFA_MEMORY_REGION_FLAG_CLEAR,
		FFA_MEMORY_NOT_SPECIFIED_MEM, 0, 0,
		&total_length, &fragment_length);

	if (remaining_constituent_count != 0) {
		ERROR("Transaction descriptor initialization failed!\n");
		return TEST_RESULT_FAIL;
	}

	handle = memory_send(mb.send, FFA_MEM_LEND_SMC32, constituents,
			     constituents_count, remaining_constituent_count,
			     fragment_length, total_length, &ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Memory Share failed!\n");
		return TEST_RESULT_FAIL;
	}

	VERBOSE("Memory has been shared!\n");

	ret = cactus_mem_send_cmd(SENDER, RECEIVER, FFA_MEM_LEND_SMC32, handle,
				  FFA_MEMORY_REGION_FLAG_CLEAR, nr_words_to_write);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		ERROR("Failed memory send operation!\n");
		return TEST_RESULT_FAIL;
	}

	ret = ffa_mem_reclaim(handle, 0);

	if (is_ffa_call_error(ret)) {
		ERROR("Memory reclaim failed!\n");
		return TEST_RESULT_FAIL;
	}

	ptr = (uint32_t *)constituents[0].address;

	/* Check that borrower used the memory as expected for this test. */
	if (!check_written_words(ptr, FFA_MEM_LEND_SMC32, nr_words_to_write)) {
		ERROR("Words written to shared memory, not as expected.\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Print `region` if LOG_LEVEL >= LOG_LEVEL_VERBOSE
 */
static void print_memory_region(struct ffa_memory_region *region)
{
	VERBOSE("region.sender = %d\n", region->sender);
	VERBOSE("region.attributes.shareability = %d\n",
		region->attributes.shareability);
	VERBOSE("region.attributes.cacheability = %d\n",
		region->attributes.cacheability);
	VERBOSE("region.attributes.type = %d\n", region->attributes.type);
	VERBOSE("region.attributes.security = %d\n",
		region->attributes.security);
	VERBOSE("region.flags = %d\n", region->flags);
	VERBOSE("region.handle = %lld\n", region->handle);
	VERBOSE("region.tag = %lld\n", region->tag);
	VERBOSE("region.memory_access_desc_size = %d\n",
		region->memory_access_desc_size);
	VERBOSE("region.receiver_count = %d\n", region->receiver_count);
	VERBOSE("region.receivers_offset = %d\n", region->receivers_offset);
}

/**
 * Used by hypervisor retrieve request test: validate descriptors provided by
 * SPMC.
 */
static bool verify_retrieve_response(const struct ffa_memory_region *region1,
				     const struct ffa_memory_region *region2)
{
	if (region1->sender != region2->sender) {
		ERROR("region1.sender=%d, expected %d\n", region1->sender,
		      region2->sender);
		return false;
	}
	if (region1->attributes.shareability != region2->attributes.shareability) {
		ERROR("region1.attributes.shareability=%d, expected %d\n",
		      region1->attributes.shareability,
		      region2->attributes.shareability);
		return false;
	}
	if (region1->attributes.cacheability != region2->attributes.cacheability) {
		ERROR("region1.attributes.cacheability=%d, expected %d\n",
		      region1->attributes.cacheability,
		      region2->attributes.cacheability);
		return false;
	}
	if (region1->attributes.type != region2->attributes.type) {
		ERROR("region1.attributes.type=%d, expected %d\n",
		      region1->attributes.type, region2->attributes.type);
		return false;
	}
	if (region1->attributes.security != region2->attributes.security) {
		ERROR("region1.attributes.security=%d, expected %d\n",
		      region1->attributes.security, region2->attributes.security);
		return false;
	}
	if (region1->flags != region2->flags) {
		ERROR("region1->flags=%d, expected %d\n", region1->flags,
		      region2->flags);
		return false;
	}
	if (region1->handle != region2->handle) {
		ERROR("region1.handle=%lld, expected %lld\n", region1->handle,
		      region2->handle);
		return false;
	}
	if (region1->tag != region2->tag) {
		ERROR("region1.tag=%lld, expected %lld\n", region1->tag, region2->tag);
		return false;
	}
	if (region1->memory_access_desc_size != region2->memory_access_desc_size) {
		ERROR("region1.memory_access_desc_size=%d, expected %d\n",
		      region1->memory_access_desc_size,
		      region2->memory_access_desc_size);
		return false;
	}
	if (region1->receiver_count != region2->receiver_count) {
		ERROR("region1.receiver_count=%d, expected %d\n",
		      region1->receiver_count, region2->receiver_count);
		return false;
	}
	if (region1->receivers_offset != region2->receivers_offset) {
		ERROR("region1.receivers_offset=%d, expected %d\n",
		      region1->receivers_offset, region2->receivers_offset);
		return false;
	}
	for (uint32_t i = 0; i < 3; i++) {
		if (region1->reserved[i] != 0) {
			ERROR("region.reserved[%d]=%d, expected 0\n", i,
			      region1->reserved[i]);
			return false;
		}
	}
	return true;
}

/**
 * Used by hypervisor retrieve request test: validate descriptors provided by
 * SPMC.
 */
static bool
verify_constituent(struct ffa_memory_region_constituent *constituent,
		   void *address, uint32_t page_count)
{
	if (constituent->address != address) {
		ERROR("constituent.address=%p, expected %p\n",
		      constituent->address, address);
		return false;
	}
	if (constituent->page_count != page_count) {
		ERROR("constituent.page_count=%d, expected %d\n",
		      constituent->page_count, page_count);
		return false;
	}
	if (constituent->reserved != 0) {
		ERROR("constituent.reserved=%d, expected 0\n",
		      constituent->reserved);
		return false;
	}
	return true;
}

/**
 * Used by hypervisor retrieve request test: validate descriptors provided by
 * SPMC.
 */
static bool verify_composite(struct ffa_composite_memory_region *composite,
			     struct ffa_memory_region_constituent *constituent,
			     uint32_t page_count, uint32_t constituent_count)
{
	if (composite->page_count != page_count) {
		ERROR("composite.page_count=%d, expected %d\n",
		      composite->page_count, page_count);
		return false;
	}
	if (composite->constituent_count != constituent_count) {
		ERROR("composite.constituent_count=%d, expected %d\n",
		      composite->constituent_count, constituent_count);
		return false;
	}
	if (composite->reserved_0 != 0) {
		ERROR("composite.reserved_0=%llu, expected 0\n",
		      composite->reserved_0);
		return false;
	}
	for (uint32_t j = 0; j < composite->constituent_count; j++) {
		if (!verify_constituent(constituent, share_page, 1)) {
			return false;
		}
	}
	return true;
}

/**
 * Helper for performing a hypervisor retrieve request test.
 */
static test_result_t
hypervisor_retrieve_request_test_helper(uint32_t mem_func,
					bool multiple_receivers)
{
	static struct ffa_memory_region_constituent
		sent_constituents[FRAGMENTED_SHARE_PAGE_COUNT];
	uint32_t sent_constituents_count = 1;

	__aligned(PAGE_SIZE) static uint8_t page[PAGE_SIZE * 2] = {0};
	struct ffa_memory_region *hypervisor_retrieve_response =
		(struct ffa_memory_region *)page;
	struct ffa_memory_region expected_response;
	struct mailbox_buffers mb;
	ffa_memory_handle_t handle;
	struct ffa_value ret;

	uint32_t expected_flags = 0;

	ffa_memory_attributes_t expected_attrs = {
		.cacheability = FFA_MEMORY_CACHE_WRITE_BACK,
		.shareability = FFA_MEMORY_INNER_SHAREABLE,
		.security = FFA_MEMORY_SECURITY_NON_SECURE,
		.type = (!multiple_receivers && mem_func != FFA_MEM_SHARE_SMC32)
				? FFA_MEMORY_NOT_SPECIFIED_MEM
				: FFA_MEMORY_NORMAL_MEM,
	};

	struct ffa_memory_access receivers[2] = {
		ffa_memory_access_init_permissions_from_mem_func(SP_ID(1),
								 mem_func),
		ffa_memory_access_init_permissions_from_mem_func(SP_ID(2),
								 mem_func),
	};

	/*
	 * Only pass 1 receiver to `memory_init_and_send` if we are not testing
	 * the multiple-receivers functionality of the hypervisor retrieve
	 * request.
	 */
	uint32_t receiver_count =
		multiple_receivers ? ARRAY_SIZE(receivers) : 1;

	/* Add a page per constituent, so that we exhaust the size of a single
	 * fragment (for testing). In a real world scenario, the whole region
	 * could be described in a single constituent.
	 */
	for (uint32_t i = 0; i < sent_constituents_count; i++) {
		sent_constituents[i].address = share_page + i * PAGE_SIZE;
		sent_constituents[i].page_count = 1;
		sent_constituents[i].reserved = 0;
	}

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);
	GET_TFTF_MAILBOX(mb);

	switch (mem_func) {
	case FFA_MEM_SHARE_SMC32:
		expected_flags = FFA_MEMORY_REGION_TRANSACTION_TYPE_SHARE;
		break;
	case FFA_MEM_LEND_SMC32:
		expected_flags = FFA_MEMORY_REGION_TRANSACTION_TYPE_LEND;
		break;
	case FFA_MEM_DONATE_SMC32:
		expected_flags = FFA_MEMORY_REGION_TRANSACTION_TYPE_DONATE;
		break;
	default:
		ERROR("Invalid mem_func: %d\n", mem_func);
		panic();
	}

	handle = memory_init_and_send(mb.send, MAILBOX_SIZE, SENDER, receivers, receiver_count,
				      sent_constituents,
				      sent_constituents_count, mem_func, &ret);
	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Memory share failed: %d\n", ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	/*
	 * Send Hypervisor Retrieve request according to section 17.4.3 of FFA
	 * v1.2-REL0 specification.
	 */
	if (!hypervisor_retrieve_request(&mb, handle, page, sizeof(page))) {
		return TEST_RESULT_FAIL;
	}

	print_memory_region(hypervisor_retrieve_response);

	/*
	 * Verify the received `FFA_MEM_RETRIEVE_RESP` aligns with
	 * transaction description sent above.
	 */
	expected_response = (struct ffa_memory_region){
		.sender = SENDER,
		.attributes = expected_attrs,
		.flags = expected_flags,
		.handle = handle,
		.tag = 0,
		.memory_access_desc_size = sizeof(struct ffa_memory_access),
		.receiver_count = 1,
		.receivers_offset =
			offsetof(struct ffa_memory_region, receivers),
	};
	if (!verify_retrieve_response(hypervisor_retrieve_response, &expected_response)) {
		return TEST_RESULT_FAIL;
	}

	{
		uint32_t i = 0;
		struct ffa_composite_memory_region *composite =
			ffa_memory_region_get_composite(
				hypervisor_retrieve_response, i);
		if (composite == NULL) {
			ERROR("composite %d is null\n", i);
			return TEST_RESULT_FAIL;
		}

		if (!verify_composite(composite, &composite->constituents[i],
				      sent_constituents_count, sent_constituents_count)) {
			return TEST_RESULT_FAIL;
		}
	}

	/*
	 * Reclaim for the SPMC to deallocate any data related to the handle.
	 */
	ret = ffa_mem_reclaim(handle, 0);
	if (is_ffa_call_error(ret)) {
		ERROR("Memory reclaim failed: %d\n", ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_hypervisor_share_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_SHARE_SMC32, false);
}

test_result_t test_hypervisor_lend_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_LEND_SMC32, false);
}

test_result_t test_hypervisor_donate_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_DONATE_SMC32, false);
}

test_result_t test_hypervisor_share_retrieve_multiple_receivers(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_SHARE_SMC32, true);
}

test_result_t test_hypervisor_lend_retrieve_multiple_receivers(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_LEND_SMC32, true);
}
