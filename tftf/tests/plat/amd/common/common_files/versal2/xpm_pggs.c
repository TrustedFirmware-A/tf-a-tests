/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal Gen 2 PGGS access implementation.
 *
 * Uses IOCTL_READ_REG / IOCTL_MASK_WRITE_REG with the PM_REG_PGGS
 * regnode (configured in CDO).
 */

#include <assert.h>
#include <stdint.h>

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid_plat.h"
#include "xpm_pggs.h"

/* Byte offset of the target register within the PGGS regnode. */
#define PGGS_REG_OFFSET		0U

/*
 * Number of registers to read. The register-access IOCTL requires this
 * to be 1 (per the XilPM IOCTL_READ_REG definition).
 */
#define PGGS_REG_READ_COUNT	1U

/* Read-modify-write mask covering the full 32-bit register. */
#define PGGS_REG_WRITE_MASK	UINT32_MAX

/* Trailing IOCTL argument that is unused by the read path. */
#define PGGS_IOCTL_UNUSED_ARG	0U

int xpm_pggs_read(uint32_t *const val)
{
	assert(val != NULL);

	return xpm_ioctl(PM_REG_PGGS, IOCTL_READ_REG, PGGS_REG_OFFSET,
			 PGGS_REG_READ_COUNT, PGGS_IOCTL_UNUSED_ARG, val);
}

int xpm_pggs_write(const uint32_t val)
{
	return xpm_ioctl(PM_REG_PGGS, IOCTL_MASK_WRITE_REG, PGGS_REG_OFFSET,
			 PGGS_REG_WRITE_MASK, val, NULL);
}
