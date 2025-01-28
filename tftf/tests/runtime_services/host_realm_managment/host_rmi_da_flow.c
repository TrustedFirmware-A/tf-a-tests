/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <heap/page_alloc.h>
#include <host_crypto_utils.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <mmio.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <pcie_spec.h>
#include <platform.h>
#include <spdm.h>
#include <test_helpers.h>

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

static struct host_pdev gbl_host_pdev;
static struct host_vdev gbl_host_vdev;

static const char * const pdev_state_str[] = {
	"PDEV_STATE_NEW",
	"PDEV_STATE_NEEDS_KEY",
	"PDEV_STATE_HAS_KEY",
	"PDEV_STATE_READY",
	"PDEV_STATE_COMMUNICATING",
	"PDEV_STATE_STOPPED",
	"RMI_PDEV_STATE_ERROR"
};

struct host_vdev *find_host_vdev_from_id(unsigned long vdev_id)
{
	struct host_vdev *h_vdev = &gbl_host_vdev;

	if (h_vdev->vdev_id == vdev_id) {
		return h_vdev;
	}

	return NULL;
}

struct host_pdev *find_host_pdev_from_pdev_ptr(unsigned long pdev_ptr)
{
	struct host_pdev *h_pdev = &gbl_host_pdev;

	if (h_pdev->pdev == (void *)pdev_ptr) {
		return h_pdev;
	}

	return NULL;
}

struct host_vdev *find_host_vdev_from_vdev_ptr(unsigned long vdev_ptr)
{
	struct host_vdev *h_vdev = &gbl_host_vdev;

	if (h_vdev->vdev_ptr == (void *)vdev_ptr) {
		return h_vdev;
	}

	return NULL;
}

struct host_pdev *find_host_pdev_from_vdev_ptr(unsigned long vdev_ptr)
{
	struct host_vdev *h_vdev = &gbl_host_vdev;

	if (h_vdev->vdev_ptr == (void *)vdev_ptr) {
		return find_host_pdev_from_pdev_ptr(
			(unsigned long)h_vdev->pdev_ptr);
	}

	return NULL;
}

static int host_pdev_get_state(struct host_pdev *h_pdev, u_register_t *state)
{
	u_register_t ret;

	ret = host_rmi_pdev_get_state((u_register_t)h_pdev->pdev, state);
	if (ret != RMI_SUCCESS) {
		return -1;
	}
	return 0;
}

static bool is_host_pdev_state(struct host_pdev *h_pdev, u_register_t exp_state)
{
	u_register_t cur_state;

	if (host_pdev_get_state(h_pdev, &cur_state) != 0) {
		return false;
	}

	if (cur_state != exp_state) {
		return false;
	}

	return true;
}

