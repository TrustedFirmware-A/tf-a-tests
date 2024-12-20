/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYSREG128_H
#define SYSREG128_H

#include <stdint.h>

#ifdef __aarch64__

/* Assembly function prototypes. */
uint128_t read128_par_el1(void);
uint128_t read128_ttbr0_el1(void);
uint128_t read128_ttbr1_el1(void);
uint128_t read128_ttbr0_el2(void);
uint128_t read128_ttbr1_el2(void);
uint128_t read128_vttbr_el2(void);
uint128_t read128_rcwmask_el1(void);
uint128_t read128_rcwsmask_el1(void);

void write128_par_el1(uint128_t v);
void write128_ttbr0_el1(uint128_t v);
void write128_ttbr1_el1(uint128_t v);
void write128_ttbr0_el2(uint128_t v);
void write128_ttbr1_el2(uint128_t v);
void write128_vttbr_el2(uint128_t v);
void write128_rcwmask_el1(uint128_t v);
void write128_rcwsmask_el1(uint128_t v);

#endif /* __aarch64__ */

#endif /* SYSREG128_H */
