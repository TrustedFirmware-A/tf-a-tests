/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_D128_H
#define TEST_D128_H

/* Create Mask setting all used bits except PA */
#define PAR_EL1_MASK_HIGH		0x0000000000000001
#define PAR_EL1_MASK_LOW		0xFF00000000000F81
#define PAR_EL1_MASK_FULL		(((uint128_t)PAR_EL1_MASK_HIGH) << 64U | PAR_EL1_MASK_LOW)

/* Masks created by setting all used bits */
#define TTBR_REG_MASK_HIGH		0x0000000000FF0000
#define TTBR_REG_MASK_LOW		0xFFFFFFFFFFFFFFE7
#define TTBR_REG_MASK_FULL		(((uint128_t)TTBR_REG_MASK_HIGH) << 64U | TTBR_REG_MASK_LOW)

#endif /* TEST_D128_H */
