/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>

#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <ffa_svc.h>
#include <smccc.h>

struct ffa_value ffa_service_call(struct ffa_value *args)
{
#if IMAGE_IVY
	ffa_svc(args);
#else
	ffa_smc(args);
#endif
	return *args;
}

/*-----------------------------------------------------------------------------
 * FFA_RUN
 *
 * Parameters
 *     uint32 Function ID (w0): 0x8400006D
 *     uint32 Target information (w1): Information to identify target SP/VM
 *         -Bits[31:16]: ID of SP/VM.
 *         -Bits[15:0]: ID of vCPU of SP/VM to run.
 *     Other Parameter registers w2-w7/x2-x7: Reserved (MBZ)
 *
 * On failure, returns FFA_ERROR in w0 and error code in w2:
 *     -INVALID_PARAMETERS: Unrecognized endpoint or vCPU ID
 *     -NOT_SUPPORTED: This function is not implemented at this FFA instance
 *     -DENIED: Callee is not in a state to handle this request
 *     -BUSY: vCPU is busy and caller must retry later
 *     -ABORTED: vCPU or VM ran into an unexpected error and has aborted
 */
struct ffa_value ffa_run(uint32_t dest_id, uint32_t vcpu_id)
{
	struct ffa_value args = {
		FFA_RUN,
		(dest_id << 16) | vcpu_id,
		0, 0, 0, 0, 0, 0
	};

	return ffa_service_call(&args);
}

/*-----------------------------------------------------------------------------
 * FFA_MSG_SEND_DIRECT_REQ
 *
 * Parameters
 *     uint32 Function ID (w0): 0x8400006F / 0xC400006F
 *     uint32 Source/Destination IDs (w1): Source and destination endpoint IDs
 *         -Bit[31:16]: Source endpoint ID
 *         -Bit[15:0]: Destination endpoint ID
 *     uint32/uint64 (w2/x2) - RFU MBZ
 *     w3-w7 - Implementation defined
 *
 * On failure, returns FFA_ERROR in w0 and error code in w2:
 *     -INVALID_PARAMETERS: Invalid endpoint ID or non-zero reserved register
 *     -DENIED: Callee is not in a state to handle this request
 *     -NOT_SUPPORTED: This function is not implemented at this FFA instance
 *     -BUSY: Message target is busy
 *     -ABORTED: Message target ran into an unexpected error and has aborted
 */
