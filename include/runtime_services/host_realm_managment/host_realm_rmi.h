/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HOST_REALM_RMI_H
#define HOST_REALM_RMI_H

#include <stdint.h>
#include <stdbool.h>

#include <realm_def.h>
#include <smccc.h>
#include <utils_def.h>

#define RMI_FNUM_MIN_VALUE	U(0x150)
#define RMI_FNUM_MAX_VALUE	U(0x18F)

/*
 * Defines member of structure and reserves space
 * for the next member with specified offset.
 */
#define SET_MEMBER_RMI	SET_MEMBER

/* Get RMI fastcall std FID from offset */
#define SMC64_RMI_FID(_offset)					\
	((SMC_TYPE_FAST << FUNCID_TYPE_SHIFT) |			\
	(SMC_64 << FUNCID_CC_SHIFT) |				\
	(OEN_STD_START << FUNCID_OEN_SHIFT) |			\
	(((RMI_FNUM_MIN_VALUE + (_offset)) & FUNCID_NUM_MASK)	\
	<< FUNCID_NUM_SHIFT))

#define RMI_ABI_VERSION_GET_MAJOR(_version)	(((_version) >> 16U) & 0x8FFF)
#define RMI_ABI_VERSION_GET_MINOR(_version)	((_version) & 0xFFFF)
#define RMI_ABI_VERSION_MAJOR			U(1)
#define RMI_ABI_VERSION_MINOR			U(0)
#define RMI_ABI_VERSION_VAL			((RMI_ABI_VERSION_MAJOR << 16U) | \
						 RMI_ABI_VERSION_MINOR)

#define __ALIGN_MASK(x, mask)		(((x) + (mask)) & ~(mask))
#define __ALIGN(x, a)			__ALIGN_MASK(x, (typeof(x))(a) - 1U)
#define ALIGN(x, a)			__ALIGN((x), (a))

/*
 * SMC_RMM_INIT_COMPLETE is the only function in the RMI that originates from
 * the Realm world and is handled by the RMMD. The remaining functions are
 * always invoked by the Normal world, forwarded by RMMD and handled by the
 * RMM
 */
/* RMI SMC64 FIDs handled by the RMMD */

/*
 * FID: 0xC4000150
 *
 * arg0: Requested interface version
 *
 * ret0: Command return status
 * ret1: Lower implemented interface revision
 * ret2: Higher implemented interface revision
 */
#define SMC_RMI_VERSION				SMC64_RMI_FID(U(0x0))

/*
 * FID: 0xC4000151
 *
 * arg0 == target granule address
 */
#define SMC_RMI_GRANULE_DELEGATE		SMC64_RMI_FID(U(0x1))

/*
 * FID: 0xC4000152
 *
 * arg0 == target granule address
 */
#define SMC_RMI_GRANULE_UNDELEGATE		SMC64_RMI_FID(U(0x2))

/* RmiDataMeasureContent type */
#define RMI_NO_MEASURE_CONTENT	U(0)
#define RMI_MEASURE_CONTENT	U(1)

/*
 * FID: 0xC4000153
 *
 * arg0 == RD address
 * arg1 == data address
 * arg2 == map address
 * arg3 == SRC address
 * arg4 == flags
 */
#define SMC_RMI_DATA_CREATE			SMC64_RMI_FID(U(0x3))

/*
 * FID: 0xC4000154
 *
 * arg0 == RD address
 * arg1 == data address
 * arg2 == map address
 */
#define SMC_RMI_DATA_CREATE_UNKNOWN		SMC64_RMI_FID(U(0x4))

/*
 * FID: 0xC4000155
 *
 * arg0 == RD address
 * arg1 == map address
 *
 * ret1 == Address(PA) of the DATA granule, if ret0 == RMI_SUCCESS.
 *         Otherwise, undefined.
 * ret2 == Top of the non-live address region. Only valid
 *         if ret0 == RMI_SUCCESS or ret0 == (RMI_ERROR_RTT, x)
 */
#define SMC_RMI_DATA_DESTROY			SMC64_RMI_FID(U(0x5))

/*
 * FID: 0xC4000156
 */
#define SMC_RMI_PDEV_AUX_COUNT			SMC64_RMI_FID(U(0x6))

/*
 * FID: 0xC4000157
 *
 * arg0 == RD address
 */
#define SMC_RMI_REALM_ACTIVATE			SMC64_RMI_FID(U(0x7))

/*
 * FID: 0xC4000158
 *
 * arg0 == RD address
 * arg1 == struct rmi_realm_params address
 */
#define SMC_RMI_REALM_CREATE			SMC64_RMI_FID(U(0x8))

/*
 * FID: 0xC4000159
 *
 * arg0 == RD address
 */
#define SMC_RMI_REALM_DESTROY			SMC64_RMI_FID(U(0x9))

/*
 * FID: 0xC400015A
 *
 * arg0 == RD address
 * arg1 == REC address
 * arg2 == struct rmm_rec address
 */
#define SMC_RMI_REC_CREATE			SMC64_RMI_FID(U(0xA))

/*
 * FID: 0xC400015B
 *
 * arg0 == REC address
 */
#define SMC_RMI_REC_DESTROY			SMC64_RMI_FID(U(0xB))

/*
 * FID: 0xC400015C
 *
 * arg0 == rec address
 * arg1 == struct rec_run address
 */
#define SMC_RMI_REC_ENTER			SMC64_RMI_FID(U(0xC))

/*
 * FID: 0xC400015D
 *
 * arg0 == RD address
 * arg1 == RTT address
 * arg2 == map address
 * arg3 == level
 */
#define SMC_RMI_RTT_CREATE			SMC64_RMI_FID(U(0xD))

/*
 * FID: 0xC400015E
 *
 * arg0 == RD address
 * arg1 == RTT address
 * arg2 == map address
 * arg3 == level
 * arg4 == RTT tree index
 *
 * ret0 == status/index
 */
#define RMI_RTT_AUX_CREATE		SMC64_RMI_FID(U(0x2D))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 * arg3 == RTT tree index
 *
 * ret1 == Address (PA) of the RTT, if ret0 == RMI_SUCCESS
 *         Otherwise, undefined.
 * ret2 == Top of the non-live address region. Only valid
 *         if ret0 == RMI_SUCCESS or ret0 == (RMI_ERROR_RTT_WALK, x)
 */
#define RMI_RTT_AUX_DESTROY		SMC64_RMI_FID(U(0x2E))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 *
 * ret1 == Address (PA) of the RTT, if ret0 == RMI_SUCCESS
 *         Otherwise, undefined.
 * ret2 == Top of the non-live address region. Only valid
 *         if ret0 == RMI_SUCCESS or ret0 == (RMI_ERROR_RTT, x)
 */
#define SMC_RMI_RTT_DESTROY			SMC64_RMI_FID(U(0xE))

/*
 * FID: 0xC400015F
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 * arg3 == s2tte
 */
#define SMC_RMI_RTT_MAP_UNPROTECTED		SMC64_RMI_FID(U(0xF))

/*
 * FID: 0xC4000160
 */
#define SMC_RMI_VDEV_AUX_COUNT			SMC64_RMI_FID(U(0x10))

/*
 * FID: 0xC4000161
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 *
 * ret1 == level
 * ret2 == s2tte type
 * ret3 == s2tte
 * ret4 == ripas
 * if ret0 == RMI_SUCCESS, otherwise, undefined.
 */
#define SMC_RMI_RTT_READ_ENTRY			SMC64_RMI_FID(U(0x11))

