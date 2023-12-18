/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sdei_fuzz_helper.h>

void tftf_test_sdei_noarg(int64_t (*sdei_func)(void), char *funcstr)
{
		int64_t ret = (*sdei_func)();

		if (ret < 0) {
			tftf_testcase_printf("%s failed: 0x%llx\n", funcstr, ret);
		}
}

void tftf_test_sdei_singlearg(int64_t (*sdei_func)(uint64_t), char *funcstr)
{
		int64_t ret = (*sdei_func)(0);

		if (ret < 0) {
			tftf_testcase_printf("%s failed: 0x%llx\n", funcstr, ret);
		}
}


void run_sdei_fuzz(char *funcstr)
{
	if (strcmp(funcstr, "sdei_version") == CMP_SUCCESS) {
		long long ret = sdei_version();

		if (ret != MAKE_SDEI_VERSION(1, 0, 0)) {
			tftf_testcase_printf("Unexpected SDEI version: 0x%llx\n",
					     ret);
		}
	} else if (strcmp(funcstr, "sdei_pe_unmask") == CMP_SUCCESS) {
		tftf_test_sdei_noarg(sdei_pe_unmask, "sdei_pe_unmask");
	} else if (strcmp(funcstr, "sdei_pe_mask") == CMP_SUCCESS) {
		tftf_test_sdei_noarg(sdei_pe_mask, "sdei_pe_mask");
	} else if (strcmp(funcstr, "sdei_event_status") == CMP_SUCCESS) {
		tftf_test_sdei_singlearg((int64_t (*)(uint64_t))sdei_event_status,
		"sdei_event_status");
	} else if (strcmp(funcstr, "sdei_event_signal") == CMP_SUCCESS) {
		tftf_test_sdei_singlearg(sdei_event_signal, "sdei_event_signal");
	} else if (strcmp(funcstr, "sdei_private_reset") == CMP_SUCCESS) {
		tftf_test_sdei_noarg(sdei_private_reset, "sdei_private_reset");
	} else if (strcmp(funcstr, "sdei_shared_reset") == CMP_SUCCESS) {
		tftf_test_sdei_noarg(sdei_shared_reset, "sdei_shared_reset");
	}
}
