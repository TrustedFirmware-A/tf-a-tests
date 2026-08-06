/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ZynqMP test targets and platform values other than node IDs.
 */

#ifndef XPM_DEFS_PLAT_H_
#define XPM_DEFS_PLAT_H_

#include "xpm_nodeid_plat.h"

/* A blocking acknowledge is needed for the firmware status to be returned. */
#define PM_REQ_ACK_BLOCKING		2U
#define PM_REQ_ACK_DEFAULT		PM_REQ_ACK_BLOCKING

#endif /* XPM_DEFS_PLAT_H_ */
