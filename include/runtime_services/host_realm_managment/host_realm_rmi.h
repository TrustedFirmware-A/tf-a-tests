/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HOST_REALM_RMI_H
#define HOST_REALM_RMI_H

#include <stdint.h>
#include <stdbool.h>

#include <realm_def.h>
#include <smccc.h>
#include <utils_def.h>

#define RMI_FNUM_MIN_VALUE	U(0x150)
#define RMI_FNUM_MAX_VALUE	U(0x18F)

/* Get RMI fastcall std FID from offset */
#define SMC64_RMI_FID(_offset)					\
	((SMC_TYPE_FAST << FUNCID_TYPE_SHIFT) |			\
	(SMC_64 << FUNCID_CC_SHIFT) |				\
	(OEN_STD_START << FUNCID_OEN_SHIFT) |			\
	(((RMI_FNUM_MIN_VALUE + (_offset)) & FUNCID_NUM_MASK)	\
	<< FUNCID_NUM_SHIFT))

#define RMI_ABI_VERSION_GET_MAJOR(_version)	(((_version) >> 16U) & 0x8FFF)
#define RMI_ABI_VERSION_GET_MINOR(_version)	((_version) & 0xFFFF)
#define RMI_ABI_VERSION_MAJOR			U(1)
#define RMI_ABI_VERSION_MINOR			U(0)
#define RMI_ABI_VERSION_VAL			((RMI_ABI_VERSION_MAJOR << 16U) | \
						 RMI_ABI_VERSION_MINOR)

#define __ALIGN_MASK(x, mask)		(((x) + (mask)) & ~(mask))
#define __ALIGN(x, a)			__ALIGN_MASK(x, (typeof(x))(a) - 1U)
#define ALIGN(x, a)			__ALIGN((x), (a))

/*
 * SMC_RMM_INIT_COMPLETE is the only function in the RMI that originates from
 * the Realm world and is handled by the RMMD. The remaining functions are
 * always invoked by the Normal world, forwarded by RMMD and handled by the
 * RMM
 */
/* RMI SMC64 FIDs handled by the RMMD */
/* no parameters */
#define RMI_VERSION			SMC64_RMI_FID(U(0x0))

/*
 * arg0 == target granule address
 */
#define RMI_GRANULE_DELEGATE		SMC64_RMI_FID(U(0x1))

/*
 * arg0 == target granule address
 */
#define RMI_GRANULE_UNDELEGATE		SMC64_RMI_FID(U(0x2))

/*
 * arg0 == RD address
 * arg1 == data address
 * arg2 == map address
 * arg3 == SRC address
 */
#define RMI_DATA_CREATE			SMC64_RMI_FID(U(0x3))

/*
 * arg0 == RD address
 * arg1 == data address
 * arg2 == map address
 */
#define RMI_DATA_CREATE_UNKNOWN		SMC64_RMI_FID(U(0x4))

/*
 * arg0 == RD address
 * arg1 == map address
 *
 * ret1 == Address(PA) of the DATA granule, if ret0 == RMI_SUCCESS.
 *         Otherwise, undefined.
 * ret2 == Top of the non-live address region. Only valid
 *         if ret0 == RMI_SUCCESS or ret0 == (RMI_ERROR_RTT_WALK, x)
 */
#define RMI_DATA_DESTROY		SMC64_RMI_FID(U(0x5))

/*
 * arg0 == RD address
 */
#define RMI_REALM_ACTIVATE		SMC64_RMI_FID(U(0x7))

/*
 * arg0 == RD address
 * arg1 == struct rmi_realm_params address
 */
#define RMI_REALM_CREATE		SMC64_RMI_FID(U(0x8))

/*
 * arg0 == RD address
 */
#define RMI_REALM_DESTROY		SMC64_RMI_FID(U(0x9))

/*
 * arg0 == RD address
 * arg1 == REC address
 * arg2 == struct rmm_rec address
 */
#define RMI_REC_CREATE			SMC64_RMI_FID(U(0xA))

/*
 * arg0 == REC address
 */
#define RMI_REC_DESTROY			SMC64_RMI_FID(U(0xB))

/*
 * arg0 == rec address
 * arg1 == struct rec_run address
 */
#define RMI_REC_ENTER			SMC64_RMI_FID(U(0xC))

/*
 * arg0 == RD address
 * arg1 == RTT address
 * arg2 == map address
 * arg3 == level
 */
