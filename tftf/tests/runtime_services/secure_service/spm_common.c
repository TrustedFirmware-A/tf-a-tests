/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "stdint.h"

#include "ffa_helpers.h"
#include <cactus_test_cmds.h>
#include <debug.h>
#include <ffa_endpoints.h>
#include <assert.h>
#include <ffa_svc.h>
#include <lib/extensions/sve.h>
#include <spm_common.h>
#include <xlat_tables_v2.h>

/**
 * Helper to log errors after FF-A calls.
 */
bool is_ffa_call_error(struct ffa_value ret)
{
	if (ffa_func_id(ret) == FFA_ERROR) {
		VERBOSE("FF-A call returned error: %s\n",
			ffa_error_name(ffa_error_code(ret)));
		return true;
	}
	return false;
}

bool is_expected_ffa_error(struct ffa_value ret, int32_t expected_error)
{
	uint32_t received_func;
	int32_t received_error;

	received_func = ffa_func_id(ret);
	if (received_func != FFA_ERROR) {
		ERROR("Expected FFA_ERROR, got %s instead\n",
		      ffa_func_name(received_func));
		return false;
	}

	received_error = ffa_error_code(ret);
	if (received_error != expected_error) {
		ERROR("Expected %s, got %s instead\n",
		      ffa_error_name(expected_error),
		      ffa_error_name(received_error));
		return false;
	}

	return true;
}

/**
 * Helper to verify return of FF-A call is an FFA_MSG_SEND_DIRECT_RESP.
 * Should be used after FFA_MSG_SEND_DIRECT_REQ, or after sending a test command
 * to an SP.
 */
bool is_ffa_direct_response(struct ffa_value ret)
{
	if ((ffa_func_id(ret) == FFA_MSG_SEND_DIRECT_RESP_SMC32) ||
	    (ffa_func_id(ret) == FFA_MSG_SEND_DIRECT_RESP_SMC64)) {
		return true;
	}

	VERBOSE("%s is not FF-A response.\n", ffa_func_name(ffa_func_id(ret)));
	/* To log error in case it is FFA_ERROR*/
	is_ffa_call_error(ret);

	return false;
}

/**
 * Helper to check the return value of FF-A call is as expected.
 */
bool is_expected_ffa_return(struct ffa_value ret, uint32_t func_id)
{
	if (ffa_func_id(ret) == func_id) {
		return true;
	}

	VERBOSE("Expecting %s, FF-A return was %s\n", ffa_func_name(func_id),
		ffa_func_name(ffa_func_id(ret)));

	return false;
}

bool is_expected_cactus_response(struct ffa_value ret, uint32_t expected_resp,
				 uint32_t arg)
{
	if (!is_ffa_direct_response(ret)) {
		return false;
	}

	if (cactus_get_response(ret) != expected_resp ||
	    (uint32_t)ret.arg4 != arg) {
		VERBOSE("Expected response %x and %x; "
		      "Obtained %x and %x\n",
		      expected_resp, arg, cactus_get_response(ret),
		      (int32_t)ret.arg4);
		return false;
	}

	return true;
}

void dump_ffa_value(struct ffa_value ret)
{
	NOTICE("FF-A value: %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx\n",
		ret.fid,
		ret.arg1,
		ret.arg2,
		ret.arg3,
		ret.arg4,
		ret.arg5,
		ret.arg6,
		ret.arg7);
}

/*
 * check_spmc_execution_level
 *
 * Attempt sending impdef protocol messages to OP-TEE through direct messaging.
 * Criteria for detecting OP-TEE presence is that responses match defined
 * version values. In the case of SPMC running at S-EL2 (and Cactus instances
 * running at S-EL1) the response will not match the pre-defined version IDs.
 *
 * Returns true if SPMC is probed as being OP-TEE at S-EL1.
 *
 */
bool check_spmc_execution_level(void)
{
	unsigned int is_optee_spmc_criteria = 0U;
	struct ffa_value ret_values;

	/*
	 * Send a first OP-TEE-defined protocol message through
	 * FFA direct message. Expect it to implement either v1.0 or v1.1.
	 */
	ret_values = ffa_msg_send_direct_req32(HYP_ID, SP_ID(1),
					       OPTEE_FFA_GET_API_VERSION, 0,
					       0, 0, 0);
	if (ret_values.arg3 == 1 &&
	    (ret_values.arg4 == 0 || ret_values.arg4 == 1)) {
		is_optee_spmc_criteria++;
	}

	/*
	 * Send a second OP-TEE-defined protocol message through
	 * FFA direct message.
	 */
	ret_values = ffa_msg_send_direct_req32(HYP_ID, SP_ID(1),
					       OPTEE_FFA_GET_OS_VERSION,
					       0, 0, 0, 0);
	if ((ret_values.arg3 == OPTEE_FFA_GET_OS_VERSION_MAJOR) &&
	    (ret_values.arg4 == OPTEE_FFA_GET_OS_VERSION_MINOR)) {
		is_optee_spmc_criteria++;
	}

	return (is_optee_spmc_criteria == 2U);
}

