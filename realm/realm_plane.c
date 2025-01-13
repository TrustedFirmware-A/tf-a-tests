/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <stdio.h>
#include <arch.h>
#include <arch_features.h>
#include <assert.h>
#include <debug.h>

#include <host_realm_helper.h>
#include <realm_rsi.h>
#include <sync.h>

bool is_plane0;

void realm_plane_init(void)
{
	u_register_t ret;

	ret = rsi_get_version(RSI_ABI_VERSION_VAL);
	if (ret == RSI_ERROR_STATE) {
		is_plane0 = false;
	} else {
		is_plane0 = true;
	}
}

static void restore_plane_context(rsi_plane_run *run)
{
	for (unsigned int i = 0U; i < RSI_PLANE_NR_GPRS; i++) {
		run->enter.gprs[i] = run->exit.gprs[i];
	}

	run->enter.pc = run->exit.elr;
}

/* return true to re-enter PlaneN, false to exit to P0 */
static bool handle_plane_exit(u_register_t plane_index,
		u_register_t perm_index,
		rsi_plane_run *run)
{
	u_register_t ec = EC_BITS(run->exit.esr);

	/* Disallow SMC from Plane N */
	if (ec == EC_AARCH64_SMC) {
		restore_plane_context(run);
		run->enter.gprs[0] = RSI_ERROR_STATE;
		return true;
	}
	return false;
}

static bool plane_common_init(u_register_t plane_index,
		u_register_t perm_index,
		u_register_t base,
		rsi_plane_run *run)
{
	u_register_t ret;

	memset(run, 0, sizeof(rsi_plane_run));
	run->enter.pc = base;

	/* Perm init */
	ret = rsi_mem_set_perm_value(plane_index, perm_index, PERM_LABEL_RW_upX);
	if (ret != RSI_SUCCESS) {
		ERROR("rsi_mem_set_perm_value failed %u\n", plane_index);
		return false;
	}
	return true;
}

bool realm_plane_enter(u_register_t plane_index,
		u_register_t perm_index,
		u_register_t base,
		u_register_t flags,
		rsi_plane_run *run)
{
	u_register_t ret;
	bool ret1;

	ret1 = plane_common_init(plane_index, perm_index, base, run);
	if (!ret1) {
		return ret1;
	}

	run->enter.flags = flags;

	while (ret1) {
		ret = rsi_plane_enter(plane_index, (u_register_t)run);
		if (ret != RSI_SUCCESS) {
			ERROR("Plane %u enter failed ret= 0x%lx\n", plane_index, ret);
			return false;
		}

		VERBOSE("plane exit_reason=0x%lx esr=0x%lx hpfar=0x%lx far=0x%lx\n",
				run->exit.exit_reason,
				run->exit.esr,
				run->exit.hpfar,
				run->exit.far);

		ret1 = handle_plane_exit(plane_index, perm_index, run);
	}
	return true;
}

