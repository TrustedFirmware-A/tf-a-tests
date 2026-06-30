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
#define LFA_INVALID			(LFA_CANCEL + 1U)

#define RMM_X1				UL(0x564bf212a662076c)
#define RMM_X2				UL(0xd90636638fbacb92)
#define BL31_X1				UL(0x4698fe4c6d08d447)
#define BL31_X2				UL(0x005abdcb5029959b)

#define NUM_CTX_REGISTERS		U(30)

/* Assembly helper function prototypes. */
uint64_t set_ns_ep_context(uint32_t context_id);
void get_ns_ep_context(uint32_t context_id);
void atomic_add(int *ptr, int value);
int atomic_read(int *ptr);

#endif /* LFA_H */
