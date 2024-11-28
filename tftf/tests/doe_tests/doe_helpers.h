/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef DOE_HELPERS_H
#define DOE_HELPERS_H

#include <stdint.h>

int find_doe_device(uint32_t *bdf_ptr, uint32_t *cap_base_ptr);
int doe_discovery(uint32_t bdf, uint32_t doe_cap_base);
int get_spdm_version(uint32_t bdf, uint32_t doe_cap_base);

#endif /* DOE_HELPERS_H */
