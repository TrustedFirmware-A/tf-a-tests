/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_DA_FLOW_HELPER_H
#define HOST_DA_FLOW_HELPER_H

#include <host_da_helper.h>
#include <tftf_lib.h>

int realm_assign_unassign_devices(struct realm *realm_ptr);
int tsm_connect_device(struct host_pdev *h_pdev);
int tsm_connect_first_device(struct host_pdev **h_pdev);
int tsm_connect_devices(unsigned int *count);
int tsm_disconnect_device(struct host_pdev *h_pdev);
int tsm_disconnect_devices(void);
int realm_assign_device(struct realm *realm_ptr,
			struct host_vdev *h_vdev,
			unsigned long tdi_id,
			void *pdev_ptr);
int realm_assign_unassign_device(struct realm *realm_ptr,
				 struct host_vdev *h_vdev,
				 unsigned long tdi_id,
				 void *pdev_ptr);

#endif /* HOST_DA_FLOW_HELPER_H */
