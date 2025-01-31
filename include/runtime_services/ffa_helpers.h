/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FFA_HELPERS_H
#define FFA_HELPERS_H

#include <ffa_svc.h>
#include <tftf_lib.h>
#include <utils_def.h>

#include <xlat_tables_defs.h>

/* FFA_VERSION helpers */
/**
 * The version number of a Firmware Framework implementation is a 31-bit
 * unsigned integer, with the upper 15 bits denoting the major revision,
 * and the lower 16 bits denoting the minor revision.
 */
enum ffa_version {
	FFA_VERSION_1_0 = 0x10000,
	FFA_VERSION_1_1 = 0x10001,
	FFA_VERSION_1_2 = 0x10002,
	FFA_VERSION_COMPILED = FFA_VERSION_1_2,
};

#define FFA_VERSION_MBZ_BIT (1U << 31U)
#define FFA_VERSION_MAJOR_SHIFT (16U)
#define FFA_VERSION_MAJOR_MASK (0x7FFFU)
#define FFA_VERSION_MINOR_SHIFT (0U)
#define FFA_VERSION_MINOR_MASK (0xFFFFU)

/** Return true if the version is valid (i.e. bit 31 is 0). */
static inline bool ffa_version_is_valid(uint32_t version)
{
	return (version & FFA_VERSION_MBZ_BIT) == 0;
}

/** Construct a version from a pair of major and minor components. */
static inline enum ffa_version make_ffa_version(uint16_t major, uint16_t minor)
{
	return (enum ffa_version)((major << FFA_VERSION_MAJOR_SHIFT) |
				  (minor << FFA_VERSION_MINOR_SHIFT));
}

/** Get the major component of the version. */
static inline uint16_t ffa_version_get_major(enum ffa_version version)
{
	return (version >> FFA_VERSION_MAJOR_SHIFT) & FFA_VERSION_MAJOR_MASK;
}

/** Get the minor component of the version. */
static inline uint16_t ffa_version_get_minor(enum ffa_version version)
{
	return (version >> FFA_VERSION_MINOR_SHIFT) & FFA_VERSION_MINOR_MASK;
}

/**
 * Check major versions are equal and the minor version of the caller is
 * less than or equal to the minor version of the callee.
 */
static inline bool ffa_versions_are_compatible(enum ffa_version caller,
					       enum ffa_version callee)
{
	return ffa_version_get_major(caller) == ffa_version_get_major(callee) &&
	       ffa_version_get_minor(caller) <= ffa_version_get_minor(callee);
}

/* This error code must be different to the ones used by FFA */
#define FFA_TFTF_ERROR		-42

typedef unsigned short ffa_id_t;
typedef unsigned short ffa_vm_count_t;
typedef unsigned short ffa_vcpu_count_t;
typedef uint64_t ffa_memory_handle_t;
/** Flags to indicate properties of receivers during memory region retrieval. */
typedef uint8_t ffa_memory_receiver_flags_t;

struct ffa_uuid {
	uint32_t uuid[4];
};

/** Length in bytes of the name in boot information descriptor. */
#define FFA_BOOT_INFO_NAME_LEN 16

/**
 * The FF-A boot info descriptor, as defined in table 5.8 of section 5.4.1, of
 * the FF-A v1.1 EAC0 specification.
 */
struct ffa_boot_info_desc {
	char name[FFA_BOOT_INFO_NAME_LEN];
	uint8_t type;
	uint8_t reserved;
	uint16_t flags;
	uint32_t size;
	uint64_t content;
};

/** FF-A boot information type mask. */
#define FFA_BOOT_INFO_TYPE_SHIFT 7
#define FFA_BOOT_INFO_TYPE_MASK (0x1U << FFA_BOOT_INFO_TYPE_SHIFT)
#define FFA_BOOT_INFO_TYPE_STD 0U
#define FFA_BOOT_INFO_TYPE_IMPDEF 1U

/** Standard boot info type IDs. */
#define FFA_BOOT_INFO_TYPE_ID_MASK 0x7FU
#define FFA_BOOT_INFO_TYPE_ID_FDT 0U
#define FFA_BOOT_INFO_TYPE_ID_HOB 1U

/** FF-A Boot Info descriptors flags. */
#define FFA_BOOT_INFO_FLAG_MBZ_MASK 0xFFF0U

/** Bits [1:0] encode the format of the name field in ffa_boot_info_desc. */
#define FFA_BOOT_INFO_FLAG_NAME_FORMAT_SHIFT 0U
#define FFA_BOOT_INFO_FLAG_NAME_FORMAT_MASK \
	(0x3U << FFA_BOOT_INFO_FLAG_NAME_FORMAT_SHIFT)
#define FFA_BOOT_INFO_FLAG_NAME_FORMAT_STRING 0x0U
#define FFA_BOOT_INFO_FLAG_NAME_FORMAT_UUID 0x1U

/** Bits [3:2] encode the format of the content field in ffa_boot_info_desc. */
#define FFA_BOOT_INFO_FLAG_CONTENT_FORMAT_SHIFT 2
#define FFA_BOOT_INFO_FLAG_CONTENT_FORMAT_MASK \
	(0x3U << FFA_BOOT_INFO_FLAG_CONTENT_FORMAT_SHIFT)
