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

bool host_create_realm_payload(u_register_t realm_payload_adr,
		u_register_t plat_mem_pool_adr,
		u_register_t plat_mem_pool_size,
		u_register_t realm_pages_size,
		u_register_t feature_flag,
		const u_register_t *rec_flag,
		unsigned int rec_count);
bool host_create_shared_mem(
		u_register_t ns_shared_mem_adr,
		u_register_t ns_shared_mem_size);
bool host_destroy_realm(void);
void host_rec_send_sgi(unsigned int sgi, unsigned int rec_num);
bool host_enter_realm_execute(uint8_t cmd, struct realm **realm_ptr,
			      unsigned int test_exit_reason,
			      unsigned int rec_num);
test_result_t host_cmp_result(void);
void realm_print_handler(unsigned int rec_num);

#endif /* HOST_REALM_HELPER_H */
