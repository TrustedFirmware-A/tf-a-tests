/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_RSI_H
#define REALM_RSI_H

#include <stdint.h>
#include <host_shared_data.h>
#include <tftf_lib.h>

#define SMC_RSI_CALL_BASE	0xC4000190
#define SMC_RSI_FID(_x)		(SMC_RSI_CALL_BASE + (_x))
/*
 * This file describes the Realm Services Interface (RSI) Application Binary
 * Interface (ABI) for SMC calls made from within the Realm to the RMM and
 * serviced by the RMM.
 *
 * See doc/rmm_interface.md for more details.
 */

/*
 * The major version number of the RSI implementation.  Increase this whenever
 * the binary format or semantics of the SMC calls change.
 */
#define RSI_ABI_VERSION_MAJOR		1U

/*
 * The minor version number of the RSI implementation.  Increase this when
 * a bug is fixed, or a feature is added without breaking binary compatibility.
 */
#define RSI_ABI_VERSION_MINOR		0U

#define RSI_ABI_VERSION_VAL		((RSI_ABI_VERSION_MAJOR << 16U) | \
					 RSI_ABI_VERSION_MINOR)

#define RSI_ABI_VERSION_GET_MAJOR(_version) ((_version) >> 16U)
#define RSI_ABI_VERSION_GET_MINOR(_version) ((_version) & 0xFFFFU)


/* RSI Status code enumeration as per Section D4.3.6 of the RMM Spec */
typedef enum {
	/* Command completed successfully */
	RSI_SUCCESS = 0U,

	/*
	 * The value of a command input value
	 * caused the command to fail
	 */
	RSI_ERROR_INPUT	= 1U,

	/*
	 * The state of the current Realm or current REC
	 * does not match the state expected by the command
	 */
	RSI_ERROR_STATE	= 2U,

	/* The operation requested by the command is not complete */
	RSI_INCOMPLETE = 3U,

	RSI_ERROR_COUNT
} rsi_status_t;

struct rsi_realm_config {
	/* IPA width in bits */
	SET_MEMBER(unsigned long ipa_width, 0, 0x1000);	/* Offset 0 */
};

/*
 * arg0 == IPA address of target region
 * arg1 == Size of target region in bytes
 * arg2 == RIPAS value
 * ret0 == Status / error
 * ret1 == Top of modified IPA range
 */

#define RSI_HOST_CALL_NR_GPRS		31U

struct rsi_host_call {
	SET_MEMBER(struct {
		/* Immediate value */
		unsigned int imm;		/* Offset 0 */
		/* Registers */
		unsigned long gprs[RSI_HOST_CALL_NR_GPRS];
		}, 0, 0x100);
};

/*
 * arg0 == struct rsi_host_call address
 */
#define RSI_HOST_CALL		SMC_RSI_FID(9U)


#define RSI_VERSION		SMC_RSI_FID(0U)

/*
 * arg0 == struct rsi_realm_config address
 */
#define RSI_REALM_CONFIG	SMC_RSI_FID(6U)
#define RSI_IPA_STATE_SET	SMC_RSI_FID(7U)
#define RSI_IPA_STATE_GET	SMC_RSI_FID(8U)

typedef enum {
	RSI_EMPTY = 0U,
	RSI_RAM,
	RSI_DESTROYED
} rsi_ripas_type;

typedef enum {
	RSI_ACCEPT = 0U,
	RSI_REJECT
} rsi_ripas_respose_type;

#define RSI_NO_CHANGE_DESTROYED	0UL
#define RSI_CHANGE_DESTROYED	1UL

/* Request RIPAS of a target IPA range to be changed to a specified value. */
u_register_t rsi_ipa_state_set(u_register_t base,
			   u_register_t top,
			   rsi_ripas_type ripas,
			   u_register_t flag,
			   u_register_t *new_base,
			   rsi_ripas_respose_type *response);

/* Request RIPAS of a target IPA */
u_register_t rsi_ipa_state_get(u_register_t adr, rsi_ripas_type *ripas);

/* This function return RSI_ABI_VERSION */
u_register_t rsi_get_version(u_register_t req_ver);

/* This function will call the Host to request IPA of the NS shared buffer */
u_register_t rsi_get_ns_buffer(void);

/* This function call Host and request to exit Realm with proper exit code */
void rsi_exit_to_host(enum host_call_cmd exit_code);

#endif /* REALM_RSI_H */