#define RMI_RTT_CREATE			SMC64_RMI_FID(U(0xD))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 *
 * ret1 == Address (PA) of the RTT, if ret0 == RMI_SUCCESS
 *         Otherwise, undefined.
 * ret2 == Top of the non-live address region. Only valid
 *         if ret0 == RMI_SUCCESS or ret0 == (RMI_ERROR_RTT_WALK, x)
 */
#define RMI_RTT_DESTROY			SMC64_RMI_FID(U(0xE))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 * arg3 == s2tte
 */
#define RMI_RTT_MAP_UNPROTECTED		SMC64_RMI_FID(U(0xF))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 * ret1 == level
 * ret2 == s2tte type
 * ret3 == s2tte
 * ret4 == ripas
 */
#define RMI_RTT_READ_ENTRY		SMC64_RMI_FID(U(0x11))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 */
#define RMI_RTT_UNMAP_UNPROTECTED	SMC64_RMI_FID(U(0x12))

/*
 * arg0 == calling rec address
 * arg1 == target rec address
 */
#define RMI_PSCI_COMPLETE		SMC64_RMI_FID(U(0x14))

/*
 * arg0 == Feature register index
 */
#define RMI_FEATURES			SMC64_RMI_FID(U(0x15))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 *
 * ret1 == Address(PA) of the RTT folded, if ret0 == RMI_SUCCESS
 */
#define RMI_RTT_FOLD			SMC64_RMI_FID(U(0x16))

/*
 * arg0 == RD address
 */
#define RMI_REC_AUX_COUNT		SMC64_RMI_FID(U(0x17))

/*
 * arg0 == RD address
 * arg1 == map address
 * arg2 == level
 */
#define RMI_RTT_INIT_RIPAS		SMC64_RMI_FID(U(0x18))

/*
 * arg0 == RD address
 * arg1 == REC address
 * arg2 == map address
 * arg3 == level
 * arg4 == ripas
 */
#define RMI_RTT_SET_RIPAS		SMC64_RMI_FID(U(0x19))

#define GRANULE_SIZE			PAGE_SIZE_4KB

/* Maximum number of auxiliary granules required for a REC */
#define MAX_REC_AUX_GRANULES		16U
#define REC_PARAMS_AUX_GRANULES		16U
#define REC_EXIT_NR_GPRS		31U

/* Size of Realm Personalization Value */
#define RPV_SIZE			64U

/* RmiDisposeResponse types */
#define RMI_DISPOSE_ACCEPT		0U
#define RMI_DISPOSE_REJECT		1U

/* RmiFeature enumerations */
#define RMI_FEATURE_FALSE		0U
#define RMI_FEATURE_TRUE		1U

/* RmiRealmFlags format */
#define RMI_REALM_FLAGS_LPA2		BIT(0)
#define RMI_REALM_FLAGS_SVE		BIT(1)
#define RMI_REALM_FLAGS_PMU		BIT(2)

/* RmiInterfaceVersion type */
#define RMI_MAJOR_VERSION		0U
#define RMI_MINOR_VERSION		0U

/* RmiRealmMeasurementAlgorithm types */
#define RMI_HASH_SHA_256		0U
#define RMI_HASH_SHA_512		1U

/* RmiRecEmulatedMmio types */
#define RMI_NOT_EMULATED_MMIO		0U
#define RMI_EMULATED_MMIO		1U

/*
 * RmiRecExitReason represents the reason for a REC exit.
 * This is returned to NS hosts via RMI_REC_ENTER::run_ptr.
 */
#define RMI_EXIT_SYNC			0U
#define RMI_EXIT_IRQ			1U
#define RMI_EXIT_FIQ			2U
#define RMI_EXIT_PSCI			3U
#define RMI_EXIT_RIPAS_CHANGE		4U
#define RMI_EXIT_HOST_CALL		5U
#define RMI_EXIT_SERROR			6U
#define RMI_EXIT_INVALID		(RMI_EXIT_SERROR + 1U)

/* RmiRecRunnable types */
#define RMI_NOT_RUNNABLE		0U
#define RMI_RUNNABLE			1U

/* RmiRttEntryState: represents the state of an RTTE */
#define RMI_UNASSIGNED			UL(0)
#define RMI_ASSIGNED			UL(1)
#define RMI_TABLE			UL(2)

/* RmmRipas enumeration representing realm IPA state */
#define RMI_EMPTY			UL(0)
#define RMI_RAM				UL(1)
#define RMI_DESTROYED			UL(2)