#define FFA_BOOT_INFO_FLAG_CONTENT_FORMAT_VALUE 0x1U
#define FFA_BOOT_INFO_FLAG_CONTENT_FORMAT_ADDR 0x0U

static inline uint16_t ffa_boot_info_content_format(
	struct ffa_boot_info_desc *desc)
{
	return (desc->flags & FFA_BOOT_INFO_FLAG_CONTENT_FORMAT_MASK) >>
	       FFA_BOOT_INFO_FLAG_CONTENT_FORMAT_SHIFT;
}

static inline uint16_t ffa_boot_info_name_format(
	struct ffa_boot_info_desc *desc)
{
	return (desc->flags & FFA_BOOT_INFO_FLAG_NAME_FORMAT_MASK) >>
	       FFA_BOOT_INFO_FLAG_NAME_FORMAT_SHIFT;
}

static inline uint8_t ffa_boot_info_type_id(struct ffa_boot_info_desc *desc)
{
	return desc->type & FFA_BOOT_INFO_TYPE_ID_MASK;
}

static inline uint8_t ffa_boot_info_type(struct ffa_boot_info_desc *desc)
{
	return (desc->type & FFA_BOOT_INFO_TYPE_MASK) >>
	       FFA_BOOT_INFO_TYPE_SHIFT;
}

/** Length in bytes of the signature in the boot descriptor. */
#define FFA_BOOT_INFO_HEADER_SIGNATURE_LEN 4

/**
 * The FF-A boot information header, as defined in table 5.9 of section 5.4.2,
 * of the FF-A v1.1 EAC0 specification.
 */
struct ffa_boot_info_header {
	uint32_t signature;
	uint32_t version;
	uint32_t info_blob_size;
	uint32_t desc_size;
	uint32_t desc_count;
	uint32_t desc_offset;
	uint64_t reserved;
	struct ffa_boot_info_desc boot_info[];
};

#ifndef __ASSEMBLY__

#include <cassert.h>
#include <stdint.h>

/**
 * FF-A Feature ID, to be used with interface FFA_FEATURES.
 * As defined in the FF-A v1.1 Beta specification, table 13.10, in section
 * 13.2.
 */

/** Query interrupt ID of Notification Pending Interrupt. */
#define FFA_FEATURE_NPI 0x1U

/** Query interrupt ID of Schedule Receiver Interrupt. */
#define FFA_FEATURE_SRI 0x2U

/** Query interrupt ID of the Managed Exit Interrupt. */
#define FFA_FEATURE_MEI 0x3U

/** Minimum and maximum buffer size reported by FFA_FEATURES(FFA_RXTX_MAP) */
#define FFA_RXTX_MAP_MIN_BUF_4K 0
#define FFA_RXTX_MAP_MAX_BUF_PAGE_COUNT 1

/** Partition property: partition supports receipt of direct requests. */
#define FFA_PARTITION_DIRECT_REQ_RECV (UINT32_C(1) << 0)

/** Partition property: partition can send direct requests. */
#define FFA_PARTITION_DIRECT_REQ_SEND (UINT32_C(1) << 1)

/** Partition property: partition can send and receive indirect messages. */
#define FFA_PARTITION_INDIRECT_MSG (UINT32_C(1) << 2)

/** Partition property: partition can receive notifications. */
#define FFA_PARTITION_NOTIFICATION (UINT32_C(1) << 3)

/** Partition property: partition runs in the AArch64 execution state. */
#define FFA_PARTITION_AARCH64_EXEC (UINT32_C(1) << 8)

/** Partition info descriptor as defined in FF-A v1.1 EAC0 Table 13.37 */
struct ffa_partition_info {
	/** The ID of the VM the information is about */
	ffa_id_t id;
	/** The number of execution contexts implemented by the partition */
	uint16_t exec_context;
	/** The Partition's properties, e.g. supported messaging methods */
	uint32_t properties;
	/** The uuid of the partition */
	struct ffa_uuid uuid;
};

/**
 * Bits[31:3] of partition properties must be zero for FF-A v1.0.
 * This corresponds to table 8.25 "Partition information descriptor"
 * in DEN0077A FF-A 1.0 REL specification.
 */
#define FFA_PARTITION_v1_0_RES_MASK (~(UINT32_C(0x7)))

/**
 * Partition info descriptor as defined in Table 8.25 of the v1.0
 * FF-A Specification (DEN0077A).
 */
struct ffa_partition_info_v1_0 {
	/** The ID of the VM the information is about */
	ffa_id_t id;
	/** The number of execution contexts implemented by the partition */
	uint16_t exec_context;
	/** The Partition's properties, e.g. supported messaging methods */
	uint32_t properties;
};

