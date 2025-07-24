/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include <arch_features.h>
#include <debug.h>
#include <heap/page_alloc.h>
#include <host_crypto_utils.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <pcie_spec.h>
#include <pcie_doe.h>

unsigned int gbl_host_pdev_count;
struct host_pdev gbl_host_pdevs[HOST_PDEV_MAX];
struct host_vdev gbl_host_vdev;

static const char * const pdev_state_str[] = {
	"PDEV_STATE_NEW",
	"PDEV_STATE_NEEDS_KEY",
	"PDEV_STATE_HAS_KEY",
	"PDEV_STATE_READY",
	"PDEV_STATE_IDE_RESETTING",
	"PDEV_STATE_COMMUNICATING",
	"PDEV_STATE_STOPPING",
	"PDEV_STATE_STOPPED",
	"RMI_PDEV_STATE_ERROR"
};

static const char * const vdev_state_str[] = {
	"RMI_VDEV_NEW",
	"RMI_VDEV_UNLOCKED",
	"RMI_VDEV_LOCKED",
	"RMI_VDEV_STARTED",
	"RMI_VDEV_ERROR"
};

static struct host_vdev *find_host_vdev_from_id(unsigned long vdev_id)
{
	struct host_vdev *h_vdev = &gbl_host_vdev;

	if (h_vdev->vdev_id == vdev_id) {
		return h_vdev;
	}

	return NULL;
}

static struct host_pdev *find_host_pdev_from_pdev_ptr(unsigned long pdev_ptr)
{
	unsigned int i;
	struct host_pdev *h_pdev;

	for (i = 0U; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->pdev == (void *)pdev_ptr) {
			return h_pdev;
		}
	}

	return NULL;
}

static struct host_vdev *find_host_vdev_from_vdev_ptr(unsigned long vdev_ptr)
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

static bool is_host_vdev_state(struct host_vdev *h_vdev, u_register_t exp_state)
{
	u_register_t cur_state;

	if (host_vdev_get_state(h_vdev, &cur_state) != 0) {
		return false;
	}

	return cur_state == exp_state;
}

/*
 * Allocate granules needed for a PDEV object like device communication data,
 * response buffer, PDEV AUX granules and memory required to store cert_chain
 */
int host_pdev_setup(struct host_pdev *h_pdev)
{
	u_register_t ret, count;
	int i;

	/* RCiEP devices not supported by RMM */
	if (h_pdev->dev->dp_type == RCiEP) {
		return -1;
	}

	/* Allocate granule for PDEV and delegate */
	h_pdev->pdev = page_alloc(PAGE_SIZE);
	if (h_pdev->pdev == NULL) {
		return -1;
	}

	memset(h_pdev->pdev, 0, GRANULE_SIZE);
	ret = host_rmi_granule_delegate((u_register_t)h_pdev->pdev);
	if (ret != RMI_SUCCESS) {
		ERROR("PDEV delegate failed 0x%lx\n", ret);
		goto err_undelegate_pdev;
	}

	/*
	 * Off chip PCIe device - set flags as non coherent device protected by
	 * end to end IDE, with SPDM.
	 */
	h_pdev->pdev_flags = 0;

	/* Set IDE based on device capability */
	if (pcie_dev_has_ide(h_pdev->dev)) {
		h_pdev->pdev_flags |= INPLACE(RMI_PDEV_FLAGS_NCOH_IDE,
					      RMI_PDEV_IDE_TRUE);
	}

	/* Supports SPDM */
	h_pdev->pdev_flags |= INPLACE(RMI_PDEV_FLAGS_SPDM, RMI_PDEV_SPDM_TRUE);

	/* Get num of aux granules required for this PDEV */
	ret = host_rmi_pdev_aux_count(h_pdev->pdev_flags, &count);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_pdev_aux_count() failed 0x%lx\n", ret);
		goto err_undelegate_pdev;
	}
	h_pdev->pdev_aux_num = count;

	/* Allocate aux granules for PDEV and delegate */
	INFO("PDEV create requires %u aux pages\n", h_pdev->pdev_aux_num);
	for (i = 0; i < h_pdev->pdev_aux_num; i++) {
		void *pdev_aux = page_alloc(PAGE_SIZE);

		if (pdev_aux == NULL) {
			ERROR("page_alloc for PDEV aux granule failed");
			goto err_undelegate_pdev_aux;
		}

		ret = host_rmi_granule_delegate((u_register_t)pdev_aux);
		if (ret != RMI_SUCCESS) {
			ERROR("PDEV aux granule delegate failed 0x%lx\n", ret);
			goto err_undelegate_pdev_aux;
		}

		h_pdev->pdev_aux[i] = pdev_aux;
	}

	/* Allocate dev_comm_data and send/recv buffer for Dev communication */
	h_pdev->dev_comm_data = (struct rmi_dev_comm_data *)page_alloc(PAGE_SIZE);
	if (h_pdev->dev_comm_data == NULL) {
		goto err_undelegate_pdev_aux;
	}

	memset(h_pdev->dev_comm_data, 0, sizeof(struct rmi_dev_comm_data));

	h_pdev->dev_comm_data->enter.req_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (h_pdev->dev_comm_data->enter.req_addr == 0UL) {
		goto err_undelegate_pdev_aux;
	}

	h_pdev->dev_comm_data->enter.resp_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (h_pdev->dev_comm_data->enter.resp_addr == 0UL) {
		goto err_undelegate_pdev_aux;
	}

	/* Allocate buffer to cache device certificate */
	h_pdev->cert_slot_id = 0;
	h_pdev->cert_chain = (uint8_t *)page_alloc(HOST_PDEV_CERT_LEN_MAX);
	h_pdev->cert_chain_len = 0;
	if (h_pdev->cert_chain == NULL) {
		goto err_undelegate_pdev_aux;
	}

	/* Allocate buffer to store extracted public key */
	h_pdev->public_key = (void *)page_alloc(PAGE_SIZE);
	if (h_pdev->public_key == NULL) {
		goto err_undelegate_pdev_aux;
	}
	h_pdev->public_key_len = PAGE_SIZE;

	/* Allocate buffer to store public key metadata */
	h_pdev->public_key_metadata = (void *)page_alloc(PAGE_SIZE);
	if (h_pdev->public_key_metadata == NULL) {
		goto err_undelegate_pdev_aux;
	}

	h_pdev->public_key_metadata_len = PAGE_SIZE;

	/* Allocate buffer to store VCA */
	h_pdev->vca = (uint8_t *)page_alloc(HOST_PDEV_VCA_LEN_MAX);
	h_pdev->vca_len = 0;
	if (h_pdev->vca == NULL) {
		goto err_undelegate_pdev_aux;
	}

	/* Set algorithm to use for device digests */
	h_pdev->pdev_hash_algo = RMI_HASH_SHA_512;

	return 0;