static const struct ffa_features_test ffa_feature_test_target[] = {
	{"FFA_ERROR_32", FFA_ERROR, FFA_SUCCESS_SMC32},
	{"FFA_SUCCESS_32", FFA_SUCCESS_SMC32, FFA_SUCCESS_SMC32},
	{"FFA_INTERRUPT_32", FFA_INTERRUPT, FFA_SUCCESS_SMC32},
	{"FFA_VERSION_32", FFA_VERSION, FFA_SUCCESS_SMC32},
	{"FFA_FEATURES_32", FFA_FEATURES, FFA_SUCCESS_SMC32},
	{"FFA_RX_RELEASE_32", FFA_RX_RELEASE, FFA_SUCCESS_SMC32},
	{"FFA_RXTX_MAP_32", FFA_RXTX_MAP_SMC32, FFA_ERROR},
	{"FFA_RXTX_MAP_64", FFA_RXTX_MAP_SMC64, FFA_SUCCESS_SMC32},
	{"FFA_RXTX_UNMAP_32", FFA_RXTX_UNMAP, FFA_SUCCESS_SMC32},
	{"FFA_PARTITION_INFO_GET_32", FFA_PARTITION_INFO_GET, FFA_SUCCESS_SMC32},
	{"FFA_ID_GET_32", FFA_ID_GET, FFA_SUCCESS_SMC32},
	{"FFA_SPM_ID_GET_32", FFA_SPM_ID_GET, FFA_SUCCESS_SMC32, 0,
		FFA_VERSION_1_1},
	{"FFA_MSG_WAIT_32", FFA_MSG_WAIT, FFA_SUCCESS_SMC32},
	{"FFA_RUN_32", FFA_RUN, FFA_SUCCESS_SMC32},
	{"FFA_MEM_DONATE_32", FFA_MEM_DONATE_SMC32, FFA_SUCCESS_SMC32},
	{"FFA_MEM_LEND_32", FFA_MEM_LEND_SMC32, FFA_SUCCESS_SMC32},
	{"FFA_MEM_SHARE_32", FFA_MEM_SHARE_SMC32, FFA_SUCCESS_SMC32},
	{"FFA_MEM_DONATE_64", FFA_MEM_DONATE_SMC64, FFA_SUCCESS_SMC32},
	{"FFA_MEM_LEND_64", FFA_MEM_LEND_SMC64, FFA_SUCCESS_SMC32},
	{"FFA_MEM_SHARE_64", FFA_MEM_SHARE_SMC64, FFA_SUCCESS_SMC32},
	{"FFA_MEM_RETRIEVE_REQ_64", FFA_MEM_RETRIEVE_REQ_SMC64,
	FFA_SUCCESS_SMC32, FFA_FEATURES_MEM_RETRIEVE_REQ_NS_SUPPORT},
	{"FFA_MEM_RETRIEVE_REQ_32", FFA_MEM_RETRIEVE_REQ_SMC32,
	FFA_SUCCESS_SMC32, FFA_FEATURES_MEM_RETRIEVE_REQ_NS_SUPPORT},
	{"FFA_MEM_RETRIEVE_RESP_32", FFA_MEM_RETRIEVE_RESP, FFA_SUCCESS_SMC32},
	{"FFA_MEM_RELINQUISH_32", FFA_MEM_RELINQUISH, FFA_SUCCESS_SMC32},
	{"FFA_MEM_RECLAIM_32", FFA_MEM_RECLAIM, FFA_SUCCESS_SMC32},
	{"FFA_NOTIFICATION_BITMAP_CREATE_32",
		FFA_NOTIFICATION_BITMAP_CREATE, FFA_SUCCESS_SMC32},
	{"FFA_NOTIFICATION_BITMAP_DESTROY_32",
		FFA_NOTIFICATION_BITMAP_DESTROY, FFA_SUCCESS_SMC32},
	{"FFA_NOTIFICATION_BIND_32", FFA_NOTIFICATION_BIND,
		FFA_SUCCESS_SMC32},
	{"FFA_NOTIFICATION_UNBIND_32", FFA_NOTIFICATION_UNBIND,
		FFA_SUCCESS_SMC32},
	{"FFA_NOTIFICATION_SET_32", FFA_NOTIFICATION_SET,
		FFA_SUCCESS_SMC32},
	{"FFA_NOTIFICATION_INFO_GET_64", FFA_NOTIFICATION_INFO_GET_SMC64,
		FFA_SUCCESS_SMC32},
	{"Check non-existent command", 0xFFFF, FFA_ERROR},
};

