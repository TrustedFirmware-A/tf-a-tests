/*
 * Copyright (c) 2023-2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <stdlib.h>

#include <sync.h>
#include <lib/extensions/fpu.h>
#include <lib/extensions/sve.h>
#include <realm_helpers.h>

#include <host_realm_simd.h>
#include <host_shared_data.h>

#define RL_SVE_OP_ARRAYSIZE		512U
#define SVE_TEST_ITERATIONS		4U

static int rl_sve_op_1[RL_SVE_OP_ARRAYSIZE];
static int rl_sve_op_2[RL_SVE_OP_ARRAYSIZE];

static sve_z_regs_t rl_sve_z_regs_write;
static sve_z_regs_t rl_sve_z_regs_read;

static sve_p_regs_t rl_sve_p_regs_write;
static sve_p_regs_t rl_sve_p_regs_read;

static sve_ffr_regs_t rl_sve_ffr_regs_write;
static sve_ffr_regs_t rl_sve_ffr_regs_read;

static fpu_cs_regs_t rl_fpu_cs_regs_write;
static fpu_cs_regs_t rl_fpu_cs_regs_read;

/* Returns the maximum supported VL. This test is called only by sve Realm */
bool test_realm_sve_rdvl(void)
{
	host_shared_data_t *sd = realm_get_my_shared_structure();
	struct sve_cmd_rdvl *output;

	assert(is_armv8_2_sve_present());

	output = (struct sve_cmd_rdvl *)sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_rdvl));

	sve_config_vq(SVE_VQ_ARCH_MAX);
	output->rdvl = sve_rdvl_1();

	return true;
}

/*
 * Reads and returns the ID_AA64PFR0_EL1 and ID_AA64ZFR0_EL1 registers
 * This test could be called from sve or non-sve Realm
 */
bool test_realm_sve_read_id_registers(void)
{
	host_shared_data_t *sd = realm_get_my_shared_structure();
	struct sve_cmd_id_regs *output;

	output = (struct sve_cmd_id_regs *)sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_id_regs));

	realm_printf("reading ID registers: ID_AA64PFR0_EL1, "
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
	host_shared_data_t *sd = realm_get_my_shared_structure();
	struct sve_cmd_probe_vl *output;

	assert(is_armv8_2_sve_present());

	output = (struct sve_cmd_probe_vl *)&sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_probe_vl));

	/* Probe all VLs */
	output->vl_bitmap = sve_probe_vl(SVE_VQ_ARCH_MAX);

	return true;
}

bool test_realm_sve_ops(void)
{
	int val, i;

	assert(is_armv8_2_sve_present());

	/* get at random value to do sve_subtract */
	val = rand();
	for (i = 0; i < RL_SVE_OP_ARRAYSIZE; i++) {
		rl_sve_op_1[i] = val - i;
		rl_sve_op_2[i] = 1;
	}

	for (i = 0; i < SVE_TEST_ITERATIONS; i++) {
		/* Config Realm with random SVE length */
		sve_config_vq(SVE_GET_RANDOM_VQ);

		/* Perform SVE operations, without world switch */
		sve_subtract_arrays(rl_sve_op_1, rl_sve_op_1, rl_sve_op_2,
				    RL_SVE_OP_ARRAYSIZE);
	}

	/* Check result of SVE operations. */
	for (i = 0; i < RL_SVE_OP_ARRAYSIZE; i++) {
		if (rl_sve_op_1[i] != (val - i - SVE_TEST_ITERATIONS)) {
			realm_printf("SVE ops failed\n");
			return false;
		}
	}

	return true;
}

/* Fill SVE Z registers with known pattern */
bool test_realm_sve_fill_regs(void)
{
	assert(is_armv8_2_sve_present());

	/* Config Realm with max SVE length */
	sve_config_vq(SVE_VQ_ARCH_MAX);

	sve_z_regs_write_rand(&rl_sve_z_regs_write);
	sve_p_regs_write_rand(&rl_sve_p_regs_write);
	sve_ffr_regs_write_rand(&rl_sve_ffr_regs_write);

	/* fpcr, fpsr common registers */
	fpu_cs_regs_write_rand(&rl_fpu_cs_regs_write);

	return true;
}

/* Compare SVE Z registers with last filled in values */
bool test_realm_sve_cmp_regs(void)
{
	bool rc = true;
	uint64_t bit_map;

	assert(is_armv8_2_sve_present());

	memset(&rl_sve_z_regs_read, 0, sizeof(rl_sve_z_regs_read));
	memset(&rl_sve_p_regs_read, 0, sizeof(rl_sve_p_regs_read));
	memset(&rl_sve_ffr_regs_read, 0, sizeof(rl_sve_ffr_regs_read));

	/* Read all SVE registers */
	sve_z_regs_read(&rl_sve_z_regs_read);
	sve_p_regs_read(&rl_sve_p_regs_read);
	sve_ffr_regs_read(&rl_sve_ffr_regs_read);

	/* Compare the read values with last written values */
	bit_map = sve_z_regs_compare(&rl_sve_z_regs_write, &rl_sve_z_regs_read);
	if (bit_map) {
		rc = false;
	}

	bit_map = sve_p_regs_compare(&rl_sve_p_regs_write, &rl_sve_p_regs_read);
	if (bit_map) {
		rc = false;
	}

	bit_map = sve_ffr_regs_compare(&rl_sve_ffr_regs_write,
				       &rl_sve_ffr_regs_read);
	if (bit_map) {
		rc = false;
	}

	/* fpcr, fpsr common registers */
	fpu_cs_regs_read(&rl_fpu_cs_regs_read);
	if (fpu_cs_regs_compare(&rl_fpu_cs_regs_write, &rl_fpu_cs_regs_read)) {
		ERROR("FPCR/FPSR mismatch\n");
		rc = false;
	}

	return rc;
}

/* Check if Realm gets undefined abort when it accesses SVE functionality */
bool test_realm_sve_undef_abort(void)
{
	realm_reset_undef_abort_count();

	/* Install exception handler to catch undefined abort */
	register_custom_sync_exception_handler(&realm_sync_exception_handler);
	(void)sve_rdvl_1();
	unregister_custom_sync_exception_handler();

	return (realm_get_undef_abort_count() != 0U);
}

/* Reads and returns the ID_AA64PFR1_EL1 and ID_AA64SMFR0_EL1 registers */
bool test_realm_sme_read_id_registers(void)
{
	host_shared_data_t *sd = realm_get_my_shared_structure();
	struct sme_cmd_id_regs *output;

	output = (struct sme_cmd_id_regs *)sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sme_cmd_id_regs));

	realm_printf("reading ID registers: ID_AA64PFR1_EL1, "
		    " ID_AA64SMFR0_EL1\n");

	output->id_aa64pfr1_el1 = read_id_aa64pfr1_el1();
	output->id_aa64smfr0_el1 = read_id_aa64smfr0_el1();

	return true;
}

/* Check if Realm gets undefined abort when it access SME functionality */
bool test_realm_sme_undef_abort(void)
{
	realm_reset_undef_abort_count();

	/* Install exception handler to catch undefined abort */
	register_custom_sync_exception_handler(&realm_sync_exception_handler);
	(void)read_svcr();
	unregister_custom_sync_exception_handler();

	return (realm_get_undef_abort_count() != 0U);
}