err_undelegate_pdev_aux:
	/* Undelegate all the delegated pages */
	for (int i = 0; i < h_pdev->pdev_aux_num; i++) {
		if (h_pdev->pdev_aux[i]) {
			host_rmi_granule_undelegate((u_register_t)
						    h_pdev->pdev_aux[i]);
		}
	}

err_undelegate_pdev:
	host_rmi_granule_undelegate((u_register_t)h_pdev->pdev);

	return -1;
}

int host_pdev_create(struct host_pdev *h_pdev)
{
	struct rmi_pdev_params *pdev_params;
	u_register_t ret;
	unsigned int i;
	int rc;

	/* Allocate granules and memory for PDEV objects like certificate, key */
	rc = host_pdev_setup(h_pdev);
	if (rc == -1) {
		ERROR("host_pdev_setup failed.\n");
		return -1;
	}

	pdev_params = (struct rmi_pdev_params *)page_alloc(PAGE_SIZE);
	memset(pdev_params, 0, GRANULE_SIZE);

	pdev_params->flags = h_pdev->pdev_flags;
	pdev_params->cert_id = h_pdev->cert_slot_id;
	pdev_params->pdev_id = h_pdev->dev->bdf;
	pdev_params->num_aux = h_pdev->pdev_aux_num;
	pdev_params->hash_algo = h_pdev->pdev_hash_algo;
	pdev_params->root_id = h_pdev->dev->rp_dev->bdf;
	pdev_params->ecam_addr = h_pdev->dev->ecam_base;
	for (i = 0U; i < h_pdev->pdev_aux_num; i++) {
		pdev_params->aux[i] = (uintptr_t)h_pdev->pdev_aux[i];
	}

	ret = host_rmi_pdev_create((u_register_t)h_pdev->pdev,
				   (u_register_t)pdev_params);
	page_free((u_register_t)pdev_params);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_pdev_set_pubkey(struct host_pdev *h_pdev)
{
	struct rmi_public_key_params *pubkey_params;
	uint8_t public_key_algo;
	u_register_t ret;
	int rc;

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
		return -1;
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

/* Call RMI_PDEV_DESTROY and free all pdev related allocations */
static int host_pdev_destroy(struct host_pdev *h_pdev)
{
	u_register_t ret;
	int rc = 0;

	ret = host_rmi_pdev_destroy((u_register_t)h_pdev->pdev);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	/* Undelegate all aux granules */
	for (int i = 0; i < h_pdev->pdev_aux_num; i++) {
		ret = host_rmi_granule_undelegate((u_register_t)h_pdev->pdev_aux[i]);
		if (ret != RMI_SUCCESS) {
			ERROR("Aux granule undelegate failed 0x%lx\n", ret);
			rc = -1;
		}

		h_pdev->pdev_aux[i] = NULL;
	}

	/* Undelegate PDEV granule */
	ret = host_rmi_granule_undelegate((u_register_t)h_pdev->pdev);
	h_pdev->pdev = NULL;
	if (ret != RMI_SUCCESS) {
		ERROR("PDEV undelegate failed 0x%lx\n", ret);
		rc = -1;
	}

	page_free((u_register_t)h_pdev->dev_comm_data->enter.req_addr);
	page_free((u_register_t)h_pdev->dev_comm_data->enter.resp_addr);

	page_free((u_register_t)h_pdev->dev_comm_data);
	page_free((u_register_t)h_pdev->cert_chain);
	page_free((u_register_t)h_pdev->public_key);
	page_free((u_register_t)h_pdev->public_key_metadata);
	page_free((u_register_t)h_pdev->vca);

	h_pdev->dev_comm_data = NULL;
	h_pdev->cert_chain = NULL;
	h_pdev->public_key = NULL;
	h_pdev->public_key_metadata = NULL;
	h_pdev->vca = NULL;
	h_pdev->cert_chain_len = 0;
	h_pdev->vca_len = 0;
	h_pdev->public_key_len = 0;
	h_pdev->public_key_metadata_len = 0;

	return rc;
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

static u_register_t host_rmi_dev_communicate(struct realm *realm,
					     struct host_pdev *h_pdev,
					     struct host_vdev *h_vdev)
{
	if (h_vdev) {
		assert(realm != NULL);
		return host_rmi_vdev_communicate((u_register_t)realm->rd,
						 (u_register_t)h_vdev->pdev_ptr,
						 (u_register_t)h_vdev->vdev_ptr,
						 (u_register_t)
						 h_vdev->dev_comm_data);
	}

	return host_rmi_pdev_communicate((u_register_t)h_pdev->pdev,
					 (u_register_t)h_pdev->dev_comm_data);
}

static int host_pdev_cache_device_object(struct host_pdev *h_pdev,
					 uint8_t dev_obj_id,
					 const uint8_t *dev_obj_buf,
					 unsigned long dev_obj_buf_len)
{
	int rc = -1;

	/*
	 * During PDEV communicate device object of type certificate or VCA is
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
	} else if (dev_obj_id == RMI_DEV_COMM_OBJECT_VCA) {
		if ((h_pdev->vca_len + dev_obj_buf_len) >
		    HOST_PDEV_VCA_LEN_MAX) {
			return -1;
		}

		INFO("%s: vca: offset: 0x%lx, len: 0x%lx\n",
		     __func__, h_pdev->vca_len, dev_obj_buf_len);

		memcpy((void *)(h_pdev->vca + h_pdev->vca_len),
		       dev_obj_buf, dev_obj_buf_len);
		h_pdev->vca_len += dev_obj_buf_len;
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
				     uint8_t *dev_obj_buf,
				     unsigned char cache_obj_id,
				     size_t cache_offset, size_t cache_len)
{
	int rc;

	if ((cache_len != 0) &&
	    ((cache_offset + cache_len) > GRANULE_SIZE)) {
		ERROR("Invalid cache offset/length\n");
		return -1;
	}

	dev_obj_buf = dev_obj_buf + cache_offset;

	if (h_vdev) {
		rc = host_vdev_cache_device_object(h_vdev,
						   cache_obj_id,
						   dev_obj_buf,
						   cache_len);
	} else {
		rc = host_pdev_cache_device_object(h_pdev,
						   cache_obj_id,
						   dev_obj_buf,
						   cache_len);
	}

	if (rc != 0) {
		ERROR("host_dev_cache_device_object failed\n");
	}

	return rc;
}

static int host_dev_cache_dev_req_resp(struct host_pdev *h_pdev,
				       struct host_vdev *h_vdev,
				       struct rmi_dev_comm_enter *dcomm_enter,
				       struct rmi_dev_comm_exit *dcomm_exit)
{
	int rc;

	if ((dcomm_exit->cache_req_len == 0) &&
	    (dcomm_exit->cache_rsp_len == 0)) {
		ERROR("Both cache_req_len and cache_rsp_len are 0\n");
		return -1;
	}

	rc = host_dev_cache_dev_object(h_pdev, h_vdev,
				       (uint8_t *)dcomm_enter->req_addr,
				       dcomm_exit->cache_obj_id,
				       dcomm_exit->cache_req_offset,
				       dcomm_exit->cache_req_len);

	if (rc != 0) {
		ERROR("host_dev_cache_device_object req failed\n");
		return -1;
	}

	rc = host_dev_cache_dev_object(h_pdev, h_vdev,
				       (uint8_t *)dcomm_enter->resp_addr,
				       dcomm_exit->cache_obj_id,
				       dcomm_exit->cache_rsp_offset,
				       dcomm_exit->cache_rsp_len);

	if (rc != 0) {
		ERROR("host_dev_cache_device_object rsp failed\n");
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
		ERROR("Invalid dev_comm_exit.protocol\n");
		return -1;
	}

	resp_len = 0UL;
	rc = pcie_doe_communicate(doe_header, h_pdev->dev->bdf,
				  h_pdev->dev->doe_cap_base,
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
static int host_dev_communicate(struct realm *realm,
				struct host_pdev *h_pdev,
				struct host_vdev *h_vdev,
				unsigned char target_state)
{
	int rc;
	u_register_t state;
	u_register_t error_state;
	u_register_t ret;
	struct rmi_dev_comm_enter *dcomm_enter;
	struct rmi_dev_comm_exit *dcomm_exit;
	bool stop;

	if (h_vdev) {
		if (h_vdev->dev_comm_data == NULL) {
			return -1;
		}

		dcomm_enter = &h_vdev->dev_comm_data->enter;
		dcomm_exit = &h_vdev->dev_comm_data->exit;

		error_state = RMI_VDEV_STATE_ERROR;
	} else {
		if (h_pdev->dev_comm_data == NULL) {
			return -1;
		}

		dcomm_enter = &h_pdev->dev_comm_data->enter;
		dcomm_exit = &h_pdev->dev_comm_data->exit;

		error_state = RMI_PDEV_STATE_ERROR;
	}

	dcomm_enter->status = RMI_DEV_COMM_ENTER_STATUS_NONE;
	dcomm_enter->resp_len = 0;

	rc = host_dev_get_state(h_pdev, h_vdev, &state);
	if (rc != 0) {
		return rc;
	}

	stop = false;
	do {
		ret = host_rmi_dev_communicate(realm, h_pdev, h_vdev);
		if (ret != RMI_SUCCESS) {
			ERROR("host_rmi_dev_communicate failed\n");
			rc = -1;
			break;
		}

		/*
		 * If cache is set, then the corresponding buffer(s) has the
		 * device object to be cached.
		 */
		if ((EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_CACHE_REQ,
			     dcomm_exit->flags) != 0U) ||
		    (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_CACHE_RSP,
			     dcomm_exit->flags) != 0U)) {
			rc = host_dev_cache_dev_req_resp(h_pdev, h_vdev,
							 dcomm_enter,
							 dcomm_exit);
			if (rc != 0) {
				ERROR("host_dev_cache_dev_object failed\n");
				break;
			}
		}

		/* Send request to PDEV's DOE and get response */
		if (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_SEND, dcomm_exit->flags)) {
			rc = host_pdev_doe_communicate(h_pdev, dcomm_enter,
						       dcomm_exit);
			if (rc != 0) {
				ERROR("host_pdev_doe_communicate failed\n");
				break;
			}
		} else {
			dcomm_enter->status = RMI_DEV_COMM_ENTER_STATUS_NONE;
		}

		rc = host_dev_get_state(h_pdev, h_vdev, &state);
		if (rc != 0) {
			break;
		}
		if (state == target_state) {
			/*
			 * The target state was reached, but for some
			 * transitions this is not enough, need to continue
			 * calling it till certain flags are cleared in the
			 * exit. wait for that to happen.
			 */
			stop = dcomm_exit->flags == 0U;
		} else if (state == error_state) {
			ERROR("Failed to reach target_state %lu instead of %u\n",
			      state, (unsigned int)target_state);
			rc = -1;
			stop = true;
		} else {
			stop = false;
		}
	} while (!stop);

	return rc;
}

