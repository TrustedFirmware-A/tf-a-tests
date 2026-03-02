/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <realm_helpers.h>
#include <stddef.h>
#include <sync.h>

void realm_mops_cpy(void *dst, const void *src, size_t len);

/*
 * Test FEAT_MOPS enablement by RMM inside a Realm.
 *
 * If FEAT_MOPS is present, RMM should enable the MOPS instructions for the
 * Realm so the CPY sequence executes without trapping. If FEAT_MOPS is absent,
 * the instructions encode as UNDEFINED and should trap to R-EL1 as EC_UNKNOWN.
 */
bool test_realm_feat_mops(void)
{
	int i;
	uint8_t src[64];
	uint8_t dst_buf[64];
	size_t len = sizeof(src);

	/* Fill source buffer with a known pattern */
	for (i = 0; i < (int)len; i++) {
		src[i] = (uint8_t)(i ^ 0x5aU);
	}

	for (i = 0; i < (int)len; i++) {
		dst_buf[i] = 0U;
	}

	realm_reset_undef_abort_count();
	register_custom_sync_exception_handler(realm_sync_exception_handler);
	realm_mops_cpy(dst_buf, src, len);
	unregister_custom_sync_exception_handler();

	if (!is_feat_mops_present()) {
		/*
		 * FEAT_MOPS is absent: each CPY instruction in the sequence
		 * should trap to EL1 as EC_UNKNOWN.  Expect exactly 3 aborts
		 * (one per CPY{F,M,E}P instruction).
		 */
		return (realm_get_undef_abort_count() == 3U);
	}

	/*
	 * FEAT_MOPS is present: RMM must have enabled it.  The copy should
	 * have completed successfully and the buffers must match.
	 */
	for (i = 0; i < (int)len; i++) {
		if (dst_buf[i] != src[i]) {
			return false;
		}
	}

	return true;
}
