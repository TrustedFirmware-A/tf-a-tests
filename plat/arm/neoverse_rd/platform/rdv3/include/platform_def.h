/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <nrd3/nrd_plat_arm_def3.h>

#define PLAT_ARM_CLUSTER_COUNT		U(16)
#define NRD_MAX_CPUS_PER_CLUSTER	U(1)
#define NRD_MAX_PE_PER_CPU		U(1)

/* GIC-600 & interrupt handling related constants */
#define PLAT_ARM_GICD_BASE		UL(0x30000000)
#define PLAT_ARM_GICR_BASE		UL(0x301C0000)
#define PLAT_ARM_GICC_BASE		UL(0x2C000000)

/* Platform specific page table and MMU setup constants */
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 46)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 46)

#endif /* PLATFORM_DEF_H */