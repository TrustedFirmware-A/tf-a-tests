/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_RSI_H
#define REALM_RSI_H

#include <arch.h>
#include <stdint.h>
#include <host_shared_data.h>
#include <tftf_lib.h>

#define SMC64_RSI_CALL_BASE	(0xC4000190)
#define SMC64_RSI_FID(_x)	(SMC64_RSI_CALL_BASE + (_x))

/*
 * Defines member of structure and reserves space
 * for the next member with specified offset.
 */
#define SET_MEMBER_RSI	SET_MEMBER

/*
 * This file describes the Realm Services Interface (RSI) Application Binary
 * Interface (ABI) for SMC calls made from within the Realm to the RMM and
 * serviced by the RMM.
 *
 * See doc/rmm_interface.md for more details.
 */

/*
 * The major version number of the RSI implementation.  Increase this whenever
 * the binary format or semantics of the SMC calls change.
 */
#define RSI_ABI_VERSION_MAJOR		1U

/*
 * The minor version number of the RSI implementation.  Increase this when
 * a bug is fixed, or a feature is added without breaking binary compatibility.
 */
#define RSI_ABI_VERSION_MINOR		0U

#define RSI_ABI_VERSION_VAL		((RSI_ABI_VERSION_MAJOR << 16U) | \
					 RSI_ABI_VERSION_MINOR)

#define RSI_ABI_VERSION_GET_MAJOR(_version) ((_version) >> 16U)
#define RSI_ABI_VERSION_GET_MINOR(_version) ((_version) & 0xFFFFU)


/* RSI Status code enumeration as per Section D4.3.6 of the RMM Spec */
typedef enum {
	/* Command completed successfully */
	RSI_SUCCESS = 0U,

	/*
	 * The value of a command input value
	 * caused the command to fail
	 */
	RSI_ERROR_INPUT	= 1U,

	/*
	 * The state of the current Realm or current REC
	 * does not match the state expected by the command
	 */
	RSI_ERROR_STATE	= 2U,

	/* The operation requested by the command is not complete */
	RSI_INCOMPLETE = 3U,

	/* The operation requested by the command failed for an unknown reason */
	RSI_ERROR_UNKNOWN = 4U,

	/*
	 * The state of a Realm device does not match the state expected by the
	 * command.
	 */
	RSI_ERROR_DEVICE = 5U,

	RSI_ERROR_COUNT_MAX
} rsi_status_t;

#define RSI_EXIT_SYNC	0U

/* Size of Realm Personalization Value */
#define RSI_RPV_SIZE			64U

/* RsiRealmConfig structure containing realm configuration */
struct rsi_realm_config {
	/* IPA width in bits */
	SET_MEMBER_RSI(unsigned long ipa_width, 0, 8);
	/* Hash algorithm */
	SET_MEMBER_RSI(unsigned char algorithm, 0x8, 0x10);
	/* Number of auxiliary Planes */
	SET_MEMBER_RSI(unsigned long num_aux_planes, 0x10, 0x18);
	/* GICv3 VGIC Type Register value */
	SET_MEMBER_RSI(unsigned long gicv3_vtr, 0x18, 0x20);
	/*
	 * If ATS is enabled, determines the stage 2 translation used by devices
	 * assigned to the Realm
	 */
	SET_MEMBER_RSI(unsigned long ats_plane, 0x20, 0x200);

	/* Realm Personalization Value */
	SET_MEMBER_RSI(unsigned char rpv[RSI_RPV_SIZE], 0x200, 0x1000);
};

#define RSI_HOST_CALL_NR_GPRS		31U

struct rsi_host_call {
	SET_MEMBER_RSI(struct {
		/* Immediate value */
		unsigned int imm;		/* Offset 0 */
		/* Registers */
		unsigned long gprs[RSI_HOST_CALL_NR_GPRS];
		}, 0, 0x100);
};

/*
 * FID: 0xC4000190
 *
 * Returns RSI version.
 * arg1: Requested interface version
 * ret0: Status / error
 * ret1: Lower implemented interface revision
 * ret2: Higher implemented interface revision
 */
#define SMC_RSI_VERSION			SMC64_RSI_FID(U(0x0))

/*
 * FID: 0xC4000191
 *
 * Returns RSI Feature register requested by index.
 * arg1: Feature register index
 * ret0: Status / error
 * ret1: Feature register value
 */
#define SMC_RSI_FEATURES		SMC64_RSI_FID(U(0x1))