struct ffa_value {
	u_register_t fid;
	u_register_t arg1;
	u_register_t arg2;
	u_register_t arg3;
	u_register_t arg4;
	u_register_t arg5;
	u_register_t arg6;
	u_register_t arg7;
	u_register_t arg8;
	u_register_t arg9;
	u_register_t arg10;
	u_register_t arg11;
	u_register_t arg12;
	u_register_t arg13;
	u_register_t arg14;
	u_register_t arg15;
	u_register_t arg16;
	u_register_t arg17;
};

/* Function to make an SMC or SVC service call depending on the exception
 * level of the SP.
 */
struct ffa_value ffa_service_call(struct ffa_value *args);

/*
 * Functions to trigger a service call.
 *
 * The arguments to pass through the service call must be stored in the
 * ffa_value structure. The return values of the service call will be stored
 * in the same structure (overriding the input arguments).
 *
 * Return the first return value. It is equivalent to args.fid but is also
 * provided as the return value for convenience.
 */
u_register_t ffa_svc(struct ffa_value *args);
u_register_t ffa_smc(struct ffa_value *args);

static inline uint32_t ffa_func_id(struct ffa_value val)
{
	return (uint32_t)val.fid;
}

static inline int32_t ffa_error_code(struct ffa_value val)
{
	return (int32_t)val.arg2;
}

static inline ffa_id_t ffa_endpoint_id(struct ffa_value val) {
	return (ffa_id_t)val.arg2 & 0xffff;
}

static inline uint32_t ffa_partition_info_count(struct ffa_value val)
{
	return (uint32_t)val.arg2;
}

static inline uint32_t ffa_partition_info_desc_size(struct ffa_value val)
{
	return (uint32_t)val.arg3;
}

static inline uint32_t ffa_feature_intid(struct ffa_value val)
{
	return (uint32_t)val.arg2;
}

static inline uint16_t ffa_partition_info_regs_get_last_idx(
	struct ffa_value args)
{
	return args.arg2 & 0xFFFF;
}

static inline uint16_t ffa_partition_info_regs_get_curr_idx(
	struct ffa_value args)
{
	return (args.arg2 >> 16) & 0xFFFF;
}

static inline uint16_t ffa_partition_info_regs_get_tag(struct ffa_value args)
{
	return (args.arg2 >> 32) & 0xFFFF;
}

static inline uint16_t ffa_partition_info_regs_get_desc_size(
	struct ffa_value args)
{
	return (args.arg2 >> 48);
}

static inline uint32_t ffa_partition_info_regs_partition_count(
		struct ffa_value args)
{
	return ffa_partition_info_regs_get_last_idx(args) + 1;
}

static inline uint32_t ffa_partition_info_regs_entry_count(
		struct ffa_value args, uint16_t start_idx)
{
	return (ffa_partition_info_regs_get_curr_idx(args) - start_idx + 1);
}

static inline uint16_t ffa_partition_info_regs_entry_size(
		struct ffa_value args)
{
	return (args.arg2 >> 48) & 0xFFFFU;
}

/**
 * FF-A Framework message helpers
 */
#define FFA_FRAMEWORK_MSG_BIT (1U << 31)

#define FFA_FRAMEWORK_MSG_MASK 0xFFU

#define FFA_FRAMEWORK_MSG_PSCI_REQ 0U
#define FFA_FRAMEWORK_MSG_PSCI_RESP 2U

static inline uint32_t ffa_framework_msg_flags(struct ffa_value args)
{
	return (uint32_t)args.arg2;
}

static inline bool ffa_is_framework_msg(struct ffa_value args)
{
	return (ffa_func_id(args) == FFA_MSG_SEND_DIRECT_REQ_SMC32 ||
		ffa_func_id(args) == FFA_MSG_SEND_DIRECT_REQ_SMC64) &&
		((ffa_framework_msg_flags(args) & FFA_FRAMEWORK_MSG_BIT) != 0);
}

static inline uint32_t ffa_get_framework_msg(struct ffa_value args)
{
	return ffa_framework_msg_flags(args) & FFA_FRAMEWORK_MSG_MASK;
}

typedef uint64_t ffa_notification_bitmap_t;

/**
 * Partition message header as specified by table 6.2 from FF-A v1.1 EAC0
 * specification.
 */
struct ffa_partition_rxtx_header {
	uint32_t flags;
	/* Reserved (SBZ). */
	uint32_t reserved_1;
	/* Offset from the beginning of the buffer to the message payload. */
	uint32_t offset;
	/* Sender(Bits[31:16]) and Receiver(Bits[15:0]) endpoint IDs. */
	ffa_id_t receiver;
	ffa_id_t sender;
	/* Size of message in buffer. */
	uint32_t size;
	/* Reserved (SBZ). Added in v1.2 */
	uint32_t reserved_2;
	/* UUID identifying the communication protocol. Added in v1.2. */
	struct ffa_uuid uuid;
};

#define FFA_RXTX_HEADER_SIZE sizeof(struct ffa_partition_rxtx_header)

static inline void ffa_rxtx_header_init(
	ffa_id_t sender, ffa_id_t receiver, uint32_t size,
	struct ffa_partition_rxtx_header *header)
{
	header->flags = 0;
	header->reserved_1 = 0;
	header->offset = FFA_RXTX_HEADER_SIZE;
	header->sender = sender;
	header->receiver = receiver;
	header->size = size;
	header->reserved_2 = 0;
	header->uuid = (struct ffa_uuid){0};
}

