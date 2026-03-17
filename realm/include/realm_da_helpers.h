/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_DA_HELPERS_H
#define REALM_DA_HELPERS_H

#include <realm_rsi.h>

unsigned long realm_rsi_vdev_get_info(u_register_t vdev_id,
				      struct rsi_vdev_info *vdev_info);

#endif /* REALM_DA_HELPERS_H */
