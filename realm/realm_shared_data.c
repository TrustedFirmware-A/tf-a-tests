/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <arch_helpers.h>
#include <assert.h>
#include <host_shared_data.h>
#include <realm_def.h>
#include <realm_helpers.h>

/**
 *   @brief    - Returns the base address of the shared region
 *   @param    - Void
 *   @return   - Base address of the shared region
 **/

static host_shared_data_arr_t guest_shared_data;

/*
 * Set guest mapped shared buffer pointer
 */
void realm_set_shared_structure(u_register_t ptr)
{
	guest_shared_data = (host_shared_data_arr_t)ptr;
}

/*
 * Get guest mapped shared buffer pointer
 */
host_shared_data_t *realm_get_my_shared_structure(void)
{
	return &(*guest_shared_data)[realm_get_my_plane_num()][REC_IDX(read_mpidr_el1())];
}

/*
 * Return Host's data at index
 */
u_register_t realm_shared_data_get_my_host_val(uint8_t index)
{
	assert(index < MAX_DATA_SIZE);
	return (*guest_shared_data)[realm_get_my_plane_num()][REC_IDX(read_mpidr_el1())].host_param_val[index];
}

/*
 * Get command sent from Host to this rec
 */
uint8_t realm_shared_data_get_my_realm_cmd(void)
{
	return (*guest_shared_data)[realm_get_my_plane_num()][REC_IDX(read_mpidr_el1())].realm_cmd;
}

/*
 * Set data to be shared from this rec to Host
 */
void realm_shared_data_set_my_realm_val(uint8_t index, u_register_t val)
{
	assert(index < MAX_DATA_SIZE);
	(*guest_shared_data)[realm_get_my_plane_num()][REC_IDX(read_mpidr_el1())].realm_out_val[index] = val;
}

void realm_shared_data_set_plane_n_val(unsigned int plane_num, unsigned int rec_num,
		uint8_t index, u_register_t val)
{
	assert(index < MAX_DATA_SIZE);
	assert(plane_num < MAX_PLANE_COUNT);
	assert(rec_num < MAX_REC_COUNT);
	assert(is_plane0);

	(*guest_shared_data)[plane_num][rec_num].realm_out_val[index] = val;
}

u_register_t realm_shared_data_get_plane_n_val(unsigned int plane_num,
		unsigned int rec_num, uint8_t index)
{
	assert(plane_num < MAX_PLANE_COUNT);
	assert(rec_num < MAX_REC_COUNT);
	assert(is_plane0);

	return (*guest_shared_data)[plane_num][rec_num].realm_out_val[index];
}

u_register_t realm_shared_data_get_plane_n_cmd(unsigned int plane_num,
		unsigned int rec_num, uint8_t index)
{
	assert(plane_num < MAX_PLANE_COUNT);
	assert(rec_num < MAX_REC_COUNT);
	assert(is_plane0);

	return (*guest_shared_data)[plane_num][rec_num].realm_cmd;
}

void realm_shared_data_set_plane_n_cmd(uint8_t cmd, unsigned int plane_num, unsigned int rec_num)
{
	assert(plane_num < MAX_PLANE_COUNT);
	assert(rec_num < MAX_REC_COUNT);
	assert(is_plane0);

	(*guest_shared_data)[plane_num][rec_num].realm_cmd = cmd;
}
