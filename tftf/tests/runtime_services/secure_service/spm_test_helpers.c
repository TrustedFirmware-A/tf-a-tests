/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <power_management.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <tftf_lib.h>

static struct mailbox_buffers test_mb = {.send = NULL, .recv = NULL};

bool reset_tftf_mailbox(void)
{
	if (is_ffa_call_error(ffa_rxtx_unmap())) {
		return false;
	}

	test_mb.send = NULL;
	test_mb.recv = NULL;

	return true;
}

bool get_tftf_mailbox(struct mailbox_buffers *mb)
{
	struct ffa_value ret;

	if (test_mb.recv == NULL || test_mb.send == NULL) {
		CONFIGURE_AND_MAP_MAILBOX(test_mb, PAGE_SIZE, ret);
		if (is_ffa_call_error(ret)) {
			return false;
		}
	}

	*mb = test_mb;

	return true;
}

test_result_t check_spmc_testing_set_up(
	uint32_t ffa_version_major, uint32_t ffa_version_minor,
	const struct ffa_uuid *ffa_uuids, size_t ffa_uuids_size)
{
	struct  mailbox_buffers mb;

	if (ffa_uuids == NULL) {
		ERROR("Invalid parameter ffa_uuids!\n");
		return TEST_RESULT_FAIL;
	}

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(ffa_version_major,
					   ffa_version_minor);

	/**********************************************************************
	 * If OP-TEE is SPMC skip the current test.
	 **********************************************************************/
	if (check_spmc_execution_level()) {
		VERBOSE("OPTEE as SPMC at S-EL1. Skipping test!\n");
		return TEST_RESULT_SKIPPED;
	}

	GET_TFTF_MAILBOX(mb);

	for (unsigned int i = 0U; i < ffa_uuids_size; i++)
		SKIP_TEST_IF_FFA_ENDPOINT_NOT_DEPLOYED(*mb, ffa_uuids[i]);

	return TEST_RESULT_SUCCESS;
}

test_result_t spm_run_multi_core_test(uintptr_t cpu_on_handler,
				      event_t *cpu_done)
{
	unsigned int lead_mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos, cpu_node, mpidr;
	int32_t ret;

	VERBOSE("Powering on all cpus.\n");

	for (unsigned int i = 0U; i < PLATFORM_CORE_COUNT; i++) {
		tftf_init_event(&cpu_done[i]);
	}

	 /* Power on each secondary CPU one after the other. */
	for_each_cpu(cpu_node) {
		mpidr = tftf_get_mpidr_from_node(cpu_node);
		if (mpidr == lead_mpid) {
			continue;
		}

		ret = tftf_cpu_on(mpidr, cpu_on_handler, 0U);
		if (ret != 0) {
			ERROR("tftf_cpu_on mpidr 0x%x returns %d\n",
			      mpidr, ret);
		}

		/* Wait for the secondary CPU to be ready. */
		core_pos = platform_get_core_pos(mpidr);
		tftf_wait_for_event(&cpu_done[core_pos]);
	}

	VERBOSE("Done exiting.\n");

	return TEST_RESULT_SUCCESS;
}
