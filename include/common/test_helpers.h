/*
 * Copyright (c) 2018-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_HELPERS_H__
#define TEST_HELPERS_H__

#include <arch_features.h>
#include <plat_topology.h>
#include <psci.h>
#include <sme.h>
#include <tftf_lib.h>
#include <trusted_os.h>
#include <tsp.h>
#include <uuid_utils.h>
#include <uuid.h>

typedef struct {
	uintptr_t addr;
	size_t size;
	unsigned int attr;
	void *arg;
} map_args_unmap_t;

typedef test_result_t (*test_function_arg_t)(void *arg);

#ifndef __aarch64__
#define SKIP_TEST_IF_AARCH32()							\
	do {									\
		tftf_testcase_printf("Test not supported on aarch32\n");	\
		return TEST_RESULT_SKIPPED;					\
	} while (0)
#else
#define SKIP_TEST_IF_AARCH32()
#endif

#define SKIP_TEST_IF_LESS_THAN_N_CLUSTERS(n)					\
	do {									\
		unsigned int clusters_cnt;					\
		clusters_cnt = tftf_get_total_clusters_count();			\
		if (clusters_cnt < (n)) {					\
			tftf_testcase_printf(					\
				"Need at least %u clusters, only found %u\n",	\
				(n), clusters_cnt);				\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_LESS_THAN_N_CPUS(n)					\
	do {									\
		unsigned int cpus_cnt;						\
		cpus_cnt = tftf_get_total_cpus_count();				\
		if (cpus_cnt < (n)) {						\
			tftf_testcase_printf(					\
				"Need at least %u CPUs, only found %u\n",	\
				(n), cpus_cnt);					\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_TRUSTED_OS_NOT_PRESENT()					\
	do {									\
		uuid_t tos_uuid;						\
										\
		if (!is_trusted_os_present(&tos_uuid)) {			\
			tftf_testcase_printf("No Trusted OS detected\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_TSP_NOT_PRESENT()						\
	do {									\
		uuid_t tos_uuid;						\
		char tos_uuid_str[UUID_STR_SIZE];				\
										\
		if (!is_trusted_os_present(&tos_uuid)) {			\
			tftf_testcase_printf("No Trusted OS detected\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
										\
		if (!uuid_equal(&tos_uuid, &tsp_uuid)) {			\
			tftf_testcase_printf(					\
				"Trusted OS is not the TSP, its UUID is: %s\n",	\
				uuid_to_str(&tos_uuid, tos_uuid_str));		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_SMCCC_VERSION_LT(major, minor)				\
	do {									\
		smc_args args = {0};						\
		smc_ret_values ret;						\
		args.fid = SMCCC_VERSION;					\
		ret = tftf_smc(&args);						\
		if ((int32_t)ret.ret0 < MAKE_SMCCC_VERSION(major, minor)) {	\
			tftf_testcase_printf(					\
				"Unexpected SMCCC version: 0x%x\n",		\
			       (int)ret.ret0);					\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_SMCCC_FUNC_NOT_SUPPORTED(func)				\
	do {									\
		smc_ret_values ret;						\
		smc_args args = {0};						\
		args.fid = SMCCC_ARCH_FEATURES;					\
		args.arg1 = func;						\
		ret = tftf_smc(&args);						\
		if ((int)ret.ret0 == SMC_ARCH_CALL_NOT_SUPPORTED) {		\
			tftf_testcase_printf(					\
				#func " is not implemented\n");			\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_DIT_NOT_SUPPORTED()					\
	do {									\
		if (!is_armv8_4_dit_present()) {				\
			tftf_testcase_printf(					\
				"DIT not supported\n");				\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_PAUTH_NOT_SUPPORTED()					\
	do {									\
		if (!is_armv8_3_pauth_present()) {				\
			tftf_testcase_printf(					\
				"Pointer Authentication not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_FGT_NOT_SUPPORTED()					\
	do {									\
		if (!is_armv8_6_fgt_present()) {				\
			tftf_testcase_printf(					\
				"Fine Grained Traps not supported\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_DEBUGV8P9_NOT_SUPPORTED()					\
	do {									\
		if (arch_get_debug_version() != 				\
				ID_AA64DFR0_V8_9_DEBUG_ARCH_SUPPORTED) {	\
			tftf_testcase_printf(					\
				"Debugv8p9 not supported\n");			\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_FGT2_NOT_SUPPORTED()					\
	do {									\
		if (!is_armv8_9_fgt2_present()) {				\
			tftf_testcase_printf(					\
				"Fine Grained Traps 2 not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_SVE_NOT_SUPPORTED()					\
	do {									\
		if (!is_armv8_2_sve_present()) {				\
			tftf_testcase_printf("SVE not supported\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_ECV_NOT_SELF_SYNC()					\
	do {									\
		if (get_armv8_6_ecv_support() !=				\
			ID_AA64MMFR0_EL1_ECV_SELF_SYNCH) {			\
			tftf_testcase_printf("ARMv8.6-ECV not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_MM_NOT_PRESENT()						\
	do {									\
		smc_args version_smc = { MM_VERSION_AARCH32 };			\
		smc_ret_values smc_ret = tftf_smc(&version_smc);		\
		uint32_t version = smc_ret.ret0;				\
										\
		if (version == (uint32_t) SMC_UNKNOWN) {			\
			tftf_testcase_printf("SPM not detected.\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_MTE_SUPPORT_LESS_THAN(n)					\
	do {									\
		if (get_armv8_5_mte_support() < (n)) {				\
			tftf_testcase_printf(					\
				"Memory Tagging Extension not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_MM_VERSION_LESS_THAN(major, minor)				\
	do {									\
		smc_args version_smc = { MM_VERSION_AARCH32 };			\
		smc_ret_values smc_ret = tftf_smc(&version_smc);		\
		uint32_t version = smc_ret.ret0;				\
										\
		if (version == (uint32_t) SMC_UNKNOWN) {			\
			tftf_testcase_printf("SPM not detected.\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
										\
		if (version < MM_VERSION_FORM(major, minor)) {			\
			tftf_testcase_printf("MM_VERSION returned %u.%u\n"	\
					"The required version is %u.%u\n",	\
					version >> MM_VERSION_MAJOR_SHIFT,	\
					version & MM_VERSION_MINOR_MASK,	\
					major, minor);				\
			return TEST_RESULT_SKIPPED;				\
		}								\
										\
		VERBOSE("MM_VERSION returned %u.%u\n",				\
			version >> MM_VERSION_MAJOR_SHIFT,			\
			version & MM_VERSION_MINOR_MASK);			\
	} while (0)

#define SKIP_TEST_IF_ARCH_DEBUG_VERSION_LESS_THAN(version)			\
	do {									\
		uint32_t debug_ver = arch_get_debug_version();			\
										\
		if (debug_ver < version) {					\
			tftf_testcase_printf("Debug version returned %d\n"	\
					     "The required version is %d\n",	\
					     debug_ver,				\
					     version);				\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_TRBE_NOT_SUPPORTED()					\
	do {									\
		if (!is_feat_trbe_present()) {					\
			tftf_testcase_printf("ARMv9-TRBE not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_TRF_NOT_SUPPORTED()					\
	do {									\
		if (!get_armv8_4_trf_support()) {				\
			tftf_testcase_printf("ARMv8.4-TRF not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_SYS_REG_TRACE_NOT_SUPPORTED()				\
	do {									\
		if (!get_armv8_0_sys_reg_trace_support()) {			\
			tftf_testcase_printf("ARMv8-system register"		\
					     "trace not supported\n");		\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_AFP_NOT_SUPPORTED()					\
	do {									\
		if (!get_feat_afp_present()) {					\
			tftf_testcase_printf("ARMv8.7-afp not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_MPAM_NOT_SUPPORTED()                                     \
	do {                                                                    \
		if(!is_feat_mpam_supported()){                                  \
			tftf_testcase_printf("ARMv8.4-mpam not supported\n");   \
			return TEST_RESULT_SKIPPED;                             \
		}                                                               \
	} while (false)

#ifdef __aarch64__
#define SKIP_TEST_IF_PA_SIZE_LESS_THAN(n)					\
	do {									\
		static const unsigned int pa_range_bits_arr[] = {		\
			PARANGE_0000, PARANGE_0001, PARANGE_0010, PARANGE_0011,\
			PARANGE_0100, PARANGE_0101, PARANGE_0110		\
		};								\
		if (pa_range_bits_arr[get_pa_range()] < n) {			\
			tftf_testcase_printf("PA size less than %d bit\n", n);	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)
#else
#define SKIP_TEST_IF_PA_SIZE_LESS_THAN(n)					\
	do {									\
		return TEST_RESULT_SKIPPED;					\
	} while (false)
#endif

#define SKIP_TEST_IF_BRBE_NOT_SUPPORTED()					\
	do {									\
		if (!get_feat_brbe_support()) {				\
			tftf_testcase_printf("FEAT_BRBE not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_WFXT_NOT_SUPPORTED()					\
	do {									\
		if (!get_feat_wfxt_present()) {					\
			tftf_testcase_printf("ARMv8.7-WFxT not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED()					\
	do {									\
		if (!is_feat_rng_trap_present()) {				\
			tftf_testcase_printf("ARMv8.5-RNG_TRAP not"		\
 					"supported\n");				\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_PMUV3_NOT_SUPPORTED()					\
	do {									\
		if (!get_feat_pmuv3_supported()) {				\
			tftf_testcase_printf("FEAT_PMUv3 not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_SME_NOT_SUPPORTED()					\
	do {									\
		if(!is_feat_sme_supported()) {					\
			tftf_testcase_printf("FEAT_SME not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_SME2_NOT_SUPPORTED()					\
	do {									\
		if(!is_feat_sme2_supported()) {					\
			tftf_testcase_printf("FEAT_SME2 not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_FPMR_NOT_SUPPORTED()					\
	do {									\
		if(!is_feat_fpmr_present()) {					\
			tftf_testcase_printf("FEAT_FPMR not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_SCTLR2_NOT_SUPPORTED()					\
	do {									\
		if (!is_feat_sctlr2_supported()) {				\
			tftf_testcase_printf("FEAT_SCTLR2 not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_THE_NOT_SUPPORTED()					\
	do {									\
		if (!is_feat_the_supported()) {					\
			tftf_testcase_printf("FEAT_THE not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_D128_NOT_SUPPORTED()					\
	do {									\
		if (!is_feat_d128_supported()) {				\
			tftf_testcase_printf("FEAT_D128 not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP()				\
	do {									\
		u_register_t retrmm = 0U;					\
										\
		if (!get_armv9_2_feat_rme_support()) {				\
			tftf_testcase_printf("FEAT_RME not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
										\
		host_rmi_init_cmp_result();					\
		retrmm = host_rmi_version(RMI_ABI_VERSION_VAL);			\
										\
		VERBOSE("RMM version is: %lu.%lu\n",				\
			RMI_ABI_VERSION_GET_MAJOR(retrmm),			\
			RMI_ABI_VERSION_GET_MINOR(retrmm));			\
										\
		/*								\
		 * TODO: Remove this once SMC_RMM_REALM_CREATE is implemented	\
		 * in TRP. For the moment skip the test if RMM is TRP, TRP	\
		 * version is always 0.						\
		 */								\
		if (retrmm == 0U) {						\
			tftf_testcase_printf("RMM is TRP\n");			\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_LS64_NOT_SUPPORTED()					\
	do {									\
		if (get_feat_ls64_support() ==					\
			ID_AA64ISAR1_LS64_NOT_SUPPORTED) {			\
			tftf_testcase_printf("ARMv8.7-ls64 not supported");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

#define SKIP_TEST_IF_LS64_ACCDATA_NOT_SUPPORTED()			\
	do {								\
		if (get_feat_ls64_support() <				\
		    ID_AA64ISAR1_LS64_ACCDATA_SUPPORTED) {		\
			tftf_testcase_printf("ARMv8.7-ls64-accdata not supported");	\
			return TEST_RESULT_SKIPPED;			\
		}							\
	} while (false)

#define SKIP_TEST_IF_DOUBLE_FAULT2_NOT_SUPPORTED()				\
	do {									\
		if (is_feat_double_fault2_present() == false) {			\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

/* Helper macro to verify if system suspend API is supported */
#define is_psci_sys_susp_supported()	\
		(tftf_get_psci_feature_info(SMC_PSCI_SYSTEM_SUSPEND)		\
					== PSCI_E_SUCCESS)

/* Helper macro to verify if PSCI_STAT_COUNT API is supported */
#define is_psci_stat_count_supported()	\
		(tftf_get_psci_feature_info(SMC_PSCI_STAT_COUNT)		\
					== PSCI_E_SUCCESS)

/*
 * Helper function to verify the system state is ready for system
 * suspend. i.e., a single CPU is running and all other CPUs are powered off.
 * Returns 1 if the system is ready to suspend, 0 otherwise.
 */
int is_sys_suspend_state_ready(void);

/*
 * Helper function to reset the system. This function shouldn't return.
 * It is not marked with __dead to help the test to catch some error in
 * TF
 */
void psci_system_reset(void);

/*
 * Helper function that enables/disables the mem_protect mechanism
 */
int psci_mem_protect(int val);


/*
 * Helper function to call PSCI MEM_PROTECT_CHECK
 */
int psci_mem_protect_check(uintptr_t addr, size_t size);


/*
 * Helper function to get a sentinel address that can be used to test mem_protect
 */
unsigned char *psci_mem_prot_get_sentinel(void);

/*
 * Helper function to memory map and unmap a region needed by a test.
 *
 * Return TEST_RESULT_FAIL if the memory could not be successfully mapped or
 * unmapped. Otherwise, return the test functions's result.
 */
test_result_t map_test_unmap(const map_args_unmap_t *args,
			     test_function_arg_t test);

/*
 * Utility function to wait for all CPUs other than the caller to be
 * OFF.
 */
void wait_for_non_lead_cpus(void);

/*
 * Utility function to wait for a given CPU other than the caller to be
 * OFF.
 */
void wait_for_core_to_turn_off(unsigned int mpidr);

/* Generate 64-bit random number */
unsigned long long rand64(void);

/* TRBE Errata */
#define CORTEX_A520_MIDR        U(0x410FD800)
#define CORTEX_X4_MIDR          U(0x410FD821)
#define RXPX_RANGE(x, y, z)     (((x >= y) && (x <= z)) ? true : false)
bool is_trbe_errata_affected_core(void);
#endif /* __TEST_HELPERS_H__ */
