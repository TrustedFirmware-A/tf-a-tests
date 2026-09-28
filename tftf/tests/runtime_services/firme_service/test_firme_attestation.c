/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>

#include <common/sha_common_macros.h>
#include <common_def.h>
#include <firme.h>
#include <firme/firme_attestation.h>
#include <libc/cdefs.h>
#include <tftf_lib.h>

#define FIRME_ATTEST_TEST_BUF_SIZE	U(0x10000)
#define FIRME_ATTEST_RETRY_MAX		U(20)

typedef struct {
	uint16_t version_major;
	uint16_t version_minor;
	uint64_t base_feat_reg1;
	uint64_t attest_feat_reg0;
	uint64_t attest_feat_reg1;
	uint64_t max_sh_buf_pg_cnt;
	size_t max_pat_pg_cnt;
	size_t min_sh_buf_size;
	size_t max_sh_buf_size;
	size_t max_pat_size;
} firme_attest_info_t;

static __aligned(FIRME_ATTEST_TEST_BUF_SIZE)
	uint8_t shared_buf[FIRME_ATTEST_TEST_BUF_SIZE];

static bool firme_get_min_sh_buf_size(uint64_t base_feat_reg1,
				      size_t *min_sh_buf_size)
{
	uint64_t encoding = (base_feat_reg1 >> FIRME_BASE_MIN_SH_BUF_SZ_SHIFT) &
		   FIRME_BASE_MIN_SH_BUF_SZ_MASK;

	switch (encoding) {
	case FIRME_BASE_MIN_SH_BUF_SZ_4KB:
		*min_sh_buf_size = SZ_4K;
		return true;
	case FIRME_BASE_MIN_SH_BUF_SZ_16KB:
		*min_sh_buf_size = SZ_16K;
		return true;
	case FIRME_BASE_MIN_SH_BUF_SZ_64KB:
		*min_sh_buf_size = SZ_64K;
		return true;
	default:
		tftf_testcase_printf(
			"Invalid FIRME MIN_SH_BUF_SZ encoding 0x%llx\n",
			encoding);
		return false;
	}
}

static void firme_pat_prepare_challenge(size_t challenge_size)
{
	size_t i;

	memset(shared_buf, 0, sizeof(shared_buf));
	for (i = 0U; i < challenge_size; i++) {
		shared_buf[i] = (uint8_t)(i + 1U);
	}
}

static test_result_t firme_pat_get_expect_invalid_parameters(
	const char *name, uint64_t shared_buf_addr, uint64_t write_offset,
	uint64_t shared_buf_page_count, uint64_t challenge_size)
{
	uint64_t written_size = 0U;
	uint64_t remaining_size = 0U;
	int32_t status;

	status = firme_attest_pat_get(shared_buf_addr, write_offset,
				      shared_buf_page_count, challenge_size,
				      &written_size, &remaining_size);
	if (status != FIRME_INVALID_PARAMETERS) {
		tftf_testcase_printf(
			"%s returned %d, expected FIRME_INVALID_PARAMETERS\n",
			name, status);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

static bool firme_attestation_discover(firme_attest_info_t *info)
{
	uint64_t max_pat_pg_cnt;
	int32_t res;

	memset(info, 0, sizeof(*info));

	res = firme_features(FIRME_BASE_SERVICE_ID, 1U,
			     &info->base_feat_reg1);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf(
			"failed to read FIRME base feature reg 1: %d\n",
			res);
		return false;
	}

	if ((info->base_feat_reg1 &
	     FIRME_BASE_SERVICE_ATTESTATION_BIT) == 0U) {
		tftf_testcase_printf(
			"FIRME attestation service not advertised\n");
		return false;
	}

	res = firme_version(FIRME_ATTESTATION_SERVICE_ID);
	if (res == FIRME_NOT_SUPPORTED) {
		tftf_testcase_printf(
			"FIRME attestation service advertised but version ABI returned NOT_SUPPORTED\n");
		return false;
	} else if (res < 0) {
		tftf_testcase_printf(
			"FIRME attestation version ABI returned %d\n",
			res);
		return false;
	}

	info->version_major = (uint16_t)((res >> FIRME_VERSION_MAJOR_SHIFT) &
					 FIRME_VERSION_MAJOR_MASK);
	info->version_minor = (uint16_t)((res >> FIRME_VERSION_MINOR_SHIFT) &
					 FIRME_VERSION_MINOR_MASK);

	if ((info->version_major != FIRME_ATTESTATION_VERSION_MAJOR) ||
	    (info->version_minor != FIRME_ATTESTATION_VERSION_MINOR)) {
		tftf_testcase_printf(
			"unexpected FIRME attestation version %u.%u\n",
			info->version_major, info->version_minor);
		return false;
	}

	res = firme_features(FIRME_ATTESTATION_SERVICE_ID, 0U,
			     &info->attest_feat_reg0);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf(
			"failed to read FIRME attestation feature reg 0: %d\n",
			res);
		return false;
	}

	res = firme_features(FIRME_ATTESTATION_SERVICE_ID, 1U,
			     &info->attest_feat_reg1);
	if (res != FIRME_SUCCESS) {
		tftf_testcase_printf(
			"failed to read FIRME attestation feature reg 1: %d\n",
			res);
		return false;
	}

	if (!firme_get_min_sh_buf_size(info->base_feat_reg1,
				       &info->min_sh_buf_size)) {
		return false;
	}

	info->max_sh_buf_pg_cnt =
		(info->base_feat_reg1 >>
		 FIRME_BASE_MAX_SH_BUF_PG_CNT_SHIFT) &
		FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK;
	info->max_sh_buf_size =
		(size_t)(info->max_sh_buf_pg_cnt + 1U) *
		info->min_sh_buf_size;

	max_pat_pg_cnt =
		(info->attest_feat_reg1 >>
		 FIRME_ATTEST_FEAT_REG1_MAX_PAT_PG_CNT_SHIFT) &
		FIRME_ATTEST_FEAT_REG1_MAX_PAT_PG_CNT_MASK;
	info->max_pat_pg_cnt = (size_t)max_pat_pg_cnt + 1U;
	info->max_pat_size = info->max_pat_pg_cnt * info->min_sh_buf_size;

	return true;
}