/*
 * Invoke RMI handler to transition PDEV state to 'to_state'
 */
int host_pdev_transition(struct host_pdev *h_pdev, unsigned char to_state)
{
	int rc;

	switch (to_state) {
	case RMI_PDEV_STATE_NEW:
		rc = host_pdev_create(h_pdev);
		break;
	case RMI_PDEV_STATE_NEEDS_KEY:
		/* Reset cached cert_chain */
		h_pdev->cert_chain_len = 0UL;
		h_pdev->vca_len = 0UL;
		rc = host_dev_communicate(NULL, h_pdev, NULL,
					  RMI_PDEV_STATE_NEEDS_KEY);
		break;
	case RMI_PDEV_STATE_HAS_KEY:
		rc = host_pdev_set_pubkey(h_pdev);
		break;
	case RMI_PDEV_STATE_READY:
		rc = host_dev_communicate(NULL, h_pdev, NULL, RMI_PDEV_STATE_READY);
		break;
	case RMI_PDEV_STATE_STOPPING:
		rc = host_pdev_stop(h_pdev);
		break;
	case RMI_PDEV_STATE_STOPPED:
		rc = host_dev_communicate(NULL, h_pdev, NULL, RMI_PDEV_STATE_STOPPED);
		break;
	default:
		rc = -1;
		break;
	}

	if (rc != 0) {
		ERROR("pdev_state_transition: failed\n");
		return rc;
	}

	if (!is_host_pdev_state(h_pdev, to_state)) {
		ERROR("pdev_state_transition: PDEV state not [%s]\n",
		      pdev_state_str[to_state]);
		return -1;
	}

	/*
	 * Upon successful transition to STOPPED, call pdev_destroy and reclaim
	 * all the allocation done to PDEV.
	 */
	if (to_state == RMI_PDEV_STATE_STOPPED) {
		rc = host_pdev_destroy(h_pdev);
	}

	return rc;
}