/*
 * FID: 0xC4000192
 *
 * Returns a measurement.
 * arg1: Measurement index (0..4), measurement (RIM or REM) to read
 * ret0: Status / error
 * ret1: Measurement value, bytes:  0 -  7
 * ret2: Measurement value, bytes:  8 - 15
 * ret3: Measurement value, bytes: 16 - 23
 * ret4: Measurement value, bytes: 24 - 31
 * ret5: Measurement value, bytes: 32 - 39
 * ret6: Measurement value, bytes: 40 - 47
 * ret7: Measurement value, bytes: 48 - 55
 * ret8: Measurement value, bytes: 56 - 63
 */
#define SMC_RSI_MEASUREMENT_READ	SMC64_RSI_FID(U(0x2))

/*
 * FID: 0xC4000193
 *
 * Extends a REM.
 * arg1:  Measurement index (1..4), measurement (REM) to extend
 * arg2:  Measurement size in bytes
 * arg3:  Challenge value, bytes:  0 -  7
 * arg4:  Challenge value, bytes:  8 - 15
 * arg5:  Challenge value, bytes: 16 - 23
 * arg6:  Challenge value, bytes: 24 - 31
 * arg7:  Challenge value, bytes: 32 - 39
 * arg8:  Challenge value, bytes: 40 - 47
 * arg9:  Challenge value, bytes: 48 - 55
 * arg10: Challenge value, bytes: 56 - 63
 * ret0:  Status / error
 */
#define SMC_RSI_MEASUREMENT_EXTEND	SMC64_RSI_FID(U(0x3))

/*
 * FID: 0xC4000194
 *
 * Initialize the operation to retrieve an attestation token.
 * arg1: Challenge value, bytes:  0 -  7
 * arg2: Challenge value, bytes:  8 - 15
 * arg3: Challenge value, bytes: 16 - 23
 * arg4: Challenge value, bytes: 24 - 31
 * arg5: Challenge value, bytes: 32 - 39
 * arg6: Challenge value, bytes: 40 - 47
 * arg7: Challenge value, bytes: 48 - 55
 * arg8: Challenge value, bytes: 56 - 63
 * ret0: Status / error
 * ret1: Upper bound on attestation token size in bytes
 */
#define SMC_RSI_ATTEST_TOKEN_INIT	SMC64_RSI_FID(U(0x4))

/*
 * FID: 0xC4000195
 *
 * Continue the operation to retrieve an attestation token.
 * arg1: IPA of the Granule to which the token will be written
 * arg2: Offset within Granule to start of buffer in bytes
 * arg3: Size of buffer in bytes
 * ret0: Status / error
 * ret1: Number of bytes written to buffer
 */
#define SMC_RSI_ATTEST_TOKEN_CONTINUE	SMC64_RSI_FID(U(0x5))

/*
 * FID: 0xC4000196
 *
 * Read configuration for the current Realm.
 * arg1 == IPA of the Granule to which the configuration data will be written
 * ret0 == Status / error
 */
#define SMC_RSI_REALM_CONFIG		SMC64_RSI_FID(U(0x6))

/*
 * FID: 0xC4000197
 *
 * arg1 == Base IPA address of target region
 * arg2 == Top address of target region
 * arg3 == RIPAS value
 * arg4 == flags
 * ret0 == Status / error
 * ret1 == Base of IPA region which was not modified by the command
 * ret2 == RSI response
 */
#define SMC_RSI_IPA_STATE_SET		SMC64_RSI_FID(U(0x7))

/*
 * FID: 0xC4000198
 *
 * arg1 == Base of target IPA region
 * arg2 == End of target IPA region
 * ret0 == Status / error
 * ret1 == Top of IPA region which has the reported RIPAS value
 * ret2 == RIPAS value
 */
#define SMC_RSI_IPA_STATE_GET		SMC64_RSI_FID(U(0x8))

/*
 * FID: 0xC4000199
 *
 * arg1 == IPA of the Host call data structure
 * ret0 == Status / error
 */
#define SMC_RSI_HOST_CALL		SMC64_RSI_FID(U(0x9))

/*
 * TODO: Update the documentation of new FIDs once the 1.1 spec has stabilized.
 */

/*
 * FID: 0xC400019A
 */
#define SMC_RSI_VSMMU_GET_INFO		SMC64_RSI_FID(U(0xA))

/*
 * FID: 0xC400019B
 */
