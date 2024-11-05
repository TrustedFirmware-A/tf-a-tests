/*
 * Copyright (c) 2020-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "arch_features.h"
#include "arch_helpers.h"
#include "ffa_helpers.h"
#include "ffa_svc.h"
#include "stdint.h"
#include "utils_def.h"
#include <debug.h>
#include "ffa_helpers.h"
#include <sync.h>

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <host_realm_rmi.h>
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
static __aligned(PAGE_SIZE) uint8_t donate_page[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t consecutive_donate_page[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t four_share_pages[PAGE_SIZE * 4];

static bool gpc_abort_triggered;

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

static bool data_abort_handler(void)
{
	uint64_t esr_elx = IS_IN_EL2() ? read_esr_el2() : read_esr_el1();

	VERBOSE("%s esr_elx %llx\n", __func__, esr_elx);

	if (EC_BITS(esr_elx) == EC_DABORT_CUR_EL) {
		/* Synchronous data abort triggered by Granule protection */
		if ((ISS_BITS(esr_elx) & ISS_DFSC_MASK) == DFSC_GPF_DABORT) {
			VERBOSE("%s GPF Data Abort caught to address: %llx\n",
				__func__, (uint64_t)read_far_el2());
			gpc_abort_triggered = true;
			return true;
		}
	}

	return false;
}

static bool get_gpc_abort_triggered(void)
{
	bool ret = gpc_abort_triggered;

	gpc_abort_triggered = false;

	return ret;
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

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	for (unsigned i = 0; i < 3; i++) {
		if (!test_memory_send_expect_denied(
			FFA_MEM_SHARE_SMC64, (void *)forbidden_address[i],
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
					 size_t constituents_count, bool is_normal_memory)
{
	struct ffa_value ret;
	ffa_memory_handle_t handle;
	uint32_t *ptr;
	struct mailbox_buffers mb;
	unsigned int rme_supported = get_armv9_2_feat_rme_support();
	const bool check_gpc_fault =
		mem_func != FFA_MEM_SHARE_SMC64 &&
		rme_supported != 0U && is_normal_memory;

	/*
	 * For normal memory arbitrarilty write 5 words after using memory.
	 * For device just write 1 so we only write in the data register of the device.
	 */
	const uint32_t nr_words_to_write = is_normal_memory ? 5 : 1;

	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(borrower,
								 mem_func);

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	/*
	 * If the RME is enabled for the platform under test, check that the
	 * GPCs are working as expected, as such setup the exception handler.
	 */
	if (check_gpc_fault) {
		register_custom_sync_exception_handler(data_abort_handler);
	}

	for (size_t i = 0; i < constituents_count; i++) {
		VERBOSE("Sharing Address: %p\n", constituents[i].address);
		ptr = (uint32_t *)constituents[i].address;
		for (size_t j = 0; j < nr_words_to_write; j++) {
			ptr[j] = mem_func + 0xFFA;
		}
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
				  nr_words_to_write, false, is_normal_memory);

	if (!is_ffa_direct_response(ret) ||
	    cactus_get_response(ret) != CACTUS_SUCCESS) {
		ffa_mem_reclaim(handle, 0);
		ERROR("Failed memory send operation!\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * If there is RME support, look to trigger an exception as soon as the
	 * security state is update, due to GPC fault.
	 */
	if (check_gpc_fault) {
		*ptr = 0xBEEF;
	}

	if (mem_func != FFA_MEM_DONATE_SMC64) {

		/* Reclaim memory entirely before checking its state. */
		if (is_ffa_call_error(ffa_mem_reclaim(handle, 0))) {
			tftf_testcase_printf("Couldn't reclaim memory\n");
			return TEST_RESULT_FAIL;
		}

		for (uint32_t i = 0; i < constituents_count; i++) {
			ptr = constituents[i].address;

			/*
			 * Check that borrower used the memory as expected
			 * for FFA_MEM_SHARE test.
			 */
			if (mem_func == FFA_MEM_SHARE_SMC64 &&
			    !check_written_words(ptr,
						 mem_func + 0xFFAU,
						 nr_words_to_write)) {
				ERROR("Fail because of state of memory.\n");
				return TEST_RESULT_FAIL;
			}
		}
	}

	if (check_gpc_fault) {
		unregister_custom_sync_exception_handler();
		if (!get_gpc_abort_triggered()) {
			ERROR("No exception due to GPC for lend/donate with RME.\n");
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

	return test_memory_send_sp(FFA_MEM_SHARE_SMC64, RECEIVER, constituents,
				   constituents_count, true);
}

test_result_t test_mem_lend_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	return test_memory_send_sp(FFA_MEM_LEND_SMC64, RECEIVER, constituents,
				   constituents_count, true);
}

test_result_t test_mem_donate_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)donate_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);
	return test_memory_send_sp(FFA_MEM_DONATE_SMC64, RECEIVER, constituents,
				   constituents_count, true);
}

test_result_t test_consecutive_donate(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)consecutive_donate_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	test_result_t ret = test_memory_send_sp(FFA_MEM_DONATE_SMC64, SP_ID(1),
						constituents,
						constituents_count, true);

	if (ret != TEST_RESULT_SUCCESS) {
		ERROR("Failed at first attempting of sharing.\n");
		return TEST_RESULT_FAIL;
	}

	if (!test_memory_send_expect_denied(FFA_MEM_DONATE_SMC64,
					    consecutive_donate_page,
					    SP_ID(1))) {
		ERROR("Memory was successfully donated again from the NWd, to "
		      "the same borrower.\n");
		return TEST_RESULT_FAIL;
	}

	if (!test_memory_send_expect_denied(FFA_MEM_DONATE_SMC64,
					    consecutive_donate_page,
					    SP_ID(2))) {
		ERROR("Memory was successfully donated again from the NWd, to "
		      "another borrower.\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Lend device memory to the Secure Partition.
 */
test_result_t test_ffa_mem_lend_device_memory_sp(void)
{
#if PLAT_fvp || PLAT_tc
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)PLAT_ARM_UART_BASE, 1, 0},
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	return test_memory_send_sp(FFA_MEM_LEND_SMC64, RECEIVER, constituents,
				   constituents_count, false);
#else
	return TEST_RESULT_SKIPPED;
#endif

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
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

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
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

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
	return test_req_mem_send_sp_to_sp(FFA_MEM_SHARE_SMC64, SP_ID(3),
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

	return test_req_mem_send_sp_to_sp(FFA_MEM_SHARE_SMC64, SP_ID(3),
					  SP_ID(2), true);
}

test_result_t test_req_mem_lend_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_LEND_SMC64, SP_ID(3),
					  SP_ID(2), false);
}

test_result_t test_req_mem_donate_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_DONATE_SMC64, SP_ID(1),
					  SP_ID(3), false);
}

