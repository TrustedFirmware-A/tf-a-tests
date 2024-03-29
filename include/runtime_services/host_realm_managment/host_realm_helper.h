/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef HOST_REALM_HELPER_H
#define HOST_REALM_HELPER_H

#include <stdlib.h>
#include <host_realm_rmi.h>
#include <tftf_lib.h>

/*
 * Creates realm, initializes heap and creates RTTs
 */
bool host_prepare_realm_payload(struct realm *realm_ptr,
		u_register_t realm_payload_adr,
		u_register_t plat_mem_pool_adr,
		u_register_t realm_pages_size,
		u_register_t feature_flag,
		const u_register_t *rec_flag,
		unsigned int rec_count);

/*
 * Creates realm, initializes heap, creates RTTs and also
 * Creates recs
 */
bool host_create_realm_payload(struct realm *realm_ptr,
		u_register_t realm_payload_adr,
		u_register_t plat_mem_pool_adr,
		u_register_t realm_pages_size,
		u_register_t feature_flag,
		const u_register_t *rec_flag,
		unsigned int rec_count);

/*
 * Creates realm, initializes heap, creates RTTs,
 * creates recs and activate realm
 */
bool host_create_activate_realm_payload(struct realm *realm_ptr,
		u_register_t realm_payload_adr,
		u_register_t plat_mem_pool_adr,
		u_register_t realm_pages_size,
		u_register_t feature_flag,
		const u_register_t *rec_flag,
		unsigned int rec_count);
bool host_create_shared_mem(struct realm *realm_ptr,
		u_register_t ns_shared_mem_adr,
		u_register_t ns_shared_mem_size);
bool host_destroy_realm(struct realm *realm_ptr);
void host_rec_send_sgi(struct realm *realm_ptr,
		unsigned int sgi, unsigned int rec_num);
bool host_enter_realm_execute(struct realm *realm_ptr, uint8_t cmd,
		int test_exit_reason, unsigned int rec_num);
test_result_t host_cmp_result(void);
void realm_print_handler(struct realm *realm_ptr, unsigned int rec_num);

#endif /* HOST_REALM_HELPER_H */
