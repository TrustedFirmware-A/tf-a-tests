/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include <arch_helpers.h>
#include <debug.h>
#include <events.h>
#include <heap/page_alloc.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_rmi.h>
#include <host_shared_data.h>
#include <platform.h>
#include <plat_topology.h>
#include <power_management.h>
#include <realm_def.h>
#include <test_helpers.h>
#include <xlat_tables_v2.h>

#define RMI_EXIT(id)	\
	[RMI_EXIT_##id] = #id

const char *rmi_exit[] = {
	RMI_EXIT(SYNC),
	RMI_EXIT(IRQ),
	RMI_EXIT(FIQ),
	RMI_EXIT(PSCI),
	RMI_EXIT(RIPAS_CHANGE),
	RMI_EXIT(HOST_CALL),
	RMI_EXIT(SERROR),
	RMI_EXIT(S2AP_CHANGE),
	RMI_EXIT(VDEV_REQUEST),
	RMI_EXIT(VDEV_MAP),
	RMI_EXIT(VDEV_P2P_BINDING)
};

/* Bitmap to track allocated realm IDs */
static unsigned int realm_id_bitmap;
static u_register_t rmm_feat_reg0;
static u_register_t rmm_feat_reg2;
static u_register_t rmm_feat_reg3;
static bool is_rmm_active;

/*
 * Ensure RMM is activated. Safe to call multiple times; activation happens
 * only once. Returns true on success, false on failure.
 */
bool host_rmm_activate(void)
{
	if (!is_rmm_active) {
		if (host_rmi_rmm_activate() != REALM_SUCCESS) {
			ERROR("host_rmi_rmm_activate failed\n");
			return false;
		}
		is_rmm_active = true;
	}
	return true;
}

/*
 * The function handler to print the Realm logged buffer,
 * executed by the secondary core
 */
void realm_print_handler(struct realm *realm_ptr, unsigned int plane_num, unsigned int rec_num)
{
	size_t str_len = 0UL;
	host_shared_data_t *host_shared_data;
	char *log_buffer;

	assert(realm_ptr != NULL);
	host_shared_data = host_get_shared_structure(realm_ptr, plane_num, rec_num);
	log_buffer = (char *)host_shared_data->log_buffer;
	str_len = strlen((const char *)log_buffer);

	/*
	 * Read Realm message from shared printf location and print
	 * them using UART
	 */
	if (str_len != 0UL) {
		/* Avoid memory overflow */
		log_buffer[MAX_BUF_SIZE - 1] = 0U;
		mp_printf("[Realm %u][Plane %u][Rec %u]: %s", realm_ptr->realm_id,
				plane_num, rec_num, log_buffer);
		(void)memset((char *)log_buffer, 0, MAX_BUF_SIZE);
	}
}

static bool host_enter_realm(struct realm *realm_ptr,
			     u_register_t *exit_reason,
			     unsigned int *host_call_result,
			     unsigned int rec_num)
{
	u_register_t ret;

	if (!realm_ptr->payload_created) {
		ERROR("%s() failed\n", "payload_created");
		return false;
	}
	if (!realm_ptr->shared_mem_created) {
		ERROR("%s() failed\n", "shared_mem_created");
		return false;
	}

	/* Enter Realm */
	ret = host_realm_rec_enter(realm_ptr, exit_reason, host_call_result, rec_num);
	if (ret != REALM_SUCCESS) {
		ERROR("%s() failed, ret=%lx\n", "host_realm_rec_enter", ret);
		return false;
	}

	return true;
}

static bool host_create_shared_mem(struct realm *realm_ptr)
{
	u_register_t ns_shared_mem_adr = (u_register_t)page_alloc(NS_REALM_SHARED_MEM_SIZE);

	if (ns_shared_mem_adr == 0U) {
		ERROR("Failed to alloc NS buffer\n");
		return false;
	}

	/* RTT map NS shared region */
	if (host_realm_map_ns_shared(realm_ptr, ns_shared_mem_adr,
				NS_REALM_SHARED_MEM_SIZE) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_map_ns_shared");
		realm_ptr->shared_mem_created = false;
		return false;
	}

	memset((void *)ns_shared_mem_adr, 0, (size_t)NS_REALM_SHARED_MEM_SIZE);
	realm_ptr->host_shared_data = ns_shared_mem_adr;
	realm_ptr->shared_mem_created = true;

	return true;
}

static bool validate_realm_params(struct realm *realm_ptr,
				   struct test_realm_params *params,
				   u_register_t feat_reg0,
				   u_register_t feat_reg3)
{
	long sl;

	/* Zero out realm_ptr in the beginning */
	(void)memset((char *)realm_ptr, 0U, sizeof(struct realm));

	if (params == NULL) {
		ERROR("params is NULL\n");
		return false;
	}

	/* Activate RMM */
	if (!host_rmm_activate()) {
		return false;
	}

	(void)memset((char *)realm_ptr, 0U, sizeof(struct realm));

	if (params->realm_payload_adr == TFTF_BASE) {
		ERROR("realm_payload_adr should be greater than TFTF_BASE\n");
		return false;
	}

	if (params->rec_count > MAX_REC_COUNT) {
		ERROR("Invalid rec_count: %u\n", params->rec_count);
		return false;
	}

	if (params->num_aux_planes > MAX_AUX_PLANE_COUNT) {
		ERROR("Invalid aux plane count: %u\n", params->num_aux_planes);
		return false;
	}

	for (unsigned int i = 0U; i < params->rec_count; i++) {
		if (params->rec_flag[i] != RMI_RUNNABLE &&
		    params->rec_flag[i] != RMI_NOT_RUNNABLE) {
			ERROR("Invalid rec_flag[%u]: 0x%lx\n", i, params->rec_flag[i]);
			return false;
		}
	}

	/* Set default s2sz if not specified */
	if (params->s2sz == 0U) {
		unsigned int s2sz = EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ, feat_reg0);

		if (!(params->lpa2)) {
			params->s2sz =
				(s2sz >= MAX_IPA_BITS ? MAX_IPA_BITS : s2sz);
		} else {
			params->s2sz = s2sz;
		}
	}

	/* Fail if IPA bits > implemented size */
	if (params->s2sz > EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ, feat_reg0)) {
		ERROR("Invalid s2sz: %lu\n", params->s2sz);
		return false;
	}

	/* Validate and set start_level */
	sl = params->sl;
	if (sl == 0L) {
		sl = params->lpa2 ? RTT_MIN_LEVEL_LPA2 : RTT_MIN_LEVEL;
	}

	if (params->lpa2) {
		if (sl < RTT_MIN_LEVEL_LPA2 || sl > RTT_MAX_LEVEL) {
			ERROR("Invalid start_level: %ld\n", sl);
			return false;
		}
	} else {
		if (sl < RTT_MIN_LEVEL || sl > RTT_MAX_LEVEL) {
			ERROR("Invalid start_level: %ld\n", sl);
			return false;
		}
	}

	/* Validate RTT tree configuration for multiple planes */
	if (params->num_aux_planes > 0U && params->rtt_tree_single) {
		if (EXTRACT(RMI_FEATURE_REGISTER_3_RTT_PLANE, feat_reg3) == RMI_PLANE_RTT_AUX) {
			ERROR("Single RTT for multiple planes not supported by RMM\n");
			return false;
		}
	}

	/* Validate S2AP encoding configuration */
	if (params->rtt_s2ap_encoding_indirect) {
		if ((feat_reg3 & RMI_FEATURE_REGISTER_3_RTT_S2AP_INDIRECT) == 0UL) {
			ERROR("S2AP Indirect Encoding not supported\n");
			return false;
		}
	}

	/* Assign realm_ptr members */

	/* s2sz is already set by validation above */
	realm_ptr->s2sz = params->s2sz;

	/* start_level is already validated above */
	realm_ptr->start_level = sl;

	/* Configure RTT tree options */
	realm_ptr->rtt_tree_single = (params->num_aux_planes > 0U && params->rtt_tree_single);
	realm_ptr->rtt_s2ap_enc_indirect = params->rtt_s2ap_encoding_indirect;

	/* Store PMU configuration */
	realm_ptr->pmu_enabled = params->pmu;
	realm_ptr->pmu_num_ctrs = params->pmu ? params->pmu_num_ctrs : 0U;

	/* Store SVE configuration */
	realm_ptr->sve_enabled = params->sve;
	realm_ptr->sve_vl = params->sve ? params->sve_vl : 0U;

	/* Use default number of breakpoints from RMM features */
	realm_ptr->num_bps = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_BPS, feat_reg0);

	/* Use default number of watchpoints from RMM features */
	realm_ptr->num_wps = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_WPS, feat_reg0);

	realm_ptr->rec_count = params->rec_count;
	for (unsigned int i = 0U; i < params->rec_count; i++) {
		realm_ptr->rec_flag[i] = params->rec_flag[i];
	}

	/* Store FEAT_LPA2 configuration */
	realm_ptr->lpa2 = params->lpa2;

	/* Store FEAT_DA configuration */
	realm_ptr->da_enabled = params->da;
	realm_ptr->shared_mec = params->shared_mec;

	realm_ptr->num_aux_planes = params->num_aux_planes;
	realm_ptr->ats_plane = 0U;

	return true;
}

