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
#include <realm_helpers.h>
#include <realm_psi.h>
#include <realm_rsi.h>
#include <realm_tests.h>
#include <realm_psci.h>
#include <tftf_lib.h>

#define CXT_ID_MAGIC	0x100
#define P1_CXT_ID_MAGIC	0x200

static uint64_t is_secondary_cpu_booted;
static spinlock_t lock;
static rsi_plane_run run[MAX_REC_COUNT] __aligned(PAGE_SIZE);
static u_register_t base, plane_index, perm_index;

static void plane0_recn_handler(u_register_t cxt_id)
{
	uint64_t rec = 0U;

	realm_printf("running on Rec= 0x%lx cxt_id= 0x%lx\n",
			read_mpidr_el1() & MPID_MASK, cxt_id);
	if (cxt_id < CXT_ID_MAGIC || cxt_id > CXT_ID_MAGIC + MAX_REC_COUNT) {
		realm_printf("Wrong cxt_id\n");
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}
	spin_lock(&lock);
	is_secondary_cpu_booted++;
	rec =  read_mpidr_el1() & MPID_MASK;
	spin_unlock(&lock);

	/* enter plane */
	u_register_t flags = 0U;

	/* Setup the initial PSTATE for the plane */
	run[rec].enter.pstate = ((SPSR_M3_0_EL1_SP_EL1 << SPSR_M3_0_SHIFT) |
				 (SPSR_M_AARCH64 << SPSR_M_SHIFT) |
				 (0xf << SPSR_DAIF_SHIFT));

	/* Use Base adr, plane_index, perm_index programmed by P0 rec0 */
	run[rec].enter.pc = base;
	realm_printf("Entering plane %ld, ep=0x%lx rec=0x%lx\n", plane_index, base, rec);
	realm_plane_enter(plane_index, perm_index, flags, &run[rec]);

	if (run[rec].exit.gprs[0] == SMC_PSCI_CPU_OFF) {
		realm_printf("Plane N did not request CPU OFF\n");
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}
	realm_cpu_off();
}

static void recn_handler(u_register_t cxt_id)
{
	realm_printf("running on Rec= 0x%lx cxt_id= 0x%lx\n",
			read_mpidr_el1() & MPID_MASK, cxt_id);

	if (cxt_id < P1_CXT_ID_MAGIC || cxt_id > P1_CXT_ID_MAGIC + MAX_REC_COUNT) {
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
	ret = realm_cpu_on(1U, (uintptr_t)recn_handler, 0x100);
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

/*
 * All Planes enter this test function.
 * P0 Rec0 Enters Plane N
 * Plane N rec 0 requests CPU ON for all other rec
 * P0 Rec0 requests CPU ON to host
 * Host enters P0 RecN from different CPU
 * P0 RecN enters PlaneN RecN
 * Rec N requests CPU OFF, exits to P0
 * P0 requests CPU OFF to host.
 * P0 verifies all other CPU are off.
 */
bool test_realm_multiple_plane_multiple_rec_multiple_cpu_cmd(void)
{
	unsigned int i = 1U, rec_count;
	u_register_t ret;
	bool ret1;

	realm_printf("Realm: running on Rec= 0x%lx\n", read_mpidr_el1() & MPID_MASK);
	rec_count = realm_shared_data_get_my_host_val(HOST_ARG3_INDEX);

	/* Check CPU_ON is supported */
	ret = realm_psci_features(SMC_PSCI_CPU_ON);
	if (ret != PSCI_E_SUCCESS) {
		realm_printf("SMC_PSCI_CPU_ON not supported\n");
		return false;
	}

	if (realm_is_plane0()) {
		/* Plane 0 all rec */
		u_register_t flags = 0U;

		plane_index = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
		base = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);
		perm_index = plane_index + 1U;

		plane_common_init(plane_index, perm_index, base, &run[0U]);

		ret1 = realm_plane_enter(plane_index, perm_index, flags, &run[0U]);
		while (ret1 && run->exit.gprs[0] == SMC_PSCI_CPU_ON_AARCH64) {
			realm_printf("Plane N requested CPU on Rec=0x%lx\n", run[0].exit.gprs[1]);

			/* Pass context tp RecN - CXT + rec idx */
			run[0].enter.gprs[0] = realm_cpu_on(run[0].exit.gprs[1],
						(uintptr_t)plane0_recn_handler,
						CXT_ID_MAGIC + run[0].exit.gprs[1]);

			/* re-enter plane N 1 to complete cpu on */
			ret1 = realm_plane_enter(plane_index, perm_index, flags, &run[0U]);
			if (!ret1) {
				realm_printf("PlaneN CPU on complete failed\n");
				rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
			}
		}

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
	} else {
		/* Plane 1 Rec 0 */
		for (unsigned int j = 1U; j < rec_count; j++) {
			realm_printf("CPU ON Rec=%u\n", j);
			ret = realm_cpu_on(j, (uintptr_t)recn_handler, P1_CXT_ID_MAGIC + j);
			if (ret != PSCI_E_SUCCESS) {
				realm_printf("SMC_PSCI_CPU_ON failed %d.\n", j);
				return false;
			}
		}
		/* Exit to Host to allow host to run all CPUs */
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);

		/* wait for all CPUs to come up */
		while (is_secondary_cpu_booted != rec_count - 1U) {
			waitms(200);
		}
		return true;
	}
	return true;
}

bool test_realm_multiple_rec_multiple_cpu_cmd(void)
{
	unsigned int i = 1U, rec_count;
	u_register_t ret;

	realm_printf("Realm: running on Rec= 0x%lx\n", read_mpidr_el1() & MPID_MASK);
	rec_count = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);

	/* Check CPU_ON is supported */
	ret = realm_psci_features(SMC_PSCI_CPU_ON);
	if (ret != PSCI_E_SUCCESS) {
		realm_printf("SMC_PSCI_CPU_ON not supported\n");
		return false;
	}

	for (unsigned int j = 1U; j < rec_count; j++) {
		ret = realm_cpu_on(j, (uintptr_t)recn_handler, P1_CXT_ID_MAGIC + j);
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
