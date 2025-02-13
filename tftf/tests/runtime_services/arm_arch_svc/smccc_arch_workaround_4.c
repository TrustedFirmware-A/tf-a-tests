/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <debug.h>
#include <plat_topology.h>
#include <power_management.h>
#include <psci.h>
#include <smccc.h>
#include <string.h>
#include <tftf_lib.h>

#ifdef __aarch64__
#define CORTEX_X2_MIDR		0x410FD480
#define CORTEX_X3_MIDR		0x410FD4E0
#define CORTEX_X4_MIDR		0x410FD821
#define CORTEX_X925_MIDR	0x410FD850
#define NEOVERSE_V2_MIDR	0x410FD4F0
#define NEOVERSE_V3_MIDR	0x410FD840

static int cortex_x2_test(void);
static int cortex_x3_test(void);
static int cortex_x4_test(void);
static int cortex_x925_test(void);
static int neoverse_v2_test(void);
static int neoverse_v3_test(void);

struct ent {
	unsigned int midr;
	int (*smc_ret)(void);
};

struct ent negative_entries[] = {
	{ .midr = CORTEX_X2_MIDR, .smc_ret = cortex_x2_test }
};

struct ent positive_entries[] = {
	{ .midr = CORTEX_X3_MIDR, .smc_ret = cortex_x3_test },
	{ .midr = CORTEX_X4_MIDR, .smc_ret = cortex_x4_test },
	{ .midr = CORTEX_X925_MIDR, .smc_ret = cortex_x925_test },
	{ .midr = NEOVERSE_V2_MIDR, .smc_ret = neoverse_v2_test},
	{ .midr = NEOVERSE_V3_MIDR, .smc_ret = neoverse_v3_test},
};

static int cortex_x2_test(void)
{
	return -1;
}
static int cortex_x3_test(void)
{
	return 0;
}

static int cortex_x4_test(void)
{
	return 0;
}

static int cortex_x925_test(void)
{
	return 0;
}

static int neoverse_v2_test(void)
{
	return 0;
}

static int neoverse_v3_test(void)
{
	return 0;
}

static test_result_t test_smccc_entrypoint(void)
{
	smc_args args;
	smc_ret_values ret;
	int32_t expected_ver;
	unsigned int my_midr, midr_mask;
	size_t i;

	/* Check if SMCCC version is at least v1.1 */
	expected_ver = MAKE_SMCCC_VERSION(1, 1);
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	ret = tftf_smc(&args);
	if ((int32_t)ret.ret0 < expected_ver) {
		tftf_testcase_printf("Unexpected SMCCC version: 0x%x\n",
		       (int)ret.ret0);
		return TEST_RESULT_SKIPPED;
	}

	/* Check if SMCCC_ARCH_WORKAROUND_4 is required or not */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_ARCH_FEATURES;
	args.arg1 = SMCCC_ARCH_WORKAROUND_4;
	ret = tftf_smc(&args);

	/*
	 * Negetive test. Checks if select non-affected cores
	 * return -1 (Not supported)
	 */
	my_midr = (unsigned int)read_midr_el1();
	midr_mask = (MIDR_IMPL_MASK << MIDR_IMPL_SHIFT) | (MIDR_PN_MASK << MIDR_PN_SHIFT);
	for (i = 0; i < ARRAY_SIZE(negative_entries); i++) {
		struct ent *entp = &negative_entries[i];
		if ((my_midr & midr_mask) == (entp->midr & midr_mask)) {
			if (entp->smc_ret() != -1) {
				return TEST_RESULT_FAIL;
			}
			break;
		}
	}

	if ((int)ret.ret0 == -1) {
		tftf_testcase_printf("SMCCC_ARCH_WORKAROUND_4 is not implemented\n");
		return TEST_RESULT_SKIPPED;
	}

	/* Check if the SMC return value matches our expectations */
	for (i = 0; i < ARRAY_SIZE(positive_entries); i++) {
		struct ent *entp = &positive_entries[i];
		if ((my_midr & midr_mask) == (entp->midr & midr_mask)) {
			if (entp->smc_ret() != 0) {
				return TEST_RESULT_FAIL;
			}
			break;
		}
	}

	if ((i == ARRAY_SIZE(positive_entries)) && ((int)ret.ret0) == 0) {
		tftf_testcase_printf("TFTF workaround table out of sync with TF-A\n");
		return TEST_RESULT_FAIL;
	}

	/* Invoke the workaround to make sure nothing nasty happens */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_ARCH_WORKAROUND_4;
	tftf_smc(&args);
	return TEST_RESULT_SUCCESS;
}

test_result_t test_smccc_arch_workaround_4(void)
{
	u_register_t lead_mpid, target_mpid;
	int cpu_node, ret;

	lead_mpid = read_mpidr_el1() & MPID_MASK;

	/* Power on all the non-lead cores. */
	for_each_cpu(cpu_node) {
		target_mpid = tftf_get_mpidr_from_node(cpu_node);
		if (lead_mpid == target_mpid) {
			continue;
		}
		ret = tftf_cpu_on(target_mpid,
		    (uintptr_t)test_smccc_entrypoint, 0);
		if (ret != PSCI_E_SUCCESS) {
			ERROR("CPU ON failed for 0x%llx\n",
			    (unsigned long long)target_mpid);
			return TEST_RESULT_FAIL;
		}
		/*
		 * Wait for test_smccc_entrypoint to return
		 * and the CPU to power down
		 */
		while (tftf_psci_affinity_info(target_mpid, MPIDR_AFFLVL0) !=
			PSCI_STATE_OFF) {
			continue;
		}
	}

	return test_smccc_entrypoint();
}
#else
test_result_t test_smccc_arch_workaround_4(void)
{
	INFO("%s skipped on AArch32\n", __func__);
	return TEST_RESULT_SKIPPED;
}
#endif
