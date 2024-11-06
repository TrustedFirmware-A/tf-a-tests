/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include <drivers/arm/sp805.h>
#include <ffa_helpers.h>
#include <sp_helpers.h>
#include "spm_common.h"
#include <spm_helpers.h>

#include <platform_def.h>

#define NOTIFICATION_PENDING_INTERRUPT_INTID 5

extern void notification_pending_interrupt_handler(void);

extern ffa_id_t g_ffa_id;
extern ffa_id_t g_dir_req_source_id;
static uint32_t managed_exit_interrupt_id;

/* Secure virtual interrupt that was last handled by Cactus SP. */
uint32_t last_serviced_interrupt[PLATFORM_CORE_COUNT];

extern spinlock_t sp_handler_lock[NUM_VINT_ID];

/*
 * Managed exit ID discoverable by querying the SPMC through
 * FFA_FEATURES API.
 */
void discover_managed_exit_interrupt_id(void)
{
	struct ffa_value ffa_ret;

	/* Interrupt ID value is returned through register W2. */
	ffa_ret = ffa_features(FFA_FEATURE_MEI);
	managed_exit_interrupt_id = ffa_feature_intid(ffa_ret);

	VERBOSE("Discovered managed exit interrupt ID: %d\n",
	     managed_exit_interrupt_id);
}

/*
 * Cactus SP does not implement application threads. Hence, once the Cactus SP
 * sends the managed exit response to the direct request originator, execution
 * is still frozen in interrupt handler context.
 * Though it moves to WAITING state, it is not able to accept new direct request
 * message from any endpoint. It can only receive a direct request message with
 * the command CACTUS_RESUME_AFTER_MANAGED_EXIT from the originator of the
 * suspended direct request message in order to return from the interrupt
 * handler context and resume the processing of suspended request.
 */
void send_managed_exit_response(void)
{
	struct ffa_value ffa_ret;
	bool waiting_resume_after_managed_exit;

	/*
	 * A secure partition performs its housekeeping and sends a direct
	 * response to signal interrupt completion. This is a pure virtual
	 * interrupt, no need for deactivation.
	 */
	ffa_ret = cactus_response(g_ffa_id, g_dir_req_source_id,
			MANAGED_EXIT_INTERRUPT_ID);
	waiting_resume_after_managed_exit = true;

	while (waiting_resume_after_managed_exit) {

		waiting_resume_after_managed_exit =
			(ffa_func_id(ffa_ret) != FFA_MSG_SEND_DIRECT_REQ_SMC32 &&
			 ffa_func_id(ffa_ret) != FFA_MSG_SEND_DIRECT_REQ_SMC64) ||
			 ffa_dir_msg_source(ffa_ret) != g_dir_req_source_id ||
			 cactus_get_cmd(ffa_ret) != CACTUS_RESUME_AFTER_MANAGED_EXIT;

		if (waiting_resume_after_managed_exit) {
			VERBOSE("Expected a direct message request from endpoint"
			      " %x with command CACTUS_RESUME_AFTER_MANAGED_EXIT\n",
			       g_dir_req_source_id);
			ffa_ret = cactus_error_resp(g_ffa_id,
						    ffa_dir_msg_source(ffa_ret),
						    CACTUS_ERROR_TEST);
		}
	}
	VERBOSE("Resuming the suspended command\n");
}

static void timer_interrupt_handler(void)
{
	/* Disable the EL1 physical arch timer. */
	write_cntp_ctl_el0(0);

	spm_interrupt_deactivate(TIMER_VIRTUAL_INTID);
	NOTICE("serviced el1 physical timer\n");
}

void register_maintenance_interrupt_handlers(void)
{
	sp_register_interrupt_handler(send_managed_exit_response,
		managed_exit_interrupt_id);
	sp_register_interrupt_handler(notification_pending_interrupt_handler,
		NOTIFICATION_PENDING_INTERRUPT_INTID);
	sp_register_interrupt_handler(timer_interrupt_handler,
		TIMER_VIRTUAL_INTID);
}

void cactus_interrupt_handler_irq(void)
{
	uint32_t intid = spm_interrupt_get();
	unsigned int core_pos = spm_get_my_core_pos();

	last_serviced_interrupt[core_pos] = intid;

	/* Invoke the handler registered by the SP. */
	spin_lock(&sp_handler_lock[intid]);
	if (sp_interrupt_handler[intid]) {
		sp_interrupt_handler[intid]();
	} else {
		ERROR("%s: Interrupt ID %x not handled!\n", __func__, intid);
		panic();
	}
	spin_unlock(&sp_handler_lock[intid]);
}

void cactus_interrupt_handler_fiq(void)
{
	uint32_t intid = spm_interrupt_get();
	unsigned int core_pos = spm_get_my_core_pos();

	last_serviced_interrupt[core_pos] = intid;

	if (intid == MANAGED_EXIT_INTERRUPT_ID) {
		/*
		 * A secure partition performs its housekeeping and sends a
		 * direct response to signal interrupt completion.
		 * This is a pure virtual interrupt, no need for deactivation.
		 */
		VERBOSE("vFIQ: Sending ME response to %x\n",
			g_dir_req_source_id);
		send_managed_exit_response();
	} else {
		/*
		 * Currently only managed exit interrupt is supported by vFIQ.
		 */
		panic();
	}
}
