/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ZynqMP PGGS access. TF-A services these IOCTLs as an MMIO access to
 * PGGS_BASEADDR + (index << 2), so the register is selected by index.
 */

#include "eemi_api.h"
#include "xpm_defs.h"
#include "xpm_nodeid.h"
#include "xpm_nodeid_plat.h"
#include "xpm_pggs.h"

/* PGGS3: kept across the reset, and not used by PMUFW. */
#define ZYNQMP_PGGS_SCRATCH_INDEX	3U

int xpm_pggs_read(uint32_t *const val)
{
	return xpm_ioctl(PM_DEV_ACPU_CORE, IOCTL_READ_PGGS,
			 ZYNQMP_PGGS_SCRATCH_INDEX, 0, 0, val);
}

int xpm_pggs_write(const uint32_t val)
{
	return xpm_ioctl(PM_DEV_ACPU_CORE, IOCTL_WRITE_PGGS,
			 ZYNQMP_PGGS_SCRATCH_INDEX, val, 0, NULL);
}
