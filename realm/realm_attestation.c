/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include <realm_rsi.h>

#include <arch_features.h>
#include <debug.h>
#include <realm_def.h>
#include <sync.h>
#include <realm_helpers.h>

#define CHALLENGE_SIZE	8

static unsigned char attest_token_buffer[REALM_TOKEN_BUF_SIZE]
	__aligned(GRANULE_SIZE);
static uint64_t attest_token_offset;

bool test_realm_attestation(void)
{
	u_register_t rsi_ret;
	u_register_t bytes_copied;
	u_register_t token_upper_bound;
	u_register_t challenge[CHALLENGE_SIZE];

	for (unsigned int i = 0U; i < CHALLENGE_SIZE; i++) {
		challenge[i] = realm_rand64();
	}

	rsi_ret = rsi_attest_token_init(challenge[0],
					challenge[1],
					challenge[2],
					challenge[3],
					challenge[4],
					challenge[5],
					challenge[6],
					challenge[7],
					&token_upper_bound);

	if (rsi_ret != RSI_SUCCESS) {
		realm_printf("RSI_ATTEST_TOKEN_INIT"
			     " returned with code %lu\n", rsi_ret);
		return false;
	}

	if (token_upper_bound > REALM_TOKEN_BUF_SIZE) {
		realm_printf("Attestation token buffer is not large enough"
				" to hold the token.\n");
		return false;
	}

	do {
		rsi_ret = rsi_attest_token_continue(
					(u_register_t)attest_token_buffer,
					attest_token_offset,
					REALM_TOKEN_BUF_SIZE,
					&bytes_copied);

		if ((rsi_ret != RSI_SUCCESS) && (rsi_ret != RSI_INCOMPLETE)) {
			realm_printf("RSI_ATTEST_TOKEN_CONTINUE"
				     " returned with code %lu\n", rsi_ret);
			return false;
		}

		attest_token_offset += bytes_copied;

	} while (rsi_ret != RSI_SUCCESS);

	return true;
}

bool test_realm_attestation_fault(void)
{
	u_register_t rsi_ret;
	u_register_t bytes_copied;

	/*
	 * This RSI call will fail as RSI_ATTEST_TOKEN_INIT has to be invoked
	 * before calling RSI_ATTEST_TOKEN_CONTINUE.
	 */
	rsi_ret = rsi_attest_token_continue(
				(u_register_t)attest_token_buffer,
				attest_token_offset,
				REALM_TOKEN_BUF_SIZE,
				&bytes_copied);

	if ((rsi_ret == RSI_SUCCESS) || (rsi_ret == RSI_INCOMPLETE)) {
		return false;
	}

	return true;
}
