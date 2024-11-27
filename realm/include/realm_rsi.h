/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_RSI_H
#define REALM_RSI_H

#include <arch.h>
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

/* Size of Realm Personalization Value */
#define RSI_RPV_SIZE			64U

struct rsi_realm_config {
	/* IPA width in bits */
	SET_MEMBER(unsigned long ipa_width, 0, 8);	/* Offset 0 */
	/* Hash algorithm */
	SET_MEMBER(unsigned long algorithm, 8, 0x10);	/* Offset 8 */
	/* Number of auxiliary Planes */
	SET_MEMBER(unsigned long num_aux_planes, 0x10, 0x200); /* Offset 0x10 */
	/* Realm Personalization Value */
	SET_MEMBER(unsigned char rpv[RSI_RPV_SIZE], 0x200, 0x1000); /* Offset 0x200 */
};

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
 * ret0 == Status / error
 */
#define RSI_HOST_CALL		SMC_RSI_FID(9U)

/*
 * arg0: Requested interface version
 * ret0: Status / error
 * ret1: Lower implemented interface revision
 * ret2: Higher implemented interface revision
 */
#define RSI_VERSION		SMC_RSI_FID(0U)

/*
 * arg0 == struct rsi_realm_config address
 * ret0 == Status / error
 */
#define RSI_REALM_CONFIG	SMC_RSI_FID(6U)

/*
 * arg0 == Base IPA address of target region
 * arg1 == Top address of target region
 * arg2 == RIPAS value
 * arg3 == flags
 * ret0 == Status / error
 * ret1 == Base of IPA region which was not modified by the command
 * ret2 == RSI response
 */
#define RSI_IPA_STATE_SET	SMC_RSI_FID(7U)

/*
 * arg0 == Base of target IPA region
 * arg1 == End of target IPA region
 * ret0 == Status / error
 * ret1 == Top of IPA region which has the reported RIPAS value
 * ret2 == RIPAS value
 */
#define RSI_IPA_STATE_GET	SMC_RSI_FID(8U)

/*
 * ret0 == Status / error
 * ret1 == Token maximum length
 */
#define RSI_ATTEST_TOKEN_INIT	SMC_RSI_FID(4U)

/*
 * arg0 == Base of buffer to write the token to
 * arg1 == Offset within the buffer
 * arg2 == Size of the buffer
 * ret0 == Status / error
 * ret1 == Size of received token hunk
 */
#define RSI_ATTEST_TOKEN_CONTINUE	SMC_RSI_FID(5U)

typedef enum {
	RSI_EMPTY = 0U,
	RSI_RAM,
	RSI_DESTROYED,
	RSI_DEV
} rsi_ripas_type;

typedef enum {
	RSI_ACCEPT = 0U,
	RSI_REJECT
} rsi_ripas_respose_type;

#define RSI_NO_CHANGE_DESTROYED	0UL
#define RSI_CHANGE_DESTROYED	1UL

/*
 * arg1 == plane index
 * arg2 == perm index
 *
 * ret0 == status
 * ret1 == perm value
 */
#define RSI_MEM_GET_PERM_VALUE	SMC_RSI_FID(0x10U)

/*
 * arg1 == base adr
 * arg2 == top adr
 * arg3 == perm index
 * arg4 == cookie
 *
 * ret0 == status
 * ret1 == new_base
 * ret2 == response
 * ret3 == new_cookie
 */
#define RSI_MEM_SET_PERM_INDEX	SMC_RSI_FID(0x11U)

/*
 * arg1 == plane index
 * arg2 == perm index
 *
 * ret0 == status
 */
#define RSI_MEM_SET_PERM_VALUE	SMC_RSI_FID(0x12U)

#define RSI_PLANE_NR_GPRS	31U
#define RSI_PLANE_GIC_NUM_LRS	16U

/*
 * Flags provided by the Primary Plane to the secondary ones upon
 * plane entry.
 */
#define RSI_PLANE_ENTRY_FLAG_TRAP_WFI	U(1UL << 0)
#define RSI_PLANE_ENTRY_FLAG_TRAP_WFE	U(1UL << 1)
#define RSI_PLANE_ENTRY_FLAG_TRAP_HC	U(1UL << 2)

/* Data structure used to pass values from P0 to the RMM on Plane entry */
struct rsi_plane_entry {
	/* Flags */
	SET_MEMBER(u_register_t flags, 0, 0x8);	/* Offset 0 */
	/* PC */
	SET_MEMBER(u_register_t pc, 0x8, 0x100);	/* Offset 0x8 */
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[RSI_PLANE_NR_GPRS], 0x100, 0x200);	/* 0x100 */
	/* EL1 system registers */
	SET_MEMBER(struct {
		/* GICv3 Hypervisor Control Register */
		u_register_t gicv3_hcr;                         /* 0x200 */
		/* GICv3 List Registers */
		u_register_t gicv3_lrs[RSI_PLANE_GIC_NUM_LRS];        /* 0x208 */
	}, 0x200, 0x800);
};