/* RmiPmuOverflowStatus enumeration representing PMU overflow status */
#define RMI_PMU_OVERFLOW_NOT_ACTIVE	0U
#define RMI_PMU_OVERFLOW_ACTIVE		1U

/* RmiFeatureRegister0 format */
#define RMI_FEATURE_REGISTER_0_S2SZ_SHIFT		0UL
#define RMI_FEATURE_REGISTER_0_S2SZ_WIDTH		8UL
#define RMI_FEATURE_REGISTER_0_LPA2			BIT(8)
#define RMI_FEATURE_REGISTER_0_SVE_EN			BIT(9)
#define RMI_FEATURE_REGISTER_0_SVE_VL_SHIFT		10UL
#define RMI_FEATURE_REGISTER_0_SVE_VL_WIDTH		4UL
#define RMI_FEATURE_REGISTER_0_NUM_BPS_SHIFT		14UL
#define RMI_FEATURE_REGISTER_0_NUM_BPS_WIDTH		4UL
#define RMI_FEATURE_REGISTER_0_NUM_WPS_SHIFT		18UL
#define RMI_FEATURE_REGISTER_0_NUM_WPS_WIDTH		4UL
#define RMI_FEATURE_REGISTER_0_PMU_EN			BIT(22)
#define RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS_SHIFT	23UL
#define RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS_WIDTH	5UL
#define RMI_FEATURE_REGISTER_0_HASH_SHA_256		BIT(28)
#define RMI_FEATURE_REGISTER_0_HASH_SHA_512		BIT(29)

/*
 * Format of feature_flag[63:32].
 * Value -1 indicates not set field, and parameter will be set
 * from the corresponding field of feature register 0.
 */
#define FEATURE_SVE_VL_SHIFT				32UL
#define FEATURE_SVE_VL_WIDTH				8UL
#define FEATURE_NUM_BPS_SHIFT				40UL
#define FEATURE_NUM_BPS_WIDTH				8UL
#define FEATURE_NUM_WPS_SHIFT				48UL
#define FEATURE_NUM_WPS_WIDTH				8UL
#define FEATURE_PMU_NUM_CTRS_SHIFT			56UL
#define FEATURE_PMU_NUM_CTRS_WIDTH			8UL

/* RmiStatusCode types */
/*
 * Status codes which can be returned from RMM commands.
 *
 * For each code, the meaning of return_code_t::index is stated.
 */
typedef enum {
	/*
	 * Command completed successfully.
	 *
	 * index is zero.
	 */
	RMI_SUCCESS = 0,
	/*
	 * The value of a command input value caused the command to fail.
	 *
	 * index is zero.
	 */
	RMI_ERROR_INPUT = 1,
	/*
	 * An attribute of a Realm does not match the expected value.
	 *
	 * index varies between usages.
	 */
	RMI_ERROR_REALM = 2,
	/*
	 * An attribute of a REC does not match the expected value.
	 *
	 * index is zero.
	 */
	RMI_ERROR_REC = 3,
	/*
	 * An RTT walk terminated before reaching the target RTT level,
	 * or reached an RTTE with an unexpected value.
	 *
	 * index: RTT level at which the walk terminated
	 */
	RMI_ERROR_RTT = 4,
	RMI_ERROR_COUNT
} status_t;

#define RMI_RETURN_STATUS(ret)		((ret) & 0xFF)
#define RMI_RETURN_INDEX(ret)		(((ret) >> 8U) & 0xFF)
#define RTT_MAX_LEVEL			3U
#define ALIGN_DOWN(x, a)		((uint64_t)(x) & ~(((uint64_t)(a)) - 1ULL))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a)-1U)) == 0U)
#define PAGE_SHIFT			FOUR_KB_SHIFT
#define RTT_LEVEL_SHIFT(l)		XLAT_ADDR_SHIFT(l)
#define RTT_L2_BLOCK_SIZE		(1UL << RTT_LEVEL_SHIFT(2U))

#define REC_CREATE_NR_GPRS		8U
#define REC_HVC_NR_GPRS			7U
#define REC_GIC_NUM_LRS			16U

/*
 * The Realm attribute parameters are shared by the Host via
 * RMI_REALM_CREATE::params_ptr. The values can be observed or modified
 * either by the Host or by the Realm.
 */
