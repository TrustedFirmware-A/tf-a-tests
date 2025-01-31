/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <assert.h>
#include <cassert.h>
#include <host_realm_mem_layout.h>
#include <host_realm_rmi.h>
#include <host_shared_data.h>

CASSERT(NS_REALM_SHARED_MEM_SIZE > (MAX_REC_COUNT * MAX_PLANE_COUNT *
				sizeof(host_shared_data_t)),
		too_small_realm_shared_mem_size);

/*
 * Return shared buffer pointer mapped as host_shared_data_t structure
 */
host_shared_data_t *host_get_shared_structure(struct realm *realm_ptr,
		unsigned int plane_num, unsigned int rec_num)
{
	host_shared_data_arr_t host_shared_data;

	assert(realm_ptr != NULL);
	assert(rec_num < MAX_REC_COUNT);
	host_shared_data = (host_shared_data_arr_t)realm_ptr->host_shared_data;
	return (host_shared_data_t *)&(*host_shared_data)[plane_num][rec_num];
}

/*
 * Set data to be shared from Host to realm
 */
void host_shared_data_set_host_val(struct realm *realm_ptr,
		unsigned int plane_num, unsigned int rec_num, uint8_t index, u_register_t val)
{
	host_shared_data_arr_t host_shared_data;

	assert(realm_ptr != NULL);
	assert(rec_num < MAX_REC_COUNT);
	assert(index < MAX_DATA_SIZE);
	host_shared_data = (host_shared_data_arr_t)realm_ptr->host_shared_data;
	(*host_shared_data)[plane_num][rec_num].host_param_val[index] = val;
}

/*
 * Return data shared by realm in realm_out_val.
 */
u_register_t host_shared_data_get_realm_val(struct realm *realm_ptr,
		unsigned int plane_num, unsigned int rec_num, uint8_t index)
{
	host_shared_data_arr_t host_shared_data;

	assert(realm_ptr != NULL);
	assert(rec_num < MAX_REC_COUNT);
	assert(index < MAX_DATA_SIZE);
	host_shared_data = (host_shared_data_arr_t)realm_ptr->host_shared_data;
	return (*host_shared_data)[plane_num][rec_num].realm_out_val[index];
}

/*
 * Set command to be send from Host to realm
 */
void host_shared_data_set_realm_cmd(struct realm *realm_ptr,
		uint8_t cmd, unsigned int plane_num, unsigned int rec_num)
{
	host_shared_data_arr_t host_shared_data;

	assert(realm_ptr != NULL);
	assert(rec_num < MAX_REC_COUNT);
	host_shared_data = (host_shared_data_arr_t)realm_ptr->host_shared_data;
	(*host_shared_data)[plane_num][rec_num].realm_cmd = cmd;
}