bool host_prepare_realm_payload(struct realm *realm_ptr,
		       struct test_realm_params *params)
{
	/* Read Realm Feature Registers once */
	if (host_rmi_features(RMI_FEATURE_REGISTER_0_INDEX, &rmm_feat_reg0) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	if (host_rmi_features(RMI_FEATURE_REGISTER_2_INDEX, &rmm_feat_reg2) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	if (host_rmi_features(RMI_FEATURE_REGISTER_3_INDEX, &rmm_feat_reg3) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	/* Validate parameters (including feature-dependent checks) and assign realm_ptr members */
	if (!validate_realm_params(realm_ptr, params, rmm_feat_reg0, rmm_feat_reg3)) {
		return false;
	}

	/* Create Realm */
	if (host_realm_create(realm_ptr) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_create");
		return false;
	}

	/* RTT map Realm image */
	if (host_realm_map_payload_image(realm_ptr, params->realm_payload_adr) !=
			REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_map_payload_image");
		goto destroy_realm;
	}

	if (!host_create_shared_mem(realm_ptr)) {
		ERROR("%s() failed\n", "host_create_shared_mem");
		goto destroy_realm;
	}

	realm_ptr->payload_created = true;

	/* Allocate unique realm ID after successful creation */
	for (unsigned int i = 0U; i < MAX_REALM_COUNT; i++) {
		if ((realm_id_bitmap & (1U << i)) == 0U) {
			realm_ptr->realm_id = i;
			/* @TODO make thread-safe */
			realm_id_bitmap |= (1U << i);
			break;
		}
	}

	return true;

	/* Free test resources */
destroy_realm:
	if (host_realm_destroy(realm_ptr) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_destroy");
	}
	realm_ptr->payload_created = false;

	return false;
}

bool host_create_realm_payload(struct realm *realm_ptr,
		       struct test_realm_params *params)
{
	bool ret;

	ret = host_prepare_realm_payload(realm_ptr,
			params);
	if (!ret) {
		goto destroy_realm;
	} else {
		/* Create REC */
		if (host_realm_rec_create(realm_ptr) != REALM_SUCCESS) {
			ERROR("%s() failed\n", "host_realm_rec_create");
			goto destroy_realm;
		}

		/*
		 * Realm tests only access the region between TFTF_BASE and Realm Pool,
		 * Change RIPAS of this region to RAM.
		 */
		if (host_realm_init_ipa_state(realm_ptr, realm_ptr->start_level,
					      TFTF_BASE, PAGE_POOL_END) != RMI_SUCCESS) {
			ERROR("%s() failed\n", "host_realm_init_ipa_state");
			goto destroy_realm;
		}
	}
	return true;

destroy_realm:
	if (host_realm_destroy(realm_ptr) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_destroy");
	}
	realm_ptr->payload_created = false;
	return false;
}

bool host_create_activate_realm_payload(struct realm *realm_ptr,
			struct test_realm_params *params)

{
	bool ret;

	ret = host_create_realm_payload(realm_ptr,
			params);
	if (!ret) {
		goto destroy_realm;
	} else {
		/* Activate Realm */
		if (host_realm_activate(realm_ptr) != REALM_SUCCESS) {
			ERROR("%s() failed\n", "host_realm_activate");
			goto destroy_realm;
		}
	}
	return true;

destroy_realm:
	if (host_realm_destroy(realm_ptr) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_destroy");
	}
	realm_ptr->payload_created = false;
	return false;
}

bool host_destroy_realm(struct realm *realm_ptr)
{
	if (!realm_ptr->payload_created) {
		ERROR("%s() failed\n", "payload_created");
		return false;
	}

	realm_ptr->payload_created = false;

	if (host_realm_destroy(realm_ptr) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_destroy");
		return false;
	}

	/* Free realm ID */
	/* @TODO make thread-safe */
	realm_id_bitmap &= ~(1U << realm_ptr->realm_id);

	memset((char *)realm_ptr, 0U, sizeof(struct realm));

	return true;
}

/*
 * Enter Realm and run command passed in 'cmd' and compare the exit reason with
 * 'test_exit_reason'.
 *
 * Returns:
 *	true:  On success. 'test_exit_reason' matches Realm exit reason. For
 *	       RMI_EXIT_HOST_CALL exit reason, the 'host_call_result' is
 *	       TEST_RESULT_SUCCESS.
 *	false: On error.
 */
bool host_enter_realm_execute(struct realm *realm_ptr,
			      uint8_t cmd,
			      int test_exit_reason,
			      unsigned int rec_num)
{
	u_register_t realm_exit_reason = RMI_EXIT_INVALID;
	unsigned int host_call_result = TEST_RESULT_FAIL;

	if (realm_ptr == NULL || realm_ptr->payload_created == false) {
		return false;
	}

	if (test_exit_reason >= RMI_EXIT_INVALID) {
		ERROR("Invalid RmiRecExitReason\n");
		return false;
	}

	if (rec_num >= realm_ptr->rec_count) {
		ERROR("Invalid Rec Count\n");
		return false;
	}
	host_shared_data_set_realm_cmd(realm_ptr, cmd, PRIMARY_PLANE_ID, rec_num);
	if (!host_enter_realm(realm_ptr, &realm_exit_reason, &host_call_result, rec_num)) {
		return false;
	}

	if (test_exit_reason == realm_exit_reason) {
		if (realm_exit_reason != RMI_EXIT_HOST_CALL) {
			return true;
		} else if (host_call_result == TEST_RESULT_SUCCESS) {
			return true;
		}
	}

	if (realm_exit_reason < RMI_EXIT_INVALID) {
		if ((realm_exit_reason == RMI_EXIT_HOST_CALL) &&
		    (test_exit_reason == realm_exit_reason)) {
			ERROR("%s(%u) RMI_EXIT_HOST_CALL failed\n", __func__,
			      cmd);
		} else {
			ERROR("%s(%u) Got RMI_EXIT_%s. Expected RMI_EXIT_%s.\n",
			      __func__, cmd, rmi_exit[realm_exit_reason],
			      rmi_exit[test_exit_reason]);
		}
	} else {
		ERROR("%s(%u) Unknown or unsupported RmiRecExitReason: 0x%lx\n",
		__func__, cmd, realm_exit_reason);
	}
	return false;
}

test_result_t host_cmp_result(void)
{
	if (host_rmi_get_cmp_result()) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI registers comparison failed\n");
	return TEST_RESULT_FAIL;
}

/*
 * Returns Host core position for specified Rec
 * Host mpidr is saved on every rec enter
 */
static unsigned int host_realm_find_core_pos_by_rec(struct realm *realm_ptr,
		unsigned int rec_num)
{
	if (rec_num < MAX_REC_COUNT && realm_ptr->run[rec_num] != 0U) {
		return platform_get_core_pos(realm_ptr->host_mpidr[rec_num]);
	}
	return (unsigned int)-1;
}

/*
 * Send SGI on core running specified Rec
 * API can be used to forcefully exit from Realm
 */
void host_rec_send_sgi(struct realm *realm_ptr,
		       unsigned int sgi,
		       unsigned int rec_num)
{
	unsigned int core_pos = host_realm_find_core_pos_by_rec(realm_ptr, rec_num);
	if (core_pos < PLATFORM_CORE_COUNT) {
		tftf_send_sgi(sgi, core_pos);
	}
}

/*
 * Set Args for Aux Plane Enter
 * Realm helpers copy the same realm image for each plane
 * Entrypoint for aux plane = realm.par_base + (plane_num * realm..par_size)
 */
void host_realm_set_aux_plane_args(struct realm *realm_ptr,
		unsigned int plane_num, unsigned int rec_num)
{
	/* Plane Index */
	host_shared_data_set_host_val(realm_ptr, PRIMARY_PLANE_ID, rec_num,
			HOST_ARG1_INDEX, plane_num);

	/* Plane entrypoint */
	host_shared_data_set_host_val(realm_ptr, PRIMARY_PLANE_ID, rec_num, HOST_ARG2_INDEX,
			realm_ptr->par_base + (plane_num * realm_ptr->par_size));
}