/* The maximum length possible for a single message. */
#define FFA_PARTITION_MSG_PAYLOAD_MAX (PAGE_SIZE - FFA_RXTX_HEADER_SIZE)

struct ffa_partition_msg {
	struct ffa_partition_rxtx_header header;
	char payload[FFA_PARTITION_MSG_PAYLOAD_MAX];
};

/* The maximum length possible for a single message. */
#define FFA_MSG_PAYLOAD_MAX PAGE_SIZE

#define FFA_NOTIFICATION(ID)		(UINT64_C(1) << ID)

#define MAX_FFA_NOTIFICATIONS		UINT32_C(64)

#define FFA_NOTIFICATIONS_FLAG_PER_VCPU	UINT32_C(0x1 << 0)

/** Flag to delay Schedule Receiver Interrupt. */
#define FFA_NOTIFICATIONS_FLAG_DELAY_SRI	UINT32_C(0x1 << 1)

#define FFA_NOTIFICATIONS_FLAGS_VCPU_ID(id) UINT32_C((id & 0xFFFF) << 16)

#define FFA_NOTIFICATIONS_FLAG_BITMAP_SP	UINT32_C(0x1 << 0)
#define FFA_NOTIFICATIONS_FLAG_BITMAP_VM	UINT32_C(0x1 << 1)
#define FFA_NOTIFICATIONS_FLAG_BITMAP_SPM	UINT32_C(0x1 << 2)
#define FFA_NOTIFICATIONS_FLAG_BITMAP_HYP	UINT32_C(0x1 << 3)

#define FFA_NOTIFICATION_SPM_BUFFER_FULL_MASK FFA_NOTIFICATION(0)
#define FFA_NOTIFICATION_HYP_BUFFER_FULL_MASK FFA_NOTIFICATION(32)

/**
 * The following is an SGI ID, that the SPMC configures as non-secure, as
 * suggested by the FF-A v1.1 specification, in section 9.4.1.
 */
#define FFA_SCHEDULE_RECEIVER_INTERRUPT_ID 8

/**
 * Helper function to assemble a 64-bit sized bitmap, from the 32-bit sized lo
 * and hi.
 * Helpful as FF-A specification defines that the notifications interfaces
 * arguments are 32-bit registers.
 */
static inline ffa_notification_bitmap_t ffa_notification_bitmap(uint32_t lo,
								uint32_t hi)
{
	return (ffa_notification_bitmap_t)hi << 32U | lo;
}

static inline ffa_notification_bitmap_t ffa_notification_get_from_sp(
       struct ffa_value val)
{
	return ffa_notification_bitmap((uint32_t)val.arg2,
				       (uint32_t)val.arg3);
}

static inline ffa_notification_bitmap_t ffa_notification_get_from_vm(
       struct ffa_value val)
{
	return ffa_notification_bitmap((uint32_t)val.arg4,
				       (uint32_t)val.arg5);
}

static inline ffa_notification_bitmap_t ffa_notification_get_from_framework(
	struct ffa_value val)
{
	return ffa_notification_bitmap((uint32_t)val.arg6,
				       (uint32_t)val.arg7);
}

 /**
 * Helper functions to check for buffer full notification.
 */
static inline bool is_ffa_hyp_buffer_full_notification(
	ffa_notification_bitmap_t framework)
{
	return (framework & FFA_NOTIFICATION_HYP_BUFFER_FULL_MASK) != 0U;
}

static inline bool is_ffa_spm_buffer_full_notification(
	ffa_notification_bitmap_t framework)
{
	return (framework & FFA_NOTIFICATION_SPM_BUFFER_FULL_MASK) != 0U;
}


/*
 * FFA_NOTIFICATION_INFO_GET is a SMC64 interface.
 * The following macros are defined for SMC64 implementation.
 */
#define FFA_NOTIFICATIONS_INFO_GET_MAX_IDS		20U

#define FFA_NOTIFICATIONS_INFO_GET_FLAG_MORE_PENDING	UINT64_C(0x1)

#define FFA_NOTIFICATIONS_LISTS_COUNT_SHIFT		0x7U
#define FFA_NOTIFICATIONS_LISTS_COUNT_MASK		0x1FU
#define FFA_NOTIFICATIONS_LIST_SHIFT(l) 		(2 * (l - 1) + 12)
#define FFA_NOTIFICATIONS_LIST_SIZE_MASK 		0x3U

static inline uint32_t ffa_notification_info_get_lists_count(
	struct ffa_value ret)
{
	return (uint32_t)(ret.arg2 >> FFA_NOTIFICATIONS_LISTS_COUNT_SHIFT)
	       & FFA_NOTIFICATIONS_LISTS_COUNT_MASK;
}

static inline uint32_t ffa_notification_info_get_list_size(
	struct ffa_value ret, uint32_t list)
{
	return (uint32_t)(ret.arg2 >> FFA_NOTIFICATIONS_LIST_SHIFT(list)) &
	       FFA_NOTIFICATIONS_LIST_SIZE_MASK;
}