int host_pdev_state_transition(struct host_pdev *h_pdev,
			       const unsigned char pdev_states[],
			       size_t pdev_states_max)
{
	unsigned int i;
	u_register_t state;
	int rc = 0;

	if ((h_pdev == NULL) || (pdev_states_max > PDEV_STATE_TRANSITION_MAX)) {
		return -1;
	}

	for (i = 0U; i < pdev_states_max; i++) {
		if (pdev_states[i] == (unsigned char)-1) {
			break;
		}

		if (pdev_states[i] > RMI_PDEV_STATE_STOPPED) {
			ERROR("Invalid PDEV state: %d\n", pdev_states[i]);
			rc = -1;
			break;
		}

		if (i == 0U) {
			INFO("PDEV transition to [%s]\n",
			     pdev_state_str[pdev_states[i]]);
		} else {
			INFO("PDEV transition from [%s] -> [%s]\n",
			     pdev_state_str[pdev_states[i - 1]],
			     pdev_state_str[pdev_states[i]]);
		}

		rc = host_pdev_transition(h_pdev, pdev_states[i]);
		if (rc != 0) {
			ERROR("PDEV transition: to [%s] failed\n",
			      pdev_state_str[pdev_states[i]]);
			rc = -1;
			break;
		}
	}

	/*
	 * On error transition the PDEV to STOPPED state and cleanup the
	 * resources held by PDEV.
	 */
	if ((rc != 0) && (host_pdev_get_state(h_pdev, &state) == 0)) {
		INFO("PDEV transition from [%s] -> [%s]\n",
		     pdev_state_str[state],
		     pdev_state_str[RMI_PDEV_STATE_STOPPING]);

		if (host_pdev_transition(h_pdev, RMI_PDEV_STATE_STOPPING) == 0) {
			INFO("PDEV transition from [%s] -> [%s]\n",
			     pdev_state_str[RMI_PDEV_STATE_STOPPING],
			     pdev_state_str[RMI_PDEV_STATE_STOPPED]);

			(void)host_pdev_transition(h_pdev,
						   RMI_PDEV_STATE_STOPPED);
		}
	}

	return rc;
}

