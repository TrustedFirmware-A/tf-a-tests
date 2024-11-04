/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_LS64_H
#define TEST_LS64_H

#include <stdint.h>

void ls64_store(uint64_t *input_buffer, uint64_t *store_address);
void ls64_load(uint64_t *store_address, uint64_t *output_buffer);

#endif /* TEST_LS64_H */