struct rmi_realm_params {
	/* Flags */
	SET_MEMBER(unsigned long flags, 0, 0x8);		/* Offset 0 */
	/* Requested IPA width */
	SET_MEMBER(unsigned int s2sz, 0x8, 0x10);		/* 0x8 */
	/* Requested SVE vector length */
	SET_MEMBER(unsigned int sve_vl, 0x10, 0x18);		/* 0x10 */
	/* Requested number of breakpoints */
	SET_MEMBER(unsigned int num_bps, 0x18, 0x20);		/* 0x18 */
	/* Requested number of watchpoints */
	SET_MEMBER(unsigned int num_wps, 0x20, 0x28);		/* 0x20 */
	/* Requested number of PMU counters */
	SET_MEMBER(unsigned int pmu_num_ctrs, 0x28, 0x30);	/* 0x28 */
	/* Measurement algorithm */
	SET_MEMBER(unsigned char hash_algo, 0x30, 0x400);	/* 0x30 */
	/* Realm Personalization Value */
	SET_MEMBER(unsigned char rpv[RPV_SIZE], 0x400, 0x800);	/* 0x400 */
	SET_MEMBER(struct {
		/* Virtual Machine Identifier */
		unsigned short vmid;				/* 0x800 */
		/* Realm Translation Table base */
		u_register_t rtt_base;				/* 0x808 */
		/* RTT starting level */
		long rtt_level_start;				/* 0x810 */
		/* Number of starting level RTTs */
		unsigned int rtt_num_start;			/* 0x818 */
	}, 0x800, 0x1000);
};

/*
 * The REC attribute parameters are shared by the Host via
 * MI_REC_CREATE::params_ptr. The values can be observed or modified
 * either by the Host or by the Realm which owns the REC.
 */
struct rmi_rec_params {
	/* Flags */
	SET_MEMBER(u_register_t flags, 0, 0x100);		/* Offset 0 */
	/* MPIDR of the REC */
	SET_MEMBER(u_register_t mpidr, 0x100, 0x200);		/* 0x100 */
	/* Program counter */
	SET_MEMBER(u_register_t pc, 0x200, 0x300);		/* 0x200 */
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[REC_CREATE_NR_GPRS], 0x300, 0x800); /* 0x300 */
	SET_MEMBER(struct {
		/* Number of auxiliary Granules */
		u_register_t num_aux;				/* 0x800 */
		/* Addresses of auxiliary Granules */
		u_register_t aux[MAX_REC_AUX_GRANULES];		/* 0x808 */
	}, 0x800, 0x1000);
};

/*
 * Structure contains data passed from the Host to the RMM on REC entry
 */
struct rmi_rec_entry {
	/* Flags */
	SET_MEMBER(u_register_t flags, 0, 0x200);		/* Offset 0 */
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[REC_EXIT_NR_GPRS], 0x200, 0x300); /* 0x200 */
	SET_MEMBER(struct {
		/* GICv3 Hypervisor Control Register */
		u_register_t gicv3_hcr;				/* 0x300 */
		/* GICv3 List Registers */
		u_register_t gicv3_lrs[REC_GIC_NUM_LRS];	/* 0x308 */
	}, 0x300, 0x800);
};

/*
 * Structure contains data passed from the RMM to the Host on REC exit
 */
struct rmi_rec_exit {
	/* Exit reason */
	SET_MEMBER(u_register_t exit_reason, 0, 0x100);/* Offset 0 */
	SET_MEMBER(struct {
		/* Exception Syndrome Register */
		u_register_t esr;				/* 0x100 */
		/* Fault Address Register */
		u_register_t far;				/* 0x108 */
		/* Hypervisor IPA Fault Address register */
		u_register_t hpfar;				/* 0x110 */
	}, 0x100, 0x200);
	/* General-purpose registers */
	SET_MEMBER(u_register_t gprs[REC_EXIT_NR_GPRS], 0x200, 0x300); /* 0x200 */
	SET_MEMBER(struct {
		/* GICv3 Hypervisor Control Register */
		u_register_t gicv3_hcr;				/* 0x300 */
		/* GICv3 List Registers */
		u_register_t gicv3_lrs[REC_GIC_NUM_LRS];	/* 0x308 */
		/* GICv3 Maintenance Interrupt State Register */
		u_register_t gicv3_misr;			/* 0x388 */
		/* GICv3 Virtual Machine Control Register */
		u_register_t gicv3_vmcr;			/* 0x390 */
	}, 0x300, 0x400);
	SET_MEMBER(struct {
		/* Counter-timer Physical Timer Control Register */
		u_register_t cntp_ctl;				/* 0x400 */
		/* Counter-timer Physical Timer CompareValue Register */
		u_register_t cntp_cval;				/* 0x408 */
		/* Counter-timer Virtual Timer Control Register */
		u_register_t cntv_ctl;				/* 0x410 */
		/* Counter-timer Virtual Timer CompareValue Register */
		u_register_t cntv_cval;				/* 0x418 */
	}, 0x400, 0x500);
	SET_MEMBER(struct {
		/* Base address of pending RIPAS change */
		u_register_t ripas_base;			/* 0x500 */
		/* Size of pending RIPAS change */
		u_register_t ripas_size;			/* 0x508 */
		/* RIPAS value of pending RIPAS change */
		unsigned char ripas_value;			/* 0x510 */
	}, 0x500, 0x600);
	/* Host call immediate value */
	SET_MEMBER(unsigned int imm, 0x600, 0x700);		/* 0x600 */
	/* PMU overflow status */
	SET_MEMBER(unsigned long pmu_ovf_status, 0x700, 0x800);	/* 0x700 */
};