int host_vdev_get_interface_report(struct realm *realm,
			       struct host_vdev *h_vdev,
			       unsigned char target_state)
{
	int rc;
	struct host_pdev *h_pdev;

	h_pdev = find_host_pdev_from_vdev_ptr((unsigned long)h_vdev->vdev_ptr);
	if (h_pdev == NULL) {
		ERROR("Failed to look up pdev for vdev\n");
		return -1;
	}

	/* Initialise the vdev measurements */
	h_vdev->ifc_report_len = 0;

	rc = host_rmi_vdev_get_interface_report(realm->rd, (u_register_t)h_pdev->pdev,
						(u_register_t)h_vdev->vdev_ptr);
	if (rc != 0) {
		return rc;
	}
	rc = host_dev_communicate(realm, h_pdev, h_vdev, target_state);

	return rc;
}

int host_vdev_map(struct realm *realm, struct host_vdev *h_vdev, u_register_t ipa,
		  u_register_t level, u_register_t addr)
{
	return host_rmi_vdev_map(realm->rd, (u_register_t)h_vdev->vdev_ptr,
			       ipa, level, addr);
}

int host_vdev_unlock(struct realm *realm,
		     struct host_vdev *h_vdev,
		     unsigned char target_state)
{
	int rc;
	struct host_pdev *h_pdev;

	h_pdev = find_host_pdev_from_vdev_ptr((unsigned long)h_vdev->vdev_ptr);
	if (h_pdev == NULL) {
		ERROR("Failed to look up pdev for vdev\n");
		return -1;
	}

	rc = host_rmi_vdev_unlock(realm->rd, (u_register_t)h_pdev->pdev,
				  (u_register_t)h_vdev->vdev_ptr);
	if (rc != 0) {
		return rc;
	}

	rc = host_vdev_transition(realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
	return rc;
}

int host_vdev_get_measurements(struct realm *realm,
			       struct host_vdev *h_vdev,
			       unsigned char target_state)
{
	int rc;
	struct host_pdev *h_pdev;
	struct rmi_vdev_measure_params *vdev_measure_params;

	h_pdev = find_host_pdev_from_vdev_ptr((unsigned long)h_vdev->vdev_ptr);
	if (h_pdev == NULL) {
		ERROR("Failed to look up pdev for vdev\n");
		return -1;
	}

	vdev_measure_params = (struct rmi_vdev_measure_params *)page_alloc(PAGE_SIZE);
	memset(vdev_measure_params, 0, GRANULE_SIZE);

	/* Initialise the vdev measurements */
	h_vdev->meas_len = 0;

	/*
	 * vdev_measure_params->flags is left as 0 as we don't want signed nor
	 * raw output.
	 */
	/*
	 * Set indices value  to (1 << 254) to retrieve all measurements
	 * supported by the device.
	 */
	vdev_measure_params->indices[31] = (unsigned char)1U << 6U;

	rc = host_rmi_vdev_get_measurements(realm->rd, (u_register_t)h_pdev->pdev,
					    (u_register_t)h_vdev->vdev_ptr,
					    (u_register_t)vdev_measure_params);
	if (rc != 0) {
		return rc;
	}
	return host_dev_communicate(realm, h_pdev, h_vdev, target_state);
}

/*
 * Invoke RMI handler to transition VDEV state to 'to_state'
 */
int host_vdev_transition(struct realm *realm,
			 struct host_vdev *h_vdev,
			 unsigned char to_state)
{
	int rc;
	struct host_pdev *h_pdev;

	h_pdev = find_host_pdev_from_vdev_ptr((unsigned long)h_vdev->vdev_ptr);
	if (h_pdev == NULL) {
		ERROR("Failed to look up pdev for vdev\n");
		return -1;
	}

	switch (to_state) {
	case RMI_VDEV_STATE_UNLOCKED:
		rc = host_dev_communicate(realm, h_pdev, h_vdev,
					  RMI_VDEV_STATE_UNLOCKED);
		break;
	case RMI_VDEV_STATE_LOCKED:
		rc = host_rmi_vdev_lock(realm->rd, (u_register_t)h_pdev->pdev,
					(u_register_t)h_vdev->vdev_ptr);
		if (rc != 0) {
			break;
		}
		rc = host_dev_communicate(realm, h_pdev, h_vdev,
					  RMI_VDEV_STATE_LOCKED);
		break;
	case RMI_VDEV_STATE_STARTED:
		rc = host_rmi_vdev_start(realm->rd, (u_register_t)h_pdev->pdev,
					 (u_register_t)h_vdev->vdev_ptr);
		if (rc != 0) {
			break;
		}
		rc = host_dev_communicate(realm, h_pdev, h_vdev,
					  RMI_VDEV_STATE_STARTED);
		break;

	default:
		ERROR("Unknown target state\n");
		rc = -1;
	}

	if (rc != 0) {
		ERROR("RMI command failed\n");
		return rc;
	}

	if (!is_host_vdev_state(h_vdev, to_state)) {
		ERROR("VDEV state not [%s]\n", vdev_state_str[to_state]);
		return -1;
	}

	return 0;
}

int host_create_realm_with_feat_da(struct realm *realm)
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
						rec_flag, 1U, 0U,
						get_test_mecid())) {
		return -1;
	}

	return 0;
}

