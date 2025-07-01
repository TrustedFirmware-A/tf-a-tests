/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_DA_HELPER_H
#define HOST_DA_HELPER_H

#include <host_realm_rmi.h>

/*
 * Skip DA test if any of the below check is true
 *   RMM is TRP
 *   FEAT_RME not supported
 *   DA is not supported in RMI features
 */
#define SKIP_DA_TEST_IF_PREREQS_NOT_MET(_reg0)					\
	do {									\
		SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();			\
										\
		/* Get feature register0 */					\
		if (host_rmi_features(0UL, &_reg0) != REALM_SUCCESS) {		\
			ERROR("Failed to get RMI feat_reg0\n");			\
			return TEST_RESULT_FAIL;				\
		}								\
										\
		/* DA not supported in RMI features? */				\
		if ((_reg0 & RMI_FEATURE_REGISTER_0_DA_EN) == 0UL) {		\
			WARN("DA not in RMI features, skipping\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

/* SPDM_MAX_CERTIFICATE_CHAIN_SIZE is 64KB */
#define HOST_PDEV_CERT_LEN_MAX		(64 * 1024)

/*
 * Measurement max supported is 4KB.
 * todo: This will be increased if device supports returning more measurements
 */
#define HOST_PDEV_MEAS_LEN_MAX		(4 * 1024)
#define HOST_VDEV_MEAS_LEN_MAX		(4 * 1024)
#define HOST_VDEV_IFC_REPORT_LEN_MAX	(8 * 1024)

#define DEV_OBJ_CERT			0U
#define DEV_OBJ_MEASUREMENTS		2U
#define DEV_OBJ_INTERFACE_REPORT	3U

struct host_pdev {
	/* PDEV related fields */
	void *pdev;
	unsigned long pdev_flags;
	void *pdev_aux[PDEV_PARAM_AUX_GRANULES_MAX];
	uint32_t pdev_aux_num;
	struct rmi_dev_comm_data *dev_comm_data;

	/* Algorithm used to generate device digests */
	uint8_t pdev_hash_algo;

	/* Certificate, public key fields */
	uint8_t cert_slot_id;
	uint8_t *cert_chain;
	size_t cert_chain_len;
	void *public_key;
	size_t public_key_len;
	void *public_key_metadata;
	size_t public_key_metadata_len;
	unsigned char public_key_sig_algo;

	/* PCIe details: bdf, DOE, Stream id, IO range */
	uint32_t bdf;
	uint32_t doe_cap_base;
};

struct host_vdev {
	void *vdev_ptr;
	void *pdev_ptr;

	/*
	 * NS host assigns the vdev_id, and this same id is used by Realm when
	 * it enumerates the PCI devices
	 */
	unsigned long vdev_id;

	/*
	 * The TEE device interface ID. Currently NS host assigns the whole
	 * device, this value is same as PDEV's bdf.
	 */
	unsigned long tdi_id;

	unsigned long flags;

	void *vdev_aux[VDEV_PARAM_AUX_GRANULES_MAX];
	uint32_t vdev_aux_num;

	struct rmi_dev_comm_data *dev_comm_data;

	/* Fields related to cached device measurements. */
	uint8_t *meas;
	size_t meas_len;

	/* Fields related to cached interface report. */
	uint8_t *ifc_report;
	size_t ifc_report_len;
};

int host_create_realm_with_feat_da(struct realm *realm);
int host_pdev_create(struct host_pdev *h_pdev);
int host_pdev_reclaim(struct host_pdev *h_pdev);
int host_pdev_setup(struct host_pdev *h_pdev);
int host_pdev_transition(struct host_pdev *h_pdev, unsigned char to_state);

int host_assign_vdev_to_realm(struct realm *realm, struct host_pdev *h_pdev,
			      struct host_vdev *h_vdev);
int host_unassign_vdev_from_realm(struct realm *realm, struct host_pdev *h_pdev,
				  struct host_vdev *h_vdev);
u_register_t host_dev_mem_map(struct realm *realm, u_register_t dev_pa,
				long map_level, u_register_t *dev_ipa);

#endif /* HOST_DA_HELPER_H */
