/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FIRME_ATTESTATION_H
#define FIRME_ATTESTATION_H

#include <stdint.h>

#include <firme.h>

#define FIRME_ATTESTATION_VERSION_MAJOR		U(1)
#define FIRME_ATTESTATION_VERSION_MINOR		U(0)

#define FIRME_ATTEST_FNUM_PAT_GET		U(0x8)

/* Feature register fields */
#define FIRME_ATTESTATION_FEATURE_REG_COUNT			U(2)
#define FIRME_ATTEST_FEAT_REG0_PAT_GET_BIT			BIT(0)
#define FIRME_ATTEST_FEAT_REG1_MAX_PAT_PG_CNT_SHIFT		U(0)
#define FIRME_ATTEST_FEAT_REG1_MAX_PAT_PG_CNT_MASK		U(0xFF)

/*
 * FIRME_ATTEST_PAT_GET
 *
 * This function requests the platform attestation token. The token is written
 * by EL3 firmware into a caller-provided shared buffer at a specified offset.
 * The buffer is also used to supply the platform challenge (nonce).
 *
 * Arguments
 *	arg0(w0): Function ID 0xC4000408
 *	arg1(x1): Shared buffer base address
 *	arg2(x2): Write offset (in bytes)
 *	arg3(x3): Shared buffer page count
 *	arg4(x4): Platform challenge size
 *
 * Return
 *	ret0(x0): Status
 *			 FIRME_SUCCESS
 *			 FIRME_NOT_SUPPORTED
 *			 FIRME_INVALID_PARAMETERS
 *			 FIRME_BUSY
 *			 FIRME_ABORTED
 *			 FIRME_INCOMPLETE
 *
 *	ret1(x1): Written size (in bytes)
 *			 Valid if status is FIRME_SUCCESS or FIRME_INCOMPLETE.
 *
 *	ret2(x2): Remaining size (in bytes)
 *			 Valid if status is FIRME_SUCCESS or FIRME_INCOMPLETE.
 */
#define FIRME_ATTEST_PAT_GET_FID			\
	SMC64_FIRME_FID(FIRME_ATTEST_FNUM_PAT_GET)

int32_t firme_attest_pat_get(uint64_t shared_buf_addr, uint64_t write_offset,
			     uint64_t shared_buf_page_count,
			     uint64_t challenge_size, uint64_t *written_size,
			     uint64_t *remaining_size);

#endif /* FIRME_ATTESTATION_H */