static int host_vdev_get_state(struct host_vdev *h_vdev, u_register_t *state)
{
	u_register_t ret;

	ret = host_rmi_vdev_get_state((u_register_t)h_vdev->vdev_ptr, state);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_pdev_create(struct host_pdev *h_pdev)
{
	struct rmi_pdev_params *pdev_params;
	u_register_t ret;
	uint32_t i;

	pdev_params = (struct rmi_pdev_params *)page_alloc(PAGE_SIZE);
	memset(pdev_params, 0, GRANULE_SIZE);

	pdev_params->flags = h_pdev->pdev_flags;
	pdev_params->cert_id = h_pdev->cert_slot_id;
	pdev_params->pdev_id = h_pdev->bdf;
	pdev_params->num_aux = h_pdev->pdev_aux_num;
	pdev_params->hash_algo = h_pdev->pdev_hash_algo;
	for (i = 0; i < h_pdev->pdev_aux_num; i++) {
		pdev_params->aux[i] = (uintptr_t)h_pdev->pdev_aux[i];
	}

	ret = host_rmi_pdev_create((u_register_t)h_pdev->pdev,
				   (u_register_t)pdev_params);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_pdev_set_pubkey(struct host_pdev *h_pdev)
{
	struct rmi_public_key_params *pubkey_params;
	u_register_t ret;

	pubkey_params = (struct rmi_public_key_params *)page_alloc(PAGE_SIZE);
	memset(pubkey_params, 0, GRANULE_SIZE);

	memcpy(pubkey_params->key, h_pdev->public_key, h_pdev->public_key_len);
	memcpy(pubkey_params->metadata, h_pdev->public_key_metadata,
	       h_pdev->public_key_metadata_len);
	pubkey_params->key_len = h_pdev->public_key_len;
	pubkey_params->metadata_len = h_pdev->public_key_metadata_len;
	pubkey_params->algo = h_pdev->public_key_sig_algo;

	ret = host_rmi_pdev_set_pubkey((u_register_t)h_pdev->pdev,
				       (u_register_t)pubkey_params);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_pdev_stop(struct host_pdev *h_pdev)
{
	u_register_t ret;

	ret = host_rmi_pdev_stop((u_register_t)h_pdev->pdev);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_pdev_destroy(struct host_pdev *h_pdev)
{
	u_register_t ret;

	ret = host_rmi_pdev_destroy((u_register_t)h_pdev->pdev);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_dev_get_state(struct host_pdev *h_pdev, struct host_vdev *h_vdev,
			      u_register_t *state)
{
	if (h_vdev) {
		return host_vdev_get_state(h_vdev, state);
	} else {
		return host_pdev_get_state(h_pdev, state);
	}
}

static u_register_t host_rmi_dev_communicate(struct host_pdev *h_pdev,
					     struct host_vdev *h_vdev)
{
	if (h_vdev) {
		return host_rmi_vdev_communicate((u_register_t)h_vdev->pdev_ptr,
						 (u_register_t)h_vdev->vdev_ptr,
						 (u_register_t)
						 h_vdev->dev_comm_data);
	} else {
		return host_rmi_pdev_communicate((u_register_t)h_pdev->pdev,
						 (u_register_t)
						 h_pdev->dev_comm_data);
	}
}

static int host_pdev_cache_device_object(struct host_pdev *h_pdev,
					 uint8_t dev_obj_id,
					 const uint8_t *dev_obj_buf,
					 unsigned long dev_obj_buf_len)
{
	int rc = -1;

	/*
	 * During PDEV communicate only device object of type certificate is
	 * cached
	 */
	if (dev_obj_id == RMI_DEV_COMM_OBJECT_CERTIFICATE) {
		if ((h_pdev->cert_chain_len + dev_obj_buf_len) >
		    HOST_PDEV_CERT_LEN_MAX) {
			return -1;
		}

		INFO("%s: cache_cert: offset: 0x%lx, len: 0x%lx\n",
		     __func__, h_pdev->cert_chain_len, dev_obj_buf_len);

		memcpy((void *)(h_pdev->cert_chain + h_pdev->cert_chain_len),
		       dev_obj_buf, dev_obj_buf_len);
		h_pdev->cert_chain_len += dev_obj_buf_len;
		rc = 0;
	}

	return rc;
}

static int host_vdev_cache_device_object(struct host_vdev *h_vdev,
					 uint8_t dev_obj_id,
					 const uint8_t *dev_obj_buf,
					 unsigned long dev_obj_buf_len)
{
	int rc = -1;

	/*
	 * During VDEV communicate either measurement or interface report is
	 * cached
	 */
	if (dev_obj_id == RMI_DEV_COMM_OBJECT_MEASUREMENTS) {
		if ((h_vdev->meas_len + dev_obj_buf_len) >
		    HOST_VDEV_MEAS_LEN_MAX) {
			return -1;
		}

		INFO("%s: cache_meas: offset: 0x%lx, len: 0x%lx\n",
		     __func__, h_vdev->meas_len, dev_obj_buf_len);

		memcpy((void *)(h_vdev->meas + h_vdev->meas_len), dev_obj_buf,
		       dev_obj_buf_len);
		h_vdev->meas_len += dev_obj_buf_len;
		rc = 0;
	} else if (dev_obj_id == RMI_DEV_COMM_OBJECT_INTERFACE_REPORT) {
		if ((h_vdev->ifc_report_len + dev_obj_buf_len) >
		    HOST_VDEV_IFC_REPORT_LEN_MAX) {
			return -1;
		}

		INFO("%s: cache_ifc_report: offset: 0x%lx, len: 0x%lx\n",
		     __func__, h_vdev->ifc_report_len, dev_obj_buf_len);

		memcpy((void *)(h_vdev->ifc_report + h_vdev->ifc_report_len),
		       dev_obj_buf, dev_obj_buf_len);
		h_vdev->ifc_report_len += dev_obj_buf_len;
		rc = 0;
	}

	return rc;
}

static int host_dev_cache_dev_object(struct host_pdev *h_pdev,
				     struct host_vdev *h_vdev,
				     struct rmi_dev_comm_enter *dcomm_enter,
				     struct rmi_dev_comm_exit *dcomm_exit)
{
	unsigned long dev_obj_buf_len;
	uint8_t *dev_obj_buf;
	int rc;

	if ((dcomm_exit->cache_rsp_len == 0) ||
	    ((dcomm_exit->cache_rsp_offset + dcomm_exit->cache_rsp_len) >
	     GRANULE_SIZE)) {
		INFO("Invalid cache offset/length\n");
		return -1;
	}

	dev_obj_buf = (uint8_t *)dcomm_enter->resp_addr +
		dcomm_exit->cache_rsp_offset;
	dev_obj_buf_len = dcomm_exit->cache_rsp_len;

	if (h_vdev) {
		rc = host_vdev_cache_device_object(h_vdev,
						   dcomm_exit->cache_obj_id,
						   dev_obj_buf,
						   dev_obj_buf_len);
	} else {
		rc = host_pdev_cache_device_object(h_pdev,
						   dcomm_exit->cache_obj_id,
						   dev_obj_buf,
						   dev_obj_buf_len);
	}

	if (rc != 0) {
		INFO("host_dev_cache_device_object failed\n");
		rc = -1;
	}

	return rc;
}

static int host_pdev_doe_communicate(struct host_pdev *h_pdev,
				     struct rmi_dev_comm_enter *dcomm_enter,
				     struct rmi_dev_comm_exit *dcomm_exit)
{
	uint32_t doe_header;
	size_t resp_len;
	int rc;

	/* todo: validate DevCommExit flags */
	if (dcomm_exit->protocol == RMI_DEV_COMM_PROTOCOL_SPDM) {
		doe_header = DOE_HEADER_1;
	} else if (dcomm_exit->protocol == RMI_DEV_COMM_PROTOCOL_SECURE_SPDM) {
		doe_header = DOE_HEADER_2;
	} else {
		INFO("Invalid dev_comm_exit.protocol\n");
		return -1;
	}

	resp_len = 0UL;
	rc = pcie_doe_communicate(doe_header, h_pdev->bdf, h_pdev->doe_cap_base,
				  (void *)dcomm_enter->req_addr,
				  dcomm_exit->req_len,
				  (void *)dcomm_enter->resp_addr, &resp_len);

	/*
	 * Set IoEnter args for next pdev_communicate. Upon
	 * success or error call pdev_communicate
	 */
	if (rc == 0) {
		dcomm_enter->status = RMI_DEV_COMM_ENTER_STATUS_RESPONSE;
		dcomm_enter->resp_len = resp_len;
	} else {
		dcomm_enter->status = RMI_DEV_COMM_ENTER_STATUS_ERROR;
		dcomm_enter->resp_len = 0;
		rc = -1;
	}

	return rc;
}

/*
 * Call either PDEV or VDEV communicate until the target state is reached for
 * either PDEV or VDEV
 */
static int host_dev_communicate(struct host_pdev *h_pdev,
				struct host_vdev *h_vdev,
				unsigned char target_state)
{
	int rc;
	u_register_t state;
	u_register_t error_state;
	u_register_t ret;
	struct rmi_dev_comm_enter *dcomm_enter;
	struct rmi_dev_comm_exit *dcomm_exit;

	if (h_vdev) {
		dcomm_enter = &h_vdev->dev_comm_data->enter;
		dcomm_exit = &h_vdev->dev_comm_data->exit;

		error_state = RMI_VDEV_STATE_ERROR;
	} else {
		dcomm_enter = &h_pdev->dev_comm_data->enter;
		dcomm_exit = &h_pdev->dev_comm_data->exit;

		error_state = RMI_PDEV_STATE_ERROR;
	}

	dcomm_enter->status = RMI_DEV_COMM_ENTER_STATUS_NONE;
	dcomm_enter->resp_len = 0;

	rc = host_dev_get_state(h_pdev, h_vdev, &state);
	if (rc != 0) {
		return -1;
	}

	do {
		ret = host_rmi_dev_communicate(h_pdev, h_vdev);
		if (ret != RMI_SUCCESS) {
			INFO("rmi_pdev_communicate failed\n");
			rc = -1;
			break;
		}

		/*
		 * If cache is set, then response buffer has the device object
		 * to be cached.
		 */
		if (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_CACHE_RSP,
			    dcomm_exit->flags)) {
			rc = host_dev_cache_dev_object(h_pdev, h_vdev,
						       dcomm_enter, dcomm_exit);
			if (rc != 0) {
				INFO("host_dev_cache_dev_object failed\n");
				rc = -1;
				break;
			}
		}

		/* Send request to PDEV's DOE and get response */
		if (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_SEND, dcomm_exit->flags)) {
			rc = host_pdev_doe_communicate(h_pdev, dcomm_enter,
						       dcomm_exit);
			if (rc != 0) {
				INFO("host_pdev_doe_communicate failed\n");
				rc = -1;
				break;
			}
		}

		rc = host_dev_get_state(h_pdev, h_vdev, &state);
		if (rc != 0) {
			rc = -1;
			break;
		}
	} while ((state != target_state) && (state != error_state));

	return rc;
}

/*
 * Invoke RMI handler to transition PDEV state to 'to_state'
 */
static int host_pdev_transition(struct host_pdev *h_pdev,
				    unsigned char to_state)
{
	int rc;

	switch (to_state) {
	case RMI_PDEV_STATE_NEW:
		rc = host_pdev_create(h_pdev);
		break;
	case RMI_PDEV_STATE_NEEDS_KEY:
		/* Reset cached cert_chain */
		h_pdev->cert_chain_len = 0UL;
		rc = host_dev_communicate(h_pdev, NULL,
					  RMI_PDEV_STATE_NEEDS_KEY);
		break;
	case RMI_PDEV_STATE_HAS_KEY:
		rc = host_pdev_set_pubkey(h_pdev);
		break;
	case RMI_PDEV_STATE_READY:
		rc = host_dev_communicate(h_pdev, NULL, RMI_PDEV_STATE_READY);
		break;
	case RMI_PDEV_STATE_STOPPING:
		rc = host_pdev_stop(h_pdev);
		break;
	case RMI_PDEV_STATE_STOPPED:
		rc = host_dev_communicate(h_pdev, NULL, RMI_PDEV_STATE_STOPPED);
		break;
	default:
		rc = -1;
	}

	if (rc != 0) {
		INFO("RMI command failed\n");
		return -1;
	}

	if (!is_host_pdev_state(h_pdev, to_state)) {
		ERROR("PDEV state not [%s]\n", pdev_state_str[to_state]);
		return -1;
	}

	return 0;
}

/*
 * Allocate granules needed for a PDEV object like device communication data,
 * response buffer, PDEV AUX granules and memory required to store cert_chain
 */
static int host_pdev_setup(struct host_pdev *h_pdev)
{
	u_register_t ret, count;
	int i;

	memset(h_pdev, 0, sizeof(struct host_pdev));

	/* Allocate granule for PDEV and delegate */
	h_pdev->pdev = page_alloc(PAGE_SIZE);
	memset(h_pdev->pdev, 0, GRANULE_SIZE);
	ret = host_rmi_granule_delegate((u_register_t)h_pdev->pdev);
	if (ret != RMI_SUCCESS) {
		ERROR("PDEV delegate failed 0x%lx\n", ret);
		return -1;
	}

	/*
	 * Off chip PCIe device - set flags as non coherent device protected by
	 * end to end IDE, with SPDM.
	 */
	h_pdev->pdev_flags = (INPLACE(RMI_PDEV_FLAGS_SPDM, RMI_PDEV_SPDM_TRUE) |
			   INPLACE(RMI_PDEV_FLAGS_IDE, RMI_PDEV_IDE_TRUE) |
			   INPLACE(RMI_PDEV_FLAGS_COHERENT,
				   RMI_PDEV_COHERENT_FALSE));

	/* Get num of aux granules required for this PDEV */
	ret = host_rmi_pdev_aux_count(h_pdev->pdev_flags, &count);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_pdev_aux_count() failed 0x%lx\n", ret);
		return -1;
	}
	h_pdev->pdev_aux_num = count;

	/* Allocate aux granules for PDEV and delegate */
	INFO("PDEV create requires %u aux pages\n", h_pdev->pdev_aux_num);
	for (i = 0; i < h_pdev->pdev_aux_num; i++) {
		h_pdev->pdev_aux[i] = page_alloc(PAGE_SIZE);
		ret = host_rmi_granule_delegate((u_register_t)h_pdev->pdev_aux[i]);
		if (ret != RMI_SUCCESS) {
			ERROR("Aux granule delegate failed 0x%lx\n", ret);
			return -1;
		}
	}

	/* Allocate dev_comm_data and send/recv buffer for Dev communication */
	h_pdev->dev_comm_data = (struct rmi_dev_comm_data *)page_alloc(PAGE_SIZE);
	memset(h_pdev->dev_comm_data, 0, sizeof(struct rmi_dev_comm_data));
	h_pdev->dev_comm_data->enter.req_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	h_pdev->dev_comm_data->enter.resp_addr = (unsigned long)
		page_alloc(PAGE_SIZE);

	/* Allocate buffer to cache device certificate */
	h_pdev->cert_slot_id = 0;
	h_pdev->cert_chain = (uint8_t *)page_alloc(HOST_PDEV_CERT_LEN_MAX);
	h_pdev->cert_chain_len = 0;
	if (h_pdev->cert_chain == NULL) {
		return -1;
	}

	/* Allocate buffer to store extracted public key */
	h_pdev->public_key = (void *)page_alloc(PAGE_SIZE);
	if (h_pdev->public_key == NULL) {
		return -1;
	}
	h_pdev->public_key_len = PAGE_SIZE;

	/* Allocate buffer to store public key metadata */
	h_pdev->public_key_metadata = (void *)page_alloc(PAGE_SIZE);
	if (h_pdev->public_key_metadata == NULL) {
		return -1;
	}
	h_pdev->public_key_metadata_len = PAGE_SIZE;

	/* Set algorithm to use for device digests */
	h_pdev->pdev_hash_algo = RMI_HASH_SHA_512;

	return 0;
}

/*
 * Stop PDEV and ternimate secure session and call PDEV destroy
 */
static int host_pdev_reclaim(struct host_pdev *h_pdev)
{
	int rc;

	/* Move the device to STOPPING state */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_STOPPING);
	if (rc != 0) {
		INFO("PDEV transition: to PDEV_STATE_STOPPING failed\n");
		return -1;
	}

	/* Do pdev_communicate to terminate secure session */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_STOPPED);
	if (rc != 0) {
		INFO("PDEV transition: to PDEV_STATE_STOPPED failed\n");
		return -1;
	}

	rc = host_pdev_destroy(h_pdev);
	if (rc != 0) {
		INFO("PDEV transition: to STATE_NULL failed\n");
		return -1;
	}

	/* Undelegate all the delegated pages */
	for (int i = 0; i < h_pdev->pdev_aux_num; i++) {
		host_rmi_granule_undelegate((u_register_t)h_pdev->pdev_aux[i]);
	}
	host_rmi_granule_undelegate((u_register_t)h_pdev->pdev);

	return rc;
}

