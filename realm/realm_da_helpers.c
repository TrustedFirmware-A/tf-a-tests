/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <debug.h>

#include <realm_da_helpers.h>
#include <realm_rsi.h>

unsigned long realm_rsi_vdev_get_info(struct rdev *rdev, struct rsi_vdev_info *vdev_info)
{
	unsigned long rsi_rc;

	(void)memset(vdev_info, 0, sizeof(struct rsi_vdev_info));
	rsi_rc = rsi_vdev_get_info(rdev->id, (u_register_t)vdev_info);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("rsi_vdev_get_info() failed 0x%lx\n", rsi_rc);
		return rsi_rc;
	}

	/* Print RDEV realm_printf */
	realm_printf("RSI_VDEV_GET_INFO:\n");
	realm_printf("\tflags: 0x%lx\n", vdev_info->flags);
	realm_printf("\tcert_id: 0x%lx\n", vdev_info->cert_id);
	realm_printf("\thash_algo: 0x%lx\n", vdev_info->hash_algo);
	realm_printf("\tlock_nonce: 0x%lx\n", vdev_info->lock_nonce);
	realm_printf("\tmeas_nonce: 0x%lx\n", vdev_info->meas_nonce);
	realm_printf("\treport_nonce: 0x%lx\n", vdev_info->report_nonce);
	realm_printf("\tdisp_version: 0x%lx\n", vdev_info->tdisp_version);
	realm_printf("\tstate: 0x%lx\n", vdev_info->state);
	realm_printf("\tvca_digest: 0x%016lx...\n",
			*((unsigned long *)vdev_info->vca_digest));
	realm_printf("\tcert_digest: 0x%016lx...\n",
			*((unsigned long *)vdev_info->cert_digest));
	realm_printf("\tpubkey_digest: 0x%016lx...\n",
			*((unsigned long *)vdev_info->pubkey_digest));
	realm_printf("\tmeas_digest: 0x%016lx...\n",
			*((unsigned long *)vdev_info->meas_digest));
	realm_printf("\treport_digest: 0x%016lx...\n",
			*((unsigned long *)vdev_info->report_digest));
	return RSI_SUCCESS;
}
