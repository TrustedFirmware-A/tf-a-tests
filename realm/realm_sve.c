/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <lib/extensions/sve.h>

#include <host_realm_sve.h>
#include <host_shared_data.h>

/* Returns the maximum supported VL. This test is called only by sve Realm */
bool test_realm_sve_rdvl(void)
{
	host_shared_data_t *sd = realm_get_shared_structure();
	struct sve_cmd_rdvl *output;

	assert(is_armv8_2_sve_present());

	output = (struct sve_cmd_rdvl *)sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_rdvl));

	sve_config_vq(SVE_VQ_ARCH_MAX);
	output->rdvl = sve_vector_length_get();

	return true;
}

/*
 * Reads and returns the ID_AA64PFR0_EL1 and ID_AA64ZFR0_EL1 registers
 * This test could be called from sve or non-sve Realm
 */
bool test_realm_sve_read_id_registers(void)
{
	host_shared_data_t *sd = realm_get_shared_structure();
	struct sve_cmd_id_regs *output;

	output = (struct sve_cmd_id_regs *)sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_id_regs));

	realm_printf("Realm: reading ID registers: ID_AA64PFR0_EL1, "
		    " ID_AA64ZFR0_EL1\n");
	output->id_aa64pfr0_el1 = read_id_aa64pfr0_el1();
	output->id_aa64zfr0_el1 = read_id_aa64zfr0_el1();

	return true;
}

/*
 * Probes all VLs and return the bitmap with the bit set for each corresponding
 * valid VQ. This test is called only by sve Realm
 */
bool test_realm_sve_probe_vl(void)
{
	host_shared_data_t *sd = realm_get_shared_structure();
	struct sve_cmd_probe_vl *output;

	assert(is_armv8_2_sve_present());

	output = (struct sve_cmd_probe_vl *)&sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_probe_vl));

	/* Probe all VLs */
	output->vl_bitmap = sve_probe_vl(SVE_VQ_ARCH_MAX);

	return true;
}
