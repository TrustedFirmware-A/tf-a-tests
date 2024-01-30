/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include <fpu.h>
#include <spm_helpers.h>
#include "spm_common.h"

/*
 * Note Test must exercise FILL and COMPARE command in
 * sequence and on same CPU.
 */
static fpu_state_t sp_fpu_state_write;
static fpu_state_t sp_fpu_state_read;
static unsigned int core_pos;
/*
 * Fill SIMD vectors from secure world side with a unique value.
 */
CACTUS_CMD_HANDLER(req_simd_fill, CACTUS_REQ_SIMD_FILL_CMD)
{
	/* Get vCPU index for currently running vCPU. */
	core_pos = spm_get_my_core_pos();
	fpu_state_write_rand(&sp_fpu_state_write);
	return cactus_response(ffa_dir_msg_dest(*args),
			       ffa_dir_msg_source(*args),
			       CACTUS_SUCCESS);
}

/*
 * compare FPU state(SIMD vectors, FPCR, FPSR) from secure world side with the previous
 * SIMD_SECURE_VALUE unique value.
 */
CACTUS_CMD_HANDLER(req_simd_compare, CACTUS_CMP_SIMD_VALUE_CMD)
{
	bool test_succeed = false;

	/* Get vCPU index for currently running vCPU. */
	unsigned int core_pos1 = spm_get_my_core_pos();
	if (core_pos1 == core_pos) {
		fpu_state_read(&sp_fpu_state_read);
		if (fpu_state_compare(&sp_fpu_state_write,
				      &sp_fpu_state_read) == 0) {
			test_succeed = true;
		}
	}
	return cactus_response(ffa_dir_msg_dest(*args),
			ffa_dir_msg_source(*args),
			test_succeed ? CACTUS_SUCCESS : CACTUS_ERROR);
}
