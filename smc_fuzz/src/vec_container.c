/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Generic vector
 */

#include <stdint.h>

#include "vec_container.h"

#ifdef SMC_FUZZ_TMALLOC
#define GENMALLOC(x)    malloc((x))
#define GENFREE(x)      free((x))
#else
#define GENMALLOC(x)    smcmalloc((x), mmod)
#define GENFREE(x)      smcfree((x), mmod)
#endif

/*
 * Initialization of data structure
 */
void vec_containerinit(struct vec_container *vc, size_t esize, struct memmod *mmod)
{
	vc->nele = 0;
	vc->element_size = esize;
}


void vec_containerfree(struct vec_container *vc, struct memmod *mmod)
{
	GENFREE(vc->elements);
	vc->nele = 0;
}

void pushvec(uint64_t *obj, struct vec_container *vc, struct memmod *mmod)
{
	uint64_t *ne = NULL;
	if ((vc->nele % VEC_CONTAINER_THRESHOLD) == 0) {
		if (vc->nele != 0) {
			ne = GENMALLOC((VEC_CONTAINER_THRESHOLD * vc->nele) * sizeof(uint64_t));
			memcpy(ne, vc->elements, (vc->element_size * vc->nele));
			GENFREE(vc->elements);
		} else {
			ne = GENMALLOC((VEC_CONTAINER_THRESHOLD) * sizeof(uint64_t));
		}
		vc->elements = ne;
	}
	memcpy(&vc->elements[vc->nele], obj, vc->element_size);
	vc->nele++;
}

