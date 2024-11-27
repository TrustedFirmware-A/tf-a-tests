/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <host_realm_rmi.h>
#include <realm_helpers.h>
#include <realm_psi.h>
#include <realm_rsi.h>
#include <smccc.h>

void psi_exit_to_plane0(u_register_t psi_cmd,
			u_register_t arg1,
			u_register_t arg2,
			u_register_t arg3,
			u_register_t arg4,
			u_register_t arg5,
			u_register_t arg6,
			u_register_t arg7)
{
	if (realm_is_plane0()) {
		return;
	}
	tftf_hvc(&(hvc_args) {psi_cmd, arg1, arg2, arg3, arg4,
			arg5, arg6, arg7});
}

u_register_t psi_get_plane_id(void)
{
	hvc_ret_values res = tftf_hvc(&(hvc_args) {PSI_CALL_GET_PLANE_ID_CMD, 0UL, 0UL,
			0UL, 0UL, 0UL, 0UL, 0UL});

	if (res.ret0 != RSI_SUCCESS) {
		return (u_register_t)-1;
	}
	return res.ret1;
}

