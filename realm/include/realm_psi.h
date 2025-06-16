/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef REALM_PSI_H
#define REALM_PSI_H

#include <stdint.h>
#include <realm_rsi.h>

/*
 * Plane Service Interface
 * ABIs are designed to be used by auxillary planes to interact with primary plane.
 * PSI calls uses HVC conduit which causes Plane exit to P0
 * PSI commands and Planes shared buffer can be used for communication.
 */
#define PSI_RETURN_TO_P0	1U
#define PSI_RETURN_TO_PN	2U

/* PSI Commands to return back to P0 */
#define PSI_P0_CALL			SMC_RSI_HOST_CALL
#define PSI_REALM_CONFIG		SMC_RSI_REALM_CONFIG
#define PSI_CALL_EXIT_PRINT_CMD		HOST_CALL_EXIT_PRINT_CMD
#define PSI_CALL_EXIT_SUCCESS_CMD	HOST_CALL_EXIT_SUCCESS_CMD
#define PSI_CALL_EXIT_FAILED_CMD	HOST_CALL_EXIT_FAILED_CMD
#define PSI_CALL_GET_PLANE_ID_CMD	HOST_CALL_GET_PLANE_ID_CMD
#define PSI_CALL_GET_SHARED_BUFF_CMD	HOST_CALL_GET_SHARED_BUFF_CMD

/* Exit back to Plane 0 */
void psi_exit_to_plane0(u_register_t psi_cmd,
			u_register_t arg1,
			u_register_t arg2,
			u_register_t arg3,
			u_register_t arg4,
			u_register_t arg5,
			u_register_t arg6,
			u_register_t arg7);

/* Request plane_id from P0 */
u_register_t psi_get_plane_id(void);

#endif /* REALM_PSI_H */