/*
 * FID: 0xC4000162
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 */
#define SMC_RMI_RTT_UNMAP_UNPROTECTED		SMC64_RMI_FID(U(0x12))

/*
 * FID: 0xC4000164
 *
 * arg0 == calling rec address
 * arg1 == target rec address
 */
#define SMC_RMI_PSCI_COMPLETE			SMC64_RMI_FID(U(0x14))

/*
 * FID: 0xC4000165
 *
 * arg0 == Feature register index
 */
#define SMC_RMI_FEATURES			SMC64_RMI_FID(U(0x15))

/*
 * FID: 0xC4000166
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 *
 * ret1 == Address(PA) of the RTT folded, if ret0 == RMI_SUCCESS
 */
#define SMC_RMI_RTT_FOLD			SMC64_RMI_FID(U(0x16))

/*
 * FID: 0xC4000167
 *
 * arg0 == RD address
 */
#define SMC_RMI_REC_AUX_COUNT			SMC64_RMI_FID(U(0x17))

/*
 * FID: 0xC4000168
 *
 * arg0 == RD address
 * arg1 == start address
 * arg2 == end address
 *
 * ret1 == Top of the address range where the RIPAS was updated,
 * if ret0 == RMI_SUCCESS
 */
#define SMC_RMI_RTT_INIT_RIPAS			SMC64_RMI_FID(U(0x18))

/*
 * FID: 0xC4000169
 *
 * arg0 == RD address
 * arg1 == REC address
 * arg2 == start address
 * arg3 == end address
 *
 * ret1 == Top of the address range where the RIPAS was updated,
 *	   if ret0 == RMI_SUCCESS
 */
#define SMC_RMI_RTT_SET_RIPAS			SMC64_RMI_FID(U(0x19))

/*
 * TODO: Update the documentation of new FIDs once the 1.1 spec has stabilized.
 */

/*
 * FID: 0xC400016A
 */
#define SMC_RMI_VSMMU_CREATE			SMC64_RMI_FID(U(0x1A))

/*
 * FID: 0xC400016B
 */
#define SMC_RMI_VSMMU_DESTROY			SMC64_RMI_FID(U(0x1B))

/*
 * FID: 0xC400016C
 */
#define SMC_RMI_VSMMU_MAP			SMC64_RMI_FID(U(0x1C))

/*
 * FID: 0xC400016D
 */
#define SMC_RMI_VSMMU_UNMAP			SMC64_RMI_FID(U(0x1D))

/*
 * FID: 0xC400016E
 */
#define SMC_RMI_PSMMU_MSI_CONFIG		SMC64_RMI_FID(U(0x1E))

/*
 * FID: 0xC400016F
 */
#define SMC_RMI_PSMMU_IRQ_NOTIFY		SMC64_RMI_FID(U(0x1F))

/*
 * FID: 0xC4000170 and 0xC4000171 are not used.
 */

/*
 * FID: 0xC4000172
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 * arg3 == PA of the target device memory
 */
#define SMC_RMI_DEV_MEM_MAP			SMC64_RMI_FID(U(0x22))

/*
 * FID: 0xC4000173
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 *
 * ret1 == Address (PA) of the device memory granule, if ret0 == RMI_SUCCESS
 *         Otherwise, undefined.
 * ret2 == Top of the non-live address region. Only valid
 *         if ret0 == RMI_SUCCESS or ret0 == (RMI_ERROR_RTT, x)
 */
#define SMC_RMI_DEV_MEM_UNMAP			SMC64_RMI_FID(U(0x23))

/*
 * FID: 0xC4000174
 */
#define SMC_RMI_PDEV_ABORT			SMC64_RMI_FID(U(0x24))

/*
 * FID: 0xC4000175
 */
#define SMC_RMI_PDEV_COMMUNICATE		SMC64_RMI_FID(U(0x25))

/*
 * FID: 0xC4000176
 */
#define SMC_RMI_PDEV_CREATE			SMC64_RMI_FID(U(0x26))

/*
 * FID: 0xC4000177
 */
#define SMC_RMI_PDEV_DESTROY			SMC64_RMI_FID(U(0x27))

/*
 * FID: 0xC4000178
 */
#define SMC_RMI_PDEV_GET_STATE			SMC64_RMI_FID(U(0x28))

/*
 * FID: 0xC4000179
 */
#define SMC_RMI_PDEV_IDE_RESET			SMC64_RMI_FID(U(0x29))

/*
 * FID: 0xC400017A
 */
#define SMC_RMI_PDEV_NOTIFY			SMC64_RMI_FID(U(0x2A))

/*
 * FID: 0xC400017B
 */
#define SMC_RMI_PDEV_SET_PUBKEY			SMC64_RMI_FID(U(0x2B))

/*
 * FID: 0xC400017C
 */
#define SMC_RMI_PDEV_STOP			SMC64_RMI_FID(U(0x2C))

/*
 * FID: 0xC400017D
 */
#define SMC_RMI_RTT_AUX_CREATE			SMC64_RMI_FID(U(0x2D))

/*
 * FID: 0xC400017E
 */
#define SMC_RMI_RTT_AUX_DESTROY			SMC64_RMI_FID(U(0x2E))

/*
 * FID: 0xC400017F
 */
#define SMC_RMI_RTT_AUX_FOLD			SMC64_RMI_FID(U(0x2F))

/*
 * FID: 0xC4000180
 */
#define SMC_RMI_RTT_AUX_MAP_PROTECTED		SMC64_RMI_FID(U(0x30))

/*
 * FID: 0xC4000181
 */
#define SMC_RMI_RTT_AUX_MAP_UNPROTECTED		SMC64_RMI_FID(U(0x31))

/*
 * FID: 0xC4000182 is not used.
 */

/*
 * FID: 0xC4000183
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == RTT tree index
 *
 * ret0 == status/index
 * ret1 == top
 * ret2 == level
 */
#define SMC_RMI_RTT_AUX_UNMAP_PROTECTED		SMC64_RMI_FID(U(0x33))

/*
 * FID: 0xC4000184
 *
 * arg0 == RD address
 * arg1 == map address
 * arg2 == RTT tree index
 *
 * ret0 == status/index
 * ret1 == top
 * ret2 == level
 */
#define SMC_RMI_RTT_AUX_UNMAP_UNPROTECTED	SMC64_RMI_FID(U(0x34))

/*
 * FID: 0xC4000185
 */
#define SMC_RMI_VDEV_ABORT			SMC64_RMI_FID(U(0x35))

/*
 * FID: 0xC4000186
 */
#define SMC_RMI_VDEV_COMMUNICATE		SMC64_RMI_FID(U(0x36))

/*
 * FID: 0xC4000187
 */
#define SMC_RMI_VDEV_CREATE			SMC64_RMI_FID(U(0x37))

/*
 * FID: 0xC4000188
 */
#define SMC_RMI_VDEV_DESTROY			SMC64_RMI_FID(U(0x38))

/*
 * FID: 0xC4000189
 */
#define SMC_RMI_VDEV_GET_STATE			SMC64_RMI_FID(U(0x39))

/*
 * FID: 0xC400018A
 */
#define SMC_RMI_VDEV_STOP			SMC64_RMI_FID(U(0x3A))

/*
 * FID: 0xC400018B
 *
 * arg0 == RD address
 * arg1 == target rec
 * arg2 == base adr
 * arg3 == top adr
 *
 * ret0 == return code
 * ret1 == Top IPA of range whose S2AP was modified
 * ret2 == Index of RTT tree in which base alignment check failed
 */
#define SMC_RMI_RTT_SET_S2AP			SMC64_RMI_FID(U(0x3B))