/*
 * Populates test_target with content of ffa_feature_test_target.
 *
 * Returns number of elements in the *test_target.
 */
size_t get_ffa_feature_test_target(const struct ffa_features_test **test_target)
{
	assert(test_target != NULL);
	*test_target = ffa_feature_test_target;
	return ARRAY_SIZE(ffa_feature_test_target);
}

/**
 * Leverages the struct ffa_feature_test and validates the result of
 * FFA_FEATURES calls.
 */
bool ffa_features_test_targets(const struct ffa_features_test *targets,
			       size_t test_target_size)
{
	bool ret = true;

	for (size_t i = 0U; i < test_target_size; i++) {
		struct ffa_value ffa_ret;
		uint32_t expected_ret;
		const struct ffa_features_test *test_target = &targets[i];

		ffa_ret = ffa_features_with_input_property(test_target->feature,
							   test_target->param);
		expected_ret = FFA_VERSION_COMPILED
				>= test_target->version_added ?
				test_target->expected_ret : FFA_ERROR;

		if (ffa_func_id(ffa_ret) != expected_ret) {
			ERROR("Unexpected return: %s (expected %s)."
			      " FFA_FEATURES test: %s.\n",
			      ffa_func_name(ffa_func_id(ffa_ret)),
			      ffa_func_name(expected_ret),
			      test_target->test_name);
			ret = false;
		}

		if (expected_ret == FFA_ERROR) {
			if (ffa_error_code(ffa_ret) !=
			    FFA_ERROR_NOT_SUPPORTED) {
				ERROR("Unexpected error code: %s (expected %s)."
				      " FFA_FEATURES test: %s.\n",
				      ffa_error_name(ffa_error_code(ffa_ret)),
				      ffa_error_name(expected_ret),
				      test_target->test_name);
				ret = false;
			}
		}
	}

	return ret;
}

bool memory_retrieve(struct mailbox_buffers *mb,
		     struct ffa_memory_region **retrieved, uint64_t handle,
		     ffa_id_t sender, struct ffa_memory_access receivers[],
		     uint32_t receiver_count, ffa_memory_region_flags_t flags,
		     bool is_normal_memory)
{
	struct ffa_value ret;
	uint32_t fragment_size;
	uint32_t total_size;
	uint32_t descriptor_size;
	enum ffa_memory_type memory_type = is_normal_memory ?
		FFA_MEMORY_NORMAL_MEM : FFA_MEMORY_DEVICE_MEM;
	enum ffa_memory_cacheability memory_cacheability = is_normal_memory ?
		FFA_MEMORY_CACHE_WRITE_BACK : FFA_MEMORY_DEV_NGNRNE;

	if (retrieved == NULL || mb == NULL) {
		ERROR("Invalid parameters!\n");
		return false;
	}

	descriptor_size = ffa_memory_retrieve_request_init(
		mb->send, handle, sender, receivers, receiver_count, 0, flags,
		memory_type, memory_cacheability, FFA_MEMORY_INNER_SHAREABLE);

	ret = ffa_mem_retrieve_req(descriptor_size, descriptor_size);

	if (ffa_func_id(ret) != FFA_MEM_RETRIEVE_RESP) {
		ERROR("%s: couldn't retrieve the memory page. Error: %s\n",
		      __func__, ffa_error_name(ffa_error_code(ret)));
		return false;
	}

	/*
	 * Following total_size and fragment_size are useful to keep track
	 * of the state of transaction. When the sum of all fragment_size of all
	 * fragments is equal to total_size, the memory transaction has been
	 * completed.
	 * This is a simple test with only one segment. As such, upon
	 * successful ffa_mem_retrieve_req, total_size must be equal to
	 * fragment_size.
	 */
	total_size = ret.arg1;
	fragment_size = ret.arg2;

	if (total_size != fragment_size) {
		ERROR("Only expect one memory segment to be sent!\n");
		return false;
	}

	if (fragment_size > PAGE_SIZE) {
		ERROR("Fragment should be smaller than RX buffer!\n");
		return false;
	}

	*retrieved = (struct ffa_memory_region *)mb->recv;

	if ((*retrieved)->receiver_count > MAX_MEM_SHARE_RECIPIENTS) {
		VERBOSE("SPMC memory sharing operations support max of %u "
			"receivers!\n", MAX_MEM_SHARE_RECIPIENTS);
		return false;
	}

	VERBOSE("Memory Retrieved!\n");

	return true;
}

/**
 * Looping part of the fragmented retrieve request.
 */
