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

#define DEV_OBJ_CERT			0U
#define DEV_OBJ_MEASUREMENTS		2U
#define DEV_OBJ_INTERFACE_REPORT	3U

struct host_tdi {
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

	/*
	 * Fields related to cached device measurements.
	 * todo: This will be moved to vdev scope
	 */
	uint8_t *meas;
	size_t meas_len;

	/* PCIe details: bdf, DOE, Stream id, IO range */
	uint32_t bdf;
	uint32_t doe_cap_base;
};

static struct host_tdi g_tdi;

static const char * const pdev_state_str[] = {
	"PDEV_STATE_NEW",
	"PDEV_STATE_NEEDS_KEY",
	"PDEV_STATE_HAS_KEY",
	"PDEV_STATE_READY",
	"PDEV_STATE_COMMUNICATING",
	"PDEV_STATE_STOPPED",
	"RMI_PDEV_STATE_ERROR"
};

static int host_tdi_pdev_get_state(struct host_tdi *tdi, u_register_t *state)
{
	u_register_t ret;

	ret = host_rmi_pdev_get_state((u_register_t)tdi->pdev, state);
	if (ret != RMI_SUCCESS) {
		return -1;
	}
	return 0;
}

static bool is_host_tdi_pdev_state(struct host_tdi *tdi, u_register_t exp_state)
{
	u_register_t cur_state;

	if (host_tdi_pdev_get_state(tdi, &cur_state) != 0) {
		return false;
	}

	if (cur_state != exp_state) {
		return false;
	}

	return true;
}

