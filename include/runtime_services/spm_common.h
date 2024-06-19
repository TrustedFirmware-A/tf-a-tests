/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPM_COMMON_H
#define SPM_COMMON_H

#include <plat/common/platform.h>

#include <stdint.h>
#include <string.h>

#include <ffa_helpers.h>

#include <lib/extensions/sve.h>

/* Hypervisor ID at physical FFA instance */
#define HYP_ID          (0)
/* SPMC ID */
#define SPMC_ID		U(0x8000)

/* ID for the first Secure Partition. */
#define SPM_VM_ID_FIRST                 SP_ID(1)

#define TIMER_VIRTUAL_INTID	U(3)

/* INTID for the managed exit virtual interrupt. */
#define MANAGED_EXIT_INTERRUPT_ID	U(4)

/* INTID for the notification pending interrupt. */
#define NOTIFICATION_PENDING_INTERRUPT_INTID 5

/* Interrupt used for testing extended SPI handling. */
#define IRQ_ESPI_TEST_INTID			5000

/** IRQ/FIQ pin used for signaling a virtual interrupt. */
enum interrupt_pin {
	INTERRUPT_TYPE_IRQ,
	INTERRUPT_TYPE_FIQ,
};

/*
 * The bit 15 of the FF-A ID indicates whether the partition is executing
 * in the normal world, in case it is a Virtual Machine (VM); or in the
 * secure world, in case it is a Secure Partition (SP).
 *
 * If bit 15 is set partition is an SP; if bit 15 is clear partition is
 * a VM.
 */
#define SP_ID_MASK	U(1 << 15)
#define SP_ID(x)	((x) | SP_ID_MASK)
#define VM_ID(x)	(x & ~SP_ID_MASK)
#define IS_SP_ID(x)	((x & SP_ID_MASK) != 0U)

#define NULL_UUID (const struct ffa_uuid) { .uuid = {0} }

struct ffa_features_test {
	const char *test_name;
	unsigned int feature;
	unsigned int expected_ret;
	unsigned int param;
	unsigned int version_added;
};

struct mailbox_buffers {
	void *recv;
	void *send;
};

#define CONFIGURE_MAILBOX(mb_name, buffers_size) 				\
	do {									\
	/* Declare RX/TX buffers at virtual FF-A instance */			\
	static struct {								\
			uint8_t rx[buffers_size];				\
			uint8_t tx[buffers_size];				\
	} __aligned(PAGE_SIZE) mb_buffers;					\
	mb_name.recv = (void *)mb_buffers.rx;					\
	mb_name.send = (void *)mb_buffers.tx;					\
	} while (false)

#define CONFIGURE_AND_MAP_MAILBOX(mb_name, buffers_size, smc_ret)		\
	do {									\
	CONFIGURE_MAILBOX(mb_name, buffers_size);				\
	smc_ret = ffa_rxtx_map(							\
				(uintptr_t)mb_name.send,			\
				(uintptr_t)mb_name.recv,			\
				buffers_size / PAGE_SIZE			\
			);							\
	} while (false)

/**
 * Helpers to evaluate returns of FF-A calls.
 */
bool is_ffa_call_error(struct ffa_value val);
bool is_expected_ffa_error(struct ffa_value ret, int32_t error_code);
bool is_ffa_direct_response(struct ffa_value ret);
bool is_expected_ffa_return(struct ffa_value ret, uint32_t func_id);
bool is_expected_cactus_response(struct ffa_value ret, uint32_t expected_resp,
				 uint32_t arg);
void dump_ffa_value(struct ffa_value ret);

bool check_spmc_execution_level(void);

size_t get_ffa_feature_test_target(
	const struct ffa_features_test **test_target);
bool ffa_features_test_targets(const struct ffa_features_test *targets,
			       size_t test_target_size);

/**
 * Helper to conduct a memory retrieve. This is to be called by the receiver
 * of a memory share operation.
 */
bool memory_retrieve(struct mailbox_buffers *mb,
		     struct ffa_memory_region **retrieved, uint64_t handle,
		     ffa_id_t sender, struct ffa_memory_access receivers[],
		     uint32_t receiver_count, ffa_memory_region_flags_t flags,
		     bool is_normal_memory);

bool hypervisor_retrieve_request_continue(
	struct mailbox_buffers *mb, uint64_t handle, void *out, uint32_t out_size,
	uint32_t total_size, uint32_t fragment_offset, bool release_rx);

bool hypervisor_retrieve_request(struct mailbox_buffers *mb, uint64_t handle,
				 void *out, uint32_t out_size);

/**
 * Helper to conduct a memory relinquish. The caller is usually the receiver,
 * after it being done with the memory shared, identified by the 'handle'.
 */
bool memory_relinquish(struct ffa_mem_relinquish *m, uint64_t handle,
		       ffa_id_t id);

ffa_memory_handle_t memory_send(
	void *send_buffer, uint32_t mem_func,
	const struct ffa_memory_region_constituent *constituents,
	uint32_t constituent_count, uint32_t remaining_constituent_count,
	uint32_t fragment_length, uint32_t total_length,
	struct ffa_value *ret);

ffa_memory_handle_t memory_init_and_send(
	void *send_buffer, size_t memory_region_max_size, ffa_id_t sender,
	struct ffa_memory_access receivers[], uint32_t receiver_count,
	const struct ffa_memory_region_constituent *constituents,
	uint32_t constituents_count, uint32_t mem_func, struct ffa_value *ret);

bool ffa_partition_info_helper(struct mailbox_buffers *mb,
			       const struct ffa_uuid uuid,
			       const struct ffa_partition_info *expected,
			       const uint16_t expected_size);
bool enable_trusted_wdog_interrupt(ffa_id_t source, ffa_id_t dest);
bool disable_trusted_wdog_interrupt(ffa_id_t source, ffa_id_t dest);

bool ffa_partition_info_regs_helper(const struct ffa_uuid uuid,
		       const struct ffa_partition_info *expected,
		       const uint16_t expected_size);

struct ffa_memory_access ffa_memory_access_init_permissions_from_mem_func(
	ffa_id_t receiver_id,
	uint32_t mem_func);

bool receive_indirect_message(void *buffer, size_t buffer_size, void *recv,
			      ffa_id_t *sender, ffa_id_t receiver,
			      ffa_id_t own_id);
struct ffa_value send_indirect_message(
		ffa_id_t from, ffa_id_t to, void *send, const void *payload,
		size_t payload_size, uint32_t send_flags);
#endif /* SPM_COMMON_H */