/*
 * FID: 0xC400018C
 */
#define SMC_RMI_MEC_SET_SHARED			SMC64_RMI_FID(U(0x3C))

/*
 * FID: 0xC400018D
 */
#define SMC_RMI_MEC_SET_PRIVATE			SMC64_RMI_FID(U(0x3D))

/*
 * FID: 0xC400018E
 */
#define SMC_RMI_VDEV_COMPLETE			SMC64_RMI_FID(U(0x3E))

/*
 * FID: 0xC400018F is not used.
 */

#define GRANULE_SIZE			PAGE_SIZE_4KB

/* Maximum number of auxiliary granules required for a REC */
#define MAX_REC_AUX_GRANULES		16U
#define REC_PARAMS_AUX_GRANULES		16U
#define REC_EXIT_NR_GPRS		31U

/* Size of Realm Personalization Value */
#define RPV_SIZE			64U

/* RmiDisposeResponse types */
#define RMI_DISPOSE_ACCEPT		0U
#define RMI_DISPOSE_REJECT		1U

/* RmiFeature enumerations */
#define RMI_FEATURE_FALSE		0U
#define RMI_FEATURE_TRUE		1U

/* RmiRealmFlags0 format */
#define RMI_REALM_FLAGS0_LPA2		BIT(0)
#define RMI_REALM_FLAGS0_SVE		BIT(1)
#define RMI_REALM_FLAGS0_PMU		BIT(2)
#define RMI_REALM_FLAGS0_DA		BIT(3)

/* RmiRealmFlags1 format */
#define RMI_REALM_FLAGS1_RTT_TREE_PP			BIT(0)
#define RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT	BIT(1)

/* RmiInterfaceVersion type */
#define RMI_MAJOR_VERSION		0U
#define RMI_MINOR_VERSION		0U

/* RmiRealmMeasurementAlgorithm types */
#define RMI_HASH_SHA_256		0U
#define RMI_HASH_SHA_512		1U

/* RmiRecEmulatedMmio types */
#define RMI_NOT_EMULATED_MMIO		0U
#define RMI_EMULATED_MMIO		1U

/*
 * RmiRecExitReason represents the reason for a REC exit.
 * This is returned to NS hosts via RMI_REC_ENTER::run_ptr.
 */
#define RMI_EXIT_SYNC			U(0)
#define RMI_EXIT_IRQ			U(1)
#define RMI_EXIT_FIQ			U(2)
#define RMI_EXIT_PSCI			U(3)
#define RMI_EXIT_RIPAS_CHANGE		U(4)
#define RMI_EXIT_HOST_CALL		U(5)
#define RMI_EXIT_SERROR			U(6)
#define RMI_EXIT_S2AP_CHANGE		U(7)
#define RMI_EXIT_VDEV_REQUEST		U(8)
#define RMI_EXIT_VDEV_COMM		U(9)
#define RMI_EXIT_DEV_MEM_MAP		U(10)
#define RMI_EXIT_INVALID		(RMI_EXIT_DEV_MEM_MAP + 1U)

/* RmiRecRunnable types */
#define RMI_NOT_RUNNABLE		0U
#define RMI_RUNNABLE			1U

/* RmiRttEntryState: represents the state of an RTTE */
#define RMI_UNASSIGNED			UL(0)
#define RMI_ASSIGNED			UL(1)
#define RMI_TABLE			UL(2)
#define RMI_ASSIGNED_DEV		UL(3)

/* RmmRipas enumeration representing realm IPA state */
#define RMI_EMPTY			UL(0)
#define RMI_RAM				UL(1)
#define RMI_DESTROYED			UL(2)
#define RMI_DEV				UL(3)

/* RmiPmuOverflowStatus enumeration representing PMU overflow status */
#define RMI_PMU_OVERFLOW_NOT_ACTIVE	0U
#define RMI_PMU_OVERFLOW_ACTIVE		1U

/* RmiFeatureRegister0 format */
#define RMI_FEATURE_REGISTER_0_S2SZ_SHIFT		0UL
#define RMI_FEATURE_REGISTER_0_S2SZ_WIDTH		8UL
#define RMI_FEATURE_REGISTER_0_LPA2			BIT(8)
#define RMI_FEATURE_REGISTER_0_SVE_EN			BIT(9)
#define RMI_FEATURE_REGISTER_0_SVE_VL_SHIFT		10UL
#define RMI_FEATURE_REGISTER_0_SVE_VL_WIDTH		4UL
#define RMI_FEATURE_REGISTER_0_NUM_BPS_SHIFT		14UL
#define RMI_FEATURE_REGISTER_0_NUM_BPS_WIDTH		6UL
#define RMI_FEATURE_REGISTER_0_NUM_WPS_SHIFT		20UL
#define RMI_FEATURE_REGISTER_0_NUM_WPS_WIDTH		6UL
#define RMI_FEATURE_REGISTER_0_PMU_EN			BIT(26)
#define RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS_SHIFT	27UL
#define RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS_WIDTH	5UL
#define RMI_FEATURE_REGISTER_0_HASH_SHA_256		BIT(32)
#define RMI_FEATURE_REGISTER_0_HASH_SHA_512		BIT(33)
#define RMI_FEATURE_REGISTER_0_GICV3_NUM_LRS_SHIFT	34UL
#define RMI_FEATURE_REGISTER_0_GICV3_NUM_LRS_WIDTH	4UL
#define RMI_FEATURE_REGISTER_0_MAX_RECS_ORDER_SHIFT	38UL
#define RMI_FEATURE_REGISTER_0_MAX_RECS_ORDER_WIDTH	4UL
#define RMI_FEATURE_REGISTER_0_DA_EN			BIT(42)
#define RMI_FEATURE_REGISTER_0_PLANE_RTT_SHIFT		43UL
#define RMI_FEATURE_REGISTER_0_PLANE_RTT_WIDTH		2UL
#define RMI_FEATURE_REGISTER_0_MAX_NUM_AUX_PLANES_SHIFT	45UL
#define RMI_FEATURE_REGISTER_0_MAX_NUM_AUX_PLANES_WIDTH	4UL

/*
 * Format of feature_flag[63:32].
 * Value -1 (0 in case of NUM_BPS and NUM_WPS) indicates not set field,
 * and parameter will be set from the corresponding field of feature register 0.
 */
#define FEATURE_SVE_VL_SHIFT				56UL
#define FEATURE_SVE_VL_WIDTH				4UL
#define FEATURE_NUM_BPS_SHIFT				14UL
#define FEATURE_NUM_BPS_WIDTH				6UL
#define FEATURE_NUM_WPS_SHIFT				20UL
#define FEATURE_NUM_WPS_WIDTH				6UL
#define FEATURE_PMU_NUM_CTRS_SHIFT			35UL
#define FEATURE_PMU_NUM_CTRS_WIDTH			4UL

#define RMI_FEATURE_REGISTER_0_RTT_S2AP_INDIRECT	BIT(49)

/* Possible values for RmiPlaneRttFeature */
#define RMI_PLANE_RTT_AUX				0UL
#define RMI_PLANE_RTT_AUX_SINGLE			1UL
#define RMI_PLANE_RTT_SINGLE				2UL

/* RmiStatusCode types */
/*
 * Status codes which can be returned from RMM commands.
 *
 * For each code, the meaning of return_code_t::index is stated.
 */