static int host_tdi_pdev_create(struct host_tdi *tdi)
{
	struct rmi_pdev_params *pdev_params;
	u_register_t ret;
	uint32_t i;

	pdev_params = (struct rmi_pdev_params *)page_alloc(PAGE_SIZE);
	memset(pdev_params, 0, GRANULE_SIZE);

	pdev_params->flags = tdi->pdev_flags;
	pdev_params->cert_id = tdi->cert_slot_id;
	pdev_params->pdev_id = tdi->bdf;
	pdev_params->num_aux = tdi->pdev_aux_num;
	pdev_params->hash_algo = tdi->pdev_hash_algo;
	for (i = 0; i < tdi->pdev_aux_num; i++) {
		pdev_params->aux[i] = (uintptr_t)tdi->pdev_aux[i];
	}

	ret = host_rmi_pdev_create((u_register_t)tdi->pdev,
				   (u_register_t)pdev_params);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_tdi_pdev_set_pubkey(struct host_tdi *tdi)
{
	struct rmi_public_key_params *pubkey_params;
	u_register_t ret;

	pubkey_params = (struct rmi_public_key_params *)page_alloc(PAGE_SIZE);
	memset(pubkey_params, 0, GRANULE_SIZE);

	memcpy(pubkey_params->key, tdi->public_key, tdi->public_key_len);
	memcpy(pubkey_params->metadata, tdi->public_key_metadata,
	       tdi->public_key_metadata_len);
	pubkey_params->key_len = tdi->public_key_len;
	pubkey_params->metadata_len = tdi->public_key_metadata_len;
	pubkey_params->algo = tdi->public_key_sig_algo;

	ret = host_rmi_pdev_set_pubkey((u_register_t)tdi->pdev,
				       (u_register_t)pubkey_params);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_tdi_pdev_stop(struct host_tdi *tdi)
{
	u_register_t ret;

	ret = host_rmi_pdev_stop((u_register_t)tdi->pdev);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_tdi_pdev_destroy(struct host_tdi *tdi)
{
	u_register_t ret;

	ret = host_rmi_pdev_destroy((u_register_t)tdi->pdev);
	if (ret != RMI_SUCCESS) {
		return -1;
	}

	return 0;
}

static int host_tdi_pdev_cache_device_object(struct host_tdi *tdi,
					     uint8_t obj_type,
					     const uint8_t *obj_buf,
					     size_t obj_len)
{
	int rc = -1;

	if (obj_type == DEV_OBJ_CERT) {
		if ((tdi->cert_chain_len + obj_len) > HOST_PDEV_CERT_LEN_MAX) {
			return -1;
		}

		INFO("%s: cache_cert: offset: 0x%lx, len: 0x%lx\n",
		     __func__, tdi->cert_chain_len, obj_len);

		memcpy((void *)(tdi->cert_chain + tdi->cert_chain_len),
		       obj_buf, obj_len);
		tdi->cert_chain_len += obj_len;
		rc = 0;
	} else if (obj_type == DEV_OBJ_MEASUREMENTS) {
		if ((tdi->meas_len + obj_len) > HOST_PDEV_MEAS_LEN_MAX) {
			return -1;
		}

		INFO("%s: cache_meas: offset: 0x%lx, len: 0x%lx\n",
		     __func__, tdi->meas_len, obj_len);

		memcpy((void *)(tdi->meas + tdi->meas_len), obj_buf, obj_len);
		tdi->meas_len += obj_len;
		rc = 0;
	}

	return rc;
}

/* Call RMI PDEV communicate until the target state is reached */
static int host_tdi_pdev_communicate(struct host_tdi *tdi,
				     unsigned char target_state)
{
	int rc;
	u_register_t state;
	u_register_t ret;
	struct rmi_dev_comm_enter *dev_comm_enter;
	struct rmi_dev_comm_exit *dev_comm_exit;
	size_t resp_len;

	dev_comm_enter = &tdi->dev_comm_data->enter;
	dev_comm_exit = &tdi->dev_comm_data->exit;

	dev_comm_enter->status = RMI_DEV_COMM_ENTER_STATUS_NONE;
	dev_comm_enter->resp_len = 0;

	if (host_tdi_pdev_get_state(tdi, &state) != 0) {
		return -1;
	}

	do {
		ret = host_rmi_pdev_communicate((u_register_t)tdi->pdev,
					(u_register_t)tdi->dev_comm_data);
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
			    dev_comm_exit->flags)) {
			uint8_t *obj_buf;
			uint8_t obj_type;

			if (dev_comm_exit->cache_rsp_len == 0 ||
			    (dev_comm_exit->cache_rsp_offset +
			     dev_comm_exit->cache_rsp_len) >
			    GRANULE_SIZE) {
				INFO("Invalid cache offset/length\n");
				rc = -1;
				break;
			}

			/* todo: use 'dev_comm_exit->cache_obj_id' */
			if (state == RMI_PDEV_STATE_NEW) {
				obj_type = DEV_OBJ_CERT;
			} else if (state == RMI_PDEV_STATE_HAS_KEY) {
				/* todo: replace with RMI_PDEV_STATE_READY */
				obj_type = DEV_OBJ_MEASUREMENTS;
			} else {
				rc = -1;
				break;
			}

			obj_buf = (uint8_t *)dev_comm_enter->resp_addr +
				dev_comm_exit->cache_rsp_offset;
			rc = host_tdi_pdev_cache_device_object(tdi, obj_type,
							       obj_buf,
					       dev_comm_exit->cache_rsp_len);
			if (rc != 0) {
				INFO("host_pdev_cache_device_object failed\n");
				rc = -1;
				break;
			}
		}

		/* Send request to spdm responder */
		if (EXTRACT(RMI_DEV_COMM_EXIT_FLAGS_SEND,
			    dev_comm_exit->flags)) {
			uint32_t doe_header;

			/* todo: validate DevCommExit flags */
			if (dev_comm_exit->protocol ==
			    RMI_DEV_COMM_PROTOCOL_SPDM) {
				doe_header = DOE_HEADER_1;
			} else if (dev_comm_exit->protocol ==
				   RMI_DEV_COMM_PROTOCOL_SECURE_SPDM) {
				doe_header = DOE_HEADER_2;
			} else {
				INFO("Invalid dev_comm_exit.protocol\n");
				rc = -1;
				break;
			}

			rc = pcie_doe_communicate(doe_header, tdi->bdf, tdi->doe_cap_base,
					  (void *)dev_comm_enter->req_addr,
						  dev_comm_exit->req_len,
					(void *)dev_comm_enter->resp_addr,
						  &resp_len);

			/*
			 * Set IoEnter args for next pdev_communicate. Upon
			 * success or error call pdev_communicate
			 */
			if (rc == 0) {
				dev_comm_enter->status =
					RMI_DEV_COMM_ENTER_STATUS_RESPONSE;
				dev_comm_enter->resp_len = resp_len;
			} else {
				dev_comm_enter->status =
					RMI_DEV_COMM_ENTER_STATUS_ERROR;
				dev_comm_enter->resp_len = 0;
			}
		}

		rc = host_tdi_pdev_get_state(tdi, &state);
		if (rc != 0) {
			break;
		}
	} while ((state != target_state) && (state != RMI_PDEV_STATE_ERROR));

	return rc;
}

/*
 * Invoke RMI handler to transition PDEV state to 'to_state'
 */
static int host_tdi_pdev_transition(struct host_tdi *tdi, unsigned char to_state)
{
	int rc;

	switch (to_state) {
	case RMI_PDEV_STATE_NEW:
		rc = host_tdi_pdev_create(tdi);
		break;
	case RMI_PDEV_STATE_NEEDS_KEY:
		rc = host_tdi_pdev_communicate(tdi, RMI_PDEV_STATE_NEEDS_KEY);
		break;
	case RMI_PDEV_STATE_HAS_KEY:
		rc = host_tdi_pdev_set_pubkey(tdi);
		break;
	case RMI_PDEV_STATE_READY:
		rc = host_tdi_pdev_communicate(tdi, RMI_PDEV_STATE_READY);
		break;
	case RMI_PDEV_STATE_STOPPING:
		rc = host_tdi_pdev_stop(tdi);
		break;
	case RMI_PDEV_STATE_STOPPED:
		rc = host_tdi_pdev_communicate(tdi, RMI_PDEV_STATE_STOPPED);
		break;
	default:
		rc = -1;
	}

	if (rc != 0) {
		INFO("RMI command failed\n");
		return -1;
	}

	if (!is_host_tdi_pdev_state(tdi, to_state)) {
		ERROR("PDEV state not [%s]\n", pdev_state_str[to_state]);
		return -1;
	}

	return 0;
}

/*
 * Allocate granules needed for a PDEV object like device communication data,
 * response buffer, PDEV AUX granules and memory required to store cert_chain
 */
static int host_tdi_pdev_setup(struct host_tdi *tdi)
{
	u_register_t ret, count;
	int i;

	memset(tdi, 0, sizeof(struct host_tdi));

	/* Allocate granule for PDEV and delegate */
	tdi->pdev = page_alloc(PAGE_SIZE);
	if (tdi->pdev == NULL) {
		return -1;
	}

	memset(tdi->pdev, 0, GRANULE_SIZE);
	ret = host_rmi_granule_delegate((u_register_t)tdi->pdev);
	if (ret != RMI_SUCCESS) {
		ERROR("PDEV delegate failed 0x%lx\n", ret);
		return -1;
	}

	/*
	 * Off chip PCIe device - set flags as non coherent device protected by
	 * end to end IDE, with SPDM.
	 */
	tdi->pdev_flags = (INPLACE(RMI_PDEV_FLAGS_SPDM, RMI_PDEV_SPDM_TRUE) |
			   INPLACE(RMI_PDEV_FLAGS_IDE, RMI_PDEV_IDE_TRUE) |
			   INPLACE(RMI_PDEV_FLAGS_COHERENT,
				   RMI_PDEV_COHERENT_FALSE));

	/* Get num of aux granules required for this PDEV */
	ret = host_rmi_pdev_aux_count(tdi->pdev_flags, &count);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_pdev_aux_count() failed 0x%lx\n", ret);
		goto err_undelegate_pdev;
	}
	tdi->pdev_aux_num = count;

	/* Allocate aux granules for PDEV and delegate */
	INFO("PDEV create requires %u aux pages\n", tdi->pdev_aux_num);
	for (i = 0; i < tdi->pdev_aux_num; i++) {
		void *pdev_aux = page_alloc(PAGE_SIZE);

		if (pdev_aux == NULL) {
			goto err_undelegate_pdev_aux;
		}

		ret = host_rmi_granule_delegate((u_register_t)pdev_aux);
		if (ret != RMI_SUCCESS) {
			ERROR("Aux granule delegate failed 0x%lx\n", ret);
			goto err_undelegate_pdev_aux;
		}

		tdi->pdev_aux[i] = pdev_aux;
	}

	/* Allocate dev_comm_data and send/recv buffer for Dev communication */
	tdi->dev_comm_data = (struct rmi_dev_comm_data *)page_alloc(PAGE_SIZE);
	if (tdi->dev_comm_data == NULL) {
		goto err_undelegate_pdev_aux;
	}

	memset(tdi->dev_comm_data, 0, sizeof(struct rmi_dev_comm_data));

	tdi->dev_comm_data->enter.req_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (tdi->dev_comm_data->enter.req_addr == 0UL) {
		goto err_undelegate_pdev_aux;
	}

	tdi->dev_comm_data->enter.resp_addr = (unsigned long)
		page_alloc(PAGE_SIZE);
	if (tdi->dev_comm_data->enter.resp_addr == 0UL) {
		goto err_undelegate_pdev_aux;
	}

	/* Allocate buffer to cache device certificate */
	tdi->cert_slot_id = 0;
	tdi->cert_chain = (uint8_t *)page_alloc(HOST_PDEV_CERT_LEN_MAX);
	tdi->cert_chain_len = 0;
	if (tdi->cert_chain == NULL) {
		goto err_undelegate_pdev_aux;
	}

	/* Allocate buffer to store extracted public key */
	tdi->public_key = (void *)page_alloc(PAGE_SIZE);
	if (tdi->public_key == NULL) {
		goto err_undelegate_pdev_aux;
	}
	tdi->public_key_len = PAGE_SIZE;

	/* Allocate buffer to store public key metadata */
	tdi->public_key_metadata = (void *)page_alloc(PAGE_SIZE);
	if (tdi->public_key_metadata == NULL) {
		goto err_undelegate_pdev_aux;
	}
	tdi->public_key_metadata_len = PAGE_SIZE;

	/* Allocate buffer to cache device measurements */
	tdi->meas = (uint8_t *)page_alloc(HOST_PDEV_MEAS_LEN_MAX);
	tdi->meas_len = 0;
	if (tdi->meas == NULL) {
		goto err_undelegate_pdev_aux;
	}

	/* Set algorithm to use for device digests */
	tdi->pdev_hash_algo = RMI_HASH_SHA_512;

	return 0;

err_undelegate_pdev_aux:
	/* Undelegate all the delegated pages */
	for (int i = 0; i < tdi->pdev_aux_num; i++) {
		if (tdi->pdev_aux[i]) {
			host_rmi_granule_undelegate((u_register_t)
						    tdi->pdev_aux[i]);
		}
	}

err_undelegate_pdev:
	host_rmi_granule_undelegate((u_register_t)tdi->pdev);

	return -1;
}