bool hypervisor_retrieve_request_continue(
	struct mailbox_buffers *mb, uint64_t handle, void *out, uint32_t out_size,
	uint32_t total_size, uint32_t fragment_offset, bool release_rx)
{
	struct ffa_value ret;
	uint32_t fragment_size;

	if (mb == NULL) {
		ERROR("Invalid parameters, please provide valid mailbox.\n");
		return false;
	}

	while (fragment_offset < total_size) {
		VERBOSE("Calling again. frag offset: %d; total: %d\n",
			fragment_offset, total_size);

		/* The first time it is called is controlled through arguments. */
		if (release_rx) {
			ret = ffa_rx_release();
			if (ret.fid != FFA_SUCCESS_SMC32) {
				ERROR("ffa_rx_release() failed: %d\n",
				      ffa_error_code(ret));
				return false;
			}
		} else {
			release_rx = true;
		}

		ret = ffa_mem_frag_rx(handle, fragment_offset);
		if (ret.fid != FFA_MEM_FRAG_TX) {
			ERROR("ffa_mem_frag_rx() failed: %d\n",
			      ffa_error_code(ret));
			return false;
		}

		if (ffa_frag_handle(ret) != handle) {
			ERROR("%s: fragment handle mismatch: expected %llu, got "
			      "%llu\n",
			      __func__, handle, ffa_frag_handle(ret));
			return false;
		}

		/* Sender MBZ at physical instance. */
		if (ffa_frag_sender(ret) != 0) {
			ERROR("%s: fragment sender mismatch: expected %d, got "
			      "%d\n",
			      __func__, 0, ffa_frag_sender(ret));
			return false;
		}

		fragment_size = ffa_mem_frag_tx_frag_size(ret);

		if (fragment_size == 0) {
			ERROR("%s: fragment size must not be 0\n", __func__);
			return false;
		}

		if (out != NULL) {
			if (fragment_offset + fragment_size > out_size) {
				ERROR("%s: fragment is too big to fit in out buffer "
				      "(%d > %d)\n",
				      __func__, fragment_offset + fragment_size,
				      out_size);
				return false;
			}

			VERBOSE("Copying fragment at offset %d with size %d\n",
				fragment_offset, fragment_size);
			memcpy((uint8_t *)out + fragment_offset, mb->recv,
			       fragment_size);
		}

		fragment_offset += fragment_size;
	}

	if (fragment_offset != total_size) {
		ERROR("%s: fragment size mismatch: expected %d, got %d\n",
		      __func__, total_size, fragment_offset);
		return false;
	}

	ret = ffa_rx_release();
	if (ret.fid != FFA_SUCCESS_SMC32) {
		ERROR("ffa_rx_release() failed: %d\n", ffa_error_code(ret));
		return false;
	}

	return true;
}

bool hypervisor_retrieve_request(struct mailbox_buffers *mb, uint64_t handle,
				 void *out, uint32_t out_size)
{
	struct ffa_value ret;
	uint32_t total_size;
	uint32_t fragment_size;
	uint32_t fragment_offset;
	struct ffa_memory_region *region_out = out;

	if (mb == NULL) {
		ERROR("Invalid parameters, please provide valid mailbox.\n");
		return false;
	}

	ffa_hypervisor_retrieve_request_init(mb->send, handle);
	ret = ffa_mem_retrieve_req(sizeof(struct ffa_memory_region),
				   sizeof(struct ffa_memory_region));

	if (ffa_func_id(ret) != FFA_MEM_RETRIEVE_RESP) {
		ERROR("%s: couldn't retrieve the memory page. Error: %d\n",
		      __func__, ffa_error_code(ret));
		return false;
	}

	/*
	 * Following total_size and fragment_size are useful to keep track
	 * of the state of transaction. When the sum of all fragment_size of all
	 * fragments is equal to total_size, the memory transaction has been
	 * completed.
	 */
	total_size = ffa_mem_retrieve_res_total_size(ret);
	fragment_size = ffa_mem_retrieve_res_frag_size(ret);

	fragment_offset = fragment_size;
	VERBOSE("total_size=%d, fragment_size=%d, fragment_offset=%d\n",
		total_size, fragment_size, fragment_offset);

	if (out != NULL) {
		if (fragment_size > PAGE_SIZE) {
			ERROR("Fragment should be smaller than RX buffer!\n");
			return false;
		}
		if (total_size > out_size) {
			ERROR("Output buffer is not large enough to store all "
			      "fragments (total_size=%d, max_size=%d)\n",
			      total_size, out_size);
			return false;
		}

		/*
		 * Copy the received message to the out buffer. This is necessary
		 * because `mb->recv` will be overwritten if sending a fragmented
		 * message.
		 */
		memcpy(out, mb->recv, fragment_size);

		if (region_out->receiver_count == 0) {
			VERBOSE("Copied region has no recivers\n");
			return false;
		}

		if (region_out->receiver_count > MAX_MEM_SHARE_RECIPIENTS) {
			VERBOSE("SPMC memory sharing operations support max of %u "
				"receivers!\n",
				MAX_MEM_SHARE_RECIPIENTS);
			return false;
		}
	} else {
		VERBOSE("%s: No output buffer provided...\n", __func__);
	}

	return hypervisor_retrieve_request_continue(
			mb, handle, out, out_size, total_size, fragment_offset, false);
}

