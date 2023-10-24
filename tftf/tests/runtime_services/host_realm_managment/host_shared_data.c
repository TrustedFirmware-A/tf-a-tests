/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <assert.h>
#include <cassert.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>

static host_shared_data_t *host_shared_data = (host_shared_data_t *) NS_REALM_SHARED_MEM_BASE;

/*
 * Currently we support only creation of a single Realm in TFTF.
 * Hence we can assume that Shared area should be sufficient for all
 * the RECs of this Realm.
 * TODO: This API will need to change for multi realm support.
 */
CASSERT(NS_REALM_SHARED_MEM_SIZE > (MAX_REC_COUNT * sizeof(host_shared_data_t)),
		too_small_realm_shared_mem_size);

/*
 * Return shared buffer pointer mapped as host_shared_data_t structure
 */
host_shared_data_t *host_get_shared_structure(unsigned int rec_num)
{
	assert(rec_num < MAX_REC_COUNT);
	return &host_shared_data[rec_num];
}

/*
 * Set data to be shared from Host to realm
 */
void host_shared_data_set_host_val(unsigned int rec_num, uint8_t index, u_register_t val)
{
	assert(rec_num < MAX_REC_COUNT);
	assert(index < MAX_DATA_SIZE);
	host_shared_data[rec_num].host_param_val[index] = val;
}

/*
 * Return data shared by realm in realm_out_val.
 */
u_register_t host_shared_data_get_realm_val(unsigned int rec_num, uint8_t index)
{
	assert(rec_num < MAX_REC_COUNT);
	assert(index < MAX_DATA_SIZE);
	return host_shared_data[rec_num].realm_out_val[index];
}

/*
 * Set command to be send from Host to realm
 */
void host_shared_data_set_realm_cmd(uint8_t cmd, unsigned int rec_num)
{
	assert(rec_num < MAX_REC_COUNT);
	host_shared_data[rec_num].realm_cmd = cmd;
}

