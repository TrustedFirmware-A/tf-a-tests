/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdlib.h>

#include <realm_da_helpers.h>
#include <realm_rsi.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <host_shared_data.h>
#include <sync.h>

unsigned long realm_rsi_vdev_get_info(struct rdev *rdev, struct rsi_vdev_info *vdev_info)
{
	unsigned long rsi_rc;

	memset(vdev_info, 0, sizeof(struct rsi_vdev_info));
	rsi_rc = rsi_vdev_get_info(rdev->id, (u_register_t)vdev_info);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_VDEV_GET_INFO failed 0x%lx\n", rsi_rc);
		return rsi_rc;
	}

	/* Print RDEV info */
	INFO("RSI_VDEV_GET_INFO:\n");
	INFO("\tflags: 0x%lx\n", vdev_info->flags);
	INFO("\tcert_id: 0x%lx\n", vdev_info->cert_id);
	INFO("\thash_algo: 0x%lx\n", vdev_info->hash_algo);
	INFO("\tlock_nonce: 0x%lx\n", vdev_info->lock_nonce);
	INFO("\tmeas_nonce: 0x%lx\n", vdev_info->meas_nonce);
	INFO("\treport_nonce: 0x%lx\n", vdev_info->report_nonce);
	INFO("\tdisp_version: 0x%lx\n", vdev_info->tdisp_version);
	INFO("\tstate: 0x%lx\n", vdev_info->state);
	INFO("\tvca_digest: 0x%016lx...\n", *((unsigned long *)vdev_info->vca_digest));
	INFO("\tcert_digest: 0x%016lx...\n", *((unsigned long *)vdev_info->cert_digest));
	INFO("\tpubkey_digest: 0x%016lx...\n", *((unsigned long *)vdev_info->pubkey_digest));
	INFO("\tmeas_digest: 0x%016lx...\n", *((unsigned long *)vdev_info->meas_digest));
	INFO("\treport_digest: 0x%016lx...\n", *((unsigned long *)vdev_info->report_digest));
	return RSI_SUCCESS;
}
