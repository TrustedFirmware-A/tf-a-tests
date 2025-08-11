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
#include <sgi.h>
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
	RMI_EXIT(VDEV_COMM),
	RMI_EXIT(DEV_MEM_MAP)
};

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
		mp_printf("[VMID %u][Rec %u]: %s", plane_num == 0U ? realm_ptr->vmid :
				realm_ptr->aux_vmid[plane_num - 1U], rec_num, log_buffer);
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

bool host_prepare_realm_payload(struct realm *realm_ptr,
			       u_register_t realm_payload_adr,
			       u_register_t feature_flag,
			       u_register_t feature_flag1,
			       long sl,
			       const u_register_t *rec_flag,
			       unsigned int rec_count,
			       unsigned int num_aux_planes,
			       unsigned short mecid)
{
	int8_t value;

	if (realm_payload_adr == TFTF_BASE) {
		ERROR("realm_payload_adr should be grater then TFTF_BASE\n");
		return false;
	}

	memset((char *)realm_ptr, 0U, sizeof(struct realm));

	/* Read Realm Feature Reg 0 */
	if (host_rmi_features(0UL, &realm_ptr->rmm_feat_reg0) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	/* Fail if IPA bits > implemented size */
	if (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ, feature_flag) >
			EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ, realm_ptr->rmm_feat_reg0)) {
		ERROR("%s() failed\n", "Invalid s2sz");
		return false;
	}

	/*
	 * Overwrite s2sz in feature flag if host passed a value
	 * if host passes default 0 use default RMI_FEATURES instead
	 */
	if (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ, feature_flag) != 0U) {
		realm_ptr->rmm_feat_reg0 &= ~MASK(RMI_FEATURE_REGISTER_0_S2SZ);
		realm_ptr->rmm_feat_reg0 |= INPLACE(RMI_FEATURE_REGISTER_0_S2SZ,
				EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ, feature_flag));
	}

	realm_ptr->rtt_tree_single = false;
	realm_ptr->rtt_s2ap_enc_indirect = false;

	if (num_aux_planes > 0U) {
		if ((EXTRACT(RMI_FEATURE_REGISTER_0_PLANE_RTT, feature_flag) ==
					RMI_PLANE_RTT_SINGLE)) {
			if ((EXTRACT(RMI_FEATURE_REGISTER_0_PLANE_RTT, realm_ptr->rmm_feat_reg0) ==
						RMI_PLANE_RTT_AUX)) {
				ERROR("%s() failed\n",
					"Single RTT for multiple planes not support by RMM");
				return false;
			} else {
				realm_ptr->rtt_tree_single = true;
			}
		}
	}

	if ((feature_flag1 & RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT) != 0U) {
		if ((realm_ptr->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_RTT_S2AP_INDIRECT) == 0UL) {
			ERROR("%s() failed\n", "S2AP Indirect Encoding not suported");
			return false;
		} else {
			realm_ptr->rtt_s2ap_enc_indirect = true;
		}
	}

	/* Disable PMU if not required */
	if ((feature_flag & RMI_FEATURE_REGISTER_0_PMU_EN) == 0UL) {
		realm_ptr->rmm_feat_reg0 &= ~RMI_FEATURE_REGISTER_0_PMU_EN;
		realm_ptr->pmu_num_ctrs = 0U;
	} else {
		/* Requested number of event counters */
		realm_ptr->pmu_num_ctrs = EXTRACT(RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS,
							feature_flag);
	}

	/* Disable SVE if not required */
	if ((feature_flag & RMI_FEATURE_REGISTER_0_SVE_EN) == 0UL) {
		realm_ptr->rmm_feat_reg0 &= ~RMI_FEATURE_REGISTER_0_SVE_EN;
		realm_ptr->sve_vl = 0U;
	} else {
		realm_ptr->sve_vl = EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL,
						feature_flag);
	}

	/* Requested number of breakpoints */
	value = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_BPS, feature_flag);
	if (value != 0) {
		realm_ptr->num_bps = (unsigned int)value;
	} else {
		realm_ptr->num_bps = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_BPS,
					realm_ptr->rmm_feat_reg0);
	}

	/* Requested number of watchpoints */
	value = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_WPS, feature_flag);
	if (value != 0) {
		realm_ptr->num_wps = (unsigned int)value;
	} else {
		realm_ptr->num_wps = EXTRACT(RMI_FEATURE_REGISTER_0_NUM_WPS,
					realm_ptr->rmm_feat_reg0);
	}

	/* Set SVE bits from feature_flag */
	realm_ptr->rmm_feat_reg0 &= ~(RMI_FEATURE_REGISTER_0_SVE_EN |
				 MASK(RMI_FEATURE_REGISTER_0_SVE_VL));
	if ((feature_flag & RMI_FEATURE_REGISTER_0_SVE_EN) != 0UL) {
		realm_ptr->rmm_feat_reg0 |= RMI_FEATURE_REGISTER_0_SVE_EN |
				       INPLACE(RMI_FEATURE_REGISTER_0_SVE_VL,
				       EXTRACT(RMI_FEATURE_REGISTER_0_SVE_VL,
						feature_flag));
	}

	/* Assign a MECID to the realm */
	/*
	 * Check if FEAT_MEC is supported and RMM supports MECIDs other than the
	 * default MECID (0) first. Otherwise, assign the default MECID.
	 */
	if (is_feat_mec_supported() &&
			(host_rmi_features(1UL, &realm_ptr->rmm_feat_reg1)
				== REALM_SUCCESS) &&
			((unsigned short)realm_ptr->rmm_feat_reg1 != DEFAULT_MECID)) {
		if (mecid > (unsigned short)realm_ptr->rmm_feat_reg1) {
			ERROR("Invalid MECID: %hu\n. Max MECID is %hu.",
				mecid, (unsigned short)realm_ptr->rmm_feat_reg1);
			return false;
		}

		realm_ptr->mecid = mecid;
	} else {
		realm_ptr->mecid = DEFAULT_MECID;
	}

	if (realm_ptr->rec_count > MAX_REC_COUNT) {
		ERROR("Invalid Rec Count\n");
		return false;
	}
	realm_ptr->rec_count = rec_count;
	for (unsigned int i = 0U; i < rec_count; i++) {
		if (rec_flag[i] == RMI_RUNNABLE ||
				rec_flag[i] == RMI_NOT_RUNNABLE) {
			realm_ptr->rec_flag[i] = rec_flag[i];
		} else {
			ERROR("Invalid Rec Flag\n");
			return false;
		}
	}

	/*
	 * Force FEAT_LPA2 to the selected configuration.
	 */
	if ((feature_flag & RMI_FEATURE_REGISTER_0_LPA2) == 0ULL) {
		realm_ptr->rmm_feat_reg0 &= ~RMI_FEATURE_REGISTER_0_LPA2;
	} else {
		realm_ptr->rmm_feat_reg0 |= RMI_FEATURE_REGISTER_0_LPA2;
	}

	realm_ptr->start_level = sl;

	if (num_aux_planes > MAX_AUX_PLANE_COUNT) {
		ERROR("Invalid aux plane count\n");
		return false;
	}
	realm_ptr->num_aux_planes = num_aux_planes;

	/* Create Realm */
	if (host_realm_create(realm_ptr) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_create");
		return false;
	}

	/* RTT map Realm image */
	if (host_realm_map_payload_image(realm_ptr, realm_payload_adr) !=
			REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_realm_map_payload_image");
		goto destroy_realm;
	}

	if (!host_create_shared_mem(realm_ptr)) {
		ERROR("%s() failed\n", "host_create_shared_mem");
		goto destroy_realm;
	}

	realm_ptr->payload_created = true;

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
			       u_register_t realm_payload_adr,
			       u_register_t feature_flag,
			       u_register_t feature_flag1,
			       long sl,
			       const u_register_t *rec_flag,
			       unsigned int rec_count,
			       unsigned int num_aux_planes,
			       unsigned short mecid)
{
	bool ret;

	ret = host_prepare_realm_payload(realm_ptr,
			realm_payload_adr,
			feature_flag,
			feature_flag1,
			sl,
			rec_flag,
			rec_count,
			num_aux_planes,
			mecid);
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
			u_register_t realm_payload_adr,
			u_register_t feature_flag,
			u_register_t feature_flag1,
			long sl,
			const u_register_t *rec_flag,
			unsigned int rec_count,
			unsigned int num_aux_planes,
			unsigned short mecid)

{
	bool ret;

	ret = host_create_realm_payload(realm_ptr,
			realm_payload_adr,
			feature_flag,
			feature_flag1,
			sl,
			rec_flag,
			rec_count,
			num_aux_planes,
			mecid);
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