struct ffa_value ffa_msg_send_direct_req64(ffa_id_t source_id,
					   ffa_id_t dest_id, uint64_t arg0,
					   uint64_t arg1, uint64_t arg2,
					   uint64_t arg3, uint64_t arg4)
{
	struct ffa_value args = {
		.fid = FFA_MSG_SEND_DIRECT_REQ_SMC64,
		.arg1 = ((uint32_t)(source_id << 16)) | (dest_id),
		.arg2 = 0,
		.arg3 = arg0,
		.arg4 = arg1,
		.arg5 = arg2,
		.arg6 = arg3,
		.arg7 = arg4,
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_msg_send_direct_req32(ffa_id_t source_id,
					   ffa_id_t dest_id, uint32_t arg0,
					   uint32_t arg1, uint32_t arg2,
					   uint32_t arg3, uint32_t arg4)
{
	struct ffa_value args = {
		.fid = FFA_MSG_SEND_DIRECT_REQ_SMC32,
		.arg1 = ((uint32_t)(source_id << 16)) | (dest_id),
		.arg2 = 0,
		.arg3 = arg0,
		.arg4 = arg1,
		.arg5 = arg2,
		.arg6 = arg3,
		.arg7 = arg4,
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_msg_send_direct_resp64(ffa_id_t source_id,
					    ffa_id_t dest_id, uint64_t arg0,
					    uint64_t arg1, uint64_t arg2,
					    uint64_t arg3, uint64_t arg4)
{
	struct ffa_value args = {
		.fid = FFA_MSG_SEND_DIRECT_RESP_SMC64,
		.arg1 = ((uint32_t)(source_id << 16)) | (dest_id),
		.arg2 = 0,
		.arg3 = arg0,
		.arg4 = arg1,
		.arg5 = arg2,
		.arg6 = arg3,
		.arg7 = arg4,
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_msg_send_direct_resp32(ffa_id_t source_id,
					    ffa_id_t dest_id, uint32_t arg0,
					    uint32_t arg1, uint32_t arg2,
					    uint32_t arg3, uint32_t arg4)
{
	struct ffa_value args = {
		.fid = FFA_MSG_SEND_DIRECT_RESP_SMC32,
		.arg1 = ((uint32_t)(source_id << 16)) | (dest_id),
		.arg2 = 0,
		.arg3 = arg0,
		.arg4 = arg1,
		.arg5 = arg2,
		.arg6 = arg3,
		.arg7 = arg4,
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_framework_msg_send_direct_resp(ffa_id_t source_id,
					    ffa_id_t dest_id, uint32_t msg,
					    uint32_t status_code)
{
	struct ffa_value args = {
		.fid = FFA_MSG_SEND_DIRECT_RESP_SMC32,
		.arg1 = ((uint32_t)(source_id << 16)) | (dest_id),
		.arg2 = ((uint32_t)(1 << 31)) | (msg & 0xFF),
		.arg3 = status_code,
	};

	return ffa_service_call(&args);
}

void ffa_memory_region_init_header(struct ffa_memory_region *memory_region,
				   ffa_id_t sender,
				   ffa_memory_attributes_t attributes,
				   ffa_memory_region_flags_t flags,
				   ffa_memory_handle_t handle, uint32_t tag,
				   uint32_t receiver_count)
{
	memory_region->sender = sender;
	memory_region->attributes = attributes;
	memory_region->flags = flags;
	memory_region->handle = handle;
	memory_region->tag = tag;
	memory_region->memory_access_desc_size =
		sizeof(struct ffa_memory_access);
	memory_region->receiver_count = receiver_count;
	memory_region->receivers_offset =
		offsetof(struct ffa_memory_region, receivers);
	memset(memory_region->reserved, 0, sizeof(memory_region->reserved));
}

/**
 * Copies as many as possible of the given constituents to the respective
 * memory region and sets the respective offset.
 *
 * Returns the number of constituents remaining which wouldn't fit, and (via
 * return parameters) the size in bytes of the first fragment of data copied to
 * `memory_region` (attributes, constituents and memory region header size), and
 * the total size of the memory sharing message including all constituents.
 */
static uint32_t ffa_memory_region_init_constituents(
	struct ffa_memory_region *memory_region, size_t memory_region_max_size,
	const struct ffa_memory_region_constituent constituents[],
	uint32_t constituent_count, uint32_t *total_length,
	uint32_t *fragment_length)
{
	struct ffa_composite_memory_region *composite_memory_region;
	uint32_t fragment_max_constituents;
	uint32_t constituents_offset;
	uint32_t count_to_copy;

	/*
	 * Note that `sizeof(struct_ffa_memory_region)` and `sizeof(struct
	 * ffa_memory_access)` must both be multiples of 16 (as verified by the
	 * asserts in `ffa_memory.c`, so it is guaranteed that the offset we
	 * calculate here is aligned to a 64-bit boundary and so 64-bit values
	 * can be copied without alignment faults.
	 * If there are multiple receiver endpoints, their respective access
	 * structure should point to the same offset value.
	 */
	for (uint32_t i = 0; i < memory_region->receiver_count; i++) {
		memory_region->receivers[i].composite_memory_region_offset =
			sizeof(struct ffa_memory_region) +
			memory_region->receiver_count *
				sizeof(struct ffa_memory_access);
	}

	composite_memory_region =
		ffa_memory_region_get_composite(memory_region, 0);
	composite_memory_region->page_count = 0;
	composite_memory_region->constituent_count = constituent_count;
	composite_memory_region->reserved_0 = 0;

	constituents_offset =
		memory_region->receivers[0].composite_memory_region_offset +
		sizeof(struct ffa_composite_memory_region);
	fragment_max_constituents =
		(memory_region_max_size - constituents_offset) /
		sizeof(struct ffa_memory_region_constituent);

	count_to_copy = constituent_count;
	if (count_to_copy > fragment_max_constituents) {
		count_to_copy = fragment_max_constituents;
	}

	for (uint32_t i = 0; i < constituent_count; ++i) {
		if (i < count_to_copy) {
			composite_memory_region->constituents[i] =
				constituents[i];
		}
		composite_memory_region->page_count +=
			constituents[i].page_count;
	}

	if (total_length != NULL) {
		*total_length =
			constituents_offset +
			composite_memory_region->constituent_count *
				sizeof(struct ffa_memory_region_constituent);
	}
	if (fragment_length != NULL) {
		*fragment_length =
			constituents_offset +
			count_to_copy *
				sizeof(struct ffa_memory_region_constituent);
	}

	return composite_memory_region->constituent_count - count_to_copy;
}

/**
 * Initialises the given `ffa_memory_region` to be used for an
 * `FFA_MEM_RETRIEVE_REQ` by the receiver of a memory transaction.
 * Initialises the given `ffa_memory_region` and copies as many as possible of
 * the given constituents to it.
 *
 * Returns the number of constituents remaining which wouldn't fit, and (via
 * return parameters) the size in bytes of the first fragment of data copied to
 * `memory_region` (attributes, constituents and memory region header size), and
 * the total size of the memory sharing message including all constituents.
 */
uint32_t ffa_memory_region_init(
	struct ffa_memory_region *memory_region, size_t memory_region_max_size,
	ffa_id_t sender, struct ffa_memory_access receivers[],
	uint32_t receiver_count,
	const struct ffa_memory_region_constituent constituents[],
	uint32_t constituent_count, uint32_t tag,
	ffa_memory_region_flags_t flags, enum ffa_memory_type type,
	enum ffa_memory_cacheability cacheability,
	enum ffa_memory_shareability shareability, uint32_t *total_length,
	uint32_t *fragment_length)
{
	ffa_memory_attributes_t attributes = {
		.type = type,
		.cacheability = cacheability,
		.shareability = shareability,
	};

	ffa_memory_region_init_header(memory_region, sender, attributes, flags,
				      0, tag, receiver_count);

	memcpy(memory_region->receivers, receivers,
	       receiver_count * sizeof(struct ffa_memory_access));

	return ffa_memory_region_init_constituents(
		memory_region, memory_region_max_size, constituents,
		constituent_count, total_length, fragment_length);
}

uint32_t ffa_memory_fragment_init(
	struct ffa_memory_region_constituent *fragment,
	size_t fragment_max_size,
	const struct ffa_memory_region_constituent constituents[],
	uint32_t constituent_count, uint32_t *fragment_length)
{
	const uint32_t fragment_max_constituents =
		fragment_max_size /
		sizeof(struct ffa_memory_region_constituent);

	uint32_t count_to_copy =
		MIN(constituent_count, fragment_max_constituents);

	for (uint32_t i = 0; i < count_to_copy; ++i) {
		fragment[i] = constituents[i];
	}

	if (fragment_length != NULL) {
		*fragment_length = count_to_copy *
				   sizeof(struct ffa_memory_region_constituent);
	}

	return constituent_count - count_to_copy;
}

/**
 * Initialises the given `ffa_memory_region` to be used for an
 * `FFA_MEM_RETRIEVE_REQ` by the receiver of a memory transaction.
 *
 * Returns the size of the message written.
 */
uint32_t ffa_memory_retrieve_request_init(
	struct ffa_memory_region *memory_region, ffa_memory_handle_t handle,
	ffa_id_t sender, struct ffa_memory_access receivers[],
	uint32_t receiver_count, uint32_t tag, ffa_memory_region_flags_t flags,
	enum ffa_memory_type type, enum ffa_memory_cacheability cacheability,
	enum ffa_memory_shareability shareability)
{
	ffa_memory_attributes_t attributes = {
		.type = type,
		.cacheability = cacheability,
		.shareability = shareability,
	};

	ffa_memory_region_init_header(memory_region, sender, attributes, flags,
				      handle, tag, receiver_count);

	memcpy(memory_region->receivers, receivers,
	       receiver_count * sizeof(struct ffa_memory_access));

	/*
	 * Offset 0 in this case means that the hypervisor should allocate the
	 * address ranges. This is the only configuration supported by Hafnium,
	 * as it enforces 1:1 mappings in the stage 2 page tables.
	 */
	for (uint32_t i = 0; i < receiver_count; i++) {
		memory_region->receivers[i].composite_memory_region_offset = 0;
		memory_region->receivers[i].reserved_0 = 0;
	}

	return sizeof(struct ffa_memory_region) +
	       memory_region->receiver_count * sizeof(struct ffa_memory_access);
}

/**
 * Configure `region` for a hypervisor retrieve request - i.e. all fields except
 * `handle` are initialized to 0.
 */
void ffa_hypervisor_retrieve_request_init(struct ffa_memory_region *region,
					  ffa_memory_handle_t handle)
{
	memset(region, 0, sizeof(struct ffa_memory_region));
	region->handle = handle;
}

/*
 * FFA Version ABI helper.
 * Version fields:
 *	-Bits[30:16]: Major version.
 *	-Bits[15:0]: Minor version.
 */
struct ffa_value ffa_version(uint32_t input_version)
{
	struct ffa_value args = {
		.fid = FFA_VERSION,
		.arg1 = input_version
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_id_get(void)
{
	struct ffa_value args = {
		.fid = FFA_ID_GET
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_spm_id_get(void)
{
	struct ffa_value args = {
		.fid = FFA_SPM_ID_GET
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_msg_wait(void)
{
	struct ffa_value args = {
		.fid = FFA_MSG_WAIT
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_error(int32_t error_code)
{
	struct ffa_value args = {
		.fid = FFA_ERROR,
		.arg1 = 0,
		.arg2 = error_code
	};

	return ffa_service_call(&args);
}

/* Query the higher EL if the requested FF-A feature is implemented. */
struct ffa_value ffa_features(uint32_t feature)
{
	struct ffa_value args = {
		.fid = FFA_FEATURES,
		.arg1 = feature
	};

	return ffa_service_call(&args);
}

/* Query the higher EL if the requested FF-A feature is implemented. */
struct ffa_value ffa_features_with_input_property(uint32_t feature, uint32_t param)
{
	struct ffa_value args = {
		.fid = FFA_FEATURES,
		.arg1 = feature,
		.arg2 = param,
	};

	return ffa_service_call(&args);
}

/* Get information about VMs or SPs based on UUID, using registers. */
struct ffa_value ffa_partition_info_get_regs(const struct ffa_uuid uuid,
					     const uint16_t start_index,
					     const uint16_t tag)
{
	uint64_t arg1 = (uint64_t)uuid.uuid[1] << 32 | uuid.uuid[0];
	uint64_t arg2 = (uint64_t)uuid.uuid[3] << 32 | uuid.uuid[2];
	uint64_t arg3 = start_index | (uint64_t)tag << 16;

	struct ffa_value args = {
		.fid = FFA_PARTITION_INFO_GET_REGS_SMC64,
		.arg1 = arg1,
		.arg2 = arg2,
		.arg3 = arg3,
	};

	return ffa_service_call(&args);
}

/* Get information about VMs or SPs based on UUID */
struct ffa_value ffa_partition_info_get(const struct ffa_uuid uuid)
{
	struct ffa_value args = {
		.fid = FFA_PARTITION_INFO_GET,
		.arg1 = uuid.uuid[0],
		.arg2 = uuid.uuid[1],
		.arg3 = uuid.uuid[2],
		.arg4 = uuid.uuid[3]
	};

	return ffa_service_call(&args);
}

/* Querying the SPMC to release the rx buffers of the VM ID. */
struct ffa_value ffa_rx_release_with_id(ffa_id_t vm_id)
{
	struct ffa_value args = {
		.fid = FFA_RX_RELEASE,
		.arg1 = (uint64_t)vm_id,
	};

	return ffa_service_call(&args);
}

/* Query SPMC that the rx buffer of the partition can be released */
struct ffa_value ffa_rx_release(void)
{
	return ffa_rx_release_with_id(0);
}

/* Map the RXTX buffer */
struct ffa_value ffa_rxtx_map(uintptr_t send, uintptr_t recv, uint32_t pages)
{
	struct ffa_value args = {
		.fid = FFA_RXTX_MAP_SMC64,
		.arg1 = send,
		.arg2 = recv,
		.arg3 = pages,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

/* Unmap the RXTX buffer allocated by the given FF-A component */
struct ffa_value ffa_rxtx_unmap(void)
{
	return ffa_rxtx_unmap_with_id(0);
}

struct ffa_value ffa_rxtx_unmap_with_id(uint32_t id)
{
	struct ffa_value args = {
		.fid = FFA_RXTX_UNMAP,
		.arg1 = id << 16,
		.arg2 = FFA_PARAM_MBZ,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ,
	};

	return ffa_service_call(&args);
}

/**
 * Copies data from the sender's send buffer to the recipient's receive buffer
 * and notifies the receiver.
 *
 * `flags` may include a 'Delay Schedule Receiver interrupt'.
 *
 * Returns FFA_SUCCESS if the message is sent, or an error code otherwise:
 *  - INVALID_PARAMETERS: one or more of the parameters do not conform.
 *  - BUSY: receiver's mailbox was full.
 *  - DENIED: receiver is not in a state to handle the request or doesn't
 *    support indirect messages.
 */
struct ffa_value ffa_msg_send2_with_id(uint32_t flags, ffa_id_t sender)
{
	struct ffa_value args = {
		.fid = FFA_MSG_SEND2,
		.arg1 = sender << 16,
		.arg2 = flags,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_msg_send2(uint32_t flags)
{
	return ffa_msg_send2_with_id(flags, 0);
}

/* Donate memory to another partition */
struct ffa_value ffa_mem_donate(uint32_t descriptor_length,
				uint32_t fragment_length)
{
	struct ffa_value args = {
		.fid = FFA_MEM_DONATE_SMC64,
		.arg1 = descriptor_length,
		.arg2 = fragment_length,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

/* Lend memory to another partition */
struct ffa_value ffa_mem_lend(uint32_t descriptor_length,
			      uint32_t fragment_length)
{
	struct ffa_value args = {
		.fid = FFA_MEM_LEND_SMC64,
		.arg1 = descriptor_length,
		.arg2 = fragment_length,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

/* Share memory with another partition */
struct ffa_value ffa_mem_share(uint32_t descriptor_length,
			       uint32_t fragment_length)
{
	struct ffa_value args = {
		.fid = FFA_MEM_SHARE_SMC64,
		.arg1 = descriptor_length,
		.arg2 = fragment_length,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

/* Retrieve memory shared by another partition */
struct ffa_value ffa_mem_retrieve_req(uint32_t descriptor_length,
				      uint32_t fragment_length)
{
	struct ffa_value args = {
		.fid = FFA_MEM_RETRIEVE_REQ_SMC64,
		.arg1 = descriptor_length,
		.arg2 = fragment_length,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

/* Relinquish access to memory region */
struct ffa_value ffa_mem_relinquish(void)
{
	struct ffa_value args = {
		.fid = FFA_MEM_RELINQUISH,
	};

	return ffa_service_call(&args);
}

/* Reclaim exclusive access to owned memory region */
struct ffa_value ffa_mem_reclaim(uint64_t handle, uint32_t flags)
{
	struct ffa_value args = {
		.fid = FFA_MEM_RECLAIM,
		.arg1 = (uint32_t) handle,
		.arg2 = (uint32_t) (handle >> 32),
		.arg3 = flags
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_mem_frag_rx(ffa_memory_handle_t handle,
				 uint32_t fragment_offset)
{
	/* Note that sender MBZ at virtual instance. */
	struct ffa_value args = {
		.fid = FFA_MEM_FRAG_RX,
		.arg1 = (uint32_t)handle,
		.arg2 = (uint32_t)(handle >> 32),
		.arg3 = fragment_offset,
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_mem_frag_tx(ffa_memory_handle_t handle,
				 uint32_t fragment_length)
{
	struct ffa_value args = {
		.fid = FFA_MEM_FRAG_TX,
		.arg1 = (uint32_t)handle,
		.arg2 = (uint32_t)(handle >> 32),
		.arg3 = fragment_length,
	};

	/* Note that sender MBZ at virtual instance. */
	return ffa_service_call(&args);
}

/** Create Notifications Bitmap for the given VM */
struct ffa_value ffa_notification_bitmap_create(ffa_id_t vm_id,
						ffa_vcpu_count_t vcpu_count)
{
	struct ffa_value args = {
		.fid = FFA_NOTIFICATION_BITMAP_CREATE,
		.arg1 = vm_id,
		.arg2 = vcpu_count,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ,
	};

	return ffa_service_call(&args);
}

/** Destroy Notifications Bitmap for the given VM */
struct ffa_value ffa_notification_bitmap_destroy(ffa_id_t vm_id)
{
	struct ffa_value args = {
		.fid = FFA_NOTIFICATION_BITMAP_DESTROY,
		.arg1 = vm_id,
		.arg2 = FFA_PARAM_MBZ,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ,
	};

	return ffa_service_call(&args);
}

/** Bind VM to all the notifications in the bitmap */
struct ffa_value ffa_notification_bind(ffa_id_t sender, ffa_id_t receiver,
				       uint32_t flags,
				       ffa_notification_bitmap_t bitmap)
{
	struct ffa_value args = {
		.fid = FFA_NOTIFICATION_BIND,
		.arg1 = (sender << 16) | (receiver),
		.arg2 = flags,
		.arg3 = (uint32_t)(bitmap & 0xFFFFFFFFU),
		.arg4 = (uint32_t)(bitmap >> 32),
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ,
	};

	return ffa_service_call(&args);
}

/** Unbind previously bound VM from notifications in bitmap */
struct ffa_value ffa_notification_unbind(ffa_id_t sender,
					 ffa_id_t receiver,
					 ffa_notification_bitmap_t bitmap)
{
	struct ffa_value args = {
		.fid = FFA_NOTIFICATION_UNBIND,
		.arg1 = (sender << 16) | (receiver),
		.arg2 = FFA_PARAM_MBZ,
		.arg3 = (uint32_t)(bitmap),
		.arg4 = (uint32_t)(bitmap >> 32),
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ,
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_notification_set(ffa_id_t sender, ffa_id_t receiver,
				      uint32_t flags,
				      ffa_notification_bitmap_t bitmap)
{
	struct ffa_value args = {
		.fid = FFA_NOTIFICATION_SET,
		.arg1 = (sender << 16) | (receiver),
		.arg2 = flags,
		.arg3 = (uint32_t)(bitmap & 0xFFFFFFFFU),
		.arg4 = (uint32_t)(bitmap >> 32),
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_notification_get(ffa_id_t receiver, uint32_t vcpu_id,
				      uint32_t flags)
{
	struct ffa_value args = {
		.fid = FFA_NOTIFICATION_GET,
		.arg1 = (vcpu_id << 16) | (receiver),
		.arg2 = flags,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

struct ffa_value ffa_notification_info_get(void)
{
	struct ffa_value args = {
		.fid = FFA_NOTIFICATION_INFO_GET_SMC64,
		.arg1 = FFA_PARAM_MBZ,
		.arg2 = FFA_PARAM_MBZ,
		.arg3 = FFA_PARAM_MBZ,
		.arg4 = FFA_PARAM_MBZ,
		.arg5 = FFA_PARAM_MBZ,
		.arg6 = FFA_PARAM_MBZ,
		.arg7 = FFA_PARAM_MBZ
	};

	return ffa_service_call(&args);
}

static size_t char_to_arg_helper(const char *message, size_t size,
				 u_register_t *arg)
{
	size_t to_write = size > sizeof(uint64_t) ? sizeof(uint64_t) : size;

	for (int i = 0; i < to_write; i++) {
		((char *)arg)[i] = message[i];
	}
	return to_write;
}

struct ffa_value ffa_console_log(const char *message, size_t char_count)
{
	struct ffa_value args = {
		.fid = FFA_CONSOLE_LOG_SMC64,
		.arg1 = char_count,
	};
	size_t written = 0;

	assert(char_count <= sizeof(uint64_t) * 6);

	written += char_to_arg_helper(&message[written], char_count - written,
			&args.arg2);
	written += char_to_arg_helper(&message[written], char_count - written,
			&args.arg3);
	written += char_to_arg_helper(&message[written], char_count - written,
			&args.arg4);
	written += char_to_arg_helper(&message[written], char_count - written,
			&args.arg5);
	written += char_to_arg_helper(&message[written], char_count - written,
			&args.arg6);
	char_to_arg_helper(&message[written], char_count - written,
			&args.arg7);

	return ffa_service_call(&args);
}

/**
 * Initializes receiver permissions in a memory transaction descriptor.
 */
struct ffa_memory_access ffa_memory_access_init(
	ffa_id_t receiver_id, enum ffa_data_access data_access,
	enum ffa_instruction_access instruction_access,
	ffa_memory_receiver_flags_t flags,
	struct ffa_memory_access_impdef *impdef)
{
	struct ffa_memory_access access;
	access.reserved_0 = 0;
	access.composite_memory_region_offset = 0;
	access.receiver_permissions.flags = flags;
	access.receiver_permissions.receiver = receiver_id;
	access.receiver_permissions.permissions.data_access = data_access;
	access.receiver_permissions.permissions.instruction_access =
		instruction_access;
	access.impdef = impdef != NULL ? *impdef :
		(struct ffa_memory_access_impdef){{0, 0}};

	return access;
}

/**
 * Initialises the given `ffa_composite_memory_region` to be used for an
 * `FFA_RXTX_MAP` forwarding in the case when Hypervisor needs the SPMC to map a
 * VM's RXTX pair.
 */
static void
ffa_composite_memory_region_init(struct ffa_composite_memory_region *composite,
				 void *address, uint32_t page_count)
{
	composite->page_count = page_count;
	composite->constituent_count = 1;
	composite->reserved_0 = 0;

	composite->constituents[0].page_count = page_count;
	composite->constituents[0].address = (void *)address;
	composite->constituents[0].reserved = 0;
}

/**
 * Initialises the given `ffa_endpoint_rx_tx_descriptor` to be used for an
 * `FFA_RXTX_MAP` forwarding in the case when Hypervisor needs the SPMC to map a
 * VM's RXTX pair.
 *
 * Each buffer is described by an `ffa_composite_memory_region` containing
 * one `ffa_memory_region_constituent`.
 */
void ffa_endpoint_rxtx_descriptor_init(
	struct ffa_endpoint_rxtx_descriptor *desc, ffa_id_t endpoint_id,
	void *rx_address, void *tx_address)
{
	desc->endpoint_id = endpoint_id;
	desc->reserved = 0;
	desc->pad = 0;

	/*
	 * RX's composite descriptor is allocated after the enpoint descriptor.
	 * `sizeof(struct ffa_endpoint_rx_tx_descriptor)` is guaranteed to be
	 * 16-byte aligned.
	 */
	desc->rx_offset = sizeof(struct ffa_endpoint_rxtx_descriptor);

	ffa_composite_memory_region_init(
		(struct ffa_composite_memory_region *)((uintptr_t)desc +
						       desc->rx_offset),
		rx_address, 1);

	/*
	 * TX's composite descriptor is allocated after the RX descriptor.
	 * `sizeof(struct ffa_composite_memory_region)`  and
	 * `sizeof(struct ffa_memory_region_constituent)` are guaranteed to be
	 * 16-byte aligned in ffa_memory.c.
	 */
	desc->tx_offset = desc->rx_offset +
			  sizeof(struct ffa_composite_memory_region) +
			  sizeof(struct ffa_memory_region_constituent);

	ffa_composite_memory_region_init(
		(struct ffa_composite_memory_region *)((uintptr_t)desc +
						       desc->tx_offset),
		tx_address, 1);
}

/**
 * Mimics a forwarded FFA_RXTX_MAP call from a hypervisor.
 */
struct ffa_value ffa_rxtx_map_forward(struct ffa_endpoint_rxtx_descriptor *desc,
				ffa_id_t endpoint_id,
				void *rx_address, void *tx_address)
{
	ffa_endpoint_rxtx_descriptor_init(desc, endpoint_id, rx_address, tx_address);
	return ffa_rxtx_map(0, 0, 0);
}