/*
 * Stop PDEV and ternimate secure session and call PDEV destroy
 */
static int host_tdi_pdev_reclaim(struct host_tdi *tdi)
{
	int rc;

	/* Move the device to STOPPING state */
	rc = host_tdi_pdev_transition(tdi, RMI_PDEV_STATE_STOPPING);
	if (rc != 0) {
		INFO("PDEV transition: to PDEV_STATE_STOPPING failed\n");
		return -1;
	}

	/* Do pdev_communicate to terminate secure session */
	rc = host_tdi_pdev_transition(tdi, RMI_PDEV_STATE_STOPPED);
	if (rc != 0) {
		INFO("PDEV transition: to PDEV_STATE_STOPPED failed\n");
		return -1;
	}

	rc = host_tdi_pdev_destroy(tdi);
	if (rc != 0) {
		INFO("PDEV transition: to STATE_NULL failed\n");
		return -1;
	}

	/* Undelegate all the delegated pages */
	for (int i = 0; i < tdi->pdev_aux_num; i++) {
		host_rmi_granule_undelegate((u_register_t)tdi->pdev_aux[i]);
	}
	host_rmi_granule_undelegate((u_register_t)tdi->pdev);

	return rc;
}

/*
 * This invokes various RMI calls related to PDEV management that does
 * PDEV create/communicate/set_key/abort/stop/destroy on a device.
 */
