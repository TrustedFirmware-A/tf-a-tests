/*
 * Copyright (c) 2018-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <amu.h>
#include <amu_private.h>
#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <assert.h>

/* Read the group 0 counter identified by the given `idx`. */
uint64_t amu_group0_cnt_read(unsigned int idx)
{
	assert(amu_get_version() != ID_PFR0_AMU_NOT_SUPPORTED);
	assert(idx < AMU_GROUP0_NR_COUNTERS);

	return amu_group0_cnt_read_internal(idx);
}

/* Read the group 1 counter identified by the given `idx`. */
uint64_t amu_group1_cnt_read(unsigned int idx)
{
	assert(amu_get_version() != ID_PFR0_AMU_NOT_SUPPORTED);
	assert(idx < AMU_GROUP1_NR_COUNTERS);

	return amu_group1_cnt_read_internal(idx);
}