test_result_t test_req_mem_share_sp_to_vm(void)
{
	return test_req_mem_send_sp_to_vm(FFA_MEM_SHARE_SMC64, SP_ID(1),
					  HYP_ID);
}

test_result_t test_req_mem_lend_sp_to_vm(void)
{
	return test_req_mem_send_sp_to_vm(FFA_MEM_LEND_SMC64, SP_ID(2),
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
	/* Arbitrarily write 10 words after using shared memory. */
	const uint32_t nr_words_to_write = 10U;

	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(
			RECEIVER, FFA_MEM_LEND_SMC64);

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

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

	handle = memory_send(mb.send, FFA_MEM_LEND_SMC64, constituents,
			     constituents_count, remaining_constituent_count,
			     fragment_length, total_length, &ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Memory Share failed!\n");
		return TEST_RESULT_FAIL;
	}

	VERBOSE("Memory has been shared!\n");

	ret = cactus_mem_send_cmd(SENDER, RECEIVER, FFA_MEM_LEND_SMC64, handle,
				  FFA_MEMORY_REGION_FLAG_CLEAR,
				  nr_words_to_write, false, true);

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

static bool verify_receivers_impdef(struct ffa_memory_access_impdef impdef1,
				    struct ffa_memory_access_impdef impdef2)
{
	if (impdef1.val[0] != impdef2.val[0] ||
	    impdef1.val[1] != impdef2.val[1]) {
		ERROR("ipmdef1.val[0]=%llu expected=%llu"
		      " ipmdef1.val[1]=%llu expected=%llu\n",
		      impdef1.val[0], impdef2.val[0],
		      impdef1.val[1], impdef2.val[1]);
		return false;
	}

	return true;
}

static bool verify_permissions(
		ffa_memory_access_permissions_t permissions1,
		ffa_memory_access_permissions_t permissions2)
{
	uint8_t access1;
	uint8_t access2;

	access1 = permissions1.data_access;
	access2 = permissions2.data_access;

	if (access1 != access2) {
		ERROR("permissions1.data_access=%u expected=%u\n",
		      access1, access2);
		return false;
	}

	access1 = permissions1.instruction_access;
	access2 = permissions2.instruction_access;

	if (access1 != access2) {
		ERROR("permissions1.instruction_access=%u expected=%u\n",
		      access1, access2);
		return false;
	}

	return true;
}

/**
 * Used by hypervisor retrieve request test: validate descriptors provided by
 * SPMC.
 */
static bool verify_receivers(struct ffa_memory_access *receivers1,
			     struct ffa_memory_access *receivers2,
			     uint32_t receivers_count)
{
	for (uint32_t i = 0; i < receivers_count; i++) {
		if (receivers1[i].receiver_permissions.receiver !=
		    receivers2[i].receiver_permissions.receiver) {
			ERROR("receivers1[%u].receiver_permissions.receiver=%x"
			      " expected=%x\n", i,
			      receivers1[i].receiver_permissions.receiver,
			      receivers2[i].receiver_permissions.receiver);
			return false;
		}

		if (receivers1[i].receiver_permissions.flags !=
		    receivers2[i].receiver_permissions.flags) {
			ERROR("receivers1[%u].receiver_permissions.flags=%u"
			      " expected=%u\n", i,
			      receivers1[i].receiver_permissions.flags,
			      receivers2[i].receiver_permissions.flags);
			return false;
		}

		if (!verify_permissions(
			receivers1[i].receiver_permissions.permissions,
			receivers2[i].receiver_permissions.permissions)) {
			return false;
		}

		if (receivers1[i].composite_memory_region_offset !=
		    receivers2[i].composite_memory_region_offset) {
			ERROR("receivers1[%u].composite_memory_region_offset=%u"
			      " expected %u\n",
			      i, receivers1[i].composite_memory_region_offset,
			      receivers2[i].composite_memory_region_offset);
			return false;
		}

		if (!verify_receivers_impdef(receivers1[i].impdef,
					     receivers1[i].impdef)) {
			return false;
		}
	}

	return true;
}

/**
 * Helper for performing a hypervisor retrieve request test.
 */
static test_result_t hypervisor_retrieve_request_test_helper(
	uint32_t mem_func, bool multiple_receivers, bool fragmented)
{
	static struct ffa_memory_region_constituent
		sent_constituents[FRAGMENTED_SHARE_PAGE_COUNT];
	__aligned(PAGE_SIZE) static uint8_t page[PAGE_SIZE * 2] = {0};
	struct ffa_memory_region *hypervisor_retrieve_response =
		(struct ffa_memory_region *)page;
	struct ffa_memory_region expected_response;
	struct mailbox_buffers mb;
	ffa_memory_handle_t handle;
	struct ffa_value ret;
	struct ffa_composite_memory_region *composite;
	struct ffa_memory_access *retrvd_receivers;
	uint32_t expected_flags = 0;

	ffa_memory_attributes_t expected_attrs = {
		.cacheability = FFA_MEMORY_CACHE_WRITE_BACK,
		.shareability = FFA_MEMORY_INNER_SHAREABLE,
		.security = FFA_MEMORY_SECURITY_NON_SECURE,
		.type = (!multiple_receivers && mem_func != FFA_MEM_SHARE_SMC64)
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

	uint32_t sent_constituents_count =
		fragmented ? ARRAY_SIZE(sent_constituents) : 1;

	/* Prepare the composite offset for the comparison. */
	for (uint32_t i = 0; i < receiver_count; i++) {
		receivers[i].composite_memory_region_offset =
			sizeof(struct ffa_memory_region) +
			receiver_count *
				sizeof(struct ffa_memory_access);
	}

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
	case FFA_MEM_SHARE_SMC64:
		expected_flags = FFA_MEMORY_REGION_TRANSACTION_TYPE_SHARE;
		break;
	case FFA_MEM_LEND_SMC64:
		expected_flags = FFA_MEMORY_REGION_TRANSACTION_TYPE_LEND;
		break;
	case FFA_MEM_DONATE_SMC64:
		expected_flags = FFA_MEMORY_REGION_TRANSACTION_TYPE_DONATE;
		break;
	default:
		ERROR("Invalid mem_func: %d\n", mem_func);
		panic();
	}

	handle = memory_init_and_send(mb.send, MAILBOX_SIZE, SENDER, receivers,
				      receiver_count, sent_constituents,
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
	expected_response = (struct ffa_memory_region) {
		.sender = SENDER,
		.attributes = expected_attrs,
		.flags = expected_flags,
		.handle = handle,
		.tag = 0,
		.memory_access_desc_size = sizeof(struct ffa_memory_access),
		.receiver_count = receiver_count,
		.receivers_offset =
			offsetof(struct ffa_memory_region, receivers),
	};

	if (!verify_retrieve_response(hypervisor_retrieve_response,
				      &expected_response)) {
		return TEST_RESULT_FAIL;
	}

	retrvd_receivers =
		ffa_memory_region_get_receiver(hypervisor_retrieve_response, 0);

	if (!verify_receivers(retrvd_receivers,
			      receivers, receiver_count)) {
		return TEST_RESULT_FAIL;
	}

	composite = ffa_memory_region_get_composite(
				hypervisor_retrieve_response, 0);

	if (!verify_composite(composite, composite->constituents,
			      sent_constituents_count, sent_constituents_count)) {
		return TEST_RESULT_FAIL;
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
	return hypervisor_retrieve_request_test_helper(FFA_MEM_SHARE_SMC64, false, false);
}

test_result_t test_hypervisor_lend_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_LEND_SMC64, false, false);
}

test_result_t test_hypervisor_donate_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_DONATE_SMC64, false, false);
}

