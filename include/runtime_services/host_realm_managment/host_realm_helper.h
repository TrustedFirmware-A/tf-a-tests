/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef HOST_REALM_HELPER_H
#define HOST_REALM_HELPER_H

#include <stdlib.h>
#include <debug.h>
#include <host_realm_rmi.h>
#include <tftf_lib.h>

/*
 * Creates realm, initializes heap and creates RTTs
 */
bool host_prepare_realm_payload(struct realm *realm_ptr,
		u_register_t realm_payload_adr,
		u_register_t feature_flag0,
		u_register_t feature_flag1,
		long sl,
		const u_register_t *rec_flag,
		unsigned int rec_count,
		unsigned int num_aux_planes);

/*
 * Creates realm, initializes heap, creates RTTs and also
 * Creates recs
 */
bool host_create_realm_payload(struct realm *realm_ptr,
		u_register_t realm_payload_adr,
		u_register_t feature_flag0,
		u_register_t feature_flag1,
		long sl,
		const u_register_t *rec_flag,
		unsigned int rec_count,
		unsigned int num_aux_planes);

/*
 * Creates realm, initializes heap, creates RTTs,
 * creates recs and activate realm
 */
bool host_create_activate_realm_payload(struct realm *realm_ptr,
		u_register_t realm_payload_adr,
		u_register_t feature_flag0,
		u_register_t feature_flag1,
		long sl,
		const u_register_t *rec_flag,
		unsigned int rec_count,
		unsigned int num_aux_planes);
bool host_destroy_realm(struct realm *realm_ptr);
void host_rec_send_sgi(struct realm *realm_ptr,
		unsigned int sgi, unsigned int rec_num);
bool host_enter_realm_execute(struct realm *realm_ptr, uint8_t cmd,
		int test_exit_reason, unsigned int rec_num);
test_result_t host_cmp_result(void);
void realm_print_handler(struct realm *realm_ptr, unsigned int plane_num, unsigned int rec_num);
bool host_ipa_is_ns(u_register_t addr, u_register_t rmm_feat_reg0);

/*
 * This functions sets the shared data args needed for entering aux plane,
 * using REALM_ENTER_PLANE_N_CMD
 */
void host_realm_set_aux_plane_args(struct realm *realm_ptr, unsigned int plane_num);

static inline bool is_single_rtt_supported(void)
{
	u_register_t feature_flag;

	/* Read Realm Feature Reg 0 */
	if (host_rmi_features(0UL, &feature_flag) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_rmi_features");
		return false;
	}

	if (EXTRACT(RMI_FEATURE_REGISTER_0_PLANE_RTT, feature_flag) != RMI_PLANE_RTT_AUX) {
		return true;
	}

	return false;
}

#endif /* HOST_REALM_HELPER_H */