#define SMC_RSI_VSMMU_ACTIVATE		SMC64_RSI_FID(U(0xB))

/*
 * FID: 0xC400019C
 *
 * arg0 == Realm device identifier
 * arg1 == Flags
 * arg2 == Index of Plane whose stage 2 permissions are observed by non-ATS
 *         requests from the device
 * arg3 == Nonce generated on most recent transition to LOCKED state
 * arg4 == GET_MEASUREMENT request sequence number
 * arg5 == GET_INTERFACE_REPORT request sequence number
 */
#define SMC_RSI_VDEV_DMA_ENABLE		SMC64_RSI_FID(U(0xC))

/*
 * FID: 0xC400019D
 *
 * arg0 == Realm device identifier
 * arg1 == IPA to which the configuration data will be written
 */
#define SMC_RSI_VDEV_GET_INFO		SMC64_RSI_FID(U(0xD))

/*
 * FID: 0xC400019E is not used.
 */

/*
 * FID: 0xC400019F
 *
 * arg0 == Realm device identifier
 * arg1 == Base of target IPA region
 * arg2 == Top of target IPA region
 * arg3 == Base of target PA region
 * arg4 == Flags
 * arg5 == Nonce generated on most recent transition to LOCKED state
 * arg6 == GET_MEASUREMENT request sequence number
 * arg7 == GET_INTERFACE_REPORT request sequence number
 *
 * ret1 == Base of IPA region which was not modified by the command
 * ret2 == Whether the Host accepted or rejected the request
 */
#define SMC_RSI_VDEV_VALIDATE_MAPPING	SMC64_RSI_FID(U(0xF))

/*
 * FID: 0xC40001A0
 *
 * arg1 == plane index
 * arg2 == perm index
 *
 * ret0 == status
 * ret1 == perm value
 */
#define SMC_RSI_MEM_GET_PERM_VALUE	SMC64_RSI_FID(U(0x10))

/*
 * FID: 0xC40001A1
 *
 * arg1 == base adr
 * arg2 == top adr
 * arg3 == perm index
 * arg4 == cookie
 *
 * ret0 == status
 * ret1 == new_base
 * ret2 == response
 * ret3 == new_cookie
 */
#define SMC_RSI_MEM_SET_PERM_INDEX	SMC64_RSI_FID(U(0x11))

/*
 * FID: 0xC40001A2
 *
 * arg1 == plane index
 * arg2 == perm index
 *
 * ret0 == status
 */
#define SMC_RSI_MEM_SET_PERM_VALUE	SMC64_RSI_FID(U(0x12))

/*
 * FID: 0xC40001A3
 */
#define SMC_RSI_PLANE_ENTER		SMC64_RSI_FID(U(0x13))

/*
 * FID: 0xC40001A4
 *
 * arg0 == Realm device identifier
 */
#define SMC_RSI_VDEV_DMA_DISABLE	SMC64_RSI_FID(U(0x14))

/*
 * FID: 0xC40001A5 - 0xC40001AD are not used.
 */

/*
 * FID: 0xC40001AE
 */
#define SMC_RSI_PLANE_SYSREG_READ	SMC64_RSI_FID(U(0x1E))

/*
 * FID: 0xC40001AF
 */
#define SMC_RSI_PLANE_SYSREG_WRITE	SMC64_RSI_FID(U(0x1F))

typedef enum {
	RSI_EMPTY = 0U,
	RSI_RAM,
	RSI_DESTROYED,
	RSI_DEV
} rsi_ripas_type;

typedef enum {
	RSI_ACCEPT = 0U,
	RSI_REJECT
} rsi_response_type;

/*
 * RsiRipasChangeDestroyed:
 * RIPAS change from DESTROYED should not be permitted
 */
#define RSI_NO_CHANGE_DESTROYED	U(0)

/* A RIPAS change from DESTROYED should be permitted */
#define RSI_CHANGE_DESTROYED	U(1)


/*
 * RsiFeature
 * Represents whether a feature is enabled.
 * Width: 1 bit
 */
#define RSI_FEATURE_FALSE			U(0)
#define RSI_FEATURE_TRUE			U(1)

/*
 * RsiFeatureRegister0
 * Fieldset contains feature register 0
 * Width: 64 bits
 */
