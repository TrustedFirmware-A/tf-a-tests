/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
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

const pcie_info_table_t *plat_pcie_get_info_table(void);

#endif	/* PLATFORM_PCIE_H */
