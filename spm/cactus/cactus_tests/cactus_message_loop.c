/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>

#include <cactus_message_loop.h>
#include <cactus_test_cmds.h>
#include <ffa_helpers.h>
#include <events.h>
#include <platform.h>
#include <spm_helpers.h>
#include <psci.h>

/**
 * Counter of the number of handled requests, for each CPU. The number of
 * requests can be accessed from another Cactus SP, or from the normal world
 * using a special test command.
 */
static uint32_t requests_counter[PLATFORM_CORE_COUNT];

/**
 * Begin and end of command handler table, respectively. Both symbols defined by
 * the linker.
 */
extern struct cactus_cmd_handler cactus_cmd_handler_begin[];
extern struct cactus_cmd_handler cactus_cmd_handler_end[];

#define PRINT_CMD(smc_ret)						\
	VERBOSE("cmd %lx; args: %lx, %lx, %lx, %lx\n",	 		\
		smc_ret.arg3, smc_ret.arg4, smc_ret.arg5, 		\
		smc_ret.arg6, smc_ret.arg7)

/* Global FFA_MSG_DIRECT_REQ source ID */
ffa_id_t g_dir_req_source_id;

/**
 * Traverses command table from section ".cactus_handler", searches for a
 * registered command and invokes the respective handler.
 */
bool cactus_handle_cmd(struct ffa_value *cmd_args, struct ffa_value *ret,
		       struct mailbox_buffers *mb)
{
	uint64_t in_cmd;

	/* Get vCPU index for currently running vCPU. */
	unsigned int core_pos = spm_get_my_core_pos();

	if (cmd_args == NULL || ret == NULL) {
		ERROR("Invalid arguments passed to %s!\n", __func__);
		return false;
	}

	/* Get the source of the Direct Request message. */
	if (ffa_func_id(*cmd_args) == FFA_MSG_SEND_DIRECT_REQ_SMC32 ||
	    ffa_func_id(*cmd_args) == FFA_MSG_SEND_DIRECT_REQ_SMC64) {
		g_dir_req_source_id = ffa_dir_msg_source(*cmd_args);
	}

	PRINT_CMD((*cmd_args));

	in_cmd = cactus_get_cmd(*cmd_args);

	for (struct cactus_cmd_handler *it_cmd = cactus_cmd_handler_begin;
	     it_cmd < cactus_cmd_handler_end;
	     it_cmd++) {
		if (it_cmd->id == in_cmd) {
			*ret = it_cmd->fn(cmd_args, mb);

			/*
			 * Increment the number of requests handled in current
			 * core.
			 */
			requests_counter[core_pos]++;

			return true;
		}
	}

	/* Handle special command. */
	if (in_cmd == CACTUS_GET_REQ_COUNT_CMD) {
		uint32_t requests_counter_resp;

		/* Read value from array. */
		requests_counter_resp = requests_counter[core_pos];
		VERBOSE("Requests Counter %u, core: %u\n", requests_counter_resp,
							   core_pos);

		*ret = cactus_success_resp(
			ffa_dir_msg_dest(*cmd_args),
			ffa_dir_msg_source(*cmd_args),
			requests_counter_resp);
		return true;
	}

	*ret = cactus_error_resp(ffa_dir_msg_dest(*cmd_args),
				 ffa_dir_msg_source(*cmd_args),
				 CACTUS_ERROR_UNHANDLED);
	return true;
}

struct ffa_value cactus_handle_framework_msg(struct ffa_value args)
{
	ffa_id_t source_id = ffa_dir_msg_source(args);
	ffa_id_t destination_id = ffa_dir_msg_dest(args);
	uint32_t status_code;

#if CACTUS_PWR_MGMT_SUPPORT == 1
	uint32_t framework_msg = ffa_get_framework_msg(args);
	uint32_t psci_function = args.arg3;

	/*
	 * As of now, Cactus supports receiving only PSCI power management
	 * request as framework message.
	 */
	if (framework_msg != FFA_FRAMEWORK_MSG_PSCI_REQ) {
		ERROR("Unsupported framework message received by SP:%x\n",
					destination_id);
		status_code = PSCI_E_DENIED;
		goto out;
	}

	/* Cactus only supports receiving CPU_OFF PSCI function as message. */
	if (psci_function != SMC_PSCI_CPU_OFF) {
		ERROR("Unsupported PSCI function(%x) received by SP:%x through "
			"framework message\n", psci_function, destination_id);
		status_code = PSCI_E_DENIED;
		goto out;
	}

	/* Only SPMC can send the PSCI framework message. */
	if (source_id != SPMC_ID) {
		ERROR("Framework message source illegal %x\n", source_id);
		status_code = PSCI_E_DENIED;
		goto out;
	}

	status_code = PSCI_E_SUCCESS;
	/*
	 * Return successful status for PSCI power management request through
	 * direct response Framework message.
	 */
	VERBOSE("PSCI power management request handled successfully by SP:%x\n",
							destination_id);
out:
#else
	status_code = PSCI_E_DENIED;
#endif
	return ffa_framework_msg_send_direct_resp(destination_id, source_id,
				FFA_FRAMEWORK_MSG_PSCI_RESP, status_code);
}