bool memory_relinquish(struct ffa_mem_relinquish *m, uint64_t handle,
		       ffa_id_t id)
{
	struct ffa_value ret;

	ffa_mem_relinquish_init(m, handle, 0, id);
	ret = ffa_mem_relinquish();
	if (ffa_func_id(ret) != FFA_SUCCESS_SMC32) {
		ERROR("%s failed to relinquish memory! error: %s\n", __func__,
		      ffa_error_name(ffa_error_code(ret)));
		return false;
	}

	VERBOSE("Memory Relinquished!\n");
	return true;
}

bool send_fragmented_memory_region(
	void *send_buffer,
	const struct ffa_memory_region_constituent constituents[],
	uint32_t constituent_count, uint32_t remaining_constituent_count,
	uint32_t sent_length, uint32_t total_length, bool allocator_is_spmc,
	struct ffa_value *ret)
{
	uint64_t handle;
	uint64_t handle_mask;
	uint64_t expected_handle_mask =
		allocator_is_spmc ? FFA_MEMORY_HANDLE_ALLOCATOR_SPMC
				  : FFA_MEMORY_HANDLE_ALLOCATOR_HYPERVISOR;
	ffa_memory_handle_t fragment_handle = FFA_MEMORY_HANDLE_INVALID;
	uint32_t fragment_length;

	/* Send the remaining fragments. */
	while (remaining_constituent_count != 0) {
		VERBOSE("%s: %d constituents left to send.\n", __func__,
			remaining_constituent_count);
		if (ret->fid != FFA_MEM_FRAG_RX) {
			ERROR("ffa_mem_frax_tx() failed: %d\n",
			      ffa_error_code(*ret));
			return false;
		}

		if (fragment_handle == FFA_MEMORY_HANDLE_INVALID) {
			fragment_handle = ffa_frag_handle(*ret);
		} else if (ffa_frag_handle(*ret) != fragment_handle) {
			ERROR("%s: fragment handle mismatch: expected %llu, got %llu\n",
			      __func__, fragment_handle, ffa_frag_handle(*ret));
			return false;
		}

		if (ret->arg3 != sent_length) {
			ERROR("%s: fragment length mismatch: expected %u, got "
			      "%lu\n",
			      __func__, sent_length, ret->arg3);
			return false;
		}

		remaining_constituent_count = ffa_memory_fragment_init(
			send_buffer, PAGE_SIZE,
			constituents + constituent_count -
				remaining_constituent_count,
			remaining_constituent_count, &fragment_length);

		*ret = ffa_mem_frag_tx(fragment_handle, fragment_length);
		sent_length += fragment_length;
	}

	if (sent_length != total_length) {
		ERROR("%s: fragment length mismatch: expected %u, got %u\n",
		      __func__, total_length, sent_length);
		return false;
	}

	if (ret->fid != FFA_SUCCESS_SMC32) {
		ERROR("%s: ffa_mem_frax_tx() failed: %d\n", __func__,
		      ffa_error_code(*ret));
		return false;
	}

	handle = ffa_mem_success_handle(*ret);
	handle_mask = (handle >> FFA_MEMORY_HANDLE_ALLOCATOR_SHIFT) &
		      FFA_MEMORY_HANDLE_ALLOCATOR_MASK;

	if (handle_mask != expected_handle_mask) {
		ERROR("%s: handle mask mismatch: expected %llu, got %llu\n",
		      __func__, expected_handle_mask, handle_mask);
		return false;
	}

	if (fragment_handle != FFA_MEMORY_HANDLE_INVALID && handle != fragment_handle) {
		ERROR("%s: fragment handle mismatch: expectd %d, got %llu\n",
		      __func__, fragment_length, handle);
		return false;
	}

	return true;
}

/**
 * Helper to call memory send function whose func id is passed as a parameter.
 */