static inline bool ffa_notification_info_get_more_pending(struct ffa_value ret)
{
	return (ret.arg2 & FFA_NOTIFICATIONS_INFO_GET_FLAG_MORE_PENDING) != 0U;
}

enum ffa_data_access {
	FFA_DATA_ACCESS_NOT_SPECIFIED,
	FFA_DATA_ACCESS_RO,
	FFA_DATA_ACCESS_RW,
	FFA_DATA_ACCESS_RESERVED,
};

enum ffa_instruction_access {
	FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED,
	FFA_INSTRUCTION_ACCESS_NX,
	FFA_INSTRUCTION_ACCESS_X,
	FFA_INSTRUCTION_ACCESS_RESERVED,
};

enum ffa_memory_type {
	FFA_MEMORY_NOT_SPECIFIED_MEM,
	FFA_MEMORY_DEVICE_MEM,
	FFA_MEMORY_NORMAL_MEM,
};

enum ffa_memory_cacheability {
	FFA_MEMORY_CACHE_RESERVED = 0x0,
	FFA_MEMORY_CACHE_NON_CACHEABLE = 0x1,
	FFA_MEMORY_CACHE_RESERVED_1 = 0x2,
	FFA_MEMORY_CACHE_WRITE_BACK = 0x3,
	FFA_MEMORY_DEV_NGNRNE = 0x0,
	FFA_MEMORY_DEV_NGNRE = 0x1,
	FFA_MEMORY_DEV_NGRE = 0x2,
	FFA_MEMORY_DEV_GRE = 0x3,
};

enum ffa_memory_shareability {
	FFA_MEMORY_SHARE_NON_SHAREABLE,
	FFA_MEMORY_SHARE_RESERVED,
	FFA_MEMORY_OUTER_SHAREABLE,
	FFA_MEMORY_INNER_SHAREABLE,
};

typedef struct {
	uint8_t data_access : 2;
	uint8_t instruction_access : 2;
} ffa_memory_access_permissions_t;

_Static_assert(sizeof(ffa_memory_access_permissions_t) == sizeof(uint8_t),
	       "ffa_memory_access_permissions_t must be 1 byte wide");

/**
 * FF-A v1.1 REL0 Table 10.18 memory region attributes descriptor NS Bit 6.
 * Per section 10.10.4.1, NS bit is reserved for FFA_MEM_DONATE/LEND/SHARE
 * and FFA_MEM_RETRIEVE_REQUEST.
 */
enum ffa_memory_security {
	FFA_MEMORY_SECURITY_UNSPECIFIED = 0,
	FFA_MEMORY_SECURITY_SECURE = 0,
	FFA_MEMORY_SECURITY_NON_SECURE,
};

/**
 * This corresponds to table 10.18 of the FF-A v1.1 EAC0 specification, "Memory
 * region attributes descriptor".
 */
typedef struct {
	uint16_t shareability : 2;
	uint16_t cacheability : 2;
	uint16_t type : 2;
	uint16_t security : 1;
} ffa_memory_attributes_t;

_Static_assert(sizeof(ffa_memory_attributes_t) == sizeof(uint16_t),
	       "ffa_memory_attributes_t must be 2 bytes wide");

#define FFA_MEMORY_HANDLE_ALLOCATOR_MASK UINT64_C(1)
#define FFA_MEMORY_HANDLE_ALLOCATOR_SHIFT 63U
#define FFA_MEMORY_HANDLE_ALLOCATOR_HYPERVISOR UINT64_C(1)
#define FFA_MEMORY_HANDLE_ALLOCATOR_SPMC UINT64_C(0)
#define FFA_MEMORY_HANDLE_INVALID (~UINT64_C(0))

/**
 * A set of contiguous pages which is part of a memory region. This corresponds
 * to table 10.14 of the FF-A v1.1 EAC0 specification, "Constituent memory
 * region descriptor".
 */
struct ffa_memory_region_constituent {
	/**
	 * The base IPA of the constituent memory region, aligned to 4 kiB page
	 * size granularity.
	 */
	void *address;
	/** The number of 4 kiB pages in the constituent memory region. */
	uint32_t page_count;
	/** Reserved field, must be 0. */
	uint32_t reserved;
};

/**
 * A set of pages comprising a memory region. This corresponds to table 10.13 of
 * the FF-A v1.1 EAC0 specification, "Composite memory region descriptor".
 */
struct ffa_composite_memory_region {
	/**
	 * The total number of 4 kiB pages included in this memory region. This
	 * must be equal to the sum of page counts specified in each
	 * `ffa_memory_region_constituent`.
	 */
	uint32_t page_count;
	/**
	 * The number of constituents (`ffa_memory_region_constituent`)
	 * included in this memory region range.
	 */
	uint32_t constituent_count;
	/** Reserved field, must be 0. */
	uint64_t reserved_0;
	/** An array of `constituent_count` memory region constituents. */
	struct ffa_memory_region_constituent constituents[];
};