#define RSI_FEATURE_REGISTER_0_INDEX		UL(0)
#define RSI_FEATURE_REGISTER_0_DA_SHIFT		UL(0)
#define RSI_FEATURE_REGISTER_0_DA_WIDTH		UL(1)
#define RSI_FEATURE_REGISTER_0_MRO_SHIFT	UL(1)
#define RSI_FEATURE_REGISTER_0_MRO_WIDTH	UL(1)
#define RSI_FEATURE_REGISTER_0_ATS_SHIFT	UL(2)
#define RSI_FEATURE_REGISTER_0_ATS_WIDTH	UL(1)

/*
 * RsiDevMemShared
 * Represents whether an device memory mapping is shared.
 * Width: 1 bit
 */
#define RSI_DEV_MEM_MAPPING_PRIVATE		U(0)
#define RSI_DEV_MEM_MAPPING_SHARED		U(1)

/*
 * RsiDevMemCoherent
 * Represents whether a device memory location is within the system coherent
 * memory space.
 * Width: 1 bit
 */
#define RSI_DEV_MEM_NON_COHERENT		U(0)
#define RSI_DEV_MEM_COHERENT			U(1)

/*
 * RsiRdevValidateIoFlags
 * Fieldset contains flags provided when requesting validation of an IO mapping.
 * Width: 64 bits
 */
/* RsiDevMemShared: Bits 0 to 1 */
#define RSI_RDEV_VALIDATE_IO_FLAGS_SHARE_SHIFT	UL(0)
#define RSI_RDEV_VALIDATE_IO_FLAGS_SHARE_WIDTH	UL(1)
/* RsiDevMemCoherent: Bits 1 to 2 */
#define RSI_RDEV_VALIDATE_IO_FLAGS_COH_SHIFT	UL(1)
#define RSI_RDEV_VALIDATE_IO_FLAGS_COH_WIDTH	UL(1)

/*
 * RsiVdevState
 * This enumeration represents the state of a VDEV.
 * Width: 8 bits.
 */
#define RSI_VDEV_STATE_NEW			U(0)
#define RSI_VDEV_STATE_UNLOCKED			U(1)
#define RSI_VDEV_STATE_LOCKED			U(2)
#define RSI_VDEV_STATE_STARTED			U(3)
#define RSI_VDEV_STATE_ERROR			U(4)

/*
 * RsiDevFlags
 * Fieldset contains flags which describe properties of a device.
 * Width: 64 bits
 */
#define RSI_DEV_FLAGS_P2P_SHIFT			UL(0)
#define RSI_DEV_FLAGS_P2P_WIDTH			UL(1)

/*
 * RsiDevAttestType
 * This enumeration represents attestation type of a device.
 * Width: 64 bits.
 */
#define RSI_DEV_ATTEST_TYPE_INDEPENDENTLY_ATTESTED	U(0)
#define RSI_DEV_ATTEST_TYPE_PLATFORM_ATTESTED		U(1)

#define RSI_VDEV_VCA_DIGEST_LEN			U(64)
#define RSI_VDEV_CERT_DIGEST_LEN		U(64)
#define RSI_VDEV_PUBKEY_DIGEST_LEN		U(64)
#define RSI_VDEV_MEAS_DIGEST_LEN		U(64)
#define RSI_VDEV_REPORT_DIGEST_LEN		U(64)

/*
 * RsiVdevFlags
 * Contains flags which describe properties of a device.
 * Width: 8 bytes
 */
#define RSI_VDEV_FLAGS_P2P_ENABLED_SHIFT	U(0)
#define RSI_VDEV_FLAGS_P2P_ENABLED_WIDTH	U(1)
#define RSI_VDEV_FLAGS_P2P_BOUND_SHIFT		U(1)
#define RSI_VDEV_FLAGS_P2P_BOUND_WIDTH		U(1)

/*
 * RsiVdevInfo
 * Contains device configuration information.
 * Width: 512 (0x200) bytes.
 */
