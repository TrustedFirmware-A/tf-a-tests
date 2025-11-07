/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LFA_H
#define LFA_H

#include <stdint.h>

#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <smccc.h>

#define LFA_VERSION_MAJOR		U(1)
#define LFA_VERSION_MINOR		U(0)
#define LFA_VERSION_MAJOR_SHIFT		U(16)
#define LFA_VERSION_MAJOR_MASK 		U(0x7FFF)
#define LFA_VERSION_MINOR_SHIFT		U(0)
#define LFA_VERSION_MINOR_MASK		U(0xFFFF)

#define LFA_VERSION			U(0xC40002E0)
#define LFA_FEATURES			U(0xC40002E1)
#define LFA_GET_INFO			U(0xC40002E2)
#define LFA_GET_INVENTORY		U(0xC40002E3)
#define LFA_PRIME			U(0xC40002E4)
#define LFA_ACTIVATE			U(0xC40002E5)
#define LFA_CANCEL			U(0xC40002E6)
#define LFA_INVALID			LFA_CANCEL + 1U

#endif /* LFA_H */
