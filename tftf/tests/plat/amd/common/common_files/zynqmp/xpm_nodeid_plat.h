/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ZynqMP node IDs, test targets and portable aliases. The IDs are the plain
 * integers of the PMU firmware enumeration, not the encoded Versal values.
 */

#ifndef XPM_NODEID_PLAT_H_
#define XPM_NODEID_PLAT_H_

#include "xpm_nodeid.h"

/* Processor nodes */
#define NODE_APU_0		2U
#define NODE_RPU_0		7U

/* Peripheral nodes */
#define NODE_USB_0		22U
#define NODE_APLL		50U
#define NODE_RTC		61U

/* Replace the Versal PM_DEV_* values from xpm_nodeid.h. */
#ifdef PM_DEV_USB_0
#undef PM_DEV_USB_0
#endif
#define PM_DEV_USB_0		NODE_USB_0

#ifdef PM_DEV_RTC
#undef PM_DEV_RTC
#endif
#define PM_DEV_RTC		NODE_RTC

/* Portable aliases for tests that use PM_DEV_* style names. */
#define PM_DEV_ACPU_CORE	NODE_APU_0
#define PM_DEV_RPU_CORE		NODE_RPU_0

#endif /* XPM_NODEID_PLAT_H_ */
