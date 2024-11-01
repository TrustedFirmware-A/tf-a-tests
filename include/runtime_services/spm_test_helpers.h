/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPM_TEST_HELPERS_H__
#define SPM_TEST_HELPERS_H__

#include <events.h>
#include <ffa_helpers.h>
#include <ffa_svc.h>
#include <spm_common.h>

#define SKIP_TEST_IF_FFA_VERSION_LESS_THAN(major, minor)			\
	do {									\
		struct ffa_value ret = ffa_version(FFA_VERSION_COMPILED);	\
		enum ffa_version version = ret.fid;				\
										\
		if (version == FFA_ERROR_NOT_SUPPORTED) {			\
			tftf_testcase_printf("FFA_VERSION not supported.\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
										\
		if (!ffa_version_is_valid(version)) {			\
			tftf_testcase_printf("FFA_VERSION bad response: %x\n",	\
					version);				\
			return TEST_RESULT_FAIL;				\
		}								\
										\
		if (version < make_ffa_version(major, minor)) {			\
			tftf_testcase_printf("FFA_VERSION returned %u.%u\n"	\
					"The required version is %u.%u\n",	\
					ffa_version_get_major(version), 	\
					ffa_version_get_minor(version), 	\
					major, minor);				\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (0)

#define SKIP_TEST_IF_FFA_ENDPOINT_NOT_DEPLOYED(mb, ffa_uuid)			\
	do {									\
		struct ffa_value sc_ret = ffa_partition_info_get(ffa_uuid);	\
		ffa_rx_release();						\
		if (ffa_func_id(sc_ret) == FFA_ERROR && 			\
		    ffa_error_code(sc_ret) == FFA_ERROR_INVALID_PARAMETER) {	\
			tftf_testcase_printf("FFA endpoint not deployed!\n");	\
			return TEST_RESULT_SKIPPED;				\
		} else if (ffa_func_id(sc_ret) != FFA_SUCCESS_SMC32) {		\
			ERROR("ffa_partition_info_get failed!\n");		\
			return TEST_RESULT_FAIL;				\
		}								\
	} while (0)

#define GET_TFTF_MAILBOX(mb)							\
	do {									\
		if (!get_tftf_mailbox(&mb)) {					\
			ERROR("Mailbox RXTX buffers not configured!\n");	\
			return TEST_RESULT_FAIL;				\
		}								\
	} while (false);

#define CHECK_SPMC_TESTING_SETUP(ffa_major, ffa_minor, expected_uuids)		\
	do {									\
		SKIP_TEST_IF_AARCH32();						\
		const size_t expected_uuids_size =				\
			 sizeof(expected_uuids) / sizeof(struct ffa_uuid);	\
		test_result_t ret = check_spmc_testing_set_up(			\
			ffa_major, ffa_minor, expected_uuids, 			\
			expected_uuids_size);					\
		if (ret != TEST_RESULT_SUCCESS) {				\
			return ret;						\
		}								\
	} while (false);

/*
 * Helper function to reset TFTF global mailbox for SPM related tests.
 * It calls the FFA_RXTX_UNMAP interface, for the SPMC to drop the current
 * address.
 */
bool reset_tftf_mailbox(void);

/*
 * Helper function to get TFTF global mailbox for SPM related tests.
 * Allocates RX/TX buffer pair and calls FFA_RXTX_MAP interface, for the SPMC
 * to map them into its own S1 translation.
 * If this function is called, and the buffers had been priorly mapped, it
 * sets 'mb' with the respective addresses.
 */
bool get_tftf_mailbox(struct mailbox_buffers *mb);

test_result_t check_spmc_testing_set_up(uint32_t ffa_version_major,
        uint32_t ffa_version_minor, const struct ffa_uuid *ffa_uuids,
        size_t ffa_uuids_size);

/**
 * Turn on all cpus to execute a test in all.
 * - 'cpu_on_handler' should have the code containing the test.
 * - 'cpu_booted' is used for notifying which cores the test has been executed.
 * This should be used in the test executed by cpu_on_handler at the end of
 * processing to make sure it complies with this function's implementation.
 */
test_result_t spm_run_multi_core_test(uintptr_t cpu_on_handler,
                                      event_t *cpu_booted);

/**
 * Initializes the Mailbox for other SPM related tests that need to use
 * RXTX buffers.
 */
bool mailbox_init(struct mailbox_buffers mb);

#endif /* __SPM_TEST_HELPERS_H__ */