ffa_memory_handle_t memory_send(
	void *send_buffer, uint32_t mem_func,
	const struct ffa_memory_region_constituent *constituents,
	uint32_t constituent_count, uint32_t remaining_constituent_count,
	uint32_t fragment_length, uint32_t total_length,
	struct ffa_value *ret)
{
	if (remaining_constituent_count == 0 && fragment_length != total_length) {
		ERROR("%s: fragment_length and total_length need "
		      "to be equal (fragment_length = %d, total_length = %d)\n",
		      __func__, fragment_length, total_length);
		return FFA_MEMORY_HANDLE_INVALID;
	}

	switch (mem_func) {
	case FFA_MEM_SHARE_SMC64:
		*ret = ffa_mem_share(total_length, fragment_length);
		break;
	case FFA_MEM_LEND_SMC64:
		*ret = ffa_mem_lend(total_length, fragment_length);
		break;
	case FFA_MEM_DONATE_SMC64:
		*ret = ffa_mem_donate(total_length, fragment_length);
		break;
	default:
		ERROR("%s: Invalid func id %x!\n", __func__, mem_func);
		return FFA_MEMORY_HANDLE_INVALID;
	}

	if (is_ffa_call_error(*ret)) {
		VERBOSE("%s: Failed to send memory: %d\n", __func__,
			ffa_error_code(*ret));
		return FFA_MEMORY_HANDLE_INVALID;
	}

	if (!send_fragmented_memory_region(
		    send_buffer, constituents, constituent_count,
		    remaining_constituent_count, fragment_length, total_length,
		    true, ret)) {
		return FFA_MEMORY_HANDLE_INVALID;
	}

	return ffa_mem_success_handle(*ret);
}

/**
 * Helper that initializes and sends a memory region. The memory region's
 * configuration is statically defined and is implementation specific. However,
 * doing it in this file for simplicity and for testing purposes.
 */
ffa_memory_handle_t memory_init_and_send(
	void *send_buffer, size_t memory_region_max_size, ffa_id_t sender,
	struct ffa_memory_access receivers[], uint32_t receiver_count,
	const struct ffa_memory_region_constituent *constituents,
	uint32_t constituents_count, uint32_t mem_func, struct ffa_value *ret)
{
	uint32_t remaining_constituent_count;
	uint32_t total_length;
	uint32_t fragment_length;

	enum ffa_memory_type type =
		(receiver_count == 1 && mem_func != FFA_MEM_SHARE_SMC64)
			? FFA_MEMORY_NOT_SPECIFIED_MEM
			: FFA_MEMORY_NORMAL_MEM;

	remaining_constituent_count = ffa_memory_region_init(
		send_buffer, memory_region_max_size, sender, receivers,
		receiver_count, constituents, constituents_count, 0, 0, type,
		FFA_MEMORY_CACHE_WRITE_BACK, FFA_MEMORY_INNER_SHAREABLE,
		&total_length, &fragment_length);

	return memory_send(send_buffer, mem_func, constituents,
			   constituents_count, remaining_constituent_count,
			   fragment_length, total_length, ret);
}

static bool ffa_uuid_equal(const struct ffa_uuid uuid1,
			   const struct ffa_uuid uuid2)
{
	return (uuid1.uuid[0] == uuid2.uuid[0]) &&
	       (uuid1.uuid[1] == uuid2.uuid[1]) &&
	       (uuid1.uuid[2] == uuid2.uuid[2]) &&
	       (uuid1.uuid[3] == uuid2.uuid[3]);
}

static bool ffa_partition_info_regs_get_part_info(
	struct ffa_value *args, uint8_t idx,
	struct ffa_partition_info *partition_info)
{
	/*
	 * The list of pointers to args in return value: arg0/func encodes ff-a
	 * function, arg1 is reserved, arg2 encodes indices. arg3 and greater
	 * values reflect partition properties.
	 */
	uint64_t *arg_ptrs = (uint64_t *)args + ((idx * 3) + 3);
	uint64_t info, uuid_lo, uuid_high;

	/*
	 * Each partition information is encoded in 3 registers, so there can be
	 * a maximum of 5 entries.
	 */
	if (idx >= 5 || !partition_info) {
		return false;
	}

	info = *arg_ptrs;

	arg_ptrs++;
	uuid_lo = *arg_ptrs;

	arg_ptrs++;
	uuid_high = *arg_ptrs;

	/*
	 * As defined in FF-A 1.2 ALP0, 14.9 FFA_PARTITION_INFO_GET_REGS.
	 */
	partition_info->id = info & 0xFFFFU;
	partition_info->exec_context = (info >> 16) & 0xFFFFU;
	partition_info->properties = (info >> 32);
	partition_info->uuid.uuid[0] = uuid_lo & 0xFFFFFFFFU;
	partition_info->uuid.uuid[1] = (uuid_lo >> 32) & 0xFFFFFFFFU;
	partition_info->uuid.uuid[2] = uuid_high & 0xFFFFFFFFU;
	partition_info->uuid.uuid[3] = (uuid_high >> 32) & 0xFFFFFFFFU;

