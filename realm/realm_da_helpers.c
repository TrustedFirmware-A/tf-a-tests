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

/* Get inst id for a known Realm device id RDEV_ID */
unsigned long realm_rdev_init(struct rdev *rdev, unsigned long rdev_id)
{
	u_register_t rsi_rc;
	u_register_t rdev_inst_id;

	memset(rdev, 0, sizeof(struct rdev));
	realm_printf("In test_realm_da_rsi_calls\n");

	rsi_rc = rsi_rdev_get_instance_id(rdev_id, &rdev_inst_id);
	if (rsi_rc != RSI_SUCCESS) {
		realm_printf("RSI_RDEV_GET_INSTANCE_ID failed: rdev_id: 0x%lx"
			     " rsi_rc: 0x%lx\n", rdev_id, rsi_rc);
		return rsi_rc;
	}

	rdev->id = rdev_id;
	rdev->inst_id = rdev_inst_id;

	realm_printf("RSI_RDEV_GET_INSTANCE_ID: rdev_id: 0x%lx, "
		     "inst_id: 0x%lx\n", rdev_id, rdev_inst_id);

	return RSI_SUCCESS;
}

unsigned long realm_rsi_rdev_get_state(struct rdev *rdev)
{
	u_register_t rdev_rsi_state;
	unsigned long rsi_rc;

	rsi_rc = rsi_rdev_get_state(rdev->id, rdev->inst_id, &rdev_rsi_state);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_RDEV_GET_STATE failed 0x%lx\n", rsi_rc);
		return rsi_rc;
	}

	INFO("Realm: RSI_RDEV_GET_STATE completed. RDEV state: 0x%lx\n",
	     rdev_rsi_state);

	return RSI_SUCCESS;
}

int realm_verify_device_attestation(struct rdev *rdev,
				    struct rsi_dev_info *rdev_info)
{
	/*
	 * todo: Implement RHI call to get cached device certificate,
	 * measurement from host and verify the digest against RMM
	 */
	return 0;
}

unsigned long realm_rsi_rdev_get_info(struct rdev *rdev,
					struct rsi_dev_info *rdev_info)
{
	unsigned long rsi_rc;

	memset(rdev_info, 0, sizeof(struct rsi_dev_info));
	rsi_rc = rsi_rdev_get_info(rdev->id, rdev->inst_id,
				   (u_register_t)rdev_info);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_RDEV_GET_INFO failed 0x%lx\n", rsi_rc);
		return rsi_rc;
	}

	/* Print RDEV info */
	INFO("RSI_RDEV_GET_INFO:\n");
	INFO("\tflags: 0x%lx\n", rdev_info->flags);
	INFO("\tattest_type: 0x%lx\n", rdev_info->attest_type);
	INFO("\tcert_id: 0x%lx\n", rdev_info->cert_id);
	INFO("\thash_algo: 0x%lx\n", rdev_info->hash_algo);

	return RSI_SUCCESS;
}

unsigned long realm_rsi_rdev_get_measurements(struct rdev *rdev,
						struct rsi_dev_measure_params *mparams)
{
	unsigned long rsi_rc;

	INFO("Realm: Call RSI_RDEV_GET_MEASUREMENTS\n");

	/* Set measurement parameters */
	memset(mparams, 0, sizeof(struct rsi_dev_measure_params));
	mparams->flags = (INPLACE(RSI_DEV_MEASURE_FLAGS_ALL,
				  RSI_DEV_MEASURE_ALL) |
			  INPLACE(RSI_DEV_MEASURE_FLAGS_SIGNED,
				  RSI_DEV_MEASURE_NOT_SIGNED) |
			  INPLACE(RSI_DEV_MEASURE_FLAGS_RAW,
				  RSI_DEV_MEASURE_NOT_RAW));


	rsi_rc = rsi_rdev_get_measurements(rdev->id, rdev->inst_id,
					   (u_register_t)mparams);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_RDEV_GET_MEASUREMENTS failed\n");
		return RSI_ERROR_STATE;
	}

	INFO("Host Realm: RDEV state after submitting meas request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	/* Do RSI RDEV continue call */
	do {
		rsi_rc = rsi_rdev_continue(rdev->id, rdev->inst_id);
	} while (rsi_rc == RSI_INCOMPLETE);

	INFO("Host Realm: RDEV state after host completing meas request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	return rsi_rc;
}

unsigned long realm_rsi_rdev_lock(struct rdev *rdev)
{
	unsigned long rsi_rc;

	INFO("Realm: Call RSI_RDEV_LOCK\n");

	rsi_rc = rsi_rdev_lock(rdev->id, rdev->inst_id);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_RDEV_LOCK failed\n");
		return RSI_ERROR_STATE;
	}

	INFO("Realm: RDEV state after submitting lock request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	/* Do RSI RDEV continue call */
	do {
		rsi_rc = rsi_rdev_continue(rdev->id, rdev->inst_id);
	} while (rsi_rc == RSI_INCOMPLETE);

	INFO("Realm: RDEV state after host completing lock request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	return rsi_rc;
}

unsigned long realm_rsi_rdev_get_interface_report(struct rdev *rdev)
{
	unsigned long rsi_rc;

	INFO("Realm: Call RSI_RDEV_GET_IFC_REPORT\n");

	rsi_rc = rsi_rdev_get_interface_report(rdev->id, rdev->inst_id,
					       RDEV_TDISP_VERSION_MAX);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_RDEV_GET_IFC_REPORT failed\n");
		return RSI_ERROR_STATE;
	}

	INFO("Realm: RDEV state after submitting IFC_REPORT request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	/* Do RSI RDEV continue call */
	do {
		rsi_rc = rsi_rdev_continue(rdev->id, rdev->inst_id);
	} while (rsi_rc == RSI_INCOMPLETE);

	INFO("Realm: RDEV state after host completing IFC_REPORT request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	return rsi_rc;
}

unsigned long realm_rsi_rdev_start(struct rdev *rdev)
{
	unsigned long rsi_rc;

	INFO("Realm: Call RSI_RDEV_start\n");

	rsi_rc = rsi_rdev_start(rdev->id, rdev->inst_id);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_RDEV_START failed\n");
		return RSI_ERROR_STATE;
	}

	INFO("Realm: RDEV state after submitting start request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	/* Do RSI RDEV continue call */
	do {
		rsi_rc = rsi_rdev_continue(rdev->id, rdev->inst_id);
	} while (rsi_rc == RSI_INCOMPLETE);

	INFO("Realm: RDEV state after host completing start request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	return rsi_rc;
}

unsigned long realm_rsi_rdev_stop(struct rdev *rdev)
{
	unsigned long rsi_rc;

	INFO("Realm: Call RSI_RDEV_STOP\n");

	rsi_rc = rsi_rdev_stop(rdev->id, rdev->inst_id);
	if (rsi_rc != RSI_SUCCESS) {
		ERROR("RSI_RDEV_STOP failed\n");
		return RSI_ERROR_STATE;
	}

	INFO("Realm: RDEV state after submitting stop request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	/* Do RSI RDEV continue call */
	do {
		rsi_rc = rsi_rdev_continue(rdev->id, rdev->inst_id);
	} while (rsi_rc == RSI_INCOMPLETE);

	INFO("Realm: RDEV state after host completing stop request\n");
	(void)realm_rsi_rdev_get_state(rdev);

	return rsi_rc;
}
