/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include <debug.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>

u_register_t host_dev_mem_map(struct realm *realm, u_register_t dev_pa,
				long map_level, u_register_t *dev_ipa)
{
	u_register_t rd = realm->rd;
	u_register_t map_addr = dev_pa;	/* 1:1 PA->IPA mapping */
	u_register_t ret;

	*dev_ipa = 0UL;

	ret = host_rmi_dev_mem_map(rd, map_addr, map_level, dev_pa);

	if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
		/* Create missing RTTs and retry */
		long level = (long)RMI_RETURN_INDEX(ret);

		ret = host_rmi_create_rtt_levels(realm, map_addr,
						 level, map_level);
		if (ret != RMI_SUCCESS) {
			tftf_testcase_printf("%s() failed, 0x%lx\n",
				"host_rmi_create_rtt_levels", ret);
			return REALM_ERROR;
		}

		ret = host_rmi_dev_mem_map(rd, map_addr, map_level, dev_pa);
	}
	if (ret != RMI_SUCCESS) {
		tftf_testcase_printf("%s() failed, 0x%lx\n",
			"host_rmi_dev_mem_map", ret);
		return REALM_ERROR;
	}

	*dev_ipa = map_addr;
	return REALM_SUCCESS;
}