	return true;
}

static bool ffa_compare_partition_info(
		const struct ffa_uuid uuid,
		const struct ffa_partition_info *info,
		const struct ffa_partition_info *expected)
{
	bool result = true;
	/*
	 * If a UUID is specified then the UUID returned in the
	 * partition info descriptor MBZ.
	 */
	struct ffa_uuid expected_uuid =
		ffa_uuid_equal(uuid, NULL_UUID) ? expected->uuid : NULL_UUID;

	if (info->id != expected->id) {
		ERROR("Wrong ID. Expected %x, got %x\n", expected->id, info->id);
		result = false;
	}

	if (info->exec_context != expected->exec_context) {
		ERROR("Wrong context. Expected %x, got %x\n",
		      expected->exec_context,
		      info->exec_context);
		result = false;
	}
	if (info->properties != expected->properties) {
		ERROR("Wrong properties. Expected %x, got %x\n",
		      expected->properties,
		      info->properties);
		result = false;
	}

	if (!ffa_uuid_equal(info->uuid, expected_uuid)) {
		ERROR("Wrong UUID. Expected %x %x %x %x, "
		      "got %x %x %x %x\n",
		      expected_uuid.uuid[0],
		      expected_uuid.uuid[1],
		      expected_uuid.uuid[2],
		      expected_uuid.uuid[3],
		      info->uuid.uuid[0],
		      info->uuid.uuid[1],
		      info->uuid.uuid[2],
		      info->uuid.uuid[3]);
		result = false;
	}

	return result;
}

/**
 * Sends a ffa_partition_info_get_regs request and returns the information
 * returned in registers in the output parameters. Validation against
 * expected results shall be done by the caller outside the function.
 */
bool ffa_partition_info_regs_helper(const struct ffa_uuid uuid,
		       const struct ffa_partition_info *expected,
		       const uint16_t expected_size)
{
	/*
	 * TODO: For now, support only one invocation. Can be enhanced easily
	 * to extend to arbitrary number of partitions.
	 */
	if (expected_size > 5) {
		ERROR("%s only supports information received in"
			" one invocation of the ABI (5 partitions)\n",
			__func__);
		return false;
	}

	struct ffa_value ret = ffa_partition_info_get_regs(uuid, 0, 0);

	if (ffa_func_id(ret) != FFA_SUCCESS_SMC64) {
		return false;
	}

	if (ffa_partition_info_regs_partition_count(ret) !=
	    expected_size) {
		ERROR("Unexpected number of partitions %d (expected %d)\n",
		      ffa_partition_info_regs_partition_count(ret),
		      expected_size);
		return false;
	}

	if (ffa_partition_info_regs_entry_size(ret) !=
	    sizeof(struct ffa_partition_info)) {
		ERROR("Unexpected partition info descriptor size %d\n",
		      ffa_partition_info_regs_entry_size(ret));
		return false;
	}

	for (unsigned int i = 0U; i < expected_size; i++) {
		struct ffa_partition_info info = { 0 };

		ffa_partition_info_regs_get_part_info(&ret, i, &info);
		if (!ffa_compare_partition_info(uuid, &info, &expected[i])) {
			return false;
		}
	}

	return true;
}

/**
 * Sends a ffa_partition_info request and checks the response against the
 * target.
 */
bool ffa_partition_info_helper(struct mailbox_buffers *mb,
			       const struct ffa_uuid uuid,
			       const struct ffa_partition_info *expected,
			       const uint16_t expected_size)
{
	bool result = true;
	struct ffa_value ret = ffa_partition_info_get(uuid);

	if (ffa_func_id(ret) == FFA_SUCCESS_SMC32) {
		if (ffa_partition_info_count(ret) != expected_size) {
			ERROR("Unexpected number of partitions %d\n",
			      ffa_partition_info_count(ret));
			return false;
		}
		if (ffa_partition_info_desc_size(ret) !=
		    sizeof(struct ffa_partition_info)) {
			ERROR("Unexpected partition info descriptor size %d\n",
			      ffa_partition_info_desc_size(ret));
			return false;
		}
		const struct ffa_partition_info *info =
			(const struct ffa_partition_info *)(mb->recv);

		for (unsigned int i = 0U; i < expected_size; i++) {
			if (!ffa_compare_partition_info(uuid, &info[i], &expected[i]))
				result = false;
		}
	}

	ret = ffa_rx_release();
	if (is_ffa_call_error(ret)) {
		ERROR("Failed to release RX buffer\n");
		result = false;
	}
	return result;
}