struct rsi_vdev_info {
	/* RsiDevFlags: Flags */
	SET_MEMBER_RSI(unsigned long flags, 0, 0x8);
	/* UInt64: cert_id*/
	SET_MEMBER_RSI(unsigned long cert_id, 0x8, 0x10);
	/* RsiHashAlgorithm: Algorithm used to generate device digests */
	SET_MEMBER_RSI(unsigned char hash_algo, 0x10, 0x18);
	/* UInt64: Nonce generated on most recent transition to LOCKED state */
	SET_MEMBER_RSI(unsigned long lock_nonce, 0x18, 0x20);
	/* UInt64: Nonce generated on most recent GET_MEASUREMENT request */
	SET_MEMBER_RSI(unsigned long meas_nonce, 0x20, 0x28);
	/* UInt64: Nonce generated on most recent GET_INTERFACE_REPORT request */
	SET_MEMBER_RSI(unsigned long report_nonce, 0x28, 0x30);
	/* UInt64: TDISP version of the device */
	SET_MEMBER_RSI(unsigned long tdisp_version, 0x30, 0x38);
	/* RsiVdevState: State of the device */
	SET_MEMBER_RSI(unsigned char state, 0x38, 0x40);
	/* Bits512: VCA digest */
	SET_MEMBER_RSI(unsigned char vca_digest[RSI_VDEV_VCA_DIGEST_LEN], 0x40, 0x80);
	/* Bits512: Certificate digest */
	SET_MEMBER_RSI(unsigned char cert_digest[RSI_VDEV_CERT_DIGEST_LEN], 0x80, 0xc0);
	/* Bits512: Public key digest */
	SET_MEMBER_RSI(unsigned char pubkey_digest[RSI_VDEV_PUBKEY_DIGEST_LEN], 0xc0, 0x100);
	/* Bits512: Measurement digest */
	SET_MEMBER_RSI(unsigned char meas_digest[RSI_VDEV_MEAS_DIGEST_LEN], 0x100, 0x140);
	/* Bits512: Interface report digest */
	SET_MEMBER_RSI(unsigned char report_digest[RSI_VDEV_REPORT_DIGEST_LEN], 0x140, 0x200);
};

/*
 * RsiDevMeasureAll
 * Represents whether all device measurements should be returned.
 * Width: 1 bit
 */
#define RSI_DEV_MEASURE_NOT_ALL			U(0)
#define RSI_DEV_MEASURE_ALL			U(1)

/*
 * RsiDevMeasureSigned
 * Represents whether a device measurement is signed.
 * Width: 1 bit
 */
#define RSI_DEV_MEASURE_NOT_SIGNED		U(0)
#define RSI_DEV_MEASURE_SIGNED			U(1)

/*
 * RsiDevMeasureRaw
 * Represents whether a device measurement is a raw bitstream.
 * Width: 1 bit
 */
#define RSI_DEV_MEASURE_NOT_RAW			U(0)
#define RSI_DEV_MEASURE_RAW			U(1)

#define RSI_PLANE_NR_GPRS	31U
#define RSI_PLANE_GIC_NUM_LRS	16U

/*
 * Flags provided by the Primary Plane to the secondary ones upon
 * plane entry.
 */
#define RSI_PLANE_ENTRY_FLAG_TRAP_WFI	U(1UL << 0)
#define RSI_PLANE_ENTRY_FLAG_TRAP_WFE	U(1UL << 1)
#define RSI_PLANE_ENTRY_FLAG_TRAP_HC	U(1UL << 2)
#define RSI_PLANE_ENTRY_FLAG_TRAP_SIMD	U(1UL << 4)

#define RSI_PLANE_ENTRY_FLAG_MASK	U(0x1F)

/* Data structure used to pass values from P0 to the RMM on Plane entry */
struct rsi_plane_entry {
	/* Flags */
	SET_MEMBER(u_register_t flags, 0, 0x8);		/* Offset 0 */
	/* PC */
	SET_MEMBER(u_register_t pc, 0x8, 0x10);		/* Offset 0x8 */
	/* PSTATE */
	SET_MEMBER(u_register_t pstate, 0x10, 0x100);	/* Offset 0x10 */
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[RSI_PLANE_NR_GPRS], 0x100, 0x200);	/* 0x100 */
	/* EL1 system registers */
	SET_MEMBER(struct {
		/* GICv3 Hypervisor Control Register */
		u_register_t gicv3_hcr;                         /* 0x200 */
		/* GICv3 List Registers */
		u_register_t gicv3_lrs[RSI_PLANE_GIC_NUM_LRS];        /* 0x208 */
	}, 0x200, 0x800);
};

