/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_DA_HELPER_H
#define HOST_DA_HELPER_H

#include <host_realm_rmi.h>

u_register_t host_dev_mem_map(struct realm *realm, u_register_t dev_pa,
				long map_level, u_register_t *dev_ipa);

#endif /* HOST_DA_HELPER_H */