static bool configure_trusted_wdog_interrupt(ffa_id_t source, ffa_id_t dest,
				bool enable)
{
	struct ffa_value ret_values;

	ret_values = cactus_interrupt_cmd(source, dest, IRQ_TWDOG_INTID,
					  enable, INTERRUPT_TYPE_IRQ);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response message while configuring"
			" TWDOG interrupt\n");
		return false;
	}

	if (cactus_get_response(ret_values) != CACTUS_SUCCESS) {
		ERROR("Failed to configure Trusted Watchdog interrupt\n");
		return false;
	}
	return true;
}

bool enable_trusted_wdog_interrupt(ffa_id_t source, ffa_id_t dest)
{
	return configure_trusted_wdog_interrupt(source, dest, true);
}

bool disable_trusted_wdog_interrupt(ffa_id_t source, ffa_id_t dest)
{
	return configure_trusted_wdog_interrupt(source, dest, false);
}

/**
 * Initializes receiver permissions in a memory transaction descriptor, using
 * `mem_func` to determine the appropriate permissions.
 */
struct ffa_memory_access ffa_memory_access_init_permissions_from_mem_func(
	ffa_id_t receiver_id, uint32_t mem_func)
{

	enum ffa_instruction_access instruction_access =
		FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED;
	enum ffa_data_access data_access =
		(mem_func == FFA_MEM_DONATE_SMC64)
			? FFA_DATA_ACCESS_NOT_SPECIFIED
			: FFA_DATA_ACCESS_RW;

	return ffa_memory_access_init(receiver_id, data_access,
				      instruction_access, 0, NULL);
}

/**
 * Receives message from the mailbox, copies it into the 'buffer', gets the
 * pending framework notifications and releases the RX buffer.
 * Returns false if it fails to copy the message to `buffer`, or true
 * otherwise.
 */
bool receive_indirect_message(void *buffer, size_t buffer_size, void *recv,
			      ffa_id_t *sender, ffa_id_t receiver, ffa_id_t own_id)
{
	const struct ffa_partition_msg *message;
	struct ffa_partition_rxtx_header header;
	ffa_id_t source_vm_id;
	const uint32_t *payload;
	struct ffa_value ret;
	ffa_notification_bitmap_t fwk_notif;

	if (buffer_size > FFA_MSG_PAYLOAD_MAX) {
		return false;
	}

	ret = ffa_notification_get(receiver, 0,
				   FFA_NOTIFICATIONS_FLAG_BITMAP_SPM);
	if (is_ffa_call_error(ret)) {
		return false;
	}

	fwk_notif = ffa_notification_get_from_framework(ret);

	if (fwk_notif == 0U) {
		ERROR("Expected Rx buffer full notification.");
		return false;
	}

	message = (const struct ffa_partition_msg *)recv;
	memcpy(&header, message, sizeof(struct ffa_partition_rxtx_header));

	source_vm_id = header.sender;

	if (is_ffa_spm_buffer_full_notification(fwk_notif)) {
		/*
		 * Expect the sender to always have been an SP.
		 */
		assert(IS_SP_ID(source_vm_id));
	}

	if (header.size > buffer_size) {
		ERROR("Error in rxtx header. Message size: %#x; "
		      "buffer size: %lu\n",
		      header.size, buffer_size);
		return false;
	}

	payload = (const uint32_t *)message->payload;

	/* Get message to free the RX buffer. */
	memcpy(buffer, payload, header.size);

	/* Check receiver ID against own ID. */
	if (receiver != own_id) {
		ret = ffa_rx_release_with_id(receiver);
	} else {
		ret = ffa_rx_release();
	}

	if (is_ffa_call_error(ret)) {
		ERROR("Failed to release the rx buffer\n");
		return false;
	}

	if (receiver != header.receiver) {
		ERROR("Header receiver: %x different than expected receiver: %x\n",
		      header.receiver, receiver);
		return false;
	}

	if (sender != NULL) {
		*sender = source_vm_id;
	}

	return true;
}

/**
 * Sends an indirect message: initializes the `ffa_rxtx_header`, copies the
 * payload to the TX buffer.
 * Uses the `send_flags` if any are provided in the call to FFA_MSG_SEND2.
 */
struct ffa_value send_indirect_message(
		ffa_id_t from, ffa_id_t to, void *send, const void *payload,
		size_t payload_size, uint32_t send_flags)
{
	struct ffa_partition_msg *message = (struct ffa_partition_msg *)send;

	/* Initialize message header. */
	ffa_rxtx_header_init(from, to, payload_size, &message->header);

	/* Fill TX buffer with payload. */
	memcpy(message->payload, payload, payload_size);

	/* Send the message. */
	return ffa_msg_send2(send_flags);
}