test_result_t test_hypervisor_share_retrieve_multiple_receivers(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_SHARE_SMC64, true, false);
}

test_result_t test_hypervisor_lend_retrieve_multiple_receivers(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_LEND_SMC64, true, false);
}

test_result_t test_hypervisor_share_retrieve_fragmented(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_SHARE_SMC64, false, true);
}

test_result_t test_hypervisor_lend_retrieve_fragmented(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_LEND_SMC64, false, true);
}

/**
 * Test helper that performs memory sharing operation, and alters the PAS
 * of the memory, to validate that SPM intersects the operation in case the PAS
 * is not coherent with its use. Relevant for the functioning of FFA_MEM_LEND
 * and FFA_MEM_DONATE from NWd to an SP.
 *
 * In cases the memory is not in NS state, the SPMC should intersect memory
 * management call with an appropriate FFA_ERROR.
 */
static test_result_t test_ffa_mem_send_realm_expect_fail(
		uint32_t mem_func, ffa_id_t borrower,
		struct ffa_memory_region_constituent *constituents,
		size_t constituents_count, uint64_t delegate_addr)
{
	struct ffa_value ret;
	uint32_t remaining_constituent_count;
	uint32_t total_length;
	uint32_t fragment_length;
	struct mailbox_buffers mb;
	u_register_t ret_rmm;
	test_result_t result = TEST_RESULT_FAIL;
	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(borrower,
								 mem_func);

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	register_custom_sync_exception_handler(data_abort_handler);

	/*
	 * Delegate page to a realm. This should make memory sharing operation
	 * fail.
	 */
	ret_rmm = host_rmi_granule_delegate((u_register_t)delegate_addr);

	if (ret_rmm != 0UL) {
		INFO("Delegate operation returns 0x%lx for address %llx\n",
		     ret_rmm, delegate_addr);
		return TEST_RESULT_FAIL;
	}

	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, MAILBOX_SIZE, SENDER,
		&receiver, 1, constituents, constituents_count, 0,
		FFA_MEMORY_REGION_FLAG_CLEAR,
		FFA_MEMORY_NOT_SPECIFIED_MEM, 0, 0,
		&total_length, &fragment_length);

	if (remaining_constituent_count != 0) {
		goto out;
	}

	switch (mem_func) {
	case FFA_MEM_LEND_SMC64:
		ret = ffa_mem_lend(total_length, fragment_length);
		break;
	case FFA_MEM_DONATE_SMC64:
		ret = ffa_mem_donate(total_length, fragment_length);
		break;
	default:
		ERROR("Not expected for func name: %x\n", mem_func);
		return TEST_RESULT_FAIL;
	}

	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED)) {
		goto out;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)delegate_addr);

	for (uint32_t i = 0; i < constituents_count; i++) {
		uint32_t *ptr = (uint32_t *)constituents[i].address;

		*ptr = 0xFFA;
	}

	if (get_gpc_abort_triggered()) {
		ERROR("Exception due to GPC for lend/donate with RME. Not"
		      " expected for this case.\n");
		result = TEST_RESULT_FAIL;
	} else {
		result = TEST_RESULT_SUCCESS;
	}