typedef enum {
	/*
	 * Command completed successfully.
	 *
	 * index is zero.
	 */
	RMI_SUCCESS = 0,
	/*
	 * The value of a command input value caused the command to fail.
	 *
	 * index is zero.
	 */
	RMI_ERROR_INPUT = 1,
	/*
	 * An attribute of a Realm does not match the expected value.
	 *
	 * index varies between usages.
	 */
	RMI_ERROR_REALM = 2,
	/*
	 * An attribute of a REC does not match the expected value.
	 *
	 * index is zero.
	 */
	RMI_ERROR_REC = 3,
	/*
	 * An RTT walk terminated before reaching the target RTT level,
	 * or reached an RTTE with an unexpected value.
	 *
	 * index: RTT level at which the walk terminated
	 */
	RMI_ERROR_RTT = 4,
	/*
	 * An attribute of a device does not match the expected value
	 */
	RMI_ERROR_DEVICE = 5,
	/*
	 * The command is not supported
	 */
	RMI_ERROR_NOT_SUPPORTED = 6,
	/*
	 * RTTE in an auxiliary RTT contained an unexpected value
	 */
	RMI_ERROR_RTT_AUX = 7,
	RMI_ERROR_COUNT
} status_t;

#define RMI_RETURN_STATUS(ret)		((ret) & 0xFF)
#define RMI_RETURN_INDEX(ret)		(((ret) >> 8U) & 0xFF)
#define RTT_MAX_LEVEL			(3L)
#define RTT_MIN_DEV_BLOCK_LEVEL		(2L)
#define RTT_MIN_LEVEL			(0L)
#define RTT_MIN_LEVEL_LPA2		(-1L)
#define ALIGN_DOWN(x, a)		((uint64_t)(x) & ~(((uint64_t)(a)) - 1ULL))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a)-1U)) == 0U)
#define PAGE_SHIFT			FOUR_KB_SHIFT
#define RTT_LEVEL_SHIFT(l)		XLAT_ADDR_SHIFT(l)
#define RTT_L2_BLOCK_SIZE		(1UL << RTT_LEVEL_SHIFT(2U))
#define RTT_MAP_SIZE(level)		(1UL << RTT_LEVEL_SHIFT(level))
#define RTT_L1_BLOCK_SIZE		(1UL << RTT_LEVEL_SHIFT(1U))

#define REC_CREATE_NR_GPRS		8U
#define REC_HVC_NR_GPRS			7U
#define REC_GIC_NUM_LRS			16U

/*
 * When FEAT_LPA2 is enabled bits [51:50] of the OA are not
 * contiguous to the rest of the OA.
 */

#define TTE_OA_50_51_SHIFT		ULL(8)
#define TTE_OA_50_51_WIDTH		ULL(2)

/* Bitfields for the 2 MSBs on an OA */
#define OA_50_51_SHIFT			ULL(50)
#define OA_50_51_WIDTH			TTE_OA_50_51_WIDTH
#define OA_50_51_MASK			MASK(OA_50_51)

/*
 * When FEAT_S2PIE is enabled PIINDEX is saved at following index
 */
#define S2TTE_PI_INDEX_BIT0_SHIFT	6
#define S2TTE_PI_INDEX_BIT1_SHIFT	51
#define S2TTE_PI_INDEX_BIT2_SHIFT	53
#define S2TTE_PI_INDEX_BIT3_SHIFT	54
#define S2TTE_PI_INDEX_MASK		((1UL << S2TTE_PI_INDEX_BIT0_SHIFT) | \
					(1UL << S2TTE_PI_INDEX_BIT1_SHIFT) | \
					(1UL << S2TTE_PI_INDEX_BIT2_SHIFT) | \
					(1UL << S2TTE_PI_INDEX_BIT3_SHIFT))
/*
 * RmiUnprotectedS2AP type
 * Encoding for S2AP base index value for UNPROTECTED IPA
 */
#define RMI_PERM_S2AP_NO_ACCESS_IDX		U(0)
#define RMI_PERM_S2AP_RO_IDX			U(1)
#define RMI_PERM_S2AP_WO_IDX			U(2)
#define RMI_PERM_S2AP_RW_IDX			U(3)

/*
 * The Realm attribute parameters are shared by the Host via
 * RMI_REALM_CREATE::params_ptr. The values can be observed or modified
 * either by the Host or by the Realm.
 */
struct rmi_realm_params {
	/* Flags */
	SET_MEMBER(unsigned long flags0, 0, 0x8);			/* Offset 0 */
	/* Requested IPA width */
	SET_MEMBER(unsigned int s2sz, 0x8, 0x10);			/* 0x8 */
	/* Requested SVE vector length */
	SET_MEMBER(unsigned int sve_vl, 0x10, 0x18);			/* 0x10 */
	/* Requested number of breakpoints */
	SET_MEMBER(unsigned int num_bps, 0x18, 0x20);			/* 0x18 */
	/* Requested number of watchpoints */
	SET_MEMBER(unsigned int num_wps, 0x20, 0x28);			/* 0x20 */
	/* Requested number of PMU counters */
	SET_MEMBER(unsigned int pmu_num_ctrs, 0x28, 0x30);		/* 0x28 */
	/* Measurement algorithm */
	SET_MEMBER(unsigned char algorithm, 0x30, 0x38);		/* 0x30 */
	/* Number of auxiliary Planes */
	SET_MEMBER(unsigned int num_aux_planes, 0x38, 0x400);		/* 0x38 */
	/* Realm Personalization Value */
	SET_MEMBER(unsigned char rpv[RPV_SIZE], 0x400, 0x800);		/* 0x400 */
	SET_MEMBER(struct {
			/* Virtual Machine Identifier */
			unsigned short vmid;				/* 0x800 */
			/* Realm Translation Table base */
			unsigned long rtt_base;				/* 0x808 */
			/* RTT starting level */
			long rtt_level_start;				/* 0x810 */
			/* Number of starting level RTTs */
			unsigned int rtt_num_start;			/* 0x818 */
		   }, 0x800, 0x820);
	/* Flags */
	SET_MEMBER(unsigned long flags1, 0x820, 0x828);			/* 0x820 */
	/* MECID */
	SET_MEMBER(long mecid, 0x828, 0xF00);				/* 0x828 */
	/* Auxiliary Virtual Machine Identifiers */
	SET_MEMBER(unsigned short aux_vmid[3], 0xF00, 0xF80);		/* 0xF00 */
	/* Base address of auxiliary RTTs */
	SET_MEMBER(unsigned long aux_rtt_base[3], 0xF80, 0x1000);	/* 0xF80 */
};

/*
 * The REC attribute parameters are shared by the Host via
 * MI_REC_CREATE::params_ptr. The values can be observed or modified
 * either by the Host or by the Realm which owns the REC.
 */
struct rmi_rec_params {
	/* Flags */
	SET_MEMBER(u_register_t flags, 0, 0x100);		/* Offset 0 */
	/* MPIDR of the REC */
	SET_MEMBER(u_register_t mpidr, 0x100, 0x200);		/* 0x100 */
	/* Program counter */
	SET_MEMBER(u_register_t pc, 0x200, 0x300);		/* 0x200 */
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[REC_CREATE_NR_GPRS], 0x300, 0x800); /* 0x300 */
	SET_MEMBER(struct {
		/* Number of auxiliary Granules */
		u_register_t num_aux;				/* 0x800 */
		/* Addresses of auxiliary Granules */
		u_register_t aux[MAX_REC_AUX_GRANULES];		/* 0x808 */
	}, 0x800, 0x1000);
};

/* Whether Host has completed emulation for an Emulatable Data Abort */
#define REC_ENTRY_FLAG_EMUL_MMIO	(UL(1) << 0)