/*
 * Allocate granules needed for a VDEV object like device communication data,
 * response buffer, VDEV AUX granules and memory required to device
 * measurements, interface report.
 */
static int host_vdev_setup(struct host_vdev *h_vdev, unsigned long tdi_id,
			   void *pdev_ptr)
{
	u_register_t ret;

	memset(h_vdev, 0, sizeof(struct host_vdev));

	/*
	 * Currently assigning one device is supported, for more than one device
	 * the VMM view of vdev_id and Realm view of device_id must match.
	 */
	h_vdev->vdev_id = 0UL;
	h_vdev->tdi_id = tdi_id;
	h_vdev->flags = 0UL;
	h_vdev->pdev_ptr = pdev_ptr;

	/* Allocate granule for VDEV and delegate */
	h_vdev->vdev_ptr = (void *)page_alloc(PAGE_SIZE);
	if (h_vdev->vdev_ptr == NULL) {
		return -1;
	}

	memset(h_vdev->vdev_ptr, 0, GRANULE_SIZE);
	ret = host_rmi_granule_delegate((u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV delegate failed 0x%lx\n", ret);
		return -1;
	}

	/* Allocate dev_comm_data and send/recv buffer for Dev communication */
	h_vdev->dev_comm_data = (struct rmi_dev_comm_data *)page_alloc(PAGE_SIZE);
	if (h_vdev->dev_comm_data == NULL) {
		goto err_undel_vdev;
	}
	memset(h_vdev->dev_comm_data, 0, sizeof(struct rmi_dev_comm_data));
	h_vdev->dev_comm_data->enter.req_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (h_vdev->dev_comm_data->enter.req_addr == 0UL) {
		goto err_undel_vdev;
	}

	h_vdev->dev_comm_data->enter.resp_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (h_vdev->dev_comm_data->enter.resp_addr == 0UL) {
		goto err_undel_vdev;
	}

	/* Allocate buffer to cache device measurements */
	h_vdev->meas = (uint8_t *)page_alloc(HOST_VDEV_MEAS_LEN_MAX);
	if (h_vdev->meas == NULL) {
		goto err_undel_vdev;
	}

	h_vdev->meas_len = 0;

	/* Allocate buffer to cache device interface report */
	h_vdev->ifc_report = (uint8_t *)page_alloc(HOST_VDEV_IFC_REPORT_LEN_MAX);
	h_vdev->ifc_report_len = 0;
	if (h_vdev->ifc_report == NULL) {
		goto err_undel_vdev;
	}

	return 0;

err_undel_vdev:
	host_rmi_granule_undelegate((u_register_t)h_vdev->vdev_ptr);
	return -1;
}

int host_assign_vdev_to_realm(struct realm *realm, struct host_vdev *h_vdev,
			      unsigned long tdi_id, void *pdev_ptr)
{
	struct rmi_vdev_params *vdev_params;
	u_register_t ret;
	int rc;

	rc = host_vdev_setup(h_vdev, tdi_id, pdev_ptr);
	if (rc != 0) {
		return rc;
	}

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
	vdev_params->tdi_id = tdi_id;

	vdev_params->flags = h_vdev->flags;
	vdev_params->num_aux = 0UL;

	ret = host_rmi_vdev_create(realm->rd, (u_register_t)pdev_ptr,
				  (u_register_t)h_vdev->vdev_ptr,
				  (u_register_t)vdev_params);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV create failed\n");
		return -1;
	}

	return 0;
}