out:
	unregister_custom_sync_exception_handler();

	if (ret_rmm != 0UL) {
		INFO("Undelegate operation returns 0x%lx for address %llx\n",
		     ret_rmm, (uint64_t)delegate_addr);
		return TEST_RESULT_FAIL;
	}

	return result;
}

/**
 * Memory to be shared between partitions is described in a composite, with
 * various constituents. In an RME system, the memory must be in NS PAS in
 * operations from NWd to an SP. In case the PAS is not following this
 * expectation memory lend/donate should fail, and all constituents must
 * remain in the NS PAS.
 *
 * This test validates that if one page in the middle of one of the constituents
 * is not in the NS PAS the operation fails.
 */
test_result_t test_ffa_mem_send_sp_realm_memory(void)
{
	test_result_t ret;
	uint32_t mem_func[] = {FFA_MEM_LEND_SMC64, FFA_MEM_DONATE_SMC64};
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	for (unsigned j = 0; j < ARRAY_SIZE(mem_func); j++) {
		for (unsigned int i = 0; i < 4; i++) {
			/* Address to be delegated to Realm PAS. */
			uint64_t realm_addr =
				(uint64_t)&four_share_pages[i * PAGE_SIZE];

			INFO("%s memory with realm addr: %llx\n",
			     mem_func[j] == FFA_MEM_LEND_SMC64
				? "Lend"
				: "Donate",
			     realm_addr);

			ret = test_ffa_mem_send_realm_expect_fail(
				mem_func[j], SP_ID(1), constituents,
				constituents_count, realm_addr);

			if (ret != TEST_RESULT_SUCCESS) {
				break;
			}
		}
	}

	return ret;
}

/**
 * Memory to be shared between partitions is described in a composite, with
 * various constituents. In an RME system, the memory must be in NS PAS in
 * operations from NWd to an SP. In case the PAS is not following this
 * expectation memory lend/donate should fail, and all constituents must
 * remain in the NS PAS.
 *
 * This test validates the case in which the memory lend/donate fail in
 * case one of the constituents in the composite is not in the NS PAS.
 */
test_result_t test_ffa_mem_lend_sp_realm_memory_separate_constituent(void)
{
	test_result_t ret;
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);
	/* Address to be delegated to Realm PAS. */
	uint64_t realm_addr = (uint64_t)&share_page[0];

	INFO("Sharing memory with realm addr: %llx\n", realm_addr);

	ret = test_ffa_mem_send_realm_expect_fail(
		FFA_MEM_LEND_SMC64, SP_ID(1), constituents,
		constituents_count, realm_addr);

	return ret;
}

/**
 * Map the NS RXTX buffers to the SPM, change RX buffer PAS to realm,
 * invoke the FFA_MEM_SHARE interface, such that SPM does NS access
 * to realm region and triggers GPF.
 */