/* Whether to inject a Synchronous External Abort into Realm */
#define REC_ENTRY_FLAG_INJECT_SEA	(UL(1) << 1)

/* Whether to trap WFI/WFE execution by Realm */
#define REC_ENTRY_FLAG_TRAP_WFI		(UL(1) << 2)
#define REC_ENTRY_FLAG_TRAP_WFE		(UL(1) << 3)

/* Host response to RIPAS change request */
#define REC_ENTRY_FLAG_RIPAS_RESPONSE_REJECT	(UL(1) << 4)

/*
 * Structure contains data passed from the Host to the RMM on REC entry
 */
struct rmi_rec_entry {
	/* Flags */
	SET_MEMBER(u_register_t flags, 0, 0x200);		/* Offset 0 */
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[REC_EXIT_NR_GPRS], 0x200, 0x300); /* 0x200 */
	SET_MEMBER(struct {
		/* GICv3 Hypervisor Control Register */
		u_register_t gicv3_hcr;				/* 0x300 */
		/* GICv3 List Registers */
		u_register_t gicv3_lrs[REC_GIC_NUM_LRS];	/* 0x308 */
	}, 0x300, 0x800);
};

/*
 * RmiVdevAction
 * Represents realm action which triggered REC exit due to device communication.
 * Width: 8 bits
 */
#define RMI_VDEV_ACTION_GET_INTERFACE_REPORT	U(0)
#define RMI_VDEV_ACTION_GET_MEASUREMENTS	U(1)
#define RMI_VDEV_ACTION_LOCK			U(2)
#define RMI_VDEV_ACTION_START			U(3)
#define RMI_VDEV_ACTION_STOP			U(4)

/*
 * RmiRecExitFlags
 * Fieldset contains flags provided by the RMM during REC exit.
 * Width: 64 bits
 */
#define RMI_REC_EXIT_FLAGS_RIPAS_DEV_SHARED_SHIFT	U(0)
#define RMI_REC_EXIT_FLAGS_RIPAS_DEV_SHARED_WIDTH	U(1)

/*
 * Structure contains data passed from the RMM to the Host on REC exit
 */
struct rmi_rec_exit {
	/* Exit reason */
	SET_MEMBER_RMI(unsigned long exit_reason, 0, 0x8);/* Offset 0 */
	/* RmiRecExitFlags: Flags */
	SET_MEMBER_RMI(unsigned long flags, 0x8, 0x100);/* 0x8 */
	SET_MEMBER_RMI(struct {
			/* Exception Syndrome Register */
			unsigned long esr;		/* 0x100 */
			/* Fault Address Register */
			unsigned long far;		/* 0x108 */
			/* Hypervisor IPA Fault Address register */
			unsigned long hpfar;		/* 0x110 */
			/* Index of RTT tree active at time of the exit */
			unsigned long rtt_tree;		/* 0x118 */
			/* Level of requested RTT */
			unsigned long rtt_level;	/* 0x120 */
		   }, 0x100, 0x200);
	/* General-purpose registers */
	SET_MEMBER_RMI(unsigned long gprs[REC_EXIT_NR_GPRS], 0x200, 0x300); /* 0x200 */
	SET_MEMBER_RMI(struct {
			/* GICv3 Hypervisor Control Register */
			unsigned long gicv3_hcr;	/* 0x300 */
			/* GICv3 List Registers */
			unsigned long gicv3_lrs[REC_GIC_NUM_LRS]; /* 0x308 */
			/* GICv3 Maintenance Interrupt State Register */
			unsigned long gicv3_misr;	/* 0x388 */
			/* GICv3 Virtual Machine Control Register */
			unsigned long gicv3_vmcr;	/* 0x390 */
		   }, 0x300, 0x400);
	SET_MEMBER_RMI(struct {
			/* Counter-timer Physical Timer Control Register */
			unsigned long cntp_ctl;		/* 0x400 */
			/* Counter-timer Physical Timer CompareValue Register */
			unsigned long cntp_cval;	/* 0x408 */
			/* Counter-timer Virtual Timer Control Register */
			unsigned long cntv_ctl;		/* 0x410 */
			/* Counter-timer Virtual Timer CompareValue Register */
			unsigned long cntv_cval;	/* 0x418 */
		   }, 0x400, 0x500);
	SET_MEMBER_RMI(struct {
			/* Base address of pending RIPAS change */
			unsigned long ripas_base;	/* 0x500 */
			/* Size of pending RIPAS change */
			unsigned long ripas_top;	/* 0x508 */
			/* RIPAS value of pending RIPAS change */
			unsigned char ripas_value;	/* 0x510 */
			/*
			 * Base PA of device memory region, if RIPAS change is
			 * pending due to execution of
			 * RSI_RDEV_VALIDATE_MAPPING
			 */
			unsigned long ripas_dev_pa; /* 0x518 */
			/* Base addr of target region for pending S2AP change */
			unsigned long s2ap_base; /* 0x520 */
			/* Top addr of target region for pending S2AP change */
			unsigned long s2ap_top; /* 0x528 */
			/* Virtual device ID */
			unsigned long vdev_id; /* 0x530 */
		   }, 0x500, 0x600);

	/* Host call immediate value */
	SET_MEMBER_RMI(unsigned int imm, 0x600, 0x608);
	/* UInt64: Plane Index */
	SET_MEMBER_RMI(unsigned long plane, 0x608, 0x610);
	/* Address: VDEV which triggered REC exit due to device communication */
	SET_MEMBER_RMI(unsigned long vdev, 0x610, 0x618);
	/* RmiVdevAction: Action which triggered REC exit due to device comm */
	SET_MEMBER_RMI(unsigned char vdev_action, 0x618, 0x620);
	/* Bits64: Base IPA of target region for device mem mapping validation */
	SET_MEMBER_RMI(unsigned long dev_mem_base, 0x620, 0x628);
	/* Bits64: Top IPA of target region for device mem mapping validation */
	SET_MEMBER_RMI(unsigned long dev_mem_top, 0x628, 0x630);
	/* Addres: Base PA of device memory region */
	SET_MEMBER_RMI(unsigned long dev_mem_pa, 0x630, 0x700);

	/* PMU overflow status */
	SET_MEMBER_RMI(unsigned long pmu_ovf_status, 0x700, 0x800);
};

/*
 * Structure contains shared information between RMM and Host
 * during REC entry and REC exit.
 */
struct rmi_rec_run {
	/* Entry information */
	SET_MEMBER(struct rmi_rec_entry entry, 0, 0x800);	/* Offset 0 */
	/* Exit information */
	SET_MEMBER(struct rmi_rec_exit exit, 0x800, 0x1000);	/* 0x800 */
};

/*
 * RmiPdevSpdm
 * Represents whether communication with the device uses SPDM.
 * Width: 1 bit
 */
#define RMI_PDEV_SPDM_FALSE			U(0)
#define RMI_PDEV_SPDM_TRUE			U(1)

/*
 * RmiPdevIde
 * Represents whether the link to the device is protected using IDE.
 * Width: 1 bit
 */
#define RMI_PDEV_IDE_FALSE			U(0)
#define RMI_PDEV_IDE_TRUE			U(1)

/*
 * RmiPdevCoherent
 * Represents the device access is non-coherent or coherent
 * Width: 1 bit
 */
#define RMI_PDEV_COHERENT_FALSE			U(0)
#define RMI_PDEV_COHERENT_TRUE			U(1)

/*
 * RmiPdevFlags
 * Fieldset contains flags provided by the Host during PDEV creation
 * Width: 64 bits
 */
