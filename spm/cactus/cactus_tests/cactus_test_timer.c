/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include "debug.h"
#include <spm_helpers.h>

uint32_t ms_to_ticks(uint64_t ms)
{
	return ms * read_cntfrq_el0() / 1000;
}

CACTUS_CMD_HANDLER(set_virtual_timer, CACTUS_SET_ARCH_TIMER_CMD)
{
	uint64_t deadline_ms = cactus_get_timer_deadline(*args);
	uint64_t wait_time = cactus_get_timer_wait_time(*args);
	uint32_t ticks = ms_to_ticks(deadline_ms);

	/* Disable the arch timer. */
	write_cntp_ctl_el0(0);

	/* Enable the arch timer virtual interrupt. */
	spm_interrupt_enable(TIMER_VIRTUAL_INTID, true, 0);

	write_cntp_tval_el0(ticks);
	write_cntp_ctl_el0(1);

	if (wait_time != 0U) {
		waitms(wait_time);
	}

	return cactus_response(ffa_dir_msg_dest(*args),
			       ffa_dir_msg_source(*args),
			       CACTUS_SUCCESS);
}
