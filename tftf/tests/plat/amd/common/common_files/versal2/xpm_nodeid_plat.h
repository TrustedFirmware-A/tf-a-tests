/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal Gen 2 platform-specific node IDs and portable alias definitions.
 */

#ifndef XPM_NODEID_PLAT_H_
#define XPM_NODEID_PLAT_H_

#include "xpm_nodeid.h"

/*
 * Versal Gen 2 multi-cluster ACPU/RPU device IDs.
 * (PM_DEV_ACPU_0_0 corresponds to ATF's PM_DEV_CLUSTER0_ACPU_0.)
 */
#define PM_DEV_ACPU_0_0         0x1810C0AFU     /* cluster 0, core 0 */
#define PM_DEV_RPU_A_0          0x181100BFU     /* RPU cluster A, core 0 */

/* Primary APU/RPU core aliases for Versal Gen 2 */
#define PM_DEV_ACPU_CORE        PM_DEV_ACPU_0_0
#define PM_DEV_RPU_CORE         PM_DEV_RPU_A_0

#endif /* XPM_NODEID_PLAT_H_ */
