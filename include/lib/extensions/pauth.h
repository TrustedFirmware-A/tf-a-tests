/*
 * Copyright (c) 2019-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PAUTH_H
#define PAUTH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __aarch64__
/* Initialize 128-bit ARMv8.3-PAuth key */
uint128_t init_apkey(void);

/* Program APIAKey_EL1 key and enable ARMv8.3-PAuth */
void pauth_init_enable(void);

/* Disable ARMv8.3-PAuth */
void pauth_disable(void);

/*
 * Fill Pauth Keys and template with random values if keys werenot initialized earlier,
 * Else Copy PAuth key registers to template.
 */
void pauth_test_lib_fill_regs_and_template(void);

/* Read and Compare PAuth registers with provided template values. */
bool pauth_test_lib_compare_template(void);

/* Read and Store PAuth registers in template. */
void pauth_test_lib_read_keys(void);

/* Test PAuth instructions. */
void pauth_test_lib_test_intrs(void);

#endif	/* __aarch64__ */

#endif /* PAUTH_H */
