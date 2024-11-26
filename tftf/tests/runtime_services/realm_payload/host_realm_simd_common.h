/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_REALM_COMMON_H
#define HOST_REALM_COMMON_H

#define NS_NORMAL_SVE		0x1U
#define NS_STREAMING_SVE	0x2U

test_result_t host_create_sve_realm_payload(struct realm *realm, bool sve_en,
					    uint8_t sve_vq);

#endif