/* Data structure used to pass values from the RMM to P0 on Plane exit */
struct rsi_plane_exit {
	/* Exit reason */
	SET_MEMBER(u_register_t exit_reason, 0, 0x100);/* Offset 0 */
	SET_MEMBER(struct {
		/* Exception Link Register */
		u_register_t elr;				/* 0x100 */
		/* Exception Syndrome Register */
		u_register_t esr;				/* 0x108 */
		/* Fault Address Register */
		u_register_t far;				/* 0x110 */
		/* Hypervisor IPA Fault Address register */
		u_register_t hpfar;				/* 0x118 */
		/* Hypervisor PSTATE */
		u_register_t pstate;				/* 0x120 */
	}, 0x100, 0x200);
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[RSI_PLANE_NR_GPRS], 0x200, 0x300); /* 0x200 */
	SET_MEMBER(struct {
		/* GICv3 Hypervisor Control Register */
		u_register_t gicv3_hcr;				/* 0x300 */
		/* GICv3 List Registers */
		u_register_t gicv3_lrs[RSI_PLANE_GIC_NUM_LRS];	/* 0x308 */
		/* GICv3 Maintenance Interrupt State Register */
		u_register_t gicv3_misr;			/* 0x388 */
		/* GICv3 Virtual Machine Control Register */
		u_register_t gicv3_vmcr;			/* 0x390 */
	}, 0x300, 0x600);
};

typedef struct {
	/* Entry information */
	SET_MEMBER(struct rsi_plane_entry enter, 0, 0x800); /* Offset 0 */
	/* Exit information */
	SET_MEMBER(struct rsi_plane_exit exit, 0x800, 0x1000);/* 0x800 */
} rsi_plane_run;

u_register_t rsi_plane_sysreg_read(u_register_t plane_index, u_register_t register_encoding,
		u_register_t *value_lo, u_register_t *value_hi);

u_register_t rsi_plane_sysreg_write(u_register_t plane_index, u_register_t register_encoding,
		u_register_t value_lo, u_register_t value_hi);

/*
 * Function to set overlay permission value for a specified
 * (plane index, overlay permission index) tuple
 */
u_register_t rsi_mem_set_perm_value(u_register_t plane_index,
	u_register_t perm_index,
	u_register_t perm);

/*
 * Function to Get overlay permission value for a specified
 * (plane index, overlay permission index) tuple
 */
u_register_t rsi_mem_get_perm_value(u_register_t plane_index,
	u_register_t perm_index,
	u_register_t *perm);

/* Function to Set overlay permission index for a specified IPA range See RSI_MEM_SET_PERM_INDEX */
u_register_t rsi_mem_set_perm_index(u_register_t base,
	u_register_t top,
	u_register_t perm_index,
	u_register_t cookie,
	u_register_t *new_base,
	u_register_t *response,
	u_register_t *new_cookie);

/* Request RIPAS of a target IPA range to be changed to a specified value */
u_register_t rsi_ipa_state_set(u_register_t base,
				u_register_t top,
				rsi_ripas_type ripas,
				u_register_t flag,
				u_register_t *new_base,
				rsi_response_type *response);

/* Request RIPAS of a target IPA range */
u_register_t rsi_ipa_state_get(u_register_t base,
				u_register_t top,
				u_register_t *out_top,
				rsi_ripas_type *ripas);

/* This function return RSI_ABI_VERSION */
u_register_t rsi_get_version(u_register_t req_ver);

/* This function returns RSI feature register at 'feature_reg_index' */
u_register_t rsi_features(u_register_t feature_reg_index,
			  u_register_t *feature_reg_value_ret);

/* This function will initialize the attestation context */
u_register_t rsi_attest_token_init(u_register_t challenge_0,
				   u_register_t challenge_1,
				   u_register_t challenge_2,
				   u_register_t challenge_3,
				   u_register_t challenge_4,
				   u_register_t challenge_5,
				   u_register_t challenge_6,
				   u_register_t challenge_7,
				   u_register_t *out_token_upper_bound);

/* This function will retrieve the (or part of) attestation token */
u_register_t rsi_attest_token_continue(u_register_t buffer_addr,
					u_register_t offset,
					u_register_t buffer_size,
					u_register_t *bytes_copied);

/* This function call Host and request to exit Realm with proper exit code */
u_register_t rsi_exit_to_host(enum host_call_cmd exit_code);

/* Function to get Realm configuration. See RSI_REALM_CONFIG */
u_register_t rsi_realm_config(struct rsi_realm_config *s);

/* Function to enter aux plane. See RSI_PLANE_ENTER */
u_register_t rsi_plane_enter(u_register_t plane_index, u_register_t run);

u_register_t rsi_vdev_get_info(u_register_t rdev_id, u_register_t rdev_info_ptr);

#endif /* REALM_RSI_H */
