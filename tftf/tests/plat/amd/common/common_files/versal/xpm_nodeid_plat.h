/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal platform-specific node IDs and portable alias definitions.
 */

#ifndef XPM_NODEID_PLAT_H_
#define XPM_NODEID_PLAT_H_

#include "xpm_nodeid.h"

/* Versal legacy ACPU/RPU device IDs */
#define PM_DEV_ACPU_0           0x1810c003U
#define PM_DEV_RPU0_0           0x18110005U

/* Primary APU/RPU core aliases for Versal */
#define PM_DEV_ACPU_CORE        PM_DEV_ACPU_0
#define PM_DEV_RPU_CORE         PM_DEV_RPU0_0

#endif /* XPM_NODEID_PLAT_H_ */
