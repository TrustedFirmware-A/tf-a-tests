/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
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

/* VDEV info. Device type and attestation evidence digest */
static struct rsi_vdev_info gbl_vdev_info[2] __aligned(GRANULE_SIZE);

/*
 * If the Realm supports DA feature, this function gets VDEV info for the
 * assigned device.
 *
 * Returns 'false' on error.
 */
bool test_realm_da_rsi_vdev_get_info(void)
{
	u_register_t rsi_feature_reg0, rsi_rc, vdev_id;
	struct rsi_vdev_info *vdev_info;

	/* Check if RSI_FEATURES support DA */
	rsi_rc = rsi_features(RSI_FEATURE_REGISTER_0_INDEX, &rsi_feature_reg0);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	if (EXTRACT(RSI_FEATURE_REGISTER_0_DA, rsi_feature_reg0) !=
	    RSI_FEATURE_TRUE) {
		realm_printf("RSI feature DA not supported for current Realm\n");
		return false;
	}

	vdev_id = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);

	/* Use idx 1 to have struct size alignment */
	vdev_info = &gbl_vdev_info[1];

	/* After meas and ifc_report, get device info */
	rsi_rc = realm_rsi_vdev_get_info(vdev_id, vdev_info);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	return true;
}
