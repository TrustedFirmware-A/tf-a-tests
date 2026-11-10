/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>

#include <common/sha_common_macros.h>
#include <common_def.h>
#include <debug.h>
#include <firme.h>
#include <firme/firme_attestation.h>
#include <host_realm_helper.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <utils_def.h>

#ifdef __aarch64__

#define FIRME_ATTEST_RETRY_MAX	U(20)

static __aligned(PAGE_SIZE) uint8_t shared_buf[PAGE_SIZE];

static void prepare_challenge(void)
{
	memset(shared_buf, 0, sizeof(shared_buf));
	for (size_t i = 0U; i < SHA256_DIGEST_SIZE; i++) {
		shared_buf[i] = (uint8_t)(i + 1U);
	}
}

/*
 * Complete a normal PAT_GET operation. Besides providing a control result,
 * this ensures that a recovered EL3 GPF did not leave the service unusable.
 */
static bool finish_pat_get(uint64_t challenge_size, uint64_t write_offset)
{
	int32_t status = FIRME_INCOMPLETE;

	for (unsigned int retry = 0U; retry < FIRME_ATTEST_RETRY_MAX;
	     retry++) {
		uint64_t written_size = 0U;
		uint64_t remaining_size = 0U;

		status = firme_attest_pat_get((uintptr_t)shared_buf,
					      write_offset, 0U,
					      challenge_size, &written_size,
					      &remaining_size);
		if (status == FIRME_BUSY) {
			continue;
		}

		if (status == FIRME_SUCCESS) {
			return true;
		}

		if ((status != FIRME_INCOMPLETE) ||
		    (written_size > sizeof(shared_buf)) ||
		    (remaining_size == 0U)) {
			break;
		}

		challenge_size = 0U;
		write_offset += written_size;
		if (write_offset >= sizeof(shared_buf)) {
			write_offset = 0U;
		}
	}

	ERROR("FIRME PAT_GET control request failed, status=%d\n", status);
	return false;
}

static bool complete_pat_get(void)
{
	prepare_challenge();
	return finish_pat_get(SHA256_DIGEST_SIZE, 0U);
}

static bool undelegate_buffer(void)
{
	u_register_t ret = host_rmi_granule_undelegate((uintptr_t)shared_buf);

	if (ret != RMI_SUCCESS) {
		ERROR("Granule undelegate failed, ret=0x%lx\n", ret);
		return false;
	}

	return true;
}

/*
 * PAT_GET copies its challenge with rme_gpf_safe_read(). Delegating the
 * challenge page makes that EL3 access generate a recoverable GPF.
 */
test_result_t test_firme_attestation_gpf_read(void)
{
	uint64_t written_size = 0U;
	uint64_t remaining_size = 0U;
	test_result_t result = TEST_RESULT_FAIL;
	u_register_t ret;
	int32_t status;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_NO_RMM();
	if (!is_firme_pat_get_supported()) {
		return TEST_RESULT_SKIPPED;
	}
	if (!host_rmm_activate()) {
		ERROR("RMM activation failed\n");
		return TEST_RESULT_FAIL;
	}

	if (!complete_pat_get()) {
		return TEST_RESULT_FAIL;
	}

	prepare_challenge();
	ret = host_rmi_granule_delegate((uintptr_t)shared_buf);
	if (ret != RMI_SUCCESS) {
		ERROR("Granule delegate failed, ret=0x%lx\n", ret);
		return TEST_RESULT_FAIL;
	}

	status = firme_attest_pat_get((uintptr_t)shared_buf, 0U, 0U,
				      SHA256_DIGEST_SIZE, &written_size,
				      &remaining_size);
	if (status != FIRME_INVALID_PARAMETERS) {
		ERROR("FIRME PAT_GET returned %d, expected %d after read GPF\n",
		      status, FIRME_INVALID_PARAMETERS);
		goto cleanup;
	}

	result = TEST_RESULT_SUCCESS;

cleanup:
	if (!undelegate_buffer()) {
		return TEST_RESULT_FAIL;
	}

	if (!complete_pat_get()) {
		return TEST_RESULT_FAIL;
	}

	return result;
}

/*
 * Force PAT_GET to return an incomplete one-byte token chunk, delegate the
 * page, and continue the request. On FVP the continuation writes the token
 * with rme_gpf_safe_write(), generating a recoverable GPF.
 */
test_result_t test_firme_attestation_gpf_write(void)
{
	uint64_t written_size = 0U;
	uint64_t remaining_size = 0U;
	test_result_t result = TEST_RESULT_FAIL;
	u_register_t ret;
	int32_t status;
	bool delegated = false;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_NO_RMM();
	if (!is_firme_pat_get_supported()) {
		return TEST_RESULT_SKIPPED;
	}
	if (!host_rmm_activate()) {
		ERROR("RMM activation failed\n");
		return TEST_RESULT_FAIL;
	}

	prepare_challenge();
	status = firme_attest_pat_get((uintptr_t)shared_buf,
				      sizeof(shared_buf) - 1U, 0U,
				      SHA256_DIGEST_SIZE, &written_size,
				      &remaining_size);
	if ((status != FIRME_INCOMPLETE) || (written_size != 1U) ||
	    (remaining_size == 0U)) {
		ERROR("Could not start chunked PAT_GET: status=%d, written=%llu, remaining=%llu\n",
		      status, (unsigned long long)written_size,
		      (unsigned long long)remaining_size);
		return TEST_RESULT_FAIL;
	}

	ret = host_rmi_granule_delegate((uintptr_t)shared_buf);
	if (ret != RMI_SUCCESS) {
		ERROR("Granule delegate failed, ret=0x%lx\n", ret);
		goto finish_request;
	}
	delegated = true;

	written_size = 0U;
	remaining_size = 0U;
	status = firme_attest_pat_get((uintptr_t)shared_buf, 0U, 0U, 0U,
				      &written_size, &remaining_size);
	if (status != FIRME_INVALID_PARAMETERS) {
		ERROR("FIRME PAT_GET returned %d, expected %d after write GPF\n",
		      status, FIRME_INVALID_PARAMETERS);
		goto cleanup;
	}

	result = TEST_RESULT_SUCCESS;

cleanup:
	if (delegated && !undelegate_buffer()) {
		return TEST_RESULT_FAIL;
	}

	if (!complete_pat_get()) {
		return TEST_RESULT_FAIL;
	}

	return result;

finish_request:
	/* Clear the active chunked request when delegation itself fails. */
	if (!finish_pat_get(0U, 0U)) {
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_FAIL;
}

#else

test_result_t test_firme_attestation_gpf_read(void)
{
	return TEST_RESULT_SKIPPED;
}

test_result_t test_firme_attestation_gpf_write(void)
{
	return TEST_RESULT_SKIPPED;
}

#endif /* __aarch64__ */