/* RmiPdevSpdm: Bit 0 */
#define RMI_PDEV_FLAGS_SPDM_SHIFT		UL(0)
#define RMI_PDEV_FLAGS_SPDM_WIDTH		UL(1)
/* RmiPdevIde: Bit 1 */
#define RMI_PDEV_FLAGS_IDE_SHIFT		UL(1)
#define RMI_PDEV_FLAGS_IDE_WIDTH		UL(1)
/* RmiPdevCoherent: Bit 2 */
#define RMI_PDEV_FLAGS_COHERENT_SHIFT		UL(2)
#define RMI_PDEV_FLAGS_COHERENT_WIDTH		UL(1)
/* RmiFeature: Bit 3 */
#define RMI_PDEV_FLAGS_P2P_SHIFT		UL(3)
#define RMI_PDEV_FLAGS_P2P_WIDTH		UL(1)

/*
 * RmiPdevEvent
 * Represents physical device event.
 * Width: 8 bits
 */
#define RMI_PDEV_EVENT_IDE_KEY_REFRESH		U(0)

/*
 * RmiPdevState
 * Represents the state of a PDEV
 * Width: 8 bits
 */
#define RMI_PDEV_STATE_NEW			U(0)
#define RMI_PDEV_STATE_NEEDS_KEY		U(1)
#define RMI_PDEV_STATE_HAS_KEY			U(2)
#define RMI_PDEV_STATE_READY			U(3)
#define RMI_PDEV_STATE_IDE_RESETTING		U(4)
#define RMI_PDEV_STATE_COMMUNICATING		U(5)
#define RMI_PDEV_STATE_STOPPING			U(6)
#define RMI_PDEV_STATE_STOPPED			U(7)
#define RMI_PDEV_STATE_ERROR			U(8)

/*
 * RmiSignatureAlgorithm
 * Represents signature algorithm used in PDEV set key RMI call.
 * Width: 8 bits
 */
#define RMI_SIGNATURE_ALGORITHM_RSASSA_3072	U(0)
#define RMI_SIGNATURE_ALGORITHM_ECDSA_P256	U(1)
#define RMI_SIGNATURE_ALGORITHM_ECDSA_P384	U(2)

/*
 * RmiDevMemShared
 * Represents whether device memory Granule should be shared
 * Width: 1 bit
 */
#define RMI_DEV_MEM_PRIVATE			U(0)
#define RMI_DEV_MEM_SHARED			U(1)

/*
 * RmiDevDelegateFlags
 * Fieldset contains flags provided by the Host during device memory granule
 * delegation.
 * Width: 64 bits
 */
/* RmiDevMemShared: Bit 0 */
#define RMI_DEV_DELEGATE_FLAGS_SHARE_SHIFT	U(0)
#define RMI_DEV_DELEGATE_FLAGS_SHARE_WIDTH	U(1)

/*
 * RmiDevCommEnterStatus (Name in Spec RmiDevCommStatus)
 * Represents status passed from the Host to the RMM during device communication.
 * Width: 8 bits
 */
#define RMI_DEV_COMM_ENTER_STATUS_NONE		U(0)
#define RMI_DEV_COMM_ENTER_STATUS_RESPONSE	U(1)
#define RMI_DEV_COMM_ENTER_STATUS_ERROR		U(2)

/*
 * RmiDevCommEnter
 * This structure contains data passed from the Host to the RMM during device
 * communication.
 * Width: 256 (0x100) bytes
 */
struct rmi_dev_comm_enter {
	/* RmiDevCommEnterStatus: Status of device transaction */
	SET_MEMBER_RMI(unsigned char status, 0, 0x8);
	/* Address: Address of request buffer */
	SET_MEMBER_RMI(unsigned long req_addr, 0x8, 0x10);
	/* Address: Address of response buffer */
	SET_MEMBER_RMI(unsigned long resp_addr, 0x10, 0x18);
	/* UInt64: Amount of valid data in response buffer in bytes */
	SET_MEMBER_RMI(unsigned long resp_len, 0x18, 0x100);
};

/*
 * RmiDevCommExitFlags
 * Fieldset contains flags provided by the RMM during a device transaction.
 * Width: 64 bits
 */
#define RMI_DEV_COMM_EXIT_FLAGS_CACHE_REQ_SHIFT		UL(0)
#define RMI_DEV_COMM_EXIT_FLAGS_CACHE_REQ_WIDTH		UL(1)
#define RMI_DEV_COMM_EXIT_FLAGS_CACHE_RSP_SHIFT		UL(1)
#define RMI_DEV_COMM_EXIT_FLAGS_CACHE_RSP_WIDTH		UL(1)
#define RMI_DEV_COMM_EXIT_FLAGS_SEND_SHIFT		UL(2)
#define RMI_DEV_COMM_EXIT_FLAGS_SEND_WIDTH		UL(1)
#define RMI_DEV_COMM_EXIT_FLAGS_WAIT_SHIFT		UL(3)
#define RMI_DEV_COMM_EXIT_FLAGS_WAIT_WIDTH		UL(1)
#define RMI_DEV_COMM_EXIT_FLAGS_MULTI_SHIFT		UL(4)
#define RMI_DEV_COMM_EXIT_FLAGS_MULTI_WIDTH		UL(1)

/*
 * RmiDevCommObject
 * This represents identifier of a device communication object which the Host is
 * requested to cache.
 * Width: 8 bits
 */
#define RMI_DEV_COMM_OBJECT_VCA			U(0)
#define RMI_DEV_COMM_OBJECT_CERTIFICATE		U(1)
#define RMI_DEV_COMM_OBJECT_MEASUREMENTS	U(2)
#define RMI_DEV_COMM_OBJECT_INTERFACE_REPORT	U(3)

/*
 * RmiDevCommProtocol
 * Represents the protocol used for device communication.
 * Width: 8 bits
 */
#define RMI_DEV_COMM_PROTOCOL_SPDM		U(0)
#define RMI_DEV_COMM_PROTOCOL_SECURE_SPDM	U(1)

/*
 * RmiDevCommExit
 * This structure contains data passed from the RMM to the Host during device
 * communication.
 * Width: 256 (0x100) bytes.
 */
struct rmi_dev_comm_exit {
	/*
	 * RmiDevCommExitFlags: Flags indicating the actions the host is
	 * requested to perform
	 */
	SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
	/*
	 * UInt64: If flags.cache_req is true, offset in the device request
	 * buffer to the start of data to be cached, in bytes
	 */
	SET_MEMBER_RMI(unsigned long cache_req_offset, 0x8, 0x10);
	/*
	 * UInt64: If flags.cache_req is true, amount of device request data to
	 * be cached, in bytes
	 */
	SET_MEMBER_RMI(unsigned long cache_req_len, 0x10, 0x18);
	/*
	 * UInt64: If flags.cache_rsp is true, offset in the device response
	 * buffer to the start of data to be cached, in bytes
	 */
	SET_MEMBER_RMI(unsigned long cache_rsp_offset, 0x18, 0x20);
	/*
	 * UInt64: If flags.cache_rsp is true, amount of device response data to
	 * be cached, in bytes.
	 */
	SET_MEMBER_RMI(unsigned long cache_rsp_len, 0x20, 0x28);
	/*
	 * RmiDevCommObject: If flags.cache_req is true and / or flags.cache_rsp
	 * is true, identifier for the object to be cachedIf flags.cache_rsp is
	 * true, amount of device response data to be cached, in bytes.
	 */
	SET_MEMBER_RMI(unsigned char cache_obj_id, 0x28, 0x30);
	/* RmiDevCommProtocol: If flags.send is true, type of request */
	SET_MEMBER_RMI(unsigned char protocol, 0x30, 0x38);
	/*
	 * UInt64: If flags.send is true, amount of valid data in request buffer
	 * in bytes
	 */
	SET_MEMBER_RMI(unsigned long req_len, 0x38, 0x40);
	/*
	 * UInt64: If flags.wait is true, amount of time to wait for device
	 * response in milliseconds
	 */
	SET_MEMBER_RMI(unsigned long timeout, 0x40, 0x100);
};

