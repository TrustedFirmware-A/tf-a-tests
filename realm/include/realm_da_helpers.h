/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_DA_HELPERS_H
#define REALM_DA_HELPERS_H

#include <realm_rsi.h>

/*
 * Currently assigning one device is supported, for more than one device the VMM
 * view of vdev_id and Realm view of device_id must match
 */
#define RDEV_ID			(0x0UL)

#define RDEV_TDISP_VERSION_MAX	(0x10)

struct rdev {
	unsigned long id;
};

unsigned long realm_rdev_init(struct rdev *rdev, unsigned long rdev_id);
unsigned long realm_rsi_vdev_get_info(struct rdev *rdev, struct rsi_vdev_info *rdev_info);

#endif /* REALM_DA_HELPERS_H */
