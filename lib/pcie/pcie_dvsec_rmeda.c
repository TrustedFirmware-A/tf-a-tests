/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common_def.h>
#include <errno.h>
#include <stddef.h>
#include <debug.h>
#include <mmio.h>
#include <pcie.h>
#include <pcie_spec.h>
#include <pcie_dvsec_rmeda.h>
#include <platform.h>
#include <tftf_lib.h>

/*
 * Check if the DVSEC header matches RME Sys Arch spec
 *
 * DVSEC_REVISION must be 0
 * DVSEC_VENDOR_ID must be Arm
 * DVSEC_ID must be RME_DA
 */
static bool is_dvsec_arm_rmeda(uint32_t rp_bdf, uint32_t dvsec_base)
{
	uint32_t hdr1 = 0U;
	uint32_t hdr2 = 0U;

	hdr1 = pcie_read_cfg(rp_bdf, dvsec_base + PCIE_ECAP_DVSEC_HDR1_OFFSET);
	hdr2 = pcie_read_cfg(rp_bdf, dvsec_base + PCIE_ECAP_DVSEC_HDR2_OFFSET);

	if ((EXTRACT(DVSEC_HDR1_VENDOR_ID, hdr1) == DVSEC_VENDOR_ID_ARM) &&
	    (EXTRACT(DVSEC_HDR1_REVISION, hdr1) == DVSEC_REVISION_0) &&
	    (EXTRACT(DVSEC_HDR2_DVSEC_ID, hdr2) == DVSEC_ID_RME_DA)) {
		return true;
	}

	return false;
}

/*
 * Traverse all DVSEC extended capability and return the first RMEDA DVSEC if the
 * header, revision are expected as mentioned in RME System Architecture.
 */
uint32_t pcie_find_rmeda_capability(uint32_t rp_bdf, uint32_t *cid_offset)
{
	uint32_t ech;
	unsigned int dvsec_offset;
	uint16_t next_cap_offset;

	dvsec_offset = PCIE_ECAP_START;
	do {
		assert((dvsec_offset + sizeof(ech)) < SZ_4K);

		ech = pcie_read_cfg(rp_bdf, dvsec_offset);

		/* Check for PCIE_ECH_CAP_VER_1 as well? */
		if ((EXTRACT(PCIE_ECH_ID, ech) == ECID_DVSEC) &&
		    is_dvsec_arm_rmeda(rp_bdf, dvsec_offset)) {
			*cid_offset = dvsec_offset;
			return PCIE_SUCCESS;
		}

		next_cap_offset = EXTRACT(PCIE_ECH_NEXT_CAP_OFFSET, ech);
		dvsec_offset += next_cap_offset;
	} while ((next_cap_offset != 0U) && (dvsec_offset < PCIE_ECAP_END));

	return PCIE_CAP_NOT_FOUND;
}
