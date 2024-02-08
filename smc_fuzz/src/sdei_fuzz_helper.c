/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fuzz_names.h>
#include <sdei_fuzz_helper.h>

/*
 * SDEI function that has no arguments
 */
void tftf_test_sdei_noarg(int64_t (*sdei_func)(void), char *funcstr)
{
		int64_t ret = (*sdei_func)();

		if (ret < 0) {
			tftf_testcase_printf("%s failed: 0x%llx\n", funcstr, ret);
		}
}

/*
 * SDEI function that has single argument
 */
void tftf_test_sdei_singlearg(int64_t (*sdei_func)(uint64_t), char *funcstr)
{
		int64_t ret = (*sdei_func)(0);

		if (ret < 0) {
			tftf_testcase_printf("%s failed: 0x%llx\n", funcstr, ret);
		}
}

/*
 * SDEI function called from fuzzer
 */
void run_sdei_fuzz(int funcid)
{
	if (funcid == sdei_version_funcid) {
		long long ret = sdei_version();

		if (ret != MAKE_SDEI_VERSION(1, 0, 0)) {
			tftf_testcase_printf("Unexpected SDEI version: 0x%llx\n",
					     ret);
		}
	} else if (funcid == sdei_pe_unmask_funcid) {
		tftf_test_sdei_noarg(sdei_pe_unmask, "sdei_pe_unmuask");
	} else if (funcid == sdei_pe_mask_funcid) {
		tftf_test_sdei_noarg(sdei_pe_mask, "sdei_pe_mask");
	} else if (funcid == sdei_event_status_funcid) {
		tftf_test_sdei_singlearg((int64_t (*)(uint64_t))sdei_event_status,
		"sdei_event_status");
	} else if (funcid == sdei_event_signal_funcid) {
		tftf_test_sdei_singlearg(sdei_event_signal, "sdei_event_signal");
	} else if (funcid == sdei_private_reset_funcid) {
		tftf_test_sdei_noarg(sdei_private_reset, "sdei_private_reset");
	} else if (funcid == sdei_shared_reset_funcid) {
		tftf_test_sdei_noarg(sdei_shared_reset, "sdei_shared_reset");
	}
}