/*
 * RmiDevCommData
 * This structure contains data structure shared between Host and RMM for
 * device communication.
 * Width: 4096 (0x1000) bytes.
 */
#define RMI_DEV_COMM_ENTER_OFFSET		0x0
#define RMI_DEV_COMM_EXIT_OFFSET		0x800
#define RMI_DEV_COMM_DATA_SIZE			0x1000
struct rmi_dev_comm_data {
	/* RmiDevCommEnter: Entry information */
	SET_MEMBER_RMI(struct rmi_dev_comm_enter enter,
		       RMI_DEV_COMM_ENTER_OFFSET, RMI_DEV_COMM_EXIT_OFFSET);
	/* RmiDevCommExit: Exit information */
	SET_MEMBER_RMI(struct rmi_dev_comm_exit exit,
		       RMI_DEV_COMM_EXIT_OFFSET, RMI_DEV_COMM_DATA_SIZE);
};

/*
 * RmiAddressRange
 * This structure contains base and top value of an address range.
 * Width: 16 (0x10) bytes.
 */
struct rmi_address_range {
	/* Address: Base of the address range (inclusive) */
	SET_MEMBER_RMI(unsigned long base, 0, 0x8);
	/* Address: Top of the address range (exclusive) */
	SET_MEMBER_RMI(unsigned long top, 0x8, 0x10);
};

/*
 * Maximum number of aux granules paramenter passed in rmi_pdev_params during
 * PDEV createto PDEV create
 */
#define PDEV_PARAM_AUX_GRANULES_MAX		U(32)

/*
 * Maximum number of device non-coherent RmiAddressRange parameter passed in
 * rmi_pdev_params during PDEV create
 */
#define PDEV_PARAM_NCOH_ADDR_RANGE_MAX		U(16)

/*
 * Maximum number of device coherent RmiAddressRange parameter passed in
 * rmi_pdev_params during PDEV create
 */
#define PDEV_PARAM_COH_ADDR_RANGE_MAX		U(4)

/*
 * RmiPdevParams
 * This structure contains parameters provided by Host during PDEV creation.
 * Width: 4096 (0x1000) bytes.
 */
struct rmi_pdev_params {
	/* RmiPdevFlags: Flags */
	SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
	/* Bits64: Physical device identifier */
	SET_MEMBER_RMI(unsigned long pdev_id, 0x8, 0x10);
	/* Bits16: Segment ID */
	SET_MEMBER_RMI(unsigned short segment_id, 0x10, 0x18);
	/* Address: ECAM base address of the PCIe configuration space */
	SET_MEMBER_RMI(unsigned long ecam_addr, 0x18, 0x20);
	/* Bits16: Root Port identifier */
	SET_MEMBER_RMI(unsigned short root_id, 0x20, 0x28);
	/* UInt64: Certificate identifier */
	SET_MEMBER_RMI(unsigned long cert_id, 0x28, 0x30);
	/* UInt64: Base requester ID range (inclusive) */
	SET_MEMBER_RMI(unsigned long rid_base, 0x30, 0x38);
	/* UInt64: Top of requester ID range (exclusive) */
	SET_MEMBER_RMI(unsigned long rid_top, 0x38, 0x40);
	/* RmiHashAlgorithm: Algorithm used to generate device digests */
	SET_MEMBER_RMI(unsigned char hash_algo, 0x40, 0x48);
	/* UInt64: Number of auxiliary granules */
	SET_MEMBER_RMI(unsigned long num_aux, 0x48, 0x50);
	/* UInt64: IDE stream identifier */
	SET_MEMBER_RMI(unsigned long ide_sid, 0x50, 0x58);
	/* UInt64: Number of device non-coherent address ranges */
	SET_MEMBER_RMI(unsigned long ncoh_num_addr_range, 0x58, 0x60);
	/* UInt64: Number of device coherent address ranges */
	SET_MEMBER_RMI(unsigned long coh_num_addr_range, 0x60, 0x100);
	/* Address: Addresses of auxiliary granules */
	SET_MEMBER_RMI(unsigned long aux[PDEV_PARAM_AUX_GRANULES_MAX], 0x100,
		       0x200);
	/* RmiAddressRange: Device non-coherent address range */
	SET_MEMBER_RMI(struct rmi_address_range
		       ncoh_addr_range[PDEV_PARAM_NCOH_ADDR_RANGE_MAX],
		       0x200, 0x300);
	/* RmiAddressRange: Device coherent address range */
	SET_MEMBER_RMI(struct rmi_address_range
		       coh_addr_range[PDEV_PARAM_COH_ADDR_RANGE_MAX],
		       0x300, 0x1000);
};

/* Max length of public key data passed in rmi_public_key_params */
#define PUBKEY_PARAM_KEY_LEN_MAX	U(1024)

/* Max length of public key metadata passed in rmi_public_key_params */
#define PUBKEY_PARAM_METADATA_LEN_MAX	U(1024)

/*
 * RmiPublicKeyParams
 * This structure contains public key parameters.
 * Width: 4096 (0x1000) bytes.
 */
struct rmi_public_key_params {
	/* Bits8: Key data */
	SET_MEMBER_RMI(unsigned char key[PUBKEY_PARAM_KEY_LEN_MAX], 0x0, 0x400);
	/* Bits8: Key metadata */
	SET_MEMBER_RMI(unsigned char metadata[PUBKEY_PARAM_METADATA_LEN_MAX],
		       0x400, 0x800);
	/* UInt64: Length of key data in bytes */
	SET_MEMBER_RMI(unsigned long key_len, 0x800, 0x808);
	/* UInt64: Length of metadata in bytes */
	SET_MEMBER_RMI(unsigned long metadata_len, 0x808, 0x810);
	/* RmiSignatureAlgorithm: Signature algorithm */
	SET_MEMBER_RMI(unsigned char algo, 0x810, 0x1000);
};

/*
 * RmiVdevFlags
 * Fieldset contains flags provided by the Host during VDEV creation.
 * Width: 64 bits
 */
#define RMI_VDEV_FLAGS_RES0_SHIFT		UL(0)
#define RMI_VDEV_FLAGS_RES0_WIDTH		UL(63)

/*
 * RmiVdevState
 * Represents the state of the VDEV
 * Width: 8 bits
 */
#define RMI_VDEV_STATE_READY			U(0)
#define RMI_VDEV_STATE_COMMUNICATING		U(1)
#define RMI_VDEV_STATE_STOPPING			U(2)
#define RMI_VDEV_STATE_STOPPED			U(3)
#define RMI_VDEV_STATE_ERROR			U(4)

/* Maximum number of aux granules paramenter passed to VDEV create */
#define VDEV_PARAM_AUX_GRANULES_MAX		U(32)

/*
 * RmiVdevParams
 * The RmiVdevParams structure contains parameters provided by the Host during
 * VDEV creation.
 * Width: 4096 (0x1000) bytes.
 */