/**
 * This corresponds to table "Memory access permissions descriptor" of the FFA
 * 1.0 specification.
 */
struct ffa_memory_region_attributes {
	/** The ID of the VM to which the memory is being given or shared. */
	ffa_id_t receiver;
	/**
	 * The permissions with which the memory region should be mapped in the
	 * receiver's page table.
	 */
	ffa_memory_access_permissions_t permissions;
	/**
	 * Flags used during FFA_MEM_RETRIEVE_REQ and FFA_MEM_RETRIEVE_RESP
	 * for memory regions with multiple borrowers.
	 */
	ffa_memory_receiver_flags_t flags;
};

/**
 * Endpoint RX/TX descriptor, as defined by Table 13.27 in FF-A v1.1 EAC0.
 * It's used by the Hypervisor to describe the RX/TX buffers mapped by a VM
 * to the SPMC, in order to allow indirect messaging.
 */
struct ffa_endpoint_rxtx_descriptor {
	ffa_id_t endpoint_id;
	uint16_t reserved;

	/*
	 * 8-byte aligned offset from the base address of this descriptor to the
	 * `ffa_composite_memory_region` describing the RX buffer.
	 */
	uint32_t rx_offset;

	/*
	 * 8-byte aligned offset from the base address of this descriptor to the
	 * `ffa_composite_memory_region` describing the TX buffer.
	 */
	uint32_t tx_offset;

	/* Pad to align on 16-byte boundary. */
	uint32_t pad;
};

_Static_assert(sizeof(struct ffa_endpoint_rxtx_descriptor) == 16,
	       "ffa_endpoint_rx_tx_descriptor must be 16 bytes wide");

/** Flags to control the behaviour of a memory sharing transaction. */
typedef uint32_t ffa_memory_region_flags_t;

/**
 * Clear memory region contents after unmapping it from the sender and before
 * mapping it for any receiver.
 */
#define FFA_MEMORY_REGION_FLAG_CLEAR 0x1U

/**
 * Whether the hypervisor may time slice the memory sharing or retrieval
 * operation.
 */
#define FFA_MEMORY_REGION_FLAG_TIME_SLICE 0x2U

/**
 * Whether the hypervisor should clear the memory region after the receiver
 * relinquishes it or is aborted.
 */
#define FFA_MEMORY_REGION_FLAG_CLEAR_RELINQUISH 0x4U

#define FFA_MEMORY_REGION_TRANSACTION_TYPE_MASK ((0x3U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_UNSPECIFIED ((0x0U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_SHARE ((0x1U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_LEND ((0x2U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_DONATE ((0x3U) << 3)

/** The maximum number of recipients a memory region may be sent to. */
#define MAX_MEM_SHARE_RECIPIENTS 2U

struct ffa_memory_access_impdef {
	uint64_t val[2];
};

/**
 * This corresponds to table "Endpoint memory access descriptor" of the FFA 1.0
 * specification.
 */
struct ffa_memory_access {
	struct ffa_memory_region_attributes receiver_permissions;
	/**
	 * Offset in bytes from the start of the outer `ffa_memory_region` to
	 * an `ffa_composite_memory_region` struct.
	 */
	uint32_t composite_memory_region_offset;
	/* Space for implementation defined information */
	struct ffa_memory_access_impdef impdef;
	uint64_t reserved_0;
};

/**
 * Information about a set of pages which are being shared. This corresponds to
 * table 10.20 of the FF-A v1.1 EAC0 specification, "Lend, donate or share
 * memory transaction descriptor". Note that it is also used for retrieve
 * requests and responses.
 */
struct ffa_memory_region {
	/**
	 * The ID of the VM which originally sent the memory region, i.e. the
	 * owner.
	 */
	ffa_id_t sender;
	ffa_memory_attributes_t attributes;
	/** Flags to control behaviour of the transaction. */
	ffa_memory_region_flags_t flags;
	ffa_memory_handle_t handle;
	/**
	 * An implementation defined value associated with the receiver and the
	 * memory region.
	 */
	uint64_t tag;
	/** Size of the memory access descriptor. */
	uint32_t memory_access_desc_size;
	/**
	 * The number of `ffa_memory_access` entries included in this
	 * transaction.
	 */
	uint32_t receiver_count;
	/**
	 * Offset of the 'receivers' field, which relates to the memory access
	 * descriptors.
	 */
	uint32_t receivers_offset;
	/** Reserved field (12 bytes) must be 0. */
	uint32_t reserved[3];
	/**
	 * An array of `receiver_count` endpoint memory access descriptors.
	 * Each one specifies a memory region offset, an endpoint and the
	 * attributes with which this memory region should be mapped in that
	 * endpoint's page table.
	 */
	struct ffa_memory_access receivers[];
};

/**
 * Descriptor used for FFA_MEM_RELINQUISH requests. This corresponds to table
 * 16.25 of the FF-A v1.1 EAC0 specification, "Descriptor to relinquish a memory
 * region".
 */
struct ffa_mem_relinquish {
	ffa_memory_handle_t handle;
	ffa_memory_region_flags_t flags;
	uint32_t endpoint_count;
	ffa_id_t endpoints[];
};

