/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <stdint.h>
#include "smcmalloc.h"

#define SANITY_LEVEL_0 0
#define SANITY_LEVEL_1 1
#define SANITY_LEVEL_2 2
#define SANITY_LEVEL_3 3

#define FUZZER_CONSTRAINT_SVALUE 0
#define FUZZER_CONSTRAINT_RANGE 1
#define FUZZER_CONSTRAINT_VECTOR 2

#define FUZZER_CONSTRAINT_ACCMODE 0
#define FUZZER_CONSTRAINT_EXCMODE 1

#define FUZZ_MAX_SHIFT_AMNT 16
#define FUZZ_MAX_REG_SIZE 64
#define FUZZ_MAX_NAME_SIZE 80

struct inputparameters {
	uint64_t x1;
	uint64_t x2;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
};

void setconstraint(int contype, uint64_t *vecinput, int veclen, int fieldnameptr, struct memmod *mmod, int mode);
struct inputparameters generate_args(int smccall, int sanity);
uint64_t get_generated_value(int fieldnameptr, struct inputparameters inp);
void print_smccall(int smccall, struct inputparameters inp);
#endif /* CONSTRAINT_H */