int host_unassign_vdev_from_realm(struct realm *realm, struct host_vdev *h_vdev)
{
	u_register_t ret;
	u_register_t state;

	ret = host_vdev_get_state(h_vdev, &state);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	if (state == RMI_VDEV_STATE_LOCKED ||
	    state == RMI_VDEV_STATE_STARTED ||
	    state == RMI_VDEV_STATE_ERROR) {
		ret = host_vdev_unlock(realm, h_vdev, RMI_VDEV_STATE_UNLOCKED);
		if (ret != RMI_SUCCESS) {
			ERROR("VDEV unlock failed\n");
			return -1;
		}
	}

	ret = host_rmi_vdev_destroy(realm->rd,
				    (u_register_t)h_vdev->pdev_ptr,
				    (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV destroy failed\n");
		return -1;
	}

	ret = host_rmi_granule_undelegate((u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV undelegate failed\n");
		return -1;
	}

	return 0;
}

void host_do_vdev_complete(u_register_t rec_ptr, unsigned long vdev_id)
{
	struct host_vdev *h_vdev;
	u_register_t ret;

	h_vdev = find_host_vdev_from_id(vdev_id);
	if (h_vdev == NULL) {
		return;
	}

	/* Complete the VDEV request */
	ret = host_rmi_vdev_complete(rec_ptr, (u_register_t)h_vdev->vdev_ptr);
	if (ret != RMI_SUCCESS) {
		ERROR("Handling VDEV request failed\n");
	}
}

void host_do_vdev_communicate(struct realm *realm, u_register_t vdev_ptr)
{
	struct host_vdev *h_vdev;
	struct host_pdev *h_pdev;
	int rc;

	h_vdev = find_host_vdev_from_vdev_ptr(vdev_ptr);
	if (h_vdev == NULL) {
		return;
	}

	h_pdev = find_host_pdev_from_vdev_ptr(vdev_ptr);
	if (h_pdev == NULL) {
		return;
	}

	rc = host_dev_communicate(realm, h_pdev, h_vdev, RMI_VDEV_STATE_NEW);
	if (rc != 0) {
		ERROR("Handling VDEV communicate failed\n");
	}
}

u_register_t host_dev_mem_map(struct realm *realm, struct host_vdev *h_vdev,
			      u_register_t dev_pa, long map_level,
			      u_register_t *dev_ipa)
{
	u_register_t map_addr = dev_pa;	/* 1:1 PA->IPA mapping */
	u_register_t ret;

	*dev_ipa = 0UL;

	ret = host_vdev_map(realm, h_vdev, map_addr, map_level, dev_pa);

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

		ret = host_vdev_map(realm, h_vdev, map_addr, map_level, dev_pa);
	}
	if (ret != RMI_SUCCESS) {
		tftf_testcase_printf("%s() failed, 0x%lx\n",
			"host_rmi_vdev_map", ret);
		return REALM_ERROR;
	}

	*dev_ipa = map_addr;
	return REALM_SUCCESS;
}

bool is_host_pdev_independently_attested(struct host_pdev *h_pdev)
{
	assert(h_pdev);
	assert(h_pdev->dev);

	if ((pcie_dev_has_doe(h_pdev->dev)) &&
	    (pcie_dev_has_ide(h_pdev->dev)) &&
	    (h_pdev->dev->rp_dev != NULL) &&
	    (pcie_dev_has_ide(h_pdev->dev->rp_dev)) &&
	    (pcie_dev_has_dvsec_rmeda(h_pdev->dev->rp_dev))) {
		return true;
	}

	return false;
}

/*
 * Returns true if all host_pdev state is clean like no granules, aux granules,
 * memory are associated with the host_pdev.
 */
static bool is_host_pdevs_state_clean(void)
{
	unsigned int i, cnt;

	for (i = 0U; i < gbl_host_pdev_count; i++) {
		struct host_pdev *h_pdev = &gbl_host_pdevs[i];

		if ((h_pdev->is_connected_to_tsm) ||
		    (h_pdev->pdev != NULL) ||
		    (h_pdev->dev_comm_data != NULL) ||
		    (h_pdev->cert_chain != NULL) ||
		    (h_pdev->vca != NULL) ||
		    (h_pdev->public_key != NULL) ||
		    (h_pdev->public_key_metadata != NULL) ||
		    (h_pdev->dev == NULL)) {
			return false;
		}

		for (cnt = 0U; cnt < PDEV_PARAM_AUX_GRANULES_MAX; cnt++) {
			if (h_pdev->pdev_aux[cnt] != NULL) {
				return false;
			}
		}
	}

	return true;
}

/*
 * Find all PCIe off-chip devices that confirm to TEE-IO standards.
 * Devices that supports DOE, IDE, TDISP with RootPort that supports
 * RME DA are initialized in host_pdevs[]
 */
void host_pdevs_init(void)
{
	static bool gbl_host_pdevs_init_done;
	pcie_device_bdf_table_t *bdf_table;
	pcie_dev_t *dev;
	unsigned int i;
	unsigned int cnt = 0U;

	if (gbl_host_pdevs_init_done) {
		/*
		 * Check if the state of host_pdev is cleared and de-inited
		 * properly by the last testcase.
		 */
		if (!is_host_pdevs_state_clean()) {
			ERROR("gbl_host_pdevs state not clean\n");
			exit(1);
		}

		return;
	}

	/* When called for the first time this does PCIe enumeration */
	pcie_init();

	INFO("Initializing host_pdevs\n");
	bdf_table = pcie_get_bdf_table();
	if ((bdf_table == NULL) || (bdf_table->num_entries == 0)) {
		goto out_init;
	}

	for (i = 0U; i < bdf_table->num_entries; i++) {
		dev = &bdf_table->device[i];

		if ((dev->dp_type != EP) && (dev->dp_type != RCiEP)) {
			continue;
		}

		if ((dev->dp_type == EP) && (dev->rp_dev == NULL)) {
			INFO("No RP found for Device %x:%x.%x\n",
			     PCIE_EXTRACT_BDF_BUS(dev->bdf),
			     PCIE_EXTRACT_BDF_DEV(dev->bdf),
			     PCIE_EXTRACT_BDF_FUNC(dev->bdf));
			continue;
		}

		/*
		 * Skip VF in multi function device as it can't be treated as
		 * PDEV
		 */
		if (PCIE_EXTRACT_BDF_FUNC(dev->bdf) != 0) {
			continue;
		}

		/* Initialize host_pdev */
		gbl_host_pdevs[cnt].dev = dev;
		cnt++;

		if (cnt == HOST_PDEV_MAX) {
			WARN("Max host_pdev count reached.\n");
			break;
		}
	}

out_init:
	gbl_host_pdevs_init_done = true;
	gbl_host_pdev_count = cnt;
}
