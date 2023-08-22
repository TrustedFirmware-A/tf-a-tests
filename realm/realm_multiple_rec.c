/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <fpu.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <psci.h>
#include "realm_def.h"
#include <realm_rsi.h>
#include <realm_tests.h>
#include <realm_psci.h>
#include <tftf_lib.h>

#define CXT_ID_MAGIC 0x100
static uint64_t is_secondary_cpu_booted;
static spinlock_t lock;

static void rec1_handler(u_register_t cxt_id)
{
	realm_printf("running on CPU = 0x%lx cxt_id= 0x%lx\n",
			read_mpidr_el1() & MPID_MASK, cxt_id);
	if (cxt_id < CXT_ID_MAGIC || cxt_id > CXT_ID_MAGIC + MAX_REC_COUNT) {
		realm_printf("Wrong cxt_id\n");
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}
	spin_lock(&lock);
	is_secondary_cpu_booted++;
	spin_unlock(&lock);
	realm_cpu_off();
}

static void rec2_handler(u_register_t cxt_id)
{
	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
}

bool test_realm_multiple_rec_psci_denied_cmd(void)
{
	u_register_t ret;

	is_secondary_cpu_booted = 0U;
	ret = realm_cpu_on(1U, (uintptr_t)rec1_handler, 0x100);
	if (ret != PSCI_E_DENIED) {
		return false;
	}

	if (is_secondary_cpu_booted != 0U) {
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}

	ret = realm_psci_affinity_info(1U, MPIDR_AFFLVL0);
	if (ret != PSCI_STATE_OFF) {
		realm_printf("CPU 1 should have been off\n");
		return false;
	}

	ret = realm_cpu_on(2U, (uintptr_t)rec2_handler, 0x102);
	if (ret != PSCI_E_ALREADY_ON) {
		realm_printf("CPU 2 should have been already on\n");
		return false;
	}
	return true;
}

bool test_realm_multiple_rec_multiple_cpu_cmd(void)
{
	unsigned int i = 1U, rec_count;
	u_register_t ret;

	realm_printf("Realm: running on CPU = 0x%lx\n", read_mpidr_el1() & MPID_MASK);
	rec_count = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);

	/* Check CPU_ON is supported */
	ret = realm_psci_features(SMC_PSCI_CPU_ON);
	if (ret != PSCI_E_SUCCESS) {
		realm_printf("SMC_PSCI_CPU_ON not supported\n");
		return false;
	}

	for (unsigned int j = 1U; j < rec_count; j++) {
		ret = realm_cpu_on(j, (uintptr_t)rec1_handler, CXT_ID_MAGIC + j);
		if (ret != PSCI_E_SUCCESS) {
			realm_printf("SMC_PSCI_CPU_ON failed %d.\n", j);
			return false;
		}
	}

	/* Exit to host to allow host to run all CPUs */
	rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	/* wait for all CPUs to come up */
	while (is_secondary_cpu_booted != rec_count - 1U) {
		waitms(200);
	}

	/* wait for all CPUs to turn off */
	while (i < rec_count) {
		ret = realm_psci_affinity_info(i, MPIDR_AFFLVL0);
		if (ret != PSCI_STATE_OFF) {
			/* wait and query again */
			realm_printf(" CPU %d is not off\n", i);
			waitms(200);
			continue;
		}
		i++;
	}
	realm_printf("All CPU are off\n");
	return true;
}
