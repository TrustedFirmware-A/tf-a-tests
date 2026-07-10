/*
 * Copyright (c) 2024-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PLATFORM_PCIE_H
#define PLATFORM_PCIE_H

#include <utils_def.h>

#include "../fvp_def.h"

/* PCIE platform config parameters */
#define PLATFORM_NUM_ECAM		1

/* Platform config parameters for ECAM_0 */
#define PLATFORM_PCIE_ECAM_BASE_ADDR_0	PCIE_CONFIG_BASE
#define PLATFORM_PCIE_SEGMENT_GRP_NUM_0	0x0
#define PLATFORM_PCIE_START_BUS_NUM_0	0x0
#define PLATFORM_PCIE_END_BUS_NUM_0	0xFF

/* PCIe BAR config parameters*/
#define PLATFORM_OVERRIDE_PCIE_BAR64_VALUE	0x4000100000
#define PLATFORM_OVERRIDE_RP_BAR64_VALUE	0x4000000000
#define PLATFORM_OVERRIDE_PCIE_BAR32NP_VALUE	0x50000000
#define PLATFORM_OVERRIDE_PCIE_BAR32P_VALUE	0x50600000
#define PLATOFRM_OVERRIDE_RP_BAR32_VALUE	0x50850000

/* Default timeout value for PCIe IDE Key management operations */
#define PLAT_IDE_KM_BUSY_TIMEOUT_MS		(5)
#define PLAT_IDE_KM_POLL_TIMEOUT_MS		(5)

#endif	/* PLATFORM_PCIE_H */
