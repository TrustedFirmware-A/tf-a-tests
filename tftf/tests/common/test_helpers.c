/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch_helpers.h>
#include <plat_topology.h>
#include <platform.h>
#include <test_helpers.h>
#include <tftf_lib.h>

int is_sys_suspend_state_ready(void)
{
	int aff_info;
	unsigned int target_node;
	u_register_t target_mpid;
	u_register_t current_mpid = read_mpidr_el1() & MPID_MASK;

	for_each_cpu(target_node) {
		target_mpid = tftf_get_mpidr_from_node(target_node);

		/* Skip current CPU, as it is powered on */
		if (target_mpid == current_mpid)
			continue;

		aff_info = tftf_psci_affinity_info(target_mpid, MPIDR_AFFLVL0);
		if (aff_info != PSCI_STATE_OFF)
			return 0;
	}

	return 1;
}

void psci_system_reset(void)
{
	smc_args args = { SMC_PSCI_SYSTEM_RESET };
	smc_ret_values ret;

	ret = tftf_smc(&args);

	/* The PSCI SYSTEM_RESET call is not supposed to return */
	tftf_testcase_printf("System didn't reboot properly (%d)\n",
			(unsigned int)ret.ret0);
}

int psci_mem_protect(int val)
{
	smc_args args = { SMC_PSCI_MEM_PROTECT};
	smc_ret_values ret;

	args.arg1 = val;
	ret = tftf_smc(&args);

	return ret.ret0;
}

int psci_mem_protect_check(uintptr_t addr, size_t size)
{
	smc_args args = { SMC_PSCI_MEM_PROTECT_CHECK };
	smc_ret_values ret;

	args.arg1 = addr;
	args.arg2 = size;
	ret = tftf_smc(&args);
	return ret.ret0;
}

/*
 * This function returns an address that can be used as
 * sentinel for mem_protect functions. The logic behind
 * it is that it has to search one region that doesn't intersect
 * with the memory used by TFTF.
 */
unsigned char *psci_mem_prot_get_sentinel(void)
{
	const mem_region_t *ranges, *rp, *lim;
	int nranges;
	IMPORT_SYM(uintptr_t, __TFTF_BASE__, tftf_base);
	IMPORT_SYM(uintptr_t, __TFTF_END__, tftf_end);
	uintptr_t p = 0;

	ranges = plat_get_prot_regions(&nranges);
	if (!ranges)
		return NULL;

	lim = &ranges[nranges];
	for (rp = ranges ; rp < lim; rp++) {
		p = rp->addr;
		if (p < tftf_base || p > tftf_end)
			break;
		p = p + (rp->size - 1);
		if (p < tftf_base || p > tftf_end)
			break;
	}

	return (rp == lim) ? NULL : (unsigned char *) p;
}

/*
 * This function maps the memory region before the
 * test and unmap it after the test is run
 */
test_result_t map_test_unmap(const map_args_unmap_t *args,
			     test_function_arg_t test)
{
	int mmap_ret;
	test_result_t test_ret;

	mmap_ret = mmap_add_dynamic_region(args->addr, args->addr,
					   args->size, args->attr);

	if (mmap_ret != 0) {
		tftf_testcase_printf("Couldn't map memory (ret = %d)\n",
				     mmap_ret);
		return TEST_RESULT_FAIL;
	}

	test_ret = (*test)(args->arg);

	mmap_ret = mmap_remove_dynamic_region(args->addr, args->size);
	if (mmap_ret != 0) {
		tftf_testcase_printf("Couldn't unmap memory (ret = %d)\n",
				     mmap_ret);
		return TEST_RESULT_FAIL;
	}

	return test_ret;
}

/*
 * Utility function to wait for all CPUs other than the caller to be
 * OFF.
 */
void wait_for_non_lead_cpus(void)
{
	unsigned int target_mpid, target_node;

	for_each_cpu(target_node) {
		target_mpid = tftf_get_mpidr_from_node(target_node);
		wait_for_core_to_turn_off(target_mpid);
	}
}

void wait_for_core_to_turn_off(unsigned int mpidr)
{
	/* Skip lead CPU, as it is powered on */
	if (mpidr == (read_mpidr_el1() & MPID_MASK))
		return;

	while (tftf_psci_affinity_info(mpidr, MPIDR_AFFLVL0) != PSCI_STATE_OFF) {
		continue;
	}
}

/* Generate 64-bit random number */
unsigned long long rand64(void)
{
	return ((unsigned long long)rand() << 32) | rand();
}

/* Check if TRBE erratums 2938996 and 2726228 applies */
bool is_trbe_errata_affected_core(void)
{
        long midr_val = read_midr();
        long rev_var = EXTRACT_REV_VAR(midr_val);

        if(EXTRACT_PARTNUM(midr_val) == EXTRACT_PARTNUM(CORTEX_A520_MIDR)) {
                if(RXPX_RANGE(rev_var, 0, 1)) {
                        return true;
                }
        } else if (EXTRACT_PARTNUM(midr_val) == EXTRACT_PARTNUM(CORTEX_X4_MIDR)) {
                if(RXPX_RANGE(rev_var, 0, 1)) {
                        return true;
                }
        }

        return false;
}