/*
 * Structure contains shared information between RMM and Host
 * during REC entry and REC exit.
 */
struct rmi_rec_run {
	/* Entry information */
	SET_MEMBER(struct rmi_rec_entry entry, 0, 0x800);	/* Offset 0 */
	/* Exit information */
	SET_MEMBER(struct rmi_rec_exit exit, 0x800, 0x1000);	/* 0x800 */
};

struct rtt_entry {
	uint64_t walk_level;
	uint64_t out_addr;
	u_register_t state;
	u_register_t ripas;
};

enum realm_state {
	REALM_STATE_NULL,
	REALM_STATE_NEW,
	REALM_STATE_ACTIVE,
	REALM_STATE_SYSTEM_OFF
};

struct realm {
	unsigned int rec_count;
	u_register_t par_base;
	u_register_t par_size;
	u_register_t rd;
	u_register_t rtt_addr;
	u_register_t rec[MAX_REC_COUNT];
	u_register_t run[MAX_REC_COUNT];
	u_register_t rec_flag[MAX_REC_COUNT];
	u_register_t mpidr[MAX_REC_COUNT];
	u_register_t host_mpidr[MAX_REC_COUNT];
	u_register_t num_aux;
	u_register_t rmm_feat_reg0;
	u_register_t ipa_ns_buffer;
	u_register_t ns_buffer_size;
	u_register_t aux_pages_all_rec[MAX_REC_COUNT][REC_PARAMS_AUX_GRANULES];
	uint8_t      sve_vl;
	uint8_t      num_bps;
	uint8_t      num_wps;
	uint8_t      pmu_num_ctrs;
	enum realm_state state;
};

/* RMI/SMC */
u_register_t host_rmi_version(u_register_t req_ver);
u_register_t host_rmi_granule_delegate(u_register_t addr);
u_register_t host_rmi_granule_undelegate(u_register_t addr);
u_register_t host_rmi_realm_create(u_register_t rd, u_register_t params_ptr);
u_register_t host_rmi_realm_destroy(u_register_t rd);
u_register_t host_rmi_features(u_register_t index, u_register_t *features);

/* Realm management */
u_register_t host_realm_create(struct realm *realm);
u_register_t host_realm_map_payload_image(struct realm *realm,
					  u_register_t realm_payload_adr);
u_register_t host_realm_map_ns_shared(struct realm *realm,
					u_register_t ns_shared_mem_adr,
					u_register_t ns_shared_mem_size);
u_register_t host_realm_rec_create(struct realm *realm);
unsigned int host_realm_find_rec_by_mpidr(unsigned int mpidr, struct realm *realm);
u_register_t host_realm_activate(struct realm *realm);
u_register_t host_realm_destroy(struct realm *realm);
u_register_t host_realm_rec_enter(struct realm *realm,
					u_register_t *exit_reason,
					unsigned int *host_call_result,
					unsigned int rec_num);
u_register_t host_realm_init_ipa_state(struct realm *realm, u_register_t level,
					u_register_t start, uint64_t end);
u_register_t host_rmi_psci_complete(u_register_t calling_rec, u_register_t target_rec,
		unsigned long status);
void host_rmi_init_cmp_result(void);
bool host_rmi_get_cmp_result(void);

#endif /* HOST_REALM_RMI_H */
