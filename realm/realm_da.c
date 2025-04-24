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

static struct rdev gbl_rdev;

/* Measurement parameters */
static struct rsi_dev_measure_params gbl_rdev_meas_params
__aligned(GRANULE_SIZE);

/* RDEV info. decice type and attestation evidence digest */
static struct rsi_dev_info gbl_rdev_info __aligned(GRANULE_SIZE);

/*
 * If the Realm supports DA feature, this function calls series of RSI RDEV
 * on the assigned device.
 *
 * Returns 'false' on success.
 */
bool test_realm_da_rsi_calls(void)
{
	struct rdev *rdev;
	unsigned long rsi_rc;
	u_register_t rsi_feature_reg0;
	struct rsi_dev_measure_params *rdev_meas_params;
	struct rsi_dev_info *rdev_info;

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

	/* Get the global RDEV. Currently only one RDEV is supported */
	rdev = &gbl_rdev;
	rdev_meas_params = &gbl_rdev_meas_params;
	rdev_info = &gbl_rdev_info;

	rsi_rc = realm_rdev_init(rdev, RDEV_ID);
	if (rdev == NULL) {
		realm_printf("realm_rdev_init failed\n");
		return false;
	}

	rsi_rc = realm_rsi_rdev_get_state(rdev);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	/* Before lock get_measurement */
	rsi_rc = realm_rsi_rdev_get_measurements(rdev, rdev_meas_params);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	rsi_rc = realm_rsi_rdev_lock(rdev);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	/* After lock get_measurement */
	rsi_rc = realm_rsi_rdev_get_measurements(rdev, rdev_meas_params);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	rsi_rc = realm_rsi_rdev_get_interface_report(rdev);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	/* After meas and ifc_report, get device info */
	rsi_rc = realm_rsi_rdev_get_info(rdev, rdev_info);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	/*
	 * Get cached device attestation from Host and verify it with device
	 * attestation digest
	 */
	(void)realm_verify_device_attestation(rdev, rdev_info);

	rsi_rc = realm_rsi_rdev_start(rdev);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	rsi_rc = realm_rsi_rdev_stop(rdev);
	if (rsi_rc != RSI_SUCCESS) {
		return false;
	}

	return true;
}
