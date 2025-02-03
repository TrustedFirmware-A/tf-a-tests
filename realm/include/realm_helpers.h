/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_HELPERS_H
#define REALM_HELPERS_H

#include <realm_rsi.h>

/* Generate 64-bit random number */
unsigned long long realm_rand64(void);
/*
 * Function to enter Aux Plane from Primary Plane
 * arg1 == plane index
 * arg2 == permission index to be used by plane
 * arg3 == base entrypoint
 * arg4 == entry flags
 * aarg5 == run object, needs to be PAGE aligned
 */
bool realm_plane_enter(u_register_t plane_index, u_register_t perm_index,
		u_register_t base, u_register_t flags, rsi_plane_run *run);

/* This function will call the Host to request IPA of the NS shared buffer */
u_register_t realm_get_ns_buffer(void);

/* This function will return plane index of current plane */
unsigned int realm_get_my_plane_num(void);

/** This function will return true for primary plane false for aux plane */
bool realm_is_plane0(void);

/* Function for initializing planes, called at Boot */
void realm_plane_init(void);
bool plane_common_init(u_register_t plane_index, u_register_t perm_index,
		u_register_t base, rsi_plane_run *run);

#endif /* REALM_HELPERS_H */

