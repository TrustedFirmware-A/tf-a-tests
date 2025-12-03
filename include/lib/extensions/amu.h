/*
 * Copyright (c) 2017-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AMU_H
#define AMU_H

#include <stdint.h>

#include <cassert.h>
#include <platform_def.h>
#include <utils_def.h>

#define AMU_GROUP0_COUNTERS_MASK	U(0xf)
#define AMU_GROUP0_NR_COUNTERS		U(4)

/* maximum number of counters */
#define AMU_GROUP1_NR_COUNTERS		U(16)

uint64_t amu_group1_num_counters(void);

uint64_t read_amevcntr0(unsigned int idx);
uint64_t read_amevcntr1(unsigned int idx);
uint64_t read_amevtyper1(unsigned int idx);
void write_amevtyper1(unsigned int idx, uint64_t val);

#if __aarch64__
uint64_t read_amevcntvoff0(unsigned int idx);
void write_amevcntvoff0(unsigned int idx, uint64_t val);
uint64_t read_amevcntvoff1(unsigned int idx);
void write_amevcntvoff1(unsigned int idx, uint64_t val);
#endif

#endif /* AMU_H */