static int host_create_realm_with_feat_da(struct realm *realm)
{
	u_register_t feature_flag = 0UL;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};

	if (is_feat_52b_on_4k_2_supported() == true) {
		feature_flag = RMI_FEATURE_REGISTER_0_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	feature_flag |= RMI_FEATURE_REGISTER_0_DA_EN;

	/* Initialise Realm payload */
	if (!host_create_activate_realm_payload(realm,
						(u_register_t)REALM_IMAGE_BASE,
						feature_flag, 0UL, sl,
						rec_flag, 1U, 0U)) {
		return -1;
	}

	return 0;
}

/*
 * Allocate granules needed for a VDEV object like device communication data,
 * response buffer, VDEV AUX granules and memory required to device
 * measurements, interface report.
 */
static int host_vdev_setup(struct host_pdev *h_pdev, struct host_vdev *h_vdev)
{
	u_register_t ret;

	memset(h_vdev, 0, sizeof(struct host_vdev));

	/*
	 * Currently assigning one device is supported, for more than one device
	 * the VMM view of vdev_id and Realm view of device_id must match.
	 */
	h_vdev->vdev_id = 0UL;
	h_vdev->tdi_id = h_pdev->bdf;
	h_vdev->flags = 0UL;
	h_vdev->pdev_ptr = h_pdev->pdev;

	/* Allocate granule for VDEV and delegate */
	h_vdev->vdev_ptr = (void *)page_alloc(PAGE_SIZE);
	memset(h_vdev->vdev_ptr, 0, GRANULE_SIZE);
	ret = host_rmi_granule_delegate((u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV delegate failed 0x%lx\n", ret);
		return -1;
	}

	/* Allocate dev_comm_data and send/recv buffer for Dev communication */
	h_vdev->dev_comm_data = (struct rmi_dev_comm_data *)page_alloc(PAGE_SIZE);
	memset(h_vdev->dev_comm_data, 0, sizeof(struct rmi_dev_comm_data));
	h_vdev->dev_comm_data->enter.req_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	h_vdev->dev_comm_data->enter.resp_addr = (unsigned long)
		page_alloc(PAGE_SIZE);

	/* Allocate buffer to cache device measurements */
	h_vdev->meas = (uint8_t *)page_alloc(HOST_VDEV_MEAS_LEN_MAX);
	h_vdev->meas_len = 0;
	if (h_vdev->meas == NULL) {
		return -1;
	}

	/* Allocate buffer to cache device interface report */
	h_vdev->ifc_report = (uint8_t *)page_alloc(HOST_VDEV_IFC_REPORT_LEN_MAX);
	h_vdev->ifc_report_len = 0;
	if (h_vdev->ifc_report == NULL) {
		return -1;
	}

	return 0;
}

static int host_assign_vdev_to_realm(struct realm *realm,
				     struct host_pdev *h_pdev,
				     struct host_vdev *h_vdev)
{
	struct rmi_vdev_params *vdev_params;
	int rc;

	rc = host_vdev_setup(h_pdev, h_vdev);

	/* Create vdev and bind it to the Realm */
	vdev_params = (struct rmi_vdev_params *)page_alloc(PAGE_SIZE);
	memset(vdev_params, 0, PAGE_SIZE);

	/*
	 * Currently assigning one device is supported, for more than one device
	 * the VMM view of vdev_id and Realm view of device_id must match.
	 */
	vdev_params->vdev_id = 0UL;

	/*
	 * This is TDI id, this must be same as PDEV ID for assigning the whole
	 * device.
	 */
	vdev_params->tdi_id = h_pdev->bdf;

	vdev_params->flags = h_vdev->flags;
	vdev_params->num_aux = 0UL;

	rc = host_rmi_vdev_create(realm->rd, (u_register_t)h_pdev->pdev,
				  (u_register_t)h_vdev->vdev_ptr,
				  (u_register_t)vdev_params);
	if (rc != RMI_SUCCESS) {
		INFO("VDEV create failed\n");
		return -1;
	}

	return rc;
}

static int host_unassign_vdev_from_realm(struct realm *realm,
					 struct host_pdev *h_pdev,
					 struct host_vdev *h_vdev)
{
	int rc;
	u_register_t state;

	rc = host_rmi_vdev_stop((u_register_t)h_vdev->vdev_ptr);
	if (rc != RMI_SUCCESS) {
		INFO("VDEV stop failed\n");
		return -1;
	}

	rc = host_vdev_get_state(h_vdev, &state);
	if (rc != RMI_SUCCESS) {
		INFO("VDEV get_state failed\n");
		return -1;
	}

	if (state != RMI_VDEV_STATE_STOPPING) {
		INFO("VDEV not in STOPPING state\n");
		return -1;
	}

	/* Do VDEV communicate to move VDEV from STOPPING to STOPPED state */
	rc = host_dev_communicate(h_pdev, h_vdev, RMI_VDEV_STATE_STOPPED);
	if (rc != 0) {
		INFO("VDEV STOPPING -> STOPPED failed\n");
		return -1;
	}

	rc = host_rmi_vdev_destroy(realm->rd, (u_register_t)h_pdev->pdev,
				   (u_register_t)h_vdev->vdev_ptr);
	if (rc != RMI_SUCCESS) {
		INFO("VDEV destroy failed\n");
		return -1;
	}

	return rc;
}

void host_do_vdev_complete(u_register_t rec_ptr, unsigned long vdev_id)
{
	struct host_vdev *h_vdev;
	u_register_t rmi_rc;

	h_vdev = find_host_vdev_from_id(vdev_id);
	if (h_vdev == NULL) {
		return;
	}

	/* Complete the VDEV request */
	rmi_rc = host_rmi_vdev_complete(rec_ptr, (u_register_t)h_vdev->vdev_ptr);
	if (rmi_rc != RMI_SUCCESS) {
		INFO("Handling VDEV request failed\n");
	}
}

void host_do_vdev_communicate(u_register_t vdev_ptr, unsigned long vdev_action)
{
	struct host_vdev *h_vdev;
	struct host_pdev *h_pdev;
	int rc;

	h_vdev = find_host_vdev_from_vdev_ptr(vdev_ptr);
	if (h_vdev == NULL) {
		return;
	}

	/* Reset cached measurement/interface report */
	if (vdev_action == RMI_VDEV_ACTION_GET_MEASUREMENTS) {
		h_vdev->meas_len = 0UL;
	} else if (vdev_action == RMI_DEV_COMM_OBJECT_INTERFACE_REPORT) {
		h_vdev->ifc_report_len = 0UL;
	}

	h_pdev = find_host_pdev_from_vdev_ptr(vdev_ptr);
	if (h_pdev == NULL) {
		return;
	}

	rc = host_dev_communicate(h_pdev, h_vdev, RMI_VDEV_STATE_READY);
	if (rc != 0) {
		INFO("Handling VDEV communicate failed\n");
		return;
	}
}

/*
 * This invokes various RMI calls related to PDEV, VDEV management that does
 * PDEV create/communicate/set_key/abort/stop/destroy and assigns the device
 * to a Realm using RMI VDEV ABIs
 *
 * 1. Create a Realm with DA feature enabled
 * 2. Find a known PCIe endpoint and connect with TSM to get_cert and establish
 *    secure session
 * 3. Assign the PCIe endpoint (a PF) to the Realm
 * 4. Call Realm to do DA related RSI calls
 * 5. Unassign the PCIe endpoint from the Realm
 * 6. Delete the Realm
 * 7. Reclaim the PCIe TDI from TSM
 */
test_result_t host_invoke_rmi_da_flow(void)
{
	u_register_t rmi_feat_reg0;
	uint32_t pdev_bdf, doe_cap_base;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;
	uint8_t public_key_algo;
	int ret, rc;
	bool realm_rc;
	struct realm realm;

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);
	SKIP_TEST_IF_DOE_NOT_SUPPORTED(pdev_bdf, doe_cap_base);

	INFO("DA on bdf: 0x%x, doe_cap_base: 0x%x\n", pdev_bdf, doe_cap_base);

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	/*
	 * 2. Create a Realm with DA feature enabled
	 *
	 * todo: creating this after host_pdev_setup cases Realm create to
	 * fail.
	 */
	rc = host_create_realm_with_feat_da(&realm);
	if (rc != 0) {
		INFO("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	INFO("Realm created with feat_da enabled\n");

	h_pdev = &gbl_host_pdev;
	h_vdev = &gbl_host_vdev;

	/* Allocate granules. Skip DA ABIs if host_pdev_setup fails */
	rc = host_pdev_setup(h_pdev);
	if (rc == -1) {
		INFO("host_pdev_setup failed. skipping DA ABIs...\n");
		return TEST_RESULT_SKIPPED;
	}

	/* todo: move to tdi_pdev_setup */
	h_pdev->bdf = pdev_bdf;
	h_pdev->doe_cap_base = doe_cap_base;

	/* Call rmi_pdev_create to transition PDEV to STATE_NEW */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEW);
	if (rc != 0) {
		ERROR("PDEV transition: NULL -> STATE_NEW failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Call rmi_pdev_communicate to transition PDEV to NEEDS_KEY */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEEDS_KEY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_NEW -> PDEV_NEEDS_KEY failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Get public key. Verifying cert_chain not done by host but by Realm? */
	rc = host_get_public_key_from_cert_chain(h_pdev->cert_chain,
						 h_pdev->cert_chain_len,
						 h_pdev->public_key,
						 &h_pdev->public_key_len,
						 h_pdev->public_key_metadata,
						 &h_pdev->public_key_metadata_len,
						 &public_key_algo);
	if (rc != 0) {
		ERROR("Get public key failed\n");
		return TEST_RESULT_FAIL;
	}

	if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P256;
	} else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P384;
	} else {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_RSASSA_3072;
	}
	INFO("DEV public key len/sig_algo: %ld/%d\n", h_pdev->public_key_len,
	     h_pdev->public_key_sig_algo);

	/* Call rmi_pdev_set_key transition PDEV to HAS_KEY */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_HAS_KEY);
	if (rc != 0) {
		INFO("PDEV transition: PDEV_NEEDS_KEY -> PDEV_HAS_KEY failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Call rmi_pdev_comminucate to transition PDEV to READY state */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_READY);
	if (rc != 0) {
		INFO("PDEV transition: PDEV_HAS_KEY -> PDEV_READY failed\n");
		return TEST_RESULT_FAIL;
	}


	/*
	 * 3 Assign VDEV (the PCIe endpoint) from the Realm
	 */
	rc = host_assign_vdev_to_realm(&realm, h_pdev, h_vdev);
	if (rc != 0) {
		INFO("VDEV assign to realm failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 4 Call Realm to do DA related RSI calls
	 */
	realm_rc = host_enter_realm_execute(&realm, REALM_DA_RSI_CALLS,
					    RMI_EXIT_HOST_CALL, 0U);
	if (!realm_rc) {
		INFO("Realm DA_RSI_CALLS failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 5 Unassign VDEV (the PCIe endpoint) from the Realm
	 */
	rc = host_unassign_vdev_from_realm(&realm, h_pdev, h_vdev);
	if (rc != 0) {
		INFO("VDEV unassign to realm failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 6 Destroy the Realm
	 */
	if (!host_destroy_realm(&realm)) {
		INFO("Realm destroy failed\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * 7 Reclaim PDEV (the PCIe TDI) from TSM
	 */
	rc = host_pdev_reclaim(h_pdev);
	if (rc != 0) {
		INFO("Reclaim PDEV from TSM failed\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * This function invokes PDEV_CREATE on TRP and tries to test
 * the EL3 RMM-EL3 IDE KM interface. Will be skipped on TF-RMM.
 */
test_result_t host_realm_test_root_port_key_management(void)
{
	struct host_pdev *h_pdev;
	int ret;

	if (host_rmi_version(RMI_ABI_VERSION_VAL) != 0U) {
		tftf_testcase_printf("RMM is not TRP\n");
		return TEST_RESULT_SKIPPED;
	}

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	h_pdev = &gbl_host_pdev;

	/*
	 * Call rmi_pdev_create with invalid pdev, expect an error
	 * to be returned from TRP.
	 */
	ret = host_pdev_create(h_pdev);
	if (ret != 0) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI_PDEV_CREATE did not return error as expected\n");
	return TEST_RESULT_FAIL;
}