/* Data structure used to pass values from the RMM to P0 on Plane exit */
struct rsi_plane_exit {
	/* Exit reason */
	SET_MEMBER(u_register_t exit_reason, 0, 0x100);/* Offset 0 */
	SET_MEMBER(struct {
		/* Exception Link Register */
		u_register_t elr;				/* 0x100 */
		/* Exception Syndrome Register */
		u_register_t esr;				/* 0x108 */
		/* Fault Address Register */
		u_register_t far;				/* 0x108 */
		/* Hypervisor IPA Fault Address register */
		u_register_t hpfar;				/* 0x110 */
	}, 0x100, 0x200);
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[RSI_PLANE_NR_GPRS], 0x200, 0x300); /* 0x200 */
	SET_MEMBER(struct {
		/* GICv3 Hypervisor Control Register */
		u_register_t gicv3_hcr;				/* 0x300 */
		/* GICv3 List Registers */
		u_register_t gicv3_lrs[RSI_PLANE_GIC_NUM_LRS];	/* 0x308 */
		/* GICv3 Maintenance Interrupt State Register */
		u_register_t gicv3_misr;			/* 0x388 */
		/* GICv3 Virtual Machine Control Register */
		u_register_t gicv3_vmcr;			/* 0x390 */
	}, 0x300, 0x600);
};

typedef struct {
	/* Entry information */
	SET_MEMBER(struct rsi_plane_entry enter, 0, 0x800); /* Offset 0 */
	/* Exit information */
	SET_MEMBER(struct rsi_plane_exit exit, 0x800, 0x1000);/* 0x800 */
} rsi_plane_run;

/*
 * arg1 == plane index
 * arg2 == run pointer
 *
 * ret0 == status
 */
#define RSI_PLANE_ENTER		SMC_RSI_FID(0x13U)

/*
 * arg1 == plane index
 * arg2 == register encoding
 *
 * ret0 == status
 * ret1 = register value
 */
#define RSI_PLANE_REG_READ	SMC_RSI_FID(0x1EU)

/*
 * arg1 == plane index
 * arg2 == register encoding
 * arg3 == register value
 *
 * ret0 == status
 */
#define RSI_PLANE_REG_WRITE	SMC_RSI_FID(0x1FU)

/*
 * Function to set overlay permission value for a specified
 * (plane index, overlay permission index) tuple
 */
u_register_t rsi_mem_set_perm_value(u_register_t plane_index,
	u_register_t perm_index,
	u_register_t perm);

/*
 * Function to Get overlay permission value for a specified
 * (plane index, overlay permission index) tuple
 */
u_register_t rsi_mem_get_perm_value(u_register_t plane_index,
	u_register_t perm_index,
	u_register_t *perm);

/* Function to Set overlay permission index for a specified IPA range See RSI_MEM_SET_PERM_INDEX */
u_register_t rsi_mem_set_perm_index(u_register_t base,
	u_register_t top,
	u_register_t perm_index,
	u_register_t cookie,
	u_register_t *new_base,
	u_register_t *response,
	u_register_t *new_cookie);

/* Request RIPAS of a target IPA range to be changed to a specified value. */
u_register_t rsi_ipa_state_set(u_register_t base,
				u_register_t top,
				rsi_ripas_type ripas,
				u_register_t flag,
				u_register_t *new_base,
				rsi_ripas_respose_type *response);

/* Request RIPAS of a target IPA range */
u_register_t rsi_ipa_state_get(u_register_t base,
				u_register_t top,
				u_register_t *out_top,
				rsi_ripas_type *ripas);

/* This function return RSI_ABI_VERSION */
u_register_t rsi_get_version(u_register_t req_ver);

/* This function will initialize the attestation context */
u_register_t rsi_attest_token_init(u_register_t challenge_0,
				   u_register_t challenge_1,
				   u_register_t challenge_2,
				   u_register_t challenge_3,
				   u_register_t challenge_4,
				   u_register_t challenge_5,
				   u_register_t challenge_6,
				   u_register_t challenge_7,
				   u_register_t *out_token_upper_bound);

/* This function will retrieve the (or part of) attestation token */
u_register_t rsi_attest_token_continue(u_register_t buffer_addr,
					u_register_t offset,
					u_register_t buffer_size,
					u_register_t *bytes_copied);

/* This function call Host and request to exit Realm with proper exit code */
u_register_t rsi_exit_to_host(enum host_call_cmd exit_code);

/* Function to get Realm configuration. See RSI_REALM_CONFIG */
u_register_t rsi_realm_config(struct rsi_realm_config *s);

/* Function to enter aux plane. See RSI_PLANE_ENTER */
u_register_t rsi_plane_enter(u_register_t plane_index, u_register_t run);

#endif /* REALM_RSI_H */