test_result_t test_ffa_mem_share_tx_realm_expect_fail(void)
{
	struct ffa_value ret;
	uint32_t total_length;
	uint32_t fragment_length;
	struct mailbox_buffers mb;
	u_register_t ret_rmm;
	struct ffa_memory_access receiver =
		ffa_memory_access_init_permissions_from_mem_func(SP_ID(1),
								 FFA_MEM_SHARE_SMC64);
	size_t remaining_constituent_count;
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)share_page, 1, 0}
	};

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, PAGE_SIZE, HYP_ID,
		&receiver, 1, constituents, 1, 0, 0,
		FFA_MEMORY_NOT_SPECIFIED_MEM, FFA_MEMORY_CACHE_WRITE_BACK,
		FFA_MEMORY_INNER_SHAREABLE,
		&total_length, &fragment_length);

	if (remaining_constituent_count != 0) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Delegate TX buffer to realm.
	 */
	ret_rmm = host_rmi_granule_delegate((u_register_t)mb.send);

	if (ret_rmm != 0UL) {
		INFO("Delegate operation returns  %#lx for address %p\n",
		     ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	ret = ffa_mem_share(total_length, fragment_length);

	/* Access to Realm region from SPMC should return FFA_ERROR_ABORTED. */
	if (!is_expected_ffa_error(ret, FFA_ERROR_ABORTED)) {
		return TEST_RESULT_FAIL;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)mb.send);

	if (ret_rmm != 0UL) {
		INFO("Undelegate operation returns 0x%lx for address %p\n",
		     ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, PAGE_SIZE, HYP_ID,
		&receiver, 1, constituents, 1, 0, 0,
		FFA_MEMORY_NOT_SPECIFIED_MEM, FFA_MEMORY_CACHE_WRITE_BACK,
		FFA_MEMORY_INNER_SHAREABLE,
		&total_length, &fragment_length);

	/* Retry but expect test to pass. */
	ret = ffa_mem_share(total_length, fragment_length);

	if (is_ffa_call_error(ret)) {
		return TEST_RESULT_FAIL;
	}

	/* Reclaim to clean-up. */
	ret = ffa_mem_reclaim(ffa_mem_success_handle(ret), 0);

	if (is_ffa_call_error(ret)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Base helper to prepare for tests that need to retrieve memory from the SPMC
 * from a VM endpoint.
 */
static ffa_memory_handle_t base_memory_send_for_nwd_retrieve(struct mailbox_buffers *mb,
							     struct ffa_memory_access receivers[],
							     size_t receivers_count)
{
	ffa_memory_handle_t handle;
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};
	const uint32_t constituents_count = ARRAY_SIZE(constituents);
	struct ffa_value ret;

	/* Prepare the composite offset for the comparison. */
	for (uint32_t i = 0; i < receivers_count; i++) {
		receivers[i].composite_memory_region_offset =
			sizeof(struct ffa_memory_region) +
			receivers_count *
				sizeof(struct ffa_memory_access);
	}

	handle = memory_init_and_send(mb->send, MAILBOX_SIZE, SENDER, receivers,
				      receivers_count, constituents,
				      constituents_count, FFA_MEM_SHARE_SMC64, &ret);
	return handle;
}

/**
 * Test FF-A memory retrieve request from a VM into the SPMC.
 * TFTF invokes all the FF-A calls expected from an hypervisor into the
 * SPMC, even those that would be initiated by a VM, and then forwarded
 * to the SPMC by the Hypervisor.
 */
test_result_t test_ffa_memory_retrieve_request_from_vm(void)
{
	struct mailbox_buffers mb;
	struct ffa_memory_region *m;
	struct ffa_memory_access receivers[2] = {
		ffa_memory_access_init_permissions_from_mem_func(VM_ID(1),
								 FFA_MEM_SHARE_SMC64),
		ffa_memory_access_init_permissions_from_mem_func(SP_ID(2),
								 FFA_MEM_SHARE_SMC64),
	};
	ffa_memory_handle_t handle;

	GET_TFTF_MAILBOX(mb);

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	handle = base_memory_send_for_nwd_retrieve(&mb, receivers, ARRAY_SIZE(receivers));

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		return TEST_RESULT_FAIL;
	}

	if (!memory_retrieve(&mb, &m, handle, 0, receivers, ARRAY_SIZE(receivers),
			     0, true)) {
		ERROR("Failed to retrieve the memory.\n");
		return TEST_RESULT_FAIL;
	}

	ffa_rx_release();

	if (!memory_relinquish(mb.send, handle, VM_ID(1))) {
		ERROR("%s: Failed to relinquish.\n", __func__);
		return TEST_RESULT_FAIL;
	}

	if (is_ffa_call_error(ffa_mem_reclaim(handle, 0))) {
		ERROR("%s: Failed to reclaim memory.\n", __func__);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t base_ffa_memory_retrieve_request_fail_buffer_realm(bool delegate_rx,
								 bool is_hypervisor_retrieve_req)
{
	struct mailbox_buffers mb;
	struct ffa_memory_access receivers[2] = {
		ffa_memory_access_init_permissions_from_mem_func(VM_ID(1),
								 FFA_MEM_SHARE_SMC64),
		ffa_memory_access_init_permissions_from_mem_func(SP_ID(2),
								 FFA_MEM_SHARE_SMC64),
	};
	ffa_memory_handle_t handle;
	u_register_t ret_rmm;
	struct ffa_value ret;
	size_t descriptor_size;
	void *to_delegate;

	GET_TFTF_MAILBOX(mb);

	to_delegate = delegate_rx ? mb.recv : mb.send;

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	handle = base_memory_send_for_nwd_retrieve(&mb, receivers, ARRAY_SIZE(receivers));

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		return TEST_RESULT_FAIL;
	}

	if (is_hypervisor_retrieve_req) {
		/* Prepare the hypervisor retrieve request. */
		ffa_hypervisor_retrieve_request_init(mb.send, handle);
		descriptor_size = sizeof(struct ffa_memory_region);
	} else {
		/* Prepare the descriptor before delegating the buffer. */
		descriptor_size = ffa_memory_retrieve_request_init(
			mb.send, handle, SENDER, receivers, ARRAY_SIZE(receivers),
			0, 0, FFA_MEMORY_NORMAL_MEM, FFA_MEMORY_CACHE_WRITE_BACK,
			FFA_MEMORY_INNER_SHAREABLE);
	}

	/* Delegate buffer to realm. */
	ret_rmm = host_rmi_granule_delegate((u_register_t)to_delegate);

	if (ret_rmm != 0UL) {
		ERROR("Delegate operation returns %#lx for address %p\n",
		      ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	ret = ffa_mem_retrieve_req(descriptor_size, descriptor_size);

	if (!is_expected_ffa_error(ret, FFA_ERROR_ABORTED)) {
		return TEST_RESULT_FAIL;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)to_delegate);

	if (ret_rmm != 0UL) {
		ERROR("Undelegate operation returns %#lx for address %p\n",
		      ret_rmm, mb.send);
		return TEST_RESULT_FAIL;
	}

	if (is_hypervisor_retrieve_req) {
		/* Prepare the hypervisor retrieve request. */
		ffa_hypervisor_retrieve_request_init(mb.send, handle);
		descriptor_size = sizeof(struct ffa_memory_region);
	} else {
		/* Prepare the descriptor before delegating the buffer. */
		descriptor_size = ffa_memory_retrieve_request_init(
			mb.send, handle, SENDER, receivers, ARRAY_SIZE(receivers),
			0, 0, FFA_MEMORY_NORMAL_MEM, FFA_MEMORY_CACHE_WRITE_BACK,
			FFA_MEMORY_INNER_SHAREABLE);
	}

	/* Retry the memory retrieve request, but this time expect success. */
	ret = ffa_mem_retrieve_req(descriptor_size, descriptor_size);

	if (is_ffa_call_error(ret)) {
		return TEST_RESULT_FAIL;
	}

	ffa_rx_release();

	if (!is_hypervisor_retrieve_req &&
	    !memory_relinquish(mb.send, handle, VM_ID(1))) {
		ERROR("%s: Failed to relinquish.\n", __func__);
		return TEST_RESULT_FAIL;
	}

	if (is_ffa_call_error(ffa_mem_reclaim(handle, 0))) {
		ERROR("%s: Failed to reclaim memory.\n", __func__);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test that a retrieve request from the hypervisor would fail if the TX buffer
 * was in realm state. This is recreating the situation in which the Hyp doesn't
 * track the state of the operation, and it is forwarding the retrieve request
 * to the SPMC.
 */
test_result_t test_ffa_memory_retrieve_request_fail_tx_realm(void)
{
	return base_ffa_memory_retrieve_request_fail_buffer_realm(false, false);
}

/**
 * Test that a retrieve request from the hypervisor would fail if the RX buffer
 * was in realm state. This is recreating the situation in which the Hyp doesn't
 * track the state of the operation, and it is forwarding the retrieve request
 * to the SPMC. The operation shall fail at the point at which the SPMC is
 * providing retrieve response. The SPMC should have reverted the change to any
 * of its share state tracking structures, such that the final reclaim would be
 * possible.
 */
test_result_t test_ffa_memory_retrieve_request_fail_rx_realm(void)
{
	return base_ffa_memory_retrieve_request_fail_buffer_realm(true, false);
}

/**
 * Test that a memory relinquish call fails smoothly if the TX buffer of the
 * Hypervisor is on realm PAS.
 */
test_result_t test_ffa_memory_relinquish_fail_tx_realm(void)
{
	struct mailbox_buffers mb;
	struct ffa_memory_region *m;
	const ffa_id_t vm_id = VM_ID(1);
	struct ffa_memory_access receivers[2] = {
		ffa_memory_access_init_permissions_from_mem_func(vm_id,
								 FFA_MEM_SHARE_SMC64),
		ffa_memory_access_init_permissions_from_mem_func(SP_ID(2),
								 FFA_MEM_SHARE_SMC64),
	};
	struct ffa_value ret;
	ffa_memory_handle_t handle;
	u_register_t ret_rmm;

	GET_TFTF_MAILBOX(mb);

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	handle = base_memory_send_for_nwd_retrieve(&mb, receivers, ARRAY_SIZE(receivers));

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		return TEST_RESULT_FAIL;
	}

	if (!memory_retrieve(&mb, &m, handle, 0, receivers, ARRAY_SIZE(receivers),
			     0, true)) {
		ERROR("Failed to retrieve the memory.\n");
		return TEST_RESULT_FAIL;
	}

	/* Prepare relinquish descriptor before calling ffa_mem_relinquish. */
	ffa_mem_relinquish_init(mb.send, handle, 0, vm_id);

	/*
	 * Delegate page to a realm. This should make memory sharing operation
	 * fail.
	 */
	ret_rmm = host_rmi_granule_delegate((u_register_t)mb.send);
	if (ret_rmm != 0UL) {
		ERROR("Delegate operation returns 0x%lx for address %llx\n",
		      ret_rmm, (uint64_t)mb.send);
		return TEST_RESULT_FAIL;
	}

	/* Access to Realm region from SPMC should return FFA_ERROR_ABORTED. */
	ret = ffa_mem_relinquish();
	if (!is_expected_ffa_error(ret, FFA_ERROR_ABORTED)) {
		return TEST_RESULT_FAIL;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)mb.send);
	if (ret_rmm != 0UL) {
		ERROR("Undelegate operation returns 0x%lx for address %llx\n",
		      ret_rmm, (uint64_t)mb.send);
		return TEST_RESULT_FAIL;
	}

	/* Prepare the descriptor. */
	ffa_mem_relinquish_init(mb.send, handle, 0, vm_id);

	/* After undelegate the relinquish is expected to succeed. */
	ret = ffa_mem_relinquish();

	if (is_ffa_call_error(ret)) {
		ERROR("Expected relinquish to succeed\n");
		return TEST_RESULT_FAIL;
	}

	ret = ffa_mem_reclaim(handle, 0);
	if (is_ffa_call_error(ret)) {
		ERROR("Memory reclaim failed!\n");
		return TEST_RESULT_FAIL;
	}

	ffa_rx_release();

	return TEST_RESULT_SUCCESS;
}

/**
 * Test that a hypervisor retrieve request would fail if the TX buffer
 * was in realm PAS.
 * The hypervisor retrieve request normally happens during an FFA_MEM_RECLAIM.
 * This validates that the SPMC is able to recover from a GPF from accessing the
 * TX buffer when reading the hypervisor retrieve request message.
 */
test_result_t test_ffa_hypervisor_retrieve_request_fail_tx_realm(void)
{
	return base_ffa_memory_retrieve_request_fail_buffer_realm(false, true);
}

/**
 * Test that a hypervisor retrieve request would fail if the RX buffer
 * was in realm PAS.
 * The hypervisor retrieve request normally happens during an FFA_MEM_RECLAIM.
 * This validates the SPMC is able to recover from a GPF from accessing the RX
 * buffer when preparing the retrieve response.
 */
test_result_t test_ffa_hypervisor_retrieve_request_fail_rx_realm(void)
{
	return base_ffa_memory_retrieve_request_fail_buffer_realm(true, true);
}

/**
 * Do a memory sharing operation over two fragments.
 * Before the 2nd fragment the TX buffer is set in the realm PAS.
 * The SPMC should fault, recover from it and return ffa_error(FFA_ERROR_ABORTED).
 */
test_result_t test_ffa_memory_share_fragmented_tx_realm(void)
{
	struct mailbox_buffers mb;
	uint32_t remaining_constituent_count = 0;
	uint32_t total_length;
	uint32_t fragment_length;
	struct ffa_memory_access receiver = ffa_memory_access_init_permissions_from_mem_func(
						SP_ID(1), FFA_MEM_SHARE_SMC32);
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};
	struct ffa_value ffa_ret;
	u_register_t ret_rmm;
	test_result_t ret;
	uint64_t handle;

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	register_custom_sync_exception_handler(data_abort_handler);

	/* Only send one constituent to start with. */
	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, MAILBOX_SIZE, SENDER,
		&receiver, 1, constituents, ARRAY_SIZE(constituents), 0,
		0, FFA_MEMORY_NOT_SPECIFIED_MEM,
		FFA_MEMORY_CACHE_WRITE_BACK,
		FFA_MEMORY_INNER_SHAREABLE,
		&total_length, &fragment_length);

	/* It should have copied them all. */
	if (remaining_constituent_count > 0) {
		ERROR("Transaction descriptor initialization failed!\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/*
	 * Take the size of a constituent from the fragment to force the
	 * operation to be fragmented.
	 */
	fragment_length -= sizeof(struct ffa_memory_region_constituent);

	ffa_ret = ffa_mem_share(total_length, fragment_length);

	if (!is_expected_ffa_return(ffa_ret, FFA_MEM_FRAG_RX)) {
		ERROR("Expected %s after the memory share.\n",
		      ffa_func_name(FFA_MEM_FRAG_RX));
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	handle = ffa_frag_handle(ffa_ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("SPMC returned an invalid handle for the operation.\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Prepare the next fragment for the operation. */
	remaining_constituent_count = ffa_memory_fragment_init(
		mb.send, PAGE_SIZE, &constituents[1], 1, &fragment_length);

	/*
	 * Delegate send/tx buffer to a realm. This should make memory sharing operation
	 * fail.
	 */
	ret_rmm = host_rmi_granule_delegate((u_register_t)mb.send);

	if (ret_rmm != 0UL) {
		INFO("Delegate operation returns 0x%lx for address %p\n",
		     ret_rmm, mb.send);
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	ffa_ret = ffa_mem_frag_tx(handle, fragment_length);

	if (!is_expected_ffa_error(ffa_ret, FFA_ERROR_ABORTED)) {
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)mb.send);
	if (ret_rmm != 0UL) {
		ERROR("Undelegate operation returns 0x%lx for address %llx\n",
		      ret_rmm, (uint64_t)mb.send);
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* This time test should pass. */
	ffa_ret = ffa_mem_frag_tx(handle, fragment_length);

	if (is_ffa_call_error(ffa_ret)) {
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Reclaim memory to be able to reuse it. */
	ffa_ret = ffa_mem_reclaim(handle, 0);

	if (is_ffa_call_error(ffa_ret)) {
		ERROR("Failed to reclaim memory to be used in next test\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	ret = TEST_RESULT_SUCCESS;

exit:
	unregister_custom_sync_exception_handler();

	return ret;
}

/**
 * Do a memory sharing operation over two fragments.
 * Before the 2nd fragment the RX buffer is set in the realm PAS.
 * The SPMC should fault, recover from it and return
 * ffa_error(FFA_ERROR_ABORTED).
 *
 * Test Sequence:
 * - Share memory with SP(1), using a force fragmented approach.
 * - Initiate an hypervisor retrieve request, and retrieve only
 *   the first fragment.
 * - Change the physical address space of NWd RX buffer.
 * - Invoke the FFA_MEM_FRAG_RX interface, which should abort because
 *   of previous step.
 * - Reestablish the PAS of the NWd RX buffer.
 * - Contiueing with hypervisor retrieve request, and obtain the 2nd
 *   fragment.
 * - Reclaim memory for clean-up of SPMC state.
 */
test_result_t test_ffa_memory_share_fragmented_rx_realm(void)
{
	struct mailbox_buffers mb;
	uint32_t remaining_constituent_count = 0;
	uint32_t total_size;
	uint32_t fragment_size;
	uint32_t fragment_offset;
	struct ffa_memory_access receiver = ffa_memory_access_init_permissions_from_mem_func(
						SP_ID(1), FFA_MEM_SHARE_SMC32);
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};
	struct ffa_value ffa_ret;
	u_register_t ret_rmm;
	test_result_t ret;
	uint64_t handle;

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	CHECK_SPMC_TESTING_SETUP(1, 2, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	register_custom_sync_exception_handler(data_abort_handler);

	/* Only send one constituent to start with. */
	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, MAILBOX_SIZE, SENDER,
		&receiver, 1, constituents, ARRAY_SIZE(constituents), 0,
		0, FFA_MEMORY_NOT_SPECIFIED_MEM,
		FFA_MEMORY_CACHE_WRITE_BACK,
		FFA_MEMORY_INNER_SHAREABLE,
		&total_size, &fragment_size);

	/* It should have copied them all. */
	if (remaining_constituent_count > 0) {
		ERROR("Transaction descriptor initialization failed!\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/*
	 * Take the size of a constituent from the fragment to force the
	 * operation to be fragmented.
	 */
	fragment_size -= sizeof(struct ffa_memory_region_constituent);

	ffa_ret = ffa_mem_share(total_size, fragment_size);

	if (!is_expected_ffa_return(ffa_ret, FFA_MEM_FRAG_RX)) {
		ERROR("Expected %s after the memory share.\n",
		      ffa_func_name(FFA_MEM_FRAG_RX));
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	handle = ffa_frag_handle(ffa_ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("SPMC returned an invalid handle for the operation.\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Prepare the next fragment for the operation. */
	remaining_constituent_count = ffa_memory_fragment_init(
		mb.send, PAGE_SIZE, &constituents[1], 1, &fragment_size);

	ffa_ret = ffa_mem_frag_tx(handle, fragment_size);

	if (is_ffa_call_error(ffa_ret)) {
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/*
	 * Request the hypervisor retrieve request.
	 * Response should be fragmented.
	 */
	ffa_hypervisor_retrieve_request_init(mb.send, handle);
	ffa_ret = ffa_mem_retrieve_req(sizeof(struct ffa_memory_region),
				   sizeof(struct ffa_memory_region));

	if (ffa_func_id(ffa_ret) != FFA_MEM_RETRIEVE_RESP) {
		ERROR("%s: couldn't retrieve the memory page. Error: %d\n",
		      __func__, ffa_error_code(ffa_ret));
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	total_size = ffa_mem_retrieve_res_total_size(ffa_ret);
	fragment_size = ffa_mem_retrieve_res_frag_size(ffa_ret);
	fragment_offset = fragment_size;

	ret_rmm = host_rmi_granule_delegate((u_register_t)mb.recv);

	if (ret_rmm != 0UL) {
		INFO("Delegate operation returns 0x%lx for address %p\n",
		     ret_rmm, mb.send);
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	ffa_ret = ffa_rx_release();
	if (is_ffa_call_error(ffa_ret)) {
		ERROR("ffa_rx_release() failed.\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Call FFA_MEM_FRAG_RX but expect it to abort. */
	ffa_ret = ffa_mem_frag_rx(handle, fragment_offset);

	if (!is_expected_ffa_error(ffa_ret, FFA_ERROR_ABORTED)) {
		ERROR("Expected FFA_MEM_FRAG_RX to have failed with"
		      "FFA_ERROR_ABORTED.\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)mb.recv);
	if (ret_rmm != 0UL) {
		ERROR("Undelegate operation returns 0x%lx for address %llx\n",
		      ret_rmm, (uint64_t)mb.send);
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Continue the hypervisor retrieve request. */
	if (!hypervisor_retrieve_request_continue(
			&mb, handle, NULL, 0, total_size, fragment_offset, false)) {
		ERROR("Failed to continue hypervisor retrieve request after"
		      " restablishing PAS.\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	/* Reclaim memory to be able to reuse it. */
	ffa_ret = ffa_mem_reclaim(handle, 0);

	if (is_ffa_call_error(ffa_ret)) {
		ERROR("Failed to reclaim memory to be used in next test\n");
		ret = TEST_RESULT_FAIL;
		goto exit;
	}

	ret = TEST_RESULT_SUCCESS;

exit:
	unregister_custom_sync_exception_handler();

	return ret;
}
