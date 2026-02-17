/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef HOST_REALM_HELPER_H
#define HOST_REALM_HELPER_H

#include <stdlib.h>
#include <arch_features.h>
#include <debug.h>
#include <host_realm_rmi.h>
#include <spinlock.h>
#include <tftf_lib.h>

/*
 * Parameters for realm creation helpers.
 * Note: This is different from rmi_realm_params. This structure contains only
 * the mandatory fields required by the helper functions. Other fields in
 * rmi_realm_params are populated accordingly by the helpers.
 */
struct test_realm_params {
	/* Mandatory fields */
	u_register_t realm_payload_adr;
	const u_register_t *rec_flag;
	unsigned int rec_count;

	/* Optional fields - MBZ if unused */
	unsigned int num_aux_planes;
	u_register_t s2sz;
	long sl;
	bool lpa2;
	bool sve;
	unsigned int sve_vl;
	bool pmu;
	unsigned int pmu_num_ctrs;
	bool da;
	bool shared_mec;
	bool rtt_tree_single;
	bool rtt_s2ap_encoding_indirect;
};

/*
 * Creates realm, initializes heap and creates RTTs
 */
bool host_prepare_realm_payload(struct realm *realm_ptr,
		struct test_realm_params *params);

/*
 * Creates realm, initializes heap, creates RTTs and also
 * Creates recs
 */
bool host_create_realm_payload(struct realm *realm_ptr,
		struct test_realm_params *params);

/*
 * Creates realm, initializes heap, creates RTTs,
 * creates recs and activate realm
 */
bool host_create_activate_realm_payload(struct realm *realm_ptr,
		struct test_realm_params *params);
bool host_destroy_realm(struct realm *realm_ptr);
void host_rec_send_sgi(struct realm *realm_ptr,
		unsigned int sgi, unsigned int rec_num);
bool host_enter_realm_execute(struct realm *realm_ptr, uint8_t cmd,
		int test_exit_reason, unsigned int rec_num);
test_result_t host_cmp_result(void);
void realm_print_handler(struct realm *realm_ptr, unsigned int plane_num, unsigned int rec_num);
bool host_ipa_is_ns(u_register_t addr, uint8_t s2sz);

/*
 * This functions sets the shared data args needed for entering aux plane,
 * using REALM_ENTER_PLANE_N_CMD
 */
void host_realm_set_aux_plane_args(struct realm *realm_ptr,
		unsigned int plane_num, unsigned int rec_num);


/* Function to check if Planes feature is supoprted */
static inline bool are_planes_supported(void)
{
	u_register_t feature_flag;

	/* Read Realm Feature Reg 3 */
	if (host_rmi_features(RMI_FEATURE_REGISTER_3_INDEX, &feature_flag) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	if (EXTRACT(RMI_FEATURE_REGISTER_3_MAX_NUM_AUX_PLANES, feature_flag) > 0UL) {
		return true;
	}

	return false;
}

/* Function to check if S2POE feature/ Single RTT is supported for multiple planes */
static inline bool is_single_rtt_supported(void)
{
	u_register_t feature_flag;

	/* Read Realm Feature Reg 0 */
	if (host_rmi_features(RMI_FEATURE_REGISTER_3_INDEX, &feature_flag) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	if (EXTRACT(RMI_FEATURE_REGISTER_3_RTT_PLANE, feature_flag) != RMI_PLANE_RTT_AUX) {
		return true;
	}

	return false;
}

/* Test helper to retrieve a test MECID */
static inline unsigned short get_test_mecid(void)
{
	static bool inited = false;
	static unsigned short max_mecid = 0, current_mecid;
	static spinlock_t lock;
	unsigned long feat_reg1;

	spin_lock(&lock);

	if (!inited) {
		if (is_feat_mec_supported() &&
				(host_rmi_features(RMI_FEATURE_REGISTER_1_INDEX,
				&feat_reg1) == 0UL &&
				feat_reg1 != 0UL)) {
			max_mecid = (unsigned short)feat_reg1;
		}

		inited = true;
	}
	if(max_mecid != 0) {
		current_mecid++;
		current_mecid %= max_mecid;
	}
	spin_unlock(&lock);
	return current_mecid;

}

/* Handle REC exit due to VDEV request */
void host_do_vdev_complete(u_register_t rec_ptr, unsigned long vdev_id);

/* Handle REC exit due to VDEV communication */
void host_do_vdev_communicate(struct realm *realm, u_register_t vdev_ptr);

#endif /* HOST_REALM_HELPER_H */
