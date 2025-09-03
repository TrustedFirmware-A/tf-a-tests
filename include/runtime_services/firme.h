/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __FIRME_H__
#define __FIRME_H__

#include <stdint.h>

 /* FIRME version currently implemented */
#define FIRME_VERSION_MAJOR		U(1)
#define FIRME_VERSION_MINOR		U(0)
#define FIRME_VERSION_MAJOR_SHIFT	U(16)
#define FIRME_VERSION_MINOR_SHIFT	U(0)
#define FIRME_VERSION_MAJOR_MASK	U(0x7FFF)
#define FIRME_VERSION_MINOR_MASK	U(0xFFFF)

/* FIRME SMC return codes */
#define FIRME_SUCCESS			0
#define FIRME_NOT_SUPPORTED		-1
#define FIRME_INVALID_PARAMETERS	-2
#define FIRME_ABORTED			-3
#define FIRME_INCOMPLETE		-4
#define FIRME_DENIED			-5
#define FIRME_RETRY			-6
#define FIRME_IN_PROGRESS		-7
#define FIRME_EXISTS			-8
#define FIRME_NO_ENTRY			-9
#define FIRME_NO_MEMORY			-10
#define FIRME_BAD_DATA			-11

/* Range of function IDs used by FIRME interface */
#define FIRME_FNUM_MIN_VALUE	U(0x400)
#define FIRME_FNUM_MAX_VALUE	U(0x40F)

/* Construct FIRME fastcall std FID from offset */
#define SMC64_FIRME_FID(_offset)                                              \
	((SMC_TYPE_FAST << FUNCID_TYPE_SHIFT) | (SMC_64 << FUNCID_CC_SHIFT) | \
	 (OEN_STD_START << FUNCID_OEN_SHIFT) |                                \
	 (((FIRME_FNUM_MIN_VALUE + (_offset)) & FUNCID_NUM_MASK)              \
	  << FUNCID_NUM_SHIFT))

#define is_firme_fid(fid)                                       \
	__extension__({                                         \
		__typeof__(fid) _fid = (fid);                   \
		((GET_SMC_NUM(_fid) >= FIRME_FNUM_MIN_VALUE) && \
		 (GET_SMC_NUM(_fid) <= FIRME_FNUM_MAX_VALUE) && \
		 (GET_SMC_TYPE(_fid) == SMC_TYPE_FAST) &&       \
		 (GET_SMC_CC(_fid) == SMC_64) &&                \
		 (GET_SMC_OEN(_fid) == OEN_STD_START) &&        \
		 ((_fid & 0x00FE0000) == 0U));                  \
	})

/* Base service feature register definitions */
#define FIRME_BASE_VERSION_BIT				BIT(0)
#define FIRME_BASE_FEATURES_BIT				BIT(1)
#define FIRME_BASE_SERVICE_LIST_SHIFT			U(16)
#define FIRME_BASE_SERVICE_LIST_MASK			U(0xFFFF)
#define FIRME_BASE_SERVICE_GRANULE_MGMT_BIT		BIT(16)

#define FIRME_BASE_SERVICE_ID				U(0)
#define FIRME_GM_SERVICE_ID				U(1)

#define FIRME_SERVICE_VERSION_FID			SMC64_FIRME_FID(0)
#define FIRME_SERVICE_FEATURES_FID			SMC64_FIRME_FID(1)
#define FIRME_SERVICE_GM_GPI_SET_FID			SMC64_FIRME_FID(2)
#define FIRME_SERVICE_IDE_KEYSET_PROG_FID		SMC64_FIRME_FID(3)
#define FIRME_SERVICE_IDE_KEYSET_GO_FID			SMC64_FIRME_FID(4)
#define FIRME_SERVICE_IDE_KEYSET_STOP_FID		SMC64_FIRME_FID(5)
#define FIRME_SERVICE_IDE_KEYSET_POLL_FID		SMC64_FIRME_FID(6)
#define FIRME_SERVICE_MEC_REFRESH_FID			SMC64_FIRME_FID(7)
#define FIRME_SERVICE_ATTEST_PAT_GET_FID		SMC64_FIRME_FID(8)
#define FIRME_SERVICE_ATTEST_RAK_GET_FID		SMC64_FIRME_FID(9)
#define FIRME_SERVICE_ATTEST_RAT_SIGN_FID		SMC64_FIRME_FID(10)

/* APIs wrapping FIRME ABIs */
int32_t firme_version(uint8_t service_id);
int32_t firme_features(uint8_t service_id, uint8_t reg_index, uint64_t *reg);

#endif /* __FIRME_H__ */
