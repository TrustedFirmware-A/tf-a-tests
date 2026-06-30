/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LFA_TEST_HELPERS_H
#define LFA_TEST_HELPERS_H

#include <stdbool.h>
#include <stdint.h>

#include <lfa.h>
#include <tftf_lib.h>

struct lfa_test_target {
	const char *name;
	uint64_t uuid1;
	uint64_t uuid2;
};

struct lfa_test_inventory_entry {
	uint64_t fw_id;
	uint64_t attributes;
	bool present;
};

static const struct lfa_test_target lfa_test_rmm = {
	.name = "RMM",
	.uuid1 = RMM_X1,
	.uuid2 = RMM_X2,
};

static const struct lfa_test_target lfa_test_bl31 = {
	.name = "BL31",
	.uuid1 = BL31_X1,
	.uuid2 = BL31_X2,
};

static inline bool lfa_test_target_matches(const struct lfa_test_target *target,
					   uint64_t uuid1,
					   uint64_t uuid2)
{
	return (target->uuid1 == uuid1) && (target->uuid2 == uuid2);
}

static inline smc_args lfa_test_init_fw_args(uint32_t fid, uint64_t fw_id)
{
	return (smc_args) {
		.fid = fid,
		.arg1 = fw_id,
	};
}

static inline void lfa_test_record_inventory_entry(
		const struct lfa_test_target *target,
		uint64_t fw_id,
		const smc_ret_values *ret,
		struct lfa_test_inventory_entry *entry)
{
	if (!lfa_test_target_matches(target, ret->ret1, ret->ret2)) {
		return;
	}

	entry->fw_id = fw_id;
	entry->attributes = ret->ret3;
	entry->present = true;
}

#endif /* LFA_TEST_HELPERS_H */