static inline ffa_memory_handle_t ffa_assemble_handle(uint32_t h1, uint32_t h2)
{
	return (ffa_notification_bitmap_t)h1 |
	       (ffa_notification_bitmap_t)h2 << 32;
}

static inline ffa_memory_handle_t ffa_mem_success_handle(struct ffa_value r)
{
	return ffa_assemble_handle(r.arg2, r.arg3);
}

static inline ffa_memory_handle_t ffa_frag_handle(struct ffa_value r)
{
	return ffa_assemble_handle(r.arg1, r.arg2);
}

static inline ffa_id_t ffa_frag_sender(struct ffa_value args)
{
	return (args.arg4 >> 16) & 0xffff;
}

/**
 * To maintain forwards compatability we can't make assumptions about the size
 * of the endpoint memory access descriptor so provide a helper function
 * to get a receiver from the receiver array using the memory access descriptor
 * size field from the memory region descriptor struct.
 * Returns NULL if we cannot return the receiver.
 */
static inline struct ffa_memory_access *ffa_memory_region_get_receiver(
	struct ffa_memory_region *memory_region, uint32_t receiver_index)
{
	uint32_t memory_access_desc_size =
		memory_region->memory_access_desc_size;

	if (receiver_index >= memory_region->receiver_count) {
		return NULL;
	}

	/*
	 * Memory access descriptor size cannot be greater than the size of
	 * the memory access descriptor defined by the current FF-A version.
	 */
	if (memory_access_desc_size > sizeof(struct ffa_memory_access)) {
		return NULL;
	}

	/* Check we cannot use receivers offset to cause overflow. */
	if (memory_region->receivers_offset !=
	    sizeof(struct ffa_memory_region)) {
		return NULL;
	}

	return (struct ffa_memory_access *)((uint8_t *)memory_region +
					    memory_region->receivers_offset +
					    (receiver_index *
					     memory_access_desc_size));
}

/**
 * Gets the `ffa_composite_memory_region` for the given receiver from an
 * `ffa_memory_region`, or NULL if it is not valid.
 */
static inline struct ffa_composite_memory_region *
ffa_memory_region_get_composite(struct ffa_memory_region *memory_region,
				uint32_t receiver_index)
{
	uint32_t offset = memory_region->receivers[receiver_index]
				  .composite_memory_region_offset;

	if (offset == 0) {
		return NULL;
	}

	return (struct ffa_composite_memory_region *)((uint8_t *)memory_region +
						      offset);
}

static inline uint32_t ffa_mem_relinquish_init(
	struct ffa_mem_relinquish *relinquish_request,
	ffa_memory_handle_t handle, ffa_memory_region_flags_t flags,
	ffa_id_t sender)
{
	relinquish_request->handle = handle;
	relinquish_request->flags = flags;
	relinquish_request->endpoint_count = 1;
	relinquish_request->endpoints[0] = sender;
	return sizeof(struct ffa_mem_relinquish) + sizeof(ffa_id_t);
}

uint32_t ffa_memory_retrieve_request_init(
	struct ffa_memory_region *memory_region, ffa_memory_handle_t handle,
	ffa_id_t sender, struct ffa_memory_access receivers[],
	uint32_t receiver_count, uint32_t tag, ffa_memory_region_flags_t flags,
	enum ffa_memory_type type, enum ffa_memory_cacheability cacheability,
	enum ffa_memory_shareability shareability);

void ffa_hypervisor_retrieve_request_init(struct ffa_memory_region *region,
					  ffa_memory_handle_t handle);

static inline uint32_t ffa_mem_retrieve_res_total_size(struct ffa_value ret)
{
	return ret.arg1;
}

static inline uint32_t ffa_mem_retrieve_res_frag_size(struct ffa_value ret)
{
	return ret.arg2;
}

static inline uint32_t ffa_mem_frag_tx_frag_size(struct ffa_value ret)
{
	return ret.arg3;
}

uint32_t ffa_memory_region_init(
	struct ffa_memory_region *memory_region, size_t memory_region_max_size,
	ffa_id_t sender, struct ffa_memory_access receivers[],
	uint32_t receiver_count,
	const struct ffa_memory_region_constituent constituents[],
	uint32_t constituent_count, uint32_t tag,
	ffa_memory_region_flags_t flags, enum ffa_memory_type type,
	enum ffa_memory_cacheability cacheability,
	enum ffa_memory_shareability shareability, uint32_t *total_length,
	uint32_t *fragment_length);

uint32_t ffa_memory_fragment_init(
	struct ffa_memory_region_constituent *fragment,
	size_t fragment_max_size,
	const struct ffa_memory_region_constituent constituents[],
	uint32_t constituent_count, uint32_t *fragment_length);

static inline ffa_id_t ffa_dir_msg_dest(struct ffa_value val) {
	return (ffa_id_t)val.arg1 & U(0xFFFF);
}

static inline ffa_id_t ffa_dir_msg_source(struct ffa_value val) {
	return (ffa_id_t)(val.arg1 >> 16U);
}

