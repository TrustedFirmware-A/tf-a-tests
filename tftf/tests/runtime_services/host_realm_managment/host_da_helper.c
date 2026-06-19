/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include <arch_features.h>
#include <debug.h>
#include <errno.h>
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
	struct host_pdev *h_pdev;

	for (unsigned int i = 0U; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->rp_pdev == (void *)pdev_ptr) {
			return h_pdev;
		}
		if (h_pdev->ep_pdev == (void *)pdev_ptr) {
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

static int pdev_get_state(void *pdev, u_register_t *state)
{
	u_register_t ret;

	ret = host_rmi_pdev_get_state((u_register_t)pdev, state);
	if (ret != RMI_SUCCESS) {
		return -1;
	}
	return 0;
}

static int host_pdev_get_state(struct host_pdev *h_pdev, bool ep_pdev, u_register_t *state)
{
	if (ep_pdev) {
		return pdev_get_state(h_pdev->ep_pdev, state);
	}
	return pdev_get_state(h_pdev->rp_pdev, state);
}

static bool is_pdev_state(void *pdev, u_register_t exp_state)
{
	u_register_t cur_state;

	if (pdev_get_state(pdev, &cur_state) != 0) {
		return false;
	}

	if (cur_state != exp_state) {
		return false;
	}

	return true;
}

static bool is_host_pdev_state(struct host_pdev *h_pdev, bool ep_pdev, u_register_t exp_state)
{
	if (ep_pdev) {
		return is_pdev_state(h_pdev->ep_pdev, exp_state);
	}
	return is_pdev_state(h_pdev->rp_pdev, exp_state);
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
static int host_pdev_setup(struct host_pdev *h_pdev, bool ep_pdev)
{
	u_register_t ret;
	void **pdev;

	/* RCiEP devices not supported by RMM */
	if (h_pdev->dev->dp_type == RCiEP) {
		return -1;
	}

	/* Allocate granule for PDEV and delegate */
	pdev = ep_pdev ? &h_pdev->ep_pdev : &h_pdev->rp_pdev;
	if (*pdev != NULL) {
		ERROR("%cP_PDEV is already allocated\n", ep_pdev?'E':'R');
		return -1;
	}
	*pdev = page_alloc(PAGE_SIZE);
	if (*pdev == NULL) {
		return -1;
	}
	memset(*pdev, 0, GRANULE_SIZE);

	ret = host_rmi_granule_delegate((u_register_t)*pdev);
	if (ret != RMI_SUCCESS) {
		ERROR("%cP_PDEV delegate failed 0x%lx\n", ep_pdev?'E':'R', ret);
		goto err_undelegate_pdev;
	}

	if (h_pdev->setup_done) {
		return 0;
	}

	/* Set IDE based on device capability */
	h_pdev->has_ide = pcie_dev_has_ide(h_pdev->dev);

	/* Allocate dev_comm_data and send/recv buffer for Dev communication */
	h_pdev->dev_comm_data = (struct rmi_dev_comm_data *)page_alloc(PAGE_SIZE);
	if (h_pdev->dev_comm_data == NULL) {
		goto err_undelegate_pdev;
	}

	memset(h_pdev->dev_comm_data, 0, sizeof(struct rmi_dev_comm_data));

	h_pdev->dev_comm_data->enter.req_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (h_pdev->dev_comm_data->enter.req_addr == 0UL) {
		goto err_undelegate_pdev;
	}

	h_pdev->dev_comm_data->enter.resp_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (h_pdev->dev_comm_data->enter.resp_addr == 0UL) {
		goto err_undelegate_pdev;
	}

	/* Allocate buffer to cache device certificate */
	h_pdev->cert_slot_id = 0;
	h_pdev->cert_chain = (uint8_t *)page_alloc(HOST_PDEV_CERT_LEN_MAX);
	h_pdev->cert_chain_len = 0;
	if (h_pdev->cert_chain == NULL) {
		goto err_undelegate_pdev;
	}

	/* Allocate buffer to store extracted public key */
	h_pdev->public_key = (void *)page_alloc(PAGE_SIZE);
	if (h_pdev->public_key == NULL) {
		goto err_undelegate_pdev;
	}
	h_pdev->public_key_len = PAGE_SIZE;

	/* Allocate buffer to store public key metadata */
	h_pdev->public_key_metadata = (void *)page_alloc(PAGE_SIZE);
	if (h_pdev->public_key_metadata == NULL) {
		goto err_undelegate_pdev;
	}

	h_pdev->public_key_metadata_len = PAGE_SIZE;

	/* Allocate buffer to store VCA */
	h_pdev->vca = (uint8_t *)page_alloc(HOST_PDEV_VCA_LEN_MAX);
	h_pdev->vca_len = 0;
	if (h_pdev->vca == NULL) {
		goto err_undelegate_pdev;
	}

	/* Set algorithm to use for device digests */
	h_pdev->pdev_hash_algo = RMI_HASH_SHA_512;

	h_pdev->setup_done = true;

	return 0;

err_undelegate_pdev:
	host_rmi_granule_undelegate((u_register_t)h_pdev->rp_pdev);
	host_rmi_granule_undelegate((u_register_t)h_pdev->ep_pdev);

	return -1;
}

int host_pdev_stream_connect(struct host_pdev *h_pdev)
{
	struct rmi_pdev_stream_params *stream_params;
	u_register_t ret;
	u_register_t handle;

	assert(!h_pdev->pdev_stream_handle_valid);

	stream_params = (struct rmi_pdev_stream_params *)page_alloc(PAGE_SIZE);
	memset(stream_params, 0, GRANULE_SIZE);

	stream_params->stream_type = RMI_PDEV_STREAM_NCOH;
	stream_params->pdev_1 = (unsigned long)h_pdev->ep_pdev;
	stream_params->pdev_2 = (unsigned long)h_pdev->rp_pdev;

	ret = host_rmi_pdev_stream_connect((u_register_t)stream_params, &handle);

	page_free((u_register_t)stream_params);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	h_pdev->pdev_stream_handle = handle;
	h_pdev->pdev_stream_handle_valid = true;

	return 0;
}

int host_pdev_stream_disconnect(struct host_pdev *h_pdev)
{
	u_register_t ret;

	assert(h_pdev->pdev_stream_handle_valid);

	ret = host_rmi_pdev_stream_disconnect(
		(unsigned long)h_pdev->ep_pdev,
		(unsigned long)h_pdev->rp_pdev,
		h_pdev->pdev_stream_handle);

	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

int host_pdev_stream_key_refresh(struct host_pdev *h_pdev)
{
	u_register_t ret;

	assert(h_pdev->pdev_stream_handle_valid);

	ret = host_rmi_pdev_stream_key_refresh(
		(unsigned long)h_pdev->ep_pdev,
		(unsigned long)h_pdev->rp_pdev,
		h_pdev->pdev_stream_handle);

	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

int host_pdev_create(struct host_pdev *h_pdev, bool ep_pdev)
{
	struct rmi_pdev_params *pdev_params;
	u_register_t ret;
	void *pdev;
	u_register_t create_handle = 0UL;
	u_register_t donate_req = 0UL;
	int rc;

	/* Allocate granules and memory for PDEV objects like certificate, key */
	rc = host_pdev_setup(h_pdev, ep_pdev);
	if (rc == -1) {
		ERROR("host_pdev_setup failed.\n");
		return -1;
	}

	/* make sure that the ecam_base 256 MB aligned */
	assert((h_pdev->dev->ecam_base & BIT_MASK_ULL(27, 0)) == 0U);

	/* make sure that bdf is in the 256 mb limit */
	assert((h_pdev->dev->bdf >> 28) == 0U);
	assert((h_pdev->dev->rp_dev->bdf >> 28) == 0U);

	pdev_params = (struct rmi_pdev_params *)page_alloc(PAGE_SIZE);
	memset(pdev_params, 0, GRANULE_SIZE);

	pdev_params->flags =
		INPLACE(RMI_PDEV_FLAGS_SPDM, RMI_PDEV_SPDM_TRUE);

	if (ep_pdev) {
		/* Create EP pdev */
		pdev_params->flags |=
			INPLACE(RMI_PDEV_FLAGS_CATEGORY, RMI_PDEV_ENDPOINT_ACCEL_OFF_CHIP);
		pdev_params->hb_base = h_pdev->dev->ecam_base;
		pdev_params->pdev_id = h_pdev->dev->bdf;
		pdev = h_pdev->ep_pdev;
	} else {
		/* Create RP pdev */
		pdev_params->flags |=
			INPLACE(RMI_PDEV_FLAGS_CATEGORY, RMI_PDEV_ROOTPORT);
		pdev_params->hb_base = h_pdev->dev->ecam_base;
		pdev_params->pdev_id = h_pdev->dev->rp_dev->bdf;
		pdev = h_pdev->rp_pdev;
	}

	pdev_params->routing_id = 0;   /* Segment id is 0 in FVP */
	pdev_params->id_index = h_pdev->cert_slot_id;
	pdev_params->hash_algo = h_pdev->pdev_hash_algo;
	pdev_params->rid_base = (unsigned int)h_pdev->dev->bdf;
	pdev_params->rid_top = pdev_params->rid_base + 1U;

	ret = host_rmi_pdev_create((u_register_t)pdev,
				   (u_register_t)pdev_params,
				   &create_handle,
				   &donate_req);

	/*
	 * Start the RSO flow for RMI_pdev_CREATE. It is expected
	 * that the host will donate all the memory in one go.
	 */
	if (ep_pdev) {
		if (RMI_RETURN_STATUS(ret) != RMI_INCOMPLETE) {
			return REALM_ERROR;
		}
		/* We expect a SRO operation at this point */
		ret = host_realm_sro_continue(ret, &create_handle, &donate_req, NULL);
	}

	if (RMI_RETURN_STATUS(ret) != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n",
				"host_rmi_pdev_create", ret);
	}

	page_free((u_register_t)pdev_params);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_pdev_set_pubkey(struct host_pdev *h_pdev, bool ep_pdev)
{
	struct rmi_public_key_params *pubkey_params;
	uint8_t public_key_algo;
	u_register_t ret;
	void *pdev;
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

	pdev = ep_pdev ? h_pdev->ep_pdev : h_pdev->rp_pdev;
	ret = host_rmi_pdev_set_pubkey((u_register_t)pdev,
				       (u_register_t)pubkey_params);

	if (ret != RMI_SUCCESS) {
		return -1;
	}
	return 0;
}

static int host_pdev_stop(struct host_pdev *h_pdev, bool ep_pdev)
{
	void *pdev;
	u_register_t ret;

	pdev = ep_pdev ? h_pdev->ep_pdev : h_pdev->rp_pdev;
	ret = host_rmi_pdev_stop((u_register_t)pdev);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_pdev_abort(struct host_pdev *h_pdev, bool ep_pdev)
{
	void *pdev;
	u_register_t ret;

	pdev = ep_pdev ? h_pdev->ep_pdev : h_pdev->rp_pdev;
	ret = host_rmi_pdev_abort((u_register_t)pdev);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

/* Call RMI_PDEV_DESTROY and free all pdev related allocations */
static int host_pdev_destroy(struct host_pdev *h_pdev, bool ep_pdev)
{
	void **pdev;
	u_register_t ret;
	int rc = 0;
	u_register_t destroy_handle = 0UL;
	u_register_t donate_req = 0UL;

	pdev = ep_pdev ? &h_pdev->ep_pdev : &h_pdev->rp_pdev;
	ret = host_rmi_pdev_destroy((u_register_t)*pdev);
	if (RMI_RETURN_STATUS(ret) == RMI_INCOMPLETE) {
		ret = host_realm_sro_continue(ret, &destroy_handle, &donate_req, NULL);
	}
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	/* Undelegate PDEV granule */
	ret = host_rmi_granule_undelegate((u_register_t)*pdev);
	*pdev = NULL;
	if (ret != RMI_SUCCESS) {
		ERROR("PDEV undelegate failed 0x%lx\n", ret);
		rc = -1;
	}

	/* Only free up resources if both pdevs are NULL. */
	if ((h_pdev->ep_pdev == NULL) && (h_pdev->rp_pdev == NULL)) {
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

		h_pdev->setup_done = false;
	}

	return rc;
}

static int host_dev_get_state(struct host_pdev *h_pdev, bool ep_pdev, struct host_vdev *h_vdev,
			      u_register_t *state)
{
	if (h_vdev) {
		return host_vdev_get_state(h_vdev, state);
	}
	return host_pdev_get_state(h_pdev, ep_pdev, state);
}

static u_register_t host_rmi_dev_communicate(struct realm *realm,
					     struct host_pdev *h_pdev,
					     bool ep_pdev,
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

	if (ep_pdev) {
		return host_rmi_pdev_communicate((u_register_t)h_pdev->ep_pdev,
						 (u_register_t)h_pdev->dev_comm_data);
	}
	return host_rmi_pdev_communicate((u_register_t)h_pdev->rp_pdev,
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

	if (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_CACHE_REQ,
			dcomm_exit->flags) != 0U) {
		rc = host_dev_cache_dev_object(h_pdev, h_vdev,
					       (uint8_t *)dcomm_enter->req_addr,
					       dcomm_exit->cache_obj_id,
					       dcomm_exit->cache_req_offset,
					       dcomm_exit->cache_req_len);
		if (rc != 0) {
			ERROR("host_dev_cache_device_object req failed\n");
			return -1;
		}
	}


	if (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_CACHE_RSP,
			dcomm_exit->flags) != 0U) {
		rc = host_dev_cache_dev_object(h_pdev, h_vdev,
					       (uint8_t *)dcomm_enter->resp_addr,
					       dcomm_exit->cache_obj_id,
					       dcomm_exit->cache_rsp_offset,
					       dcomm_exit->cache_rsp_len);

		if (rc != 0) {
			ERROR("host_dev_cache_device_object rsp failed\n");
		}
	}

	return rc;
}

static int host_pdev_doe_communicate(struct host_pdev *h_pdev,
				     bool ep_pdev,
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
	if (ep_pdev) {
		rc = pcie_doe_communicate(doe_header, h_pdev->dev->bdf,
					  h_pdev->dev->doe_cap_base,
					  (void *)dcomm_enter->req_addr,
					  dcomm_exit->req_len,
					  (void *)dcomm_enter->resp_addr, &resp_len);
	} else {
		rc = pcie_doe_communicate(doe_header, h_pdev->dev->rp_dev->bdf,
					  h_pdev->dev->rp_dev->doe_cap_base,
					  (void *)dcomm_enter->req_addr,
					  dcomm_exit->req_len,
					  (void *)dcomm_enter->resp_addr, &resp_len);
	}

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
				bool ep_pdev,
				struct host_vdev *h_vdev,
				unsigned char target_state,
				bool go_until_wait_flag_set)
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

	rc = host_dev_get_state(h_pdev, ep_pdev, h_vdev, &state);
	if (rc != 0) {
		return rc;
	}

	stop = false;
	do {
		ret = host_rmi_dev_communicate(realm, h_pdev, ep_pdev, h_vdev);
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

		if (dcomm_exit->timeout != 0) {
			VERBOSE("timeout: %ld in ms\n", dcomm_exit->timeout);
			waitms(dcomm_exit->timeout);
		}

		/* Send request to PDEV's DOE and get response */
		if (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_SEND, dcomm_exit->flags)) {
			rc = host_pdev_doe_communicate(h_pdev, ep_pdev, dcomm_enter,
						       dcomm_exit);
			if (rc != 0) {
				ERROR("host_pdev_doe_communicate failed\n");
				break;
			}
		} else {
			dcomm_enter->status = RMI_DEV_COMM_ENTER_STATUS_NONE;
		}

		rc = host_dev_get_state(h_pdev, ep_pdev, h_vdev, &state);
		if (rc != 0) {
			break;
		}
		if (state == target_state) {
			/*
			 * The target state was reached, but for some
			 * transitions this is not enough, need to continue
			 * calling it till certain flags are cleared, or wait
			 * flag is set in the exit. wait for that to happen.
			 */
			if (go_until_wait_flag_set) {
				stop = EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_STREAM_WAIT,
					       dcomm_exit->flags) != 0;
			} else {
				stop = dcomm_exit->flags == 0;
			}
		} else if (state == error_state) {
			ERROR("Failed to reach target_state: %lu instead of %u\n",
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
int host_pdev_transition(struct host_pdev *h_pdev, bool ep_pdev, unsigned char to_state)
{
	int rc;

	switch (to_state) {
	case RMI_PDEV_STATE_NEW:
		rc = host_pdev_create(h_pdev, ep_pdev);
		break;
	case RMI_PDEV_STATE_NEEDS_KEY:
		/* Reset cached cert_chain */
		h_pdev->cert_chain_len = 0UL;
		h_pdev->vca_len = 0UL;
		rc = host_dev_communicate(NULL, h_pdev, ep_pdev, NULL,
					  RMI_PDEV_STATE_NEEDS_KEY, false);
		break;
	case RMI_PDEV_STATE_HAS_KEY:
		rc = host_pdev_set_pubkey(h_pdev, ep_pdev);
		break;
	case RMI_PDEV_STATE_READY:
		rc = host_dev_communicate(NULL, h_pdev, ep_pdev, NULL, RMI_PDEV_STATE_READY, false);
		break;
	case RMI_PDEV_STATE_STOPPED:
		/* Abort communication in case the device's comm state is not idle */
		INFO("Abort communication, may return RMI_RMI_ERROR_DEVICE\n");
		(void)host_pdev_abort(h_pdev, ep_pdev);

		rc = host_pdev_stop(h_pdev, ep_pdev);
		if (rc != 0) {
			break;
		}
		rc = host_dev_communicate(NULL, h_pdev, ep_pdev,
					  NULL, RMI_PDEV_STATE_STOPPED, false);
		break;
	default:
		ERROR("pdev_state_transition: Unhandled state\n");
		rc = -1;
		break;
	}

	if (rc != 0) {
		ERROR("pdev_state_transition: failed\n");
		return rc;
	}

	if (!is_host_pdev_state(h_pdev, ep_pdev, to_state)) {
		ERROR("pdev_state_transition: PDEV state not [%s]\n",
		      pdev_state_str[to_state]);
		return -1;
	}

	/*
	 * Upon successful transition to STOPPED, call pdev_destroy and reclaim
	 * all the allocation done to PDEV.
	 */
	if (to_state == RMI_PDEV_STATE_STOPPED) {
		rc = host_pdev_destroy(h_pdev, ep_pdev);
	}
	return rc;
}

static int host_pdev_communicate_till_wait(struct host_pdev *h_pdev, bool ep_pdev)
{
	int rc;
	u_register_t state;

	rc = host_dev_get_state(h_pdev, ep_pdev, NULL, &state);
	if (rc != 0) {
		return rc;
	}

	rc = host_dev_communicate(NULL, h_pdev, ep_pdev, NULL, state, true);
	if (rc != 0) {
		ERROR("pdev_state_transition: failed\n");
	}
	return rc;
}

int host_pdev_stream_complete(struct host_pdev *h_pdev)
{
	int rc;
	u_register_t ret;

	const unsigned char pdev_stream_complete_flow[] = {
		RMI_PDEV_STATE_READY
	};

	/* Now we need to call pdev_communicate on the devices until the wait flag is set */
	rc = host_pdev_communicate_till_wait(h_pdev, false);
	if (rc != 0) {
		ERROR("PDEV TSM connect rp_pdev_connect: failed\n");
		return -1;
	}

	rc = host_pdev_communicate_till_wait(h_pdev, true);
	if (rc != 0) {
		ERROR("PDEV TSM connect ep_pdev_connect: failed\n");
		return -1;
	}

	/* call pdev communicate to let both pdevs reach PDEV_OP_STREAM_COMPLETE */
	rc = host_pdev_state_transition(h_pdev, false, pdev_stream_complete_flow,
					sizeof(pdev_stream_complete_flow));
	if (rc != 0) {
		ERROR("PDEV TSM connect state transitions complete for rp_pdev: failed\n");
		return -1;
	}

	rc = host_pdev_state_transition(h_pdev, true, pdev_stream_complete_flow,
					sizeof(pdev_stream_complete_flow));
	if (rc != 0) {
		ERROR("PDEV TSM connect state transitions complete for ep_pdev: failed\n");
		return -1;
	}

	/* Finally complete the stream operation */
	assert(h_pdev->pdev_stream_handle_valid);

	ret = host_rmi_pdev_stream_complete(
		(unsigned long)h_pdev->ep_pdev,
		(unsigned long)h_pdev->rp_pdev,
		h_pdev->pdev_stream_handle);

	if (ret != RMI_SUCCESS) {
		ERROR("PDEV TSM connect stream complete: failed\n");
		return -1;
	}

	return 0;
}


int host_pdev_state_transition(struct host_pdev *h_pdev,
			       bool ep_pdev,
			       const unsigned char pdev_states[],
			       size_t pdev_states_max)
{
	unsigned int i;
	u_register_t state;
	int rc = 0;

	const char *prefix = ep_pdev?"EP_":"RP_";

	if ((h_pdev == NULL) || (pdev_states_max > PDEV_STATE_TRANSITION_MAX)) {
		return -1;
	}

	for (i = 0U; i < pdev_states_max; i++) {
		if (pdev_states[i] == (unsigned char)-1) {
			break;
		}

		if (pdev_states[i] > RMI_PDEV_STATE_STOPPED) {
			ERROR("Invalid %sPDEV state: %d\n", prefix,
			      pdev_states[i]);
			rc = -1;
			break;
		}

		if (i == 0U) {
			INFO("%sPDEV transition to [%s]\n", prefix,
			     pdev_state_str[pdev_states[i]]);
		} else {
			INFO("%sPDEV transition from [%s] -> [%s]\n", prefix,
			     pdev_state_str[pdev_states[i - 1]],
			     pdev_state_str[pdev_states[i]]);
		}

		rc = host_pdev_transition(h_pdev, ep_pdev, pdev_states[i]);
		if (rc != 0) {
			ERROR("%sPDEV transition: to [%s] failed\n", prefix,
			      pdev_state_str[pdev_states[i]]);
			rc = -1;
			break;
		}
	}

	/*
	 * On error transition the PDEV to STOPPED state and cleanup the
	 * resources held by PDEV.
	 */
	if ((rc != 0) && (host_pdev_get_state(h_pdev, ep_pdev, &state) == 0)) {
		INFO("%sPDEV transition from [%s] -> [%s]\n", prefix,
		     pdev_state_str[state],
		     pdev_state_str[RMI_PDEV_STATE_STOPPED]);

		(void)host_pdev_transition(h_pdev, ep_pdev,
					   RMI_PDEV_STATE_STOPPED);
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

	rc = host_rmi_vdev_get_interface_report(realm->rd, (u_register_t)h_pdev->ep_pdev,
						(u_register_t)h_vdev->vdev_ptr);
	if (rc != 0) {
		return rc;
	}
	rc = host_dev_communicate(realm, h_pdev, true, h_vdev, target_state, false);

	return rc;
}

int host_rtt_dev_map(struct realm *realm, struct host_vdev *h_vdev, u_register_t base,
			u_register_t top, u_register_t flags, u_register_t oaddr,
			u_register_t *out_top)
{
	return host_rmi_rtt_dev_map(realm->rd, (u_register_t)h_vdev->vdev_ptr,
			       base, top, flags, oaddr, out_top);
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

	rc = host_rmi_vdev_unlock(realm->rd, (u_register_t)h_pdev->ep_pdev,
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
	 * vdev_measure_params->flags is left as 0 as we don't want raw output.
	 */
	rc = host_rmi_vdev_get_measurements(realm->rd, (u_register_t)h_pdev->ep_pdev,
					    (u_register_t)h_vdev->vdev_ptr,
					    (u_register_t)vdev_measure_params);
	if (rc != 0) {
		return rc;
	}
	return host_dev_communicate(realm, h_pdev, true, h_vdev, target_state, false);
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
		rc = host_dev_communicate(realm, h_pdev, true, h_vdev,
					  RMI_VDEV_STATE_UNLOCKED, false);
		break;
	case RMI_VDEV_STATE_LOCKED:
		rc = host_rmi_vdev_lock(realm->rd, (u_register_t)h_pdev->ep_pdev,
					(u_register_t)h_vdev->vdev_ptr);
		if (rc != 0) {
			break;
		}
		rc = host_dev_communicate(realm, h_pdev, true, h_vdev,
					  RMI_VDEV_STATE_LOCKED, false);
		break;
	case RMI_VDEV_STATE_STARTED:
		rc = host_rmi_vdev_start(realm->rd, (u_register_t)h_pdev->ep_pdev,
					 (u_register_t)h_vdev->vdev_ptr);
		if (rc != 0) {
			break;
		}
		rc = host_dev_communicate(realm, h_pdev, true, h_vdev,
					  RMI_VDEV_STATE_STARTED, false);
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

int host_create_realm_with_feat_da(struct realm *realm, bool activate)
{
	bool lpa2 = false;
	u_register_t s2sz = MAX_IPA_BITS;
	long sl = RTT_MIN_LEVEL;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct test_realm_params params;

	if (is_feat_52b_on_4k_2_supported() == true) {
		lpa2 = true;
		s2sz = MAX_IPA_BITS_LPA2;
		sl = RTT_MIN_LEVEL_LPA2;
	}

	params = (struct test_realm_params) {
		0,
		.realm_payload_adr = (u_register_t)REALM_IMAGE_BASE,
		.rec_flag = rec_flag,
		.rec_count = 1U,
		.s2sz = s2sz,
		.sl = sl,
		.lpa2 = lpa2,
		.da = true,
	};

	if (activate) {
		if (!host_create_activate_realm_payload(realm, &params)) {
			return -1;
		}
	} else {
		if (!host_create_realm_payload(realm, &params)) {
			return -1;
		}
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

static u_register_t host_create_vdev_with_ranges(struct realm *realm,
			      struct host_vdev *h_vdev, unsigned long tdi_id,
			      void *pdev_ptr, struct rmi_address_range *addr_range,
			      size_t num_address_range)
{
	struct rmi_vdev_params *vdev_params;
	u_register_t ret;
	int rc;

	rc = host_vdev_setup(h_vdev, tdi_id, pdev_ptr);
	if (rc != 0) {
		return RMI_ERROR_INPUT;
	}

	/* Create vdev and bind it to the Realm */
	vdev_params = (struct rmi_vdev_params *)page_alloc(PAGE_SIZE);

	(void)memset(vdev_params, 0, PAGE_SIZE);

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

	if ((addr_range != NULL) && (num_address_range > 0)) {
		memcpy(vdev_params->addr_range,
		       addr_range,
		       num_address_range * sizeof(addr_range[0]));
		vdev_params->num_addr_range = num_address_range;
	} else {
		memset(vdev_params->addr_range, 0, num_address_range * sizeof(addr_range[0]));
		vdev_params->num_addr_range = 0U;
	}

	ret = host_rmi_vdev_create(realm->rd, (u_register_t)pdev_ptr,
				  (u_register_t)h_vdev->vdev_ptr,
				  (u_register_t)vdev_params);
	page_free((u_register_t)vdev_params);
	if (ret != RMI_SUCCESS) {
		ERROR("VDEV create failed: 0x%lx\n", ret);
		host_rmi_granule_undelegate((u_register_t)h_vdev->vdev_ptr);
		return ret;
	}

	return RMI_SUCCESS;
}

u_register_t host_create_vdev(struct realm *realm, struct host_vdev *h_vdev,
			      unsigned long tdi_id, void *pdev_ptr)
{
	return host_create_vdev_with_ranges(realm, h_vdev, tdi_id, pdev_ptr,
					    NULL, 0U);
}

int host_assign_vdev_to_realm(struct realm *realm, struct host_vdev *h_vdev,
			      unsigned long tdi_id, void *pdev_ptr,
			      struct rmi_address_range *addr_range, size_t num_address_range)
{
	return (host_create_vdev_with_ranges(realm, h_vdev, tdi_id, pdev_ptr,
					     addr_range, num_address_range) ==
		RMI_SUCCESS) ? 0 : -1;
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
		WARN("VDEV with provided vdev_id not found");
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

	rc = host_dev_communicate(realm, h_pdev, true, h_vdev, RMI_VDEV_STATE_NEW, false);
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
	u_register_t out_top;
	size_t granule_count = RTT_MAP_SIZE(map_level) / GRANULE_SIZE;
	size_t i = 0;

	*dev_ipa = 0UL;

	while (i < granule_count) {
		unsigned long base = map_addr + (GRANULE_SIZE * i);
		unsigned long top = map_addr + (GRANULE_SIZE * (i + 1));

		ret = host_rtt_dev_map(realm, h_vdev, base, top, RMI_ADDR_TYPE_SINGLE, base,
				&out_top);

		if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
			/* Create a missing RTT and retry */
			long level = (long)RMI_RETURN_INDEX(ret);

			ret = host_rmi_create_rtt_level(realm, base, level + 1);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, 0x%lx\n",
					"host_rmi_create_rtt_level", ret);
				return REALM_ERROR;
			}

			/* try mapping again */
			continue;
		} else if ((ret != RMI_SUCCESS) ||
			    (out_top != top)) {
			ERROR("%s() failed, 0x%lx, out_top=0x%lx\n",
				"host_rmi_vdev_map", ret, out_top);
			return REALM_ERROR;
		}
		++i;
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

struct host_pdev *get_host_pdev_by_type(uint8_t type)
{
	if (type != DEV_TYPE_INDEPENDENTLY_ATTESTED) {
		return NULL;
	}

	/* Return the first host_pdev of 'type' */
	for (unsigned int i = 0U; i < gbl_host_pdev_count; i++) {
		if (is_host_pdev_independently_attested(&gbl_host_pdevs[i])) {
			return &gbl_host_pdevs[i];
		}
	}

	return NULL;
}

/*
 * Returns true if all host_pdev state is clean like no granules, aux granules,
 * memory are associated with the host_pdev.
 */
static bool is_host_pdevs_state_clean(void)
{
	for (unsigned int i = 0U; i < gbl_host_pdev_count; i++) {
		struct host_pdev *h_pdev = &gbl_host_pdevs[i];

		if ((h_pdev->setup_done) ||
		    (h_pdev->is_connected_to_tsm) ||
		    (h_pdev->rp_pdev != NULL) ||
		    (h_pdev->ep_pdev != NULL) ||
		    (h_pdev->dev_comm_data != NULL) ||
		    (h_pdev->cert_chain != NULL) ||
		    (h_pdev->vca != NULL) ||
		    (h_pdev->public_key != NULL) ||
		    (h_pdev->public_key_metadata != NULL) ||
		    (h_pdev->dev == NULL) ||
		    (h_pdev->pdev_stream_handle_valid)) {
			return false;
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

void host_get_addr_range(struct host_pdev *h_pdev)
{
	uint32_t offset = BAR0_OFFSET;

	h_pdev->ncoh_num_addr_range = 0UL;
	(void)memset(h_pdev->ncoh_addr_range, 0, sizeof(h_pdev->ncoh_addr_range));

	for (unsigned int i = 0U; i < NCOH_ADDR_RANGE_NUM; i++) {
		uint64_t addr, size;

		offset = pcie_get_bar(h_pdev->dev->bdf, offset, &addr, &size);
		if (size == 0ULL) {
			break;
		}

		h_pdev->ncoh_addr_range[i].base = addr;
		h_pdev->ncoh_addr_range[i].top = addr + size;
		h_pdev->ncoh_num_addr_range++;

		if (offset > BAR_TYPE_0_MAX_OFFSET) {
			break;
		}
	}
}

void host_dump_interface_report(
			pci_tdisp_device_interface_report_struct_t *ifc_report,
			size_t ifc_report_len)
{
	pci_tdisp_mmio_range_t *mmio_range;
	uint32_t index;

	assert(ifc_report_len >= sizeof(pci_tdisp_device_interface_report_struct_t));

	INFO("Interface report:\n");
	INFO("  interface_info        - 0x%04x\n",
		ifc_report->interface_info);
	INFO("  msi_x_message_control - 0x%04x\n",
		ifc_report->msi_x_message_control);
	INFO("  lnr_control           - 0x%04x\n",
		ifc_report->lnr_control);
	INFO("  tph_control           - 0x%08x\n",
		ifc_report->tph_control);
	INFO("  mmio_range_count      - 0x%08x\n",
		ifc_report->mmio_range_count);

	mmio_range = (pci_tdisp_mmio_range_t *)(ifc_report + 1UL);

	for (index = 0U; index < ifc_report->mmio_range_count; index++) {
		INFO("  mmio_range[%u]:\n", index);
		INFO("    first_page          - 0x%016llx\n",
			mmio_range[index].first_page);
		INFO("    number_of_pages     - 0x%08x\n",
			mmio_range[index].number_of_pages);
		INFO("    range_attributes    - 0x%04x\n",
			mmio_range[index].range_attributes);
		INFO("    range_id            - 0x%04x\n",
			mmio_range[index].range_id);
	}

	if (ifc_report_len > sizeof(pci_tdisp_device_interface_report_struct_t) +
		ifc_report->mmio_range_count * sizeof(pci_tdisp_mmio_range_t)) {

		uint32_t *device_specific_info_len  = (uint32_t *)&mmio_range[index];
		uint8_t *device_specific_info =
			(uint8_t *)((uintptr_t)device_specific_info_len + 1UL);

		INFO("  device_info_len       - 0x%08x\n",
			*device_specific_info_len);
		INFO("  device_info           - ");

		for (index = 0U; index < *device_specific_info_len; index++) {
			mp_printf("%02x ", device_specific_info[index]);
		}
		mp_printf("\n");
	}
}

int host_activate_psmmu(u_register_t psmmu_ptr)
{
	struct rmi_psmmu_params *params;
	u_register_t activate_handle = 0UL;
	u_register_t donate_req = 0UL;
	u_register_t res;

	/* Allocate memory for PSMMU params */
	params = (struct rmi_psmmu_params *)page_alloc(PAGE_SIZE);
	if (params == NULL) {
		ERROR("Failed to allocate memory for PSMMU params\n");
		return -ENOMEM;
	}

	/* Flags = 0, no MSI data */
	(void)memset(params, 0, PAGE_SIZE);

	/* Activate PSMMU */
	res = host_rmi_psmmu_activate(psmmu_ptr, (u_register_t)params,
					&activate_handle, &donate_req);

	if (RMI_RETURN_STATUS(res) != RMI_INCOMPLETE) {
		ERROR("res 0x%lx, expected 0x%x\n",
			RMI_RETURN_STATUS(res), RMI_INCOMPLETE);
		/* We expect a SRO operation at this point */
		return -ENODEV;
	}

	/*
	 * Start the RSO flow for RMI_PSMMU_ACTIVATE.
	 * It is expected that the host will donate all the memory in one go.
	 */
	res = host_realm_sro_continue(res, &activate_handle, &donate_req, NULL);
	if (RMI_RETURN_STATUS(res) != RMI_SUCCESS) {
		ERROR("%s(0x%lx) failed, %lu\n",
			"host_rmi_psmmu_activate", psmmu_ptr,
			RMI_RETURN_STATUS(res));
		return -ENODEV;
	}
	return 0;
}

int host_deactivate_psmmu(u_register_t psmmu_ptr)
{
	u_register_t deactivate_handle = 0UL;
	u_register_t reclaim_req = 0UL;
	u_register_t res;

	/* Deactivate PSMMU */
	res = host_rmi_psmmu_deactivate(psmmu_ptr, &deactivate_handle,
					&reclaim_req);

	if (RMI_RETURN_STATUS(res) != RMI_INCOMPLETE) {
		ERROR("res 0x%lx, expected 0x%x\n",
			RMI_RETURN_STATUS(res), RMI_INCOMPLETE);
		/* We expect a SRO operation at this point */
		return -ENODEV;
	}

	/*
	 * Start the RSO flow for RMI_PSMMU_DEACTIVATE.
	 * It is expected that the host will reclaim all the memory in one go.
	 */
	res = host_realm_sro_continue(res, &deactivate_handle, &reclaim_req, NULL);
	if (RMI_RETURN_STATUS(res) != RMI_SUCCESS) {
		ERROR("%s(0x%lx) failed, %lu\n",
			"host_rmi_psmmu_deactivate", psmmu_ptr,
			RMI_RETURN_STATUS(res));
		return -ENODEV;
	}
	return 0;
}

int host_psmmu_st_l2_create(u_register_t psmmu_ptr, unsigned int sid)
{
	u_register_t create_handle = 0UL;
	u_register_t donate_req = 0UL;
	u_register_t res;

	/*
	 * Calculate the base of the StreamID range.
	 * The size of the Level 2 Stream Table is PAGE_SIZE,
	 * containing 2^(PAGE_SIZE_SHIFT - 6) 64-byte STEs.
	 */
	sid &= ~((1UL << (PAGE_SIZE_SHIFT - 6U)) - 1UL);

	/* Create PSMMU Level 2 Stream Table */
	res = host_rmi_psmmu_st_l2_create(psmmu_ptr, (u_register_t)sid,
					  &create_handle, &donate_req);

	if (RMI_RETURN_STATUS(res) != RMI_INCOMPLETE) {
		ERROR("res 0x%lx, expected 0x%x\n",
			RMI_RETURN_STATUS(res), RMI_INCOMPLETE);
		/* We expect a SRO operation at this point */
		return -ENODEV;
	}

	/*
	 * Start the RSO flow for RMI_PSMMU_ST_L2_CREATE.
	 * It is expected that the host will donate all the memory in one go.
	 */
	res = host_realm_sro_continue(res, &create_handle, &donate_req, NULL);
	if (RMI_RETURN_STATUS(res) != RMI_SUCCESS) {
		ERROR("%s(0x%lx 0x%x) failed, %lu\n",
			"host_rmi_psmmu_st_l2_create", psmmu_ptr, sid,
			RMI_RETURN_STATUS(res));
		return -ENODEV;
	}
	return 0;
}

int host_psmmu_st_l2_destroy(u_register_t psmmu_ptr, unsigned int sid)
{
	u_register_t destroy_handle = 0UL;
	u_register_t reclaim_req = 0UL;
	u_register_t res;

	/*
	 * Calculate the base of the StreamID range.
	 * The size of the Level 2 Stream Table is PAGE_SIZE,
	 * containing 2^(PAGE_SIZE_SHIFT - 6) 64-byte STEs.
	 */
	sid &= ~((1U << (PAGE_SIZE_SHIFT - 6U)) - 1U);

	/* Destroy PSMMU Level 2 Stream Table */
	res = host_rmi_psmmu_st_l2_destroy(psmmu_ptr, (u_register_t)sid,
					   &destroy_handle, &reclaim_req);

	if (RMI_RETURN_STATUS(res) != RMI_INCOMPLETE) {
		ERROR("res 0x%lx, expected 0x%x\n",
			RMI_RETURN_STATUS(res), RMI_INCOMPLETE);
		/* We expect a SRO operation at this point */
		return -ENODEV;
	}

	/*
	 * Start the RSO flow for RMI_PSMMU_ST_L2_DESTROY.
	 * It is expected that the host will reclaim all the memory in one go.
	 */
	res = host_realm_sro_continue(res, &destroy_handle, &reclaim_req, NULL);
	if (RMI_RETURN_STATUS(res) != RMI_SUCCESS) {
		ERROR("%s(0x%lx 0x%x) failed, %lu\n",
			"host_rmi_psmmu_st_l2_destroy", psmmu_ptr, sid,
			RMI_RETURN_STATUS(res));
		return -ENODEV;
	}
	return 0;
}