test_result_t test_firme_attestation_version(void)
{
	firme_attest_info_t info;

	if (!firme_attestation_discover(&info)) {
		return TEST_RESULT_SKIPPED;
	}

	tftf_testcase_printf(
		"FIRME attestation service discovered: version %u.%u, max PAT pages=%llu\n",
		info.version_major, info.version_minor,
		(unsigned long long)info.max_pat_pg_cnt);
	return TEST_RESULT_SUCCESS;
}

static test_result_t firme_attestation_pat_get(bool chunked)
{
	firme_attest_info_t info;
	uint64_t challenge_size = SHA256_DIGEST_SIZE;
	uint64_t shared_buf_page_count;
	uint64_t write_offset;
	uint64_t total_written = 0U;
	size_t shared_buf_size;
	unsigned int data_chunks = 0U;
	int32_t status = FIRME_INCOMPLETE;

	if (!firme_attestation_discover(&info)) {
		return TEST_RESULT_SKIPPED;
	}

	if ((info.attest_feat_reg0 &
	     FIRME_ATTEST_FEAT_REG0_PAT_GET_BIT) == 0U) {
		tftf_testcase_printf("FIRME attestation PAT_GET ABI not supported\n");
		return TEST_RESULT_SKIPPED;
	}

	if (chunked) {
		/*
		 * Page count 0 encodes one minimum-size shared-buffer unit.
		 * Leave one byte for the first output chunk to force retrieval
		 * to continue.
		 */
		shared_buf_size = info.min_sh_buf_size;
		shared_buf_page_count = 0U;
		write_offset = shared_buf_size - 1U;
	} else {
		shared_buf_size = MIN(info.max_pat_size, info.max_sh_buf_size);
		shared_buf_size = MIN(shared_buf_size, sizeof(shared_buf));
		shared_buf_size -= shared_buf_size % info.min_sh_buf_size;
		if (shared_buf_size == 0U) {
			tftf_testcase_printf(
				"no suitably aligned FIRME shared buffer available\n");
			return TEST_RESULT_FAIL;
		}

		/* The ABI encodes N minimum-size units as N - 1. */
		shared_buf_page_count =
			(shared_buf_size / info.min_sh_buf_size) - 1U;
		write_offset = 0U;
	}

	firme_pat_prepare_challenge((size_t)challenge_size);

	for (unsigned int retry = 0U; retry < FIRME_ATTEST_RETRY_MAX;
	     retry++) {
		uint64_t written_size = 0U;
		uint64_t remaining_size = 0U;

		status = firme_attest_pat_get((uint64_t)(uintptr_t)shared_buf,
					      write_offset,
					      shared_buf_page_count,
					      challenge_size, &written_size,
					      &remaining_size);
		if (status == FIRME_BUSY) {
			continue;
		}

		if ((status != FIRME_SUCCESS) &&
		    (status != FIRME_INCOMPLETE)) {
			tftf_testcase_printf(
				"FIRME_ATTEST_PAT_GET returned %d\n",
				status);
			return TEST_RESULT_FAIL;
		}

		if ((written_size > shared_buf_size) ||
		    (write_offset > shared_buf_size - written_size)) {
			tftf_testcase_printf(
				"FIRME_ATTEST_PAT_GET wrote outside the shared buffer\n");
			return TEST_RESULT_FAIL;
		}

		if (written_size != 0U) {
			data_chunks++;
		}
		total_written += written_size;
		write_offset += written_size;
		if ((total_written > info.max_pat_size) ||
		    (remaining_size > info.max_pat_size - total_written)) {
			tftf_testcase_printf(
				"PAT_GET progress exceeds MAX_PAT_PG_CNT\n");
			return TEST_RESULT_FAIL;
		}

		if (status == FIRME_SUCCESS) {
			if ((written_size == 0U) || (remaining_size != 0U)) {
				tftf_testcase_printf(
					"invalid PAT_GET SUCCESS progress: written=%llu, remaining=%llu\n",
					(unsigned long long)written_size,
					(unsigned long long)remaining_size);
				return TEST_RESULT_FAIL;
			}
			break;
		}
		if ((written_size != 0U) && (remaining_size == 0U)) {
			tftf_testcase_printf(
				"PAT_GET returned INCOMPLETE with no remaining data\n");
			return TEST_RESULT_FAIL;
		}

		/* All calls after initiation continue the current retrieval. */
		challenge_size = 0U;
		if (write_offset == shared_buf_size) {
			write_offset = 0U;
		}
	}

	if (status != FIRME_SUCCESS) {
		tftf_testcase_printf(
			"FIRME_ATTEST_PAT_GET did not complete after %u attempts\n",
			FIRME_ATTEST_RETRY_MAX);
		return TEST_RESULT_FAIL;
	}

	if (chunked && (data_chunks < 2U)) {
		tftf_testcase_printf(
			"PAT_GET completed without exercising chunked retrieval\n");
		return TEST_RESULT_FAIL;
	}

	if (chunked) {
		tftf_testcase_printf(
			"Retrieved a %llu byte platform attestation token in %u chunks\n",
			(unsigned long long)total_written, data_chunks);
	} else {
		tftf_testcase_printf(
			"Retrieved a %llu byte platform attestation token\n",
			(unsigned long long)total_written);
	}
	return TEST_RESULT_SUCCESS;
}

