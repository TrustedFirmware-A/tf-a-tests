/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_DA_FLOW_HELPER_H
#define HOST_DA_FLOW_HELPER_H

#include <host_da_helper.h>
#include <tftf_lib.h>

test_result_t realm_assign_unassign_devices(struct realm *realm_ptr);
test_result_t tsm_connect_first_device(struct host_pdev **h_pdev);
test_result_t tsm_connect_devices(void);
test_result_t tsm_disconnect_device(struct host_pdev *h_pdev);
test_result_t tsm_disconnect_devices(void);

#endif /* HOST_DA_FLOW_HELPER_H */
