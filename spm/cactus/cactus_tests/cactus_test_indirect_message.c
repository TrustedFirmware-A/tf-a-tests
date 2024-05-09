/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include <cactus_test_cmds.h>
#include "debug.h"
#include "ffa_helpers.h"
#include "utils_def.h"
#include <ffa_endpoints.h>
#include <spm_common.h>

CACTUS_CMD_HANDLER(req_msg_send, CACTUS_REQ_MSG_SEND_CMD)
{
	struct ffa_value ret;
	const ffa_id_t vm_id = ffa_dir_msg_dest(*args);
	const ffa_id_t source = ffa_dir_msg_source(*args);
	const ffa_id_t receiver = cactus_msg_send_receiver(*args);
	const ffa_id_t sender = cactus_msg_send_sender(*args);
	const char message[] = "Testing FF-A message.";

	VERBOSE("%x requested to send indirect message to %x as %x(own %x)\n",
		ffa_dir_msg_source(*args), receiver, sender, vm_id);

	ret = send_indirect_message(sender, receiver, mb->send, message,
				    ARRAY_SIZE(message), 0);

	if (is_ffa_call_error(ret)) {
		ERROR("Failed to send indirect message.\n");
		return cactus_error_resp(vm_id,
					 source,
					 ffa_error_code(ret));
	}

	return cactus_success_resp(vm_id, source, 0);
}