struct rmi_vdev_params {
	/* RmiVdevFlags: Flags */
	SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
	/* Bits64: Virtual device identifier */
	SET_MEMBER_RMI(unsigned long vdev_id, 0x8, 0x10);
	/* Bits64: TDI identifier */
	SET_MEMBER_RMI(unsigned long tdi_id, 0x10, 0x18);
	/* UInt64: Number of auxiliary granules */
	SET_MEMBER_RMI(unsigned long num_aux, 0x18, 0x100);

	/* Address: Addresses of auxiliary granules */
	SET_MEMBER_RMI(unsigned long aux[VDEV_PARAM_AUX_GRANULES_MAX], 0x100,
		       0x1000);
};

struct rtt_entry {
	long walk_level;
	uint64_t out_addr;
	u_register_t state;
	u_register_t ripas;
};

enum realm_state {
	REALM_STATE_NULL,
	REALM_STATE_NEW,
	REALM_STATE_ACTIVE,
	REALM_STATE_SYSTEM_OFF
};

struct realm {
	u_register_t     host_shared_data;
	unsigned int     rec_count;
	unsigned int     num_aux_planes;
	u_register_t     par_base;
	u_register_t     par_size;
	u_register_t     rd;
	u_register_t     rtt_addr;
	u_register_t     rec[MAX_REC_COUNT];
	u_register_t     run[MAX_REC_COUNT];
	u_register_t     rec_flag[MAX_REC_COUNT];
	u_register_t     mpidr[MAX_REC_COUNT];
	u_register_t     host_mpidr[MAX_REC_COUNT];
	u_register_t     num_aux;
	u_register_t     rmm_feat_reg0;
	u_register_t     ipa_ns_buffer;
	u_register_t     ns_buffer_size;
	u_register_t     aux_pages_all_rec[MAX_REC_COUNT][REC_PARAMS_AUX_GRANULES];
	uint8_t          sve_vl;
	uint8_t          num_bps;
	uint8_t          num_wps;
	uint8_t          pmu_num_ctrs;
	bool             payload_created;
	bool             shared_mem_created;
	bool             rtt_tree_single;
	bool		 rtt_s2ap_enc_indirect;
	unsigned short   vmid;
	enum realm_state state;
	long start_level;
	u_register_t     aux_rtt_addr[MAX_AUX_PLANE_COUNT];
	unsigned short   aux_vmid[MAX_AUX_PLANE_COUNT];
};

/* RMI/SMC */
u_register_t host_rmi_version(u_register_t req_ver);
u_register_t host_rmi_granule_delegate(u_register_t addr);
u_register_t host_rmi_granule_undelegate(u_register_t addr);
u_register_t host_rmi_realm_create(u_register_t rd, u_register_t params_ptr);
u_register_t host_rmi_realm_destroy(u_register_t rd);
u_register_t host_rmi_features(u_register_t index, u_register_t *features);
u_register_t host_rmi_data_destroy(u_register_t rd,
				   u_register_t map_addr,
				   u_register_t *data,
				   u_register_t *top);
u_register_t host_rmi_rtt_readentry(u_register_t rd,
				    u_register_t map_addr,
				    long level,
				    struct rtt_entry *rtt);
u_register_t host_rmi_rtt_destroy(u_register_t rd,
				  u_register_t map_addr,
				  long level,
				  u_register_t *rtt,
				  u_register_t *top);
u_register_t host_rmi_rtt_init_ripas(u_register_t rd,
				   u_register_t start,
				   u_register_t end,
				   u_register_t *top);
u_register_t host_rmi_create_rtt_levels(struct realm *realm,
					u_register_t map_addr,
					long level, long max_level);
u_register_t host_rmi_rtt_unmap_unprotected(u_register_t rd,
					    u_register_t map_addr,
					    long level,
					    u_register_t *top);
u_register_t host_rmi_rtt_set_ripas(u_register_t rd,
				    u_register_t rec,
				    u_register_t start,
				    u_register_t end,
				    u_register_t *top);
u_register_t host_rmi_rtt_set_s2ap(u_register_t rd,
				   u_register_t rec,
				   u_register_t start,
				   u_register_t end,
				   u_register_t *top,
				   u_register_t *rtt_tree);
u_register_t host_rmi_psci_complete(u_register_t calling_rec, u_register_t target_rec,
				    unsigned long status);
void host_rmi_init_cmp_result(void);
bool host_rmi_get_cmp_result(void);

/* Realm management */
u_register_t host_realm_create(struct realm *realm);
u_register_t host_realm_map_payload_image(struct realm *realm,
					  u_register_t realm_payload_adr);
u_register_t host_realm_map_ns_shared(struct realm *realm,
				      u_register_t ns_shared_mem_adr,
				      u_register_t ns_shared_mem_size);
u_register_t host_realm_rec_create(struct realm *realm);
unsigned int host_realm_find_rec_by_mpidr(unsigned int mpidr, struct realm *realm);
u_register_t host_realm_activate(struct realm *realm);
u_register_t host_realm_destroy(struct realm *realm);
u_register_t host_realm_rec_enter(struct realm *realm,
				  u_register_t *exit_reason,
				  unsigned int *host_call_result,
				  unsigned int rec_num);
u_register_t host_realm_init_ipa_state(struct realm *realm, long level,
				       u_register_t start, uint64_t end);
u_register_t host_realm_delegate_map_protected_data(bool unknown,
					   struct realm *realm,
					   u_register_t target_pa,
					   u_register_t map_size,
					   u_register_t src_pa);
u_register_t host_realm_map_unprotected(struct realm *realm, u_register_t ns_pa,
					u_register_t map_size);
u_register_t host_realm_fold_rtt(u_register_t rd, u_register_t addr, long level);

u_register_t host_rmi_pdev_aux_count(u_register_t pdev_ptr, u_register_t *count);
u_register_t host_rmi_pdev_create(u_register_t pdev_ptr,
				  u_register_t params_ptr);
u_register_t host_rmi_pdev_get_state(u_register_t pdev_ptr, u_register_t *state);
u_register_t host_rmi_pdev_communicate(u_register_t pdev_ptr,
				       u_register_t data_ptr);
u_register_t host_rmi_pdev_set_pubkey(u_register_t pdev_ptr,
				      u_register_t pubkey_params_ptr);
u_register_t host_rmi_pdev_stop(u_register_t pdev_ptr);
u_register_t host_rmi_pdev_destroy(u_register_t pdev_ptr);

u_register_t host_rmi_dev_mem_map(u_register_t rd, u_register_t map_addr,
				  u_register_t level, u_register_t dev_mem_addr);
u_register_t host_rmi_dev_mem_unmap(u_register_t rd, u_register_t map_addr,
				    u_register_t level, u_register_t *pa,
				    u_register_t *top);

u_register_t host_rmi_vdev_create(u_register_t rd_ptr, u_register_t pdev_ptr,
				  u_register_t vdev_ptr,
				  u_register_t params_ptr);
u_register_t host_rmi_vdev_complete(u_register_t rec_ptr, u_register_t vdev_ptr);
u_register_t host_rmi_vdev_communicate(u_register_t pdev_ptr,
				       u_register_t vdev_ptr,
				       u_register_t dev_comm_data_ptr);
u_register_t host_rmi_vdev_get_state(u_register_t vdev_ptr, u_register_t *state);
u_register_t host_rmi_vdev_abort(u_register_t vdev_ptr);
u_register_t host_rmi_vdev_stop(u_register_t vdev_ptr);
u_register_t host_rmi_vdev_destroy(u_register_t rd_ptr, u_register_t pdev_ptr,
				   u_register_t vdev_ptr);
#endif /* HOST_REALM_RMI_H */