test_result_t test_firme_attestation_pat_get(void)
{
	return firme_attestation_pat_get(false);
}

test_result_t test_firme_attestation_pat_get_chunked(void)
{
	return firme_attestation_pat_get(true);
}

test_result_t test_firme_attestation_pat_get_invalid_buffer_config(void)
{
	firme_attest_info_t info;
	const uint64_t challenge_size = SHA256_DIGEST_SIZE;
	test_result_t result;

	if (!firme_attestation_discover(&info)) {
		return TEST_RESULT_SKIPPED;
	}

	if ((info.attest_feat_reg0 &
	     FIRME_ATTEST_FEAT_REG0_PAT_GET_BIT) == 0U) {
		tftf_testcase_printf("FIRME attestation PAT_GET ABI not supported\n");
		return TEST_RESULT_SKIPPED;
	}

	firme_pat_prepare_challenge((size_t)challenge_size);

	result = firme_pat_get_expect_invalid_parameters(
		"invalid write offset", (uint64_t)(uintptr_t)shared_buf,
		info.min_sh_buf_size, 0U, challenge_size);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	result = firme_pat_get_expect_invalid_parameters(
		"unaligned shared-buffer base",
		(uint64_t)(uintptr_t)&shared_buf[1], 0U, 0U, challenge_size);
	if (result != TEST_RESULT_SUCCESS) {
		return result;
	}

	return firme_pat_get_expect_invalid_parameters(
		"challenge larger than shared buffer",
		(uint64_t)(uintptr_t)shared_buf, 0U, 0U,
		info.min_sh_buf_size + 1U);
}
