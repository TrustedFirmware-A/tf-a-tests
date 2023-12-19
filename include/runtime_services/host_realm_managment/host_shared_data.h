/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_SHARED_DATA_H
#define HOST_SHARED_DATA_H

#include <stdint.h>

#include <host_realm_rmi.h>
#include <spinlock.h>

#define MAX_BUF_SIZE		10240U
#define MAX_DATA_SIZE		5U

#define REALM_CMD_BUFFER_SIZE	1024U

/*
 * This structure maps the shared memory to be used between the Host and Realm
 * payload
 */
typedef struct host_shared_data {
	/* Buffer used from Realm for logging */
	uint8_t log_buffer[MAX_BUF_SIZE];

	/* Command set from Host and used by Realm */
	uint8_t realm_cmd;

	/* array of params passed from Host to Realm */
	u_register_t host_param_val[MAX_DATA_SIZE];

	/* array of output results passed from Realm to Host */
	u_register_t realm_out_val[MAX_DATA_SIZE];

	/* Buffer to save Realm command results */
	uint8_t realm_cmd_output_buffer[REALM_CMD_BUFFER_SIZE];
} host_shared_data_t;

/*
 * Different commands that the Host can requests the Realm to perform
 */
enum realm_cmd {
	REALM_SLEEP_CMD = 1U,
	REALM_LOOP_CMD,
	REALM_MULTIPLE_REC_PSCI_DENIED_CMD,
	REALM_GET_RSI_VERSION,
	REALM_PMU_CYCLE,
	REALM_PMU_EVENT,
	REALM_PMU_PRESERVE,
	REALM_PMU_INTERRUPT,
	REALM_REQ_FPU_FILL_CMD,
	REALM_REQ_FPU_CMP_CMD,
	REALM_SET_RIPAS_CMD,
	REALM_SVE_RDVL,
	REALM_SVE_ID_REGISTERS,
	REALM_SVE_PROBE_VL,
	REALM_SVE_OPS,
	REALM_SVE_FILL_REGS,
	REALM_SVE_CMP_REGS,
	REALM_SVE_UNDEF_ABORT,
	REALM_PAUTH_SET_CMD,
	REALM_PAUTH_CHECK_CMD,
	REALM_PAUTH_FAULT,
	REALM_SME_ID_REGISTERS,
	REALM_SME_UNDEF_ABORT
};

/*
 * Index values for each parameter in the host_param_val array.
 */
enum host_param_index {
	HOST_CMD_INDEX = 0U,
	HOST_ARG1_INDEX,
	HOST_ARG2_INDEX
};

enum host_call_cmd {
        HOST_CALL_GET_SHARED_BUFF_CMD = 1U,
        HOST_CALL_EXIT_SUCCESS_CMD,
	HOST_CALL_EXIT_FAILED_CMD,
	HOST_CALL_EXIT_PRINT_CMD
};

/***************************************
 *  APIs to be invoked from Host side  *
 ***************************************/

/*
 * Return shared buffer pointer mapped as host_shared_data_t structure
 */
host_shared_data_t *host_get_shared_structure(struct realm *realm_ptr, unsigned int rec_num);

/*
 * Set data to be shared from Host to realm
 */
void host_shared_data_set_host_val(struct realm *realm_ptr,
		unsigned int rec_num, uint8_t index, u_register_t val);

/*
 * Set command to be send from Host to realm
 */
void host_shared_data_set_realm_cmd(struct realm *realm_ptr, uint8_t cmd,
		unsigned int rec_num);


/****************************************
 *  APIs to be invoked from Realm side  *
 ****************************************/

/*
 * Set guest mapped shared buffer pointer
 */
void realm_set_shared_structure(host_shared_data_t *ptr);

/*
 * Get guest mapped shared buffer pointer
 */
host_shared_data_t *realm_get_my_shared_structure(void);

/*
 * Return Host's data at index
 */
u_register_t realm_shared_data_get_my_host_val(uint8_t index);

/*
 * Get command sent from Host to my Rec
 */
uint8_t realm_shared_data_get_my_realm_cmd(void);

/*
 * Set data to be shared from my Rec to Host
 */
void realm_shared_data_set_my_realm_val(uint8_t index, u_register_t val);

#endif /* HOST_SHARED_DATA_H */