struct ffa_value ffa_msg_send_direct_req64(ffa_id_t source_id,
					   ffa_id_t dest_id, uint64_t arg0,
					   uint64_t arg1, uint64_t arg2,
					   uint64_t arg3, uint64_t arg4);

struct ffa_value ffa_msg_send_direct_req32(ffa_id_t source_id,
					   ffa_id_t dest_id, uint32_t arg0,
					   uint32_t arg1, uint32_t arg2,
					   uint32_t arg3, uint32_t arg4);

struct ffa_value ffa_msg_send_direct_resp64(ffa_id_t source_id,
					    ffa_id_t dest_id, uint64_t arg0,
					    uint64_t arg1, uint64_t arg2,
					    uint64_t arg3, uint64_t arg4);

struct ffa_value ffa_msg_send_direct_resp32(ffa_id_t source_id,
					    ffa_id_t dest_id, uint32_t arg0,
					    uint32_t arg1, uint32_t arg2,
					    uint32_t arg3, uint32_t arg4);

struct ffa_value ffa_framework_msg_send_direct_resp(ffa_id_t source_id,
					    ffa_id_t dest_id, uint32_t msg,
					    uint32_t status_code);

struct ffa_value ffa_run(uint32_t dest_id, uint32_t vcpu_id);
struct ffa_value ffa_version(uint32_t input_version);
struct ffa_value ffa_id_get(void);
struct ffa_value ffa_spm_id_get(void);
struct ffa_value ffa_msg_wait(void);
struct ffa_value ffa_error(int32_t error_code);
struct ffa_value ffa_features(uint32_t feature);
struct ffa_value ffa_features_with_input_property(uint32_t feature,
                                                  uint32_t param);
struct ffa_value ffa_partition_info_get(const struct ffa_uuid uuid);
struct ffa_value ffa_rx_release_with_id(ffa_id_t vm_id);
struct ffa_value ffa_rx_release(void);
struct ffa_value ffa_rxtx_map(uintptr_t send, uintptr_t recv, uint32_t pages);
struct ffa_value ffa_rxtx_unmap_with_id(uint32_t id);
struct ffa_value ffa_rxtx_unmap(void);
struct ffa_value ffa_msg_send2_with_id(uint32_t flags, ffa_id_t sender);
struct ffa_value ffa_msg_send2(uint32_t flags);
struct ffa_value ffa_mem_donate(uint32_t descriptor_length,
				uint32_t fragment_length);
struct ffa_value ffa_mem_lend(uint32_t descriptor_length,
			      uint32_t fragment_length);
struct ffa_value ffa_mem_share(uint32_t descriptor_length,
			       uint32_t fragment_length);
struct ffa_value ffa_mem_retrieve_req(uint32_t descriptor_length,
				      uint32_t fragment_length);
struct ffa_value ffa_mem_relinquish(void);
struct ffa_value ffa_mem_reclaim(uint64_t handle, uint32_t flags);
struct ffa_value ffa_mem_frag_rx(ffa_memory_handle_t handle,
				 uint32_t fragment_length);
struct ffa_value ffa_mem_frag_tx(ffa_memory_handle_t handle,
				 uint32_t fragment_length);
struct ffa_value ffa_notification_bitmap_create(ffa_id_t vm_id,
						ffa_vcpu_count_t vcpu_count);
struct ffa_value ffa_notification_bitmap_destroy(ffa_id_t vm_id);
struct ffa_value ffa_notification_bind(ffa_id_t sender, ffa_id_t receiver,
				       uint32_t flags,
				       ffa_notification_bitmap_t notifications);
struct ffa_value ffa_notification_unbind(ffa_id_t sender, ffa_id_t receiver,
				  ffa_notification_bitmap_t notifications);
struct ffa_value ffa_notification_set(ffa_id_t sender, ffa_id_t receiver,
				      uint32_t flags,
				      ffa_notification_bitmap_t bitmap);
struct ffa_value ffa_notification_get(ffa_id_t receiver, uint32_t vcpu_id,
				      uint32_t flags);
struct ffa_value ffa_notification_info_get(void);

struct ffa_value ffa_console_log(const char* message, size_t char_count);
struct ffa_value ffa_partition_info_get_regs(const struct ffa_uuid uuid,
					     const uint16_t start_index,
					     const uint16_t tag);

struct ffa_memory_access ffa_memory_access_init(
	ffa_id_t receiver_id, enum ffa_data_access data_access,
	enum ffa_instruction_access instruction_access,
	ffa_memory_receiver_flags_t flags,
	struct ffa_memory_access_impdef *impdef);

void ffa_endpoint_rxtx_descriptor_init(
    struct ffa_endpoint_rxtx_descriptor *desc, ffa_id_t endpoint_id,
    void *rx_address, void *tx_address);

struct ffa_value ffa_rxtx_map_forward(
	struct ffa_endpoint_rxtx_descriptor *desc, ffa_id_t endpoint_id,
	void *rx_address, void *tx_address);

#endif /* __ASSEMBLY__ */

#endif /* FFA_HELPERS_H */
