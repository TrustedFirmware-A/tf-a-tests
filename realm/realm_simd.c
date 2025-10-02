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

static rsi_plane_run run __aligned(PAGE_SIZE);

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

/*
 * Helper function to try triggering a SVE exception by executing rdvl instruction.
 */
static u_register_t read_scaled_vector_length(void)
{
	u_register_t vl;

	__asm__ volatile(
		".arch_extension sve\n"
		"rdvl %0, #1;"
		".arch_extension nosve\n"
		: "=r" (vl)
	);

	return vl;
}

/*
 * Executes as many accesses to SVE registers as indicated by HOST_ARG1_INDEX.
 *
 * On plane exit, Plane 0 receives the following return values:
 *	- HOST_ARG1_INDEX: Number of attempted accesses to SVE registers.
 *	- HOST_ARG2_INDEX: Number of completed accesses to SVE registers.
 */
bool test_realm_sve_plane_n_access(void)
{
	uint64_t iterations = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	unsigned int n_access = 0U;

	/* This should only be executed from Plane N */
	assert(!realm_is_plane0());

	realm_shared_data_set_my_realm_val(HOST_ARG1_INDEX, 0U);
	realm_shared_data_set_my_realm_val(HOST_ARG2_INDEX, 0U);

	for (; iterations > 0UL; iterations--) {
		realm_shared_data_set_my_realm_val(HOST_ARG1_INDEX, ++n_access);
		(void)read_scaled_vector_length();
		realm_shared_data_set_my_realm_val(HOST_ARG2_INDEX, n_access);
	}

	return true;
}

static bool test_realm_enter_plane_n(u_register_t flags)
{
	u_register_t base, plane_index, perm_index;
	bool ret1;

	plane_index = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	base = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);
	perm_index = plane_index + 1U;
	flags &= RSI_PLANE_ENTRY_FLAG_MASK;

	ret1 = plane_common_init(plane_index, perm_index, base, &run);
	if (!ret1) {
		return ret1;
	}

	realm_printf("Entering plane %ld, ep=0x%lx run=0x%lx\n", plane_index, base, &run);
	return realm_plane_enter(plane_index, perm_index, flags, &run);
}

/*
 * Test that combines accesses to SVE registers from Plane 0 and Plane N,
 * using different sequences of entries to Plane N with and without TRAP_SIMD
 * flag enabled.
 *
 * Expected arguments from Host:
 *	- HOST_ARG1_INDEX: Plane index for Pn to run.
 *	- HOST_ARG2_INDEX: Entry point for Pn.
 *	- HOST_ARG3_INDEX: If 0, first SIMD access is performed from P0.
 *			   Otherwise, first SIMD access is performed from Pn.
 */
bool test_realm_sve_plane_n(void)
{
	unsigned long attempted, success;
	u_register_t plane_index = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);

	if (realm_shared_data_get_my_host_val(HOST_ARG3_INDEX) == 0UL) {
		/* First SIMD access performed from P0 */
		INFO("Executing first SIMD access from P0\n");
		(void)read_scaled_vector_length();
	} else {
		INFO("Executing first SIMD access from PN\n");
	}

	/*
	 * Enter Plane N with TRAP_SIMD disabled. Plane N will attempt
	 * to executte SVE instrucctions SIMD_TRAP_ATTEMPTS times
	 * and no plane exit should happen.
	 */
	if (!test_realm_enter_plane_n(0UL)) {
		return false;
	}

	if ((run.exit.exit_reason != RSI_EXIT_SYNC) ||
	    (EC_BITS(run.exit.esr) != EC_AARCH64_HVC)) {
		ERROR("Invalid exit reason from PN: 0x%lx, EC: 0x%lx\n",
			run.exit.exit_reason, EC_BITS(run.exit.esr));
		return false;
	}

	attempted = realm_shared_data_get_plane_n_val(plane_index,
						      REC_IDX(read_mpidr_el1()),
						      HOST_ARG1_INDEX);
	success = realm_shared_data_get_plane_n_val(plane_index,
						    REC_IDX(read_mpidr_el1()),
						    HOST_ARG2_INDEX);

	if ((attempted < SIMD_TRAP_ATTEMPTS) || (success < SIMD_TRAP_ATTEMPTS)) {
		ERROR("Attempted %u executions of rdvl instruction. %u succedded\n",
		      attempted, success);
		return false;
	}

	/* Try another access to the SVE registers from Plane 0 */
	(void)read_scaled_vector_length();

	/*
	 * Restart PN again, this time with TRAP_SIMD enabled. That should
	 * cause a plane exit when attempting SVE instructions.
	 */
	if (!test_realm_enter_plane_n(RSI_PLANE_ENTRY_FLAG_TRAP_SIMD)) {
		return false;
	}

	if ((run.exit.exit_reason != RSI_EXIT_SYNC) ||
	    (EC_BITS(run.exit.esr) != EC_AARCH64_SVE)) {
		ERROR("Invalid exit reason from PN: 0x%lx, EC: 0x%lx\n",
			run.exit.exit_reason, EC_BITS(run.exit.esr));
		return false;
	}

	attempted = realm_shared_data_get_plane_n_val(plane_index,
						      REC_IDX(read_mpidr_el1()),
						      HOST_ARG1_INDEX);
	success = realm_shared_data_get_plane_n_val(plane_index,
						    REC_IDX(read_mpidr_el1()),
						    HOST_ARG2_INDEX);

	/*
	 * Plane N should have exited after 1 attempt and the access should not
	 * be completed yet.
	 */
	if ((attempted != 1U) || (success > 0U)) {
		ERROR("Attempted %u executions of rdvl instruction. %u succedded\n",
		      attempted, success);
		return false;
	}

	/* Try another access to the SIMD registers from Plane 0 */
	(void)read_scaled_vector_length();

	/*
	 * Resume Plane N with TRAP_SIMD disabled. No Plane exit should happen
	 * due to SIMD access.
	 */
	if (!realm_resume_plane_n(&run, plane_index, 0U)) {
		return false;
	}

	if ((run.exit.exit_reason != RSI_EXIT_SYNC) ||
	    (EC_BITS(run.exit.esr) != EC_AARCH64_HVC)) {
		ERROR("Invalid exit reason from PN: 0x%lx, EC: 0x%lx\n",
			run.exit.exit_reason, EC_BITS(run.exit.esr));
		return false;
	}

	attempted = realm_shared_data_get_plane_n_val(plane_index,
						      REC_IDX(read_mpidr_el1()),
						      HOST_ARG1_INDEX);
	success = realm_shared_data_get_plane_n_val(plane_index,
						    REC_IDX(read_mpidr_el1()),
						    HOST_ARG2_INDEX);

	if ((attempted < SIMD_TRAP_ATTEMPTS) || (success < SIMD_TRAP_ATTEMPTS)) {
		ERROR("Attempted %u executions of rdvl instruction. %u succedded\n",
		      attempted, success);
		return false;
	}

	return true;
}
