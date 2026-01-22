/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_DA_FLOW_HELPER_H
#define HOST_DA_FLOW_HELPER_H

#include <host_da_helper.h>

int realm_assign_unassign_devices(struct realm *realm_ptr);
int tsm_connect_device(struct host_pdev *h_pdev);
int tsm_connect_first_device(struct host_pdev **h_pdev);
int tsm_connect_devices(unsigned int *count);
int tsm_disconnect_device(struct host_pdev *h_pdev);
int tsm_disconnect_devices(void);

#endif /* HOST_DA_FLOW_HELPER_H */
