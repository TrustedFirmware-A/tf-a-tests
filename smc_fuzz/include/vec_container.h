/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef VEC_CONTAINER_H
#define VEC_CONTAINER_H

#define VEC_CONTAINER_THRESHOLD 10

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "smcmalloc.h"

struct vec_container {
	uint64_t *elements;
	int nele;
	size_t element_size;
};

void vec_containerinit(struct vec_container *vc, size_t esize, struct memmod *mmod);
void vec_containerfree(struct vec_container *vc, struct memmod *mmod);
void pushvec(uint64_t *obj, struct vec_container *vc, struct memmod *mmod);


#endif /* VEC_CONTAINER_H */