test_result_t host_test_rmi_pdev_calls(void)
{
	u_register_t rmi_feat_reg0;
	uint32_t pdev_bdf, doe_cap_base;
	struct host_tdi *tdi;
	uint8_t public_key_algo;
	int ret, rc;

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);
	SKIP_TEST_IF_DOE_NOT_SUPPORTED(pdev_bdf, doe_cap_base);

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	tdi = &g_tdi;

	/* Allocate granules. Skip DA ABIs if host_pdev_setup fails */
	rc = host_tdi_pdev_setup(tdi);
	if (rc == -1) {
		INFO("host_pdev_setup failed. skipping DA ABIs...\n");
		return TEST_RESULT_SKIPPED;
	}

	/* todo: move to tdi_pdev_setup */
	tdi->bdf = pdev_bdf;
	tdi->doe_cap_base = doe_cap_base;

	/* Call rmi_pdev_create to transition PDEV to STATE_NEW */
	rc = host_tdi_pdev_transition(tdi, RMI_PDEV_STATE_NEW);
	if (rc != 0) {
		ERROR("PDEV transition: NULL -> STATE_NEW failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Call rmi_pdev_communicate to transition PDEV to NEEDS_KEY */
	rc = host_tdi_pdev_transition(tdi, RMI_PDEV_STATE_NEEDS_KEY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_NEW -> PDEV_NEEDS_KEY failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Get public key. Verifying cert_chain not done by host but by Realm? */
	rc = host_get_public_key_from_cert_chain(tdi->cert_chain,
						 tdi->cert_chain_len,
						 tdi->public_key,
						 &tdi->public_key_len,
						 tdi->public_key_metadata,
						 &tdi->public_key_metadata_len,
						 &public_key_algo);
	if (rc != 0) {
		ERROR("Get public key failed\n");
		return TEST_RESULT_FAIL;
	}

	if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
		tdi->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P256;
	} else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
		tdi->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P384;
	} else {
		tdi->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_RSASSA_3072;
	}
	INFO("DEV public key len/sig_algo: %ld/%d\n", tdi->public_key_len,
	     tdi->public_key_sig_algo);

	/* Call rmi_pdev_set_key transition PDEV to HAS_KEY */
	rc = host_tdi_pdev_transition(tdi, RMI_PDEV_STATE_HAS_KEY);
	if (rc != 0) {
		INFO("PDEV transition: PDEV_NEEDS_KEY -> PDEV_HAS_KEY failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Call rmi_pdev_comminucate to transition PDEV to READY state */
	rc = host_tdi_pdev_transition(tdi, RMI_PDEV_STATE_READY);
	if (rc != 0) {
		INFO("PDEV transition: PDEV_HAS_KEY -> PDEV_READY failed\n");
		return TEST_RESULT_FAIL;
	}

	host_tdi_pdev_reclaim(tdi);

	return TEST_RESULT_SUCCESS;
}


/*
 * This function invokes PDEV_CREATE on TRP and tries to test
 * the EL3 RMM-EL3 IDE KM interface. Will be skipped on TF-RMM.
 */
test_result_t host_realm_test_root_port_key_management(void)
{
	struct host_tdi *tdi;
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

	tdi = &g_tdi;

	/*
	 * Call rmi_pdev_create with invalid pdev, expect an error
	 * to be returned from TRP.
	 */
	ret = host_tdi_pdev_create(tdi);
	if (ret != 0) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI_PDEV_CREATE did not return error as expected\n");
	return TEST_RESULT_FAIL;
}
