/*
 * Copyright (c) 2023-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <arg_struct_def.h>
#include "constraint.h"
#include <fuzz_names.h>
#include <psci_fuzz_helper.h>
#include "vec_container.h"

#include <debug.h>
#include <plat_topology.h>

#ifdef PSCI_INCLUDE

#define PSCI_VERSION_FID		    	U(0x84000000)
#define PSCI_CPU_SUSPEND_AARCH32_FID		U(0x84000001)
#define PSCI_CPU_SUSPEND_AARCH64_FID		U(0xc4000001)
#define PSCI_CPU_OFF_FID		    	U(0x84000002)
#define PSCI_CPU_ON_AARCH32_FID	     		U(0x84000003)
#define PSCI_CPU_ON_AARCH64_FID	     		U(0xc4000003)
#define PSCI_AFFINITY_INFO_AARCH32_FID      	U(0x84000004)
#define PSCI_AFFINITY_INFO_AARCH64_FID      	U(0xc4000004)
#define PSCI_MIG_AARCH32_FID			U(0x84000005)
#define PSCI_MIG_AARCH64_FID			U(0xc4000005)
#define PSCI_MIG_INFO_TYPE_FID	      		U(0x84000006)
#define PSCI_MIG_INFO_UP_CPU_AARCH32_FID    	U(0x84000007)
#define PSCI_MIG_INFO_UP_CPU_AARCH64_FID    	U(0xc4000007)
#define PSCI_SYSTEM_OFF_FID		 	U(0x84000008)
#define PSCI_SYSTEM_RESET_FID	       		U(0x84000009)
#define PSCI_FEATURES_FID		   	U(0x8400000A)
#define PSCI_NODE_HW_STATE_AARCH32_FID      	U(0x8400000d)
#define PSCI_NODE_HW_STATE_AARCH64_FID      	U(0xc400000d)
#define PSCI_SYSTEM_SUSPEND_AARCH32_FID     	U(0x8400000E)
#define PSCI_SYSTEM_SUSPEND_AARCH64_FID     	U(0xc400000E)
#define PSCI_SET_SUSPEND_MODE_FID	   	U(0x8400000F)
#define PSCI_STAT_RESIDENCY_AARCH32_FID     	U(0x84000010)
#define PSCI_STAT_RESIDENCY_AARCH64_FID     	U(0xc4000010)
#define PSCI_STAT_COUNT_AARCH32_FID	 	U(0x84000011)
#define PSCI_STAT_COUNT_AARCH64_FID	 	U(0xc4000011)
#define PSCI_SYSTEM_RESET2_AARCH32_FID      	U(0x84000012)
#define PSCI_SYSTEM_RESET2_AARCH64_FID      	U(0xc4000012)
#define PSCI_MEM_PROTECT_FID			U(0x84000013)
#define PSCI_MEM_CHK_RANGE_AARCH32_FID      	U(0x84000014)
#define PSCI_MEM_CHK_RANGE_AARCH64_FID      	U(0xc4000014)


void pushnegval(struct vec_container *vcin, struct vec_container *vcout, int brange, int nval, struct memmod *mmod)
{
	uint64_t rnum;
	int fnum;

	for (int i = 0; i < nval; i++) {
		rnum = rand() % (1 << brange);
		fnum = 0;
		while (fnum == 0) {
			if (vec_containerelem(vcin, rnum)) {
				rnum = rand() % (1 << brange);
			} else {
				pushvec(&rnum, vcout, mmod);
				fnum = 1;
			}
		}
	}
}

test_result_t run_psci_fuzz(int funcid, struct memmod *mmod)
{
	int ntest = 0;
	if (funcid == psci_version_funcid) {
		long long ret;

		ret = tftf_get_psci_version();

		if (ret <= 0) {
			return TEST_RESULT_FAIL;
		}
	}
	if (funcid == psci_features_funcid) {
		long long ret;
		int ntest = 0;

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {

			ntest = rand() % 2;

			if (ntest == 1) {
				int negalter = rand() % 2;

				if (negalter == 0) {
					uint64_t fidneg[4];

					fidneg[0] = PSCI_MIG_AARCH32_FID;
					fidneg[1] = PSCI_MIG_AARCH64_FID;
					fidneg[2] = PSCI_SYSTEM_RESET2_AARCH32_FID;
					fidneg[3] = PSCI_SYSTEM_RESET2_AARCH64_FID;
					setconstraint(FUZZER_CONSTRAINT_VECTOR, fidneg, 4,
					PSCI_FEATURES_CALL_ARG1_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					uint64_t fid[26];
					uint64_t lcode[26];
					uint64_t hcode[26];

					fid[0] = PSCI_VERSION_FID;
					fid[1] = PSCI_CPU_SUSPEND_AARCH32_FID;
					fid[2] = PSCI_CPU_SUSPEND_AARCH64_FID;
					fid[3] = PSCI_CPU_OFF_FID;
					fid[4] = PSCI_CPU_ON_AARCH32_FID;
					fid[5] = PSCI_CPU_ON_AARCH64_FID;
					fid[6] = PSCI_AFFINITY_INFO_AARCH32_FID;
					fid[7] = PSCI_AFFINITY_INFO_AARCH64_FID;
					fid[8] = PSCI_MIG_INFO_TYPE_FID;
					fid[9] = PSCI_MIG_INFO_UP_CPU_AARCH32_FID;
					fid[10] = PSCI_MIG_INFO_UP_CPU_AARCH64_FID;
					fid[11] = PSCI_SYSTEM_OFF_FID;
					fid[12] = PSCI_SYSTEM_RESET_FID;
					fid[13] = PSCI_FEATURES_FID;
					fid[14] = PSCI_NODE_HW_STATE_AARCH32_FID;
					fid[15] = PSCI_NODE_HW_STATE_AARCH64_FID;
					fid[16] = PSCI_SYSTEM_SUSPEND_AARCH32_FID;
					fid[17] = PSCI_SYSTEM_SUSPEND_AARCH64_FID;
					fid[18] = PSCI_SET_SUSPEND_MODE_FID;
					fid[19] = PSCI_STAT_RESIDENCY_AARCH32_FID;
					fid[20] = PSCI_STAT_RESIDENCY_AARCH64_FID;
					fid[21] = PSCI_STAT_COUNT_AARCH32_FID;
					fid[22] = PSCI_STAT_COUNT_AARCH64_FID;
					fid[23] = PSCI_MEM_PROTECT_FID;
					fid[24] = PSCI_MEM_CHK_RANGE_AARCH32_FID;
					fid[25] = PSCI_MEM_CHK_RANGE_AARCH64_FID;

					for (int i = 0; i < 26; i++) {
						lcode[i] = fid[i] & 0xFF;
						hcode[i] = fid[i] & 0xFF000000;
					}

					int flc = 0;
					uint64_t rlc;

					while (flc == 0) {
						rlc = rand() % 256;
						int mat = 0;
						for (int i = 0; i < 26; i++) {
							if (rlc == lcode[i]) {
								mat = 1;
								break;
							}
						}
						if (mat == 0) {
							flc = 1;
						}
					}

					rlc = rlc | hcode[rand() % 26];
					setconstraint(FUZZER_CONSTRAINT_SVALUE, &rlc, 1,
					PSCI_FEATURES_CALL_ARG1_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
			} else {

				uint64_t fid[26];

				fid[0] = PSCI_VERSION_FID;
				fid[1] = PSCI_CPU_SUSPEND_AARCH32_FID;
				fid[2] = PSCI_CPU_SUSPEND_AARCH64_FID;
				fid[3] = PSCI_CPU_OFF_FID;
				fid[4] = PSCI_CPU_ON_AARCH32_FID;
				fid[5] = PSCI_CPU_ON_AARCH64_FID;
				fid[6] = PSCI_AFFINITY_INFO_AARCH32_FID;
				fid[7] = PSCI_AFFINITY_INFO_AARCH64_FID;
				fid[8] = PSCI_MIG_INFO_TYPE_FID;
				fid[9] = PSCI_MIG_INFO_UP_CPU_AARCH32_FID;
				fid[10] = PSCI_MIG_INFO_UP_CPU_AARCH64_FID;
				fid[11] = PSCI_SYSTEM_OFF_FID;
				fid[12] = PSCI_SYSTEM_RESET_FID;
				fid[13] = PSCI_FEATURES_FID;
				fid[14] = PSCI_NODE_HW_STATE_AARCH32_FID;
				fid[15] = PSCI_NODE_HW_STATE_AARCH64_FID;
				fid[16] = PSCI_SYSTEM_SUSPEND_AARCH32_FID;
				fid[17] = PSCI_SYSTEM_SUSPEND_AARCH64_FID;
				fid[18] = PSCI_SET_SUSPEND_MODE_FID;
				fid[19] = PSCI_STAT_RESIDENCY_AARCH32_FID;
				fid[20] = PSCI_STAT_RESIDENCY_AARCH64_FID;
				fid[21] = PSCI_STAT_COUNT_AARCH32_FID;
				fid[22] = PSCI_STAT_COUNT_AARCH64_FID;
				fid[23] = PSCI_MEM_PROTECT_FID;
				fid[24] = PSCI_MEM_CHK_RANGE_AARCH32_FID;
				fid[25] = PSCI_MEM_CHK_RANGE_AARCH64_FID;

				setconstraint(FUZZER_CONSTRAINT_VECTOR, fid, 26,
				PSCI_FEATURES_CALL_ARG1_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
			}
		}

		struct inputparameters inp = generate_args(PSCI_FEATURES_CALL, SMC_FUZZ_SANITY_LEVEL);

		ret = tftf_get_psci_feature_info(inp.x1);

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			if (ntest == 0) {
				if (ret < 0) {
					return TEST_RESULT_FAIL;
				}
			} else {
				if (ret >= 0) {
					return TEST_RESULT_FAIL;
				}
			}
		} else {
			if (ret < -10) {
				return TEST_RESULT_FAIL;
			}
		}
	}
	if (funcid == psci_affinity_info_funcid) {
		long long ret;

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			int cpu_node;
			u_register_t target_mpid;
			struct vec_container vcaff2;
			struct vec_container vcaff1;

			vec_containerinit(&vcaff2, sizeof(uint64_t), mmod);
			vec_containerinit(&vcaff1, sizeof(uint64_t), mmod);
			for_each_cpu(cpu_node) {
				target_mpid = tftf_get_mpidr_from_node(cpu_node);

				uint64_t bitaff2 = (((uint64_t)target_mpid) >> 16) & 0xFF;
				uint64_t bitaff1 = (((uint64_t)target_mpid) >> 8) & 0xFF;

				pushvec(&bitaff2, &vcaff2, mmod);
				pushvec(&bitaff1, &vcaff1, mmod);
			}

			ntest = rand() % 2;

			if (ntest == 1) {
				struct vec_container vcaff2neg;
				struct vec_container vcaff1neg;

				vec_containerinit(&vcaff2neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcaff1neg, sizeof(uint64_t), mmod);

				pushnegval(&vcaff2, &vcaff2neg, 8, 10, mmod);
				pushnegval(&vcaff1, &vcaff1neg, 8, 10, mmod);

				int mixneg = rand() % 3;
				mixneg++;

				if (((mixneg >> 1) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2neg.elements, vcaff2neg.nele,
					PSCI_AFFINITY_INFO_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
					PSCI_AFFINITY_INFO_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if (((mixneg) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1neg.elements, vcaff1neg.nele,
					PSCI_AFFINITY_INFO_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
					PSCI_AFFINITY_INFO_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}

				vec_containerfree(&vcaff2neg, mmod);
				vec_containerfree(&vcaff1neg, mmod);
				vec_containerfree(&vcaff2, mmod);
				vec_containerfree(&vcaff1, mmod);
			} else {
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
				PSCI_AFFINITY_INFO_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
				PSCI_AFFINITY_INFO_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				vec_containerfree(&vcaff2, mmod);
				vec_containerfree(&vcaff1, mmod);
			}
		}


		struct inputparameters inp = generate_args(PSCI_AFFINITY_INFO_AARCH64_CALL, SMC_FUZZ_SANITY_LEVEL);

		ret = tftf_psci_affinity_info(inp.x1, inp.x2);

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			if (ntest == 0) {
				if (ret < 0) {
					return TEST_RESULT_FAIL;
				}
			} else {
				if (ret >= 0) {
					return TEST_RESULT_FAIL;
				}
			}
		} else {
			if (ret < -10) {
				return TEST_RESULT_FAIL;
			}
		}
	}
	if (funcid == psci_stat_residency_funcid) {
		long long ret = 0;
		int ntest = rand() % 2;

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			int cpu_node;
			u_register_t target_mpid;
			uint64_t affval;
			struct vec_container vcaff2;
			struct vec_container vcaff1;
			struct vec_container vcaff0;

			vec_containerinit(&vcaff2, sizeof(uint64_t), mmod);
			vec_containerinit(&vcaff1, sizeof(uint64_t), mmod);
			vec_containerinit(&vcaff0, sizeof(uint64_t), mmod);

			for_each_cpu(cpu_node) {
				target_mpid = tftf_get_mpidr_from_node(cpu_node);
				affval = ((target_mpid >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff2, mmod);
				affval = ((target_mpid >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff1, mmod);
				affval = ((target_mpid >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff0, mmod);
			}

			struct vec_container vcpl;
			struct vec_container vcst;

			vec_containerinit(&vcpl, sizeof(uint64_t), mmod);
			vec_containerinit(&vcst, sizeof(uint64_t), mmod);

			uint64_t val = 0;

			pushvec(&val, &vcpl, mmod);
			pushvec(&val, &vcst, mmod);
			val = 1;
			pushvec(&val, &vcpl, mmod);
			pushvec(&val, &vcst, mmod);
			val = 2;
			pushvec(&val, &vcpl, mmod);

			int nvar = 0;
			if (ntest == 1) {
				struct vec_container vcaff2neg;
				struct vec_container vcaff1neg;
				struct vec_container vcaff0neg;
				struct vec_container vcplneg;

				vec_containerinit(&vcaff2neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcaff1neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcaff0neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcplneg, sizeof(uint64_t), mmod);

				pushnegval(&vcaff2, &vcaff2neg, 8, 5, mmod);
				pushnegval(&vcaff1, &vcaff1neg, 8, 5, mmod);
				pushnegval(&vcaff0, &vcaff0neg, 8, 5, mmod);
				val = 3;
				pushvec(&val, &vcplneg, mmod);

				nvar = rand() % ((1 << 4) - 1) + 1;
				if (((nvar >> 3) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2neg.elements, vcaff2neg.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if (((nvar >> 2) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1neg.elements, vcaff1neg.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if (((nvar >> 1) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0neg.elements, vcaff0neg.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0.elements, vcaff0.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if ((nvar & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_SVALUE, vcplneg.elements, vcplneg.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG2_POWER_STATE, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_SVALUE, vcpl.elements, vcpl.nele,
					PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG2_POWER_STATE, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				vec_containerfree(&vcplneg, mmod);
				vec_containerfree(&vcaff2neg, mmod);
				vec_containerfree(&vcaff1neg, mmod);
				vec_containerfree(&vcaff0neg, mmod);
			} else {

				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
				PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
				PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0.elements, vcaff0.nele,
				PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcpl.elements, vcpl.nele,
				PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG2_POWER_STATE, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcst.elements, vcst.nele,
				PSCI_STAT_RESIDENCY_AARCH64_CALL_ARG2_STATETYPE, mmod, FUZZER_CONSTRAINT_EXCMODE);
			}
			vec_containerfree(&vcpl, mmod);
			vec_containerfree(&vcst, mmod);
			vec_containerfree(&vcaff2, mmod);
			vec_containerfree(&vcaff1, mmod);
			vec_containerfree(&vcaff0, mmod);

		}

		struct inputparameters inp = generate_args(PSCI_STAT_RESIDENCY_AARCH64_CALL, SMC_FUZZ_SANITY_LEVEL);

		ret = tftf_psci_stat_residency(inp.x1, inp.x2);

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			if (ntest == 0) {
				if (ret == 0) {
					return TEST_RESULT_FAIL;
				}
			} else {
				if (ret != 0) {
					return TEST_RESULT_FAIL;
				}
			}
		} else {
			if (ret < -10) {
				return TEST_RESULT_FAIL;
			}
		}
	}
	if (funcid == psci_stat_count_funcid) {
		long long ret;
		int ntest = rand() % 2;

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			int cpu_node;
			u_register_t target_mpid;
			uint64_t affval;
			struct vec_container vcaff2;
			struct vec_container vcaff1;
			struct vec_container vcaff0;

			vec_containerinit(&vcaff2, sizeof(uint64_t), mmod);
			vec_containerinit(&vcaff1, sizeof(uint64_t), mmod);
			vec_containerinit(&vcaff0, sizeof(uint64_t), mmod);

			for_each_cpu(cpu_node) {
				target_mpid = tftf_get_mpidr_from_node(cpu_node);
				affval = ((target_mpid >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff2, mmod);
				affval = ((target_mpid >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff1, mmod);
				affval = ((target_mpid >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff0, mmod);
			}

			struct vec_container vcpl;
			struct vec_container vcst;

			vec_containerinit(&vcpl, sizeof(uint64_t), mmod);
			vec_containerinit(&vcst, sizeof(uint64_t), mmod);

			uint64_t val = 0;

			pushvec(&val, &vcpl, mmod);
			pushvec(&val, &vcst, mmod);
			val = 1;
			pushvec(&val, &vcpl, mmod);
			pushvec(&val, &vcst, mmod);
			val = 2;
			pushvec(&val, &vcpl, mmod);

			int nvar = 0;
			if (ntest == 1) {
				struct vec_container vcaff2neg;
				struct vec_container vcaff1neg;
				struct vec_container vcaff0neg;
				struct vec_container vcplneg;

				vec_containerinit(&vcaff2neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcaff1neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcaff0neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcplneg, sizeof(uint64_t), mmod);

				pushnegval(&vcaff2, &vcaff2neg, 8, 5, mmod);
				pushnegval(&vcaff1, &vcaff1neg, 8, 5, mmod);
				pushnegval(&vcaff0, &vcaff0neg, 8, 5, mmod);
				val = 3;
				pushvec(&val, &vcplneg, mmod);

				nvar = rand() % ((1 << 4) - 1) + 1;
				if (((nvar >> 3) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2neg.elements, vcaff2neg.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if (((nvar >> 2) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1neg.elements, vcaff1neg.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if (((nvar >> 1) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0neg.elements, vcaff0neg.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0.elements, vcaff0.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if ((nvar & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_SVALUE, vcplneg.elements, vcplneg.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG2_POWER_STATE, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_SVALUE, vcpl.elements, vcpl.nele,
					PSCI_STAT_COUNT_AARCH64_CALL_ARG2_POWER_STATE, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				vec_containerfree(&vcplneg, mmod);
				vec_containerfree(&vcaff2neg, mmod);
				vec_containerfree(&vcaff1neg, mmod);
				vec_containerfree(&vcaff0neg, mmod);
			} else {

				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
				PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
				PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0.elements, vcaff0.nele,
				PSCI_STAT_COUNT_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcpl.elements, vcpl.nele,
				PSCI_STAT_COUNT_AARCH64_CALL_ARG2_POWER_STATE, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcst.elements, vcst.nele,
				PSCI_STAT_COUNT_AARCH64_CALL_ARG2_STATETYPE, mmod, FUZZER_CONSTRAINT_EXCMODE);
			}
			vec_containerfree(&vcpl, mmod);
			vec_containerfree(&vcst, mmod);
			vec_containerfree(&vcaff2, mmod);
			vec_containerfree(&vcaff1, mmod);
			vec_containerfree(&vcaff0, mmod);
		}

		struct inputparameters inp = generate_args(PSCI_STAT_COUNT_AARCH64_CALL, SMC_FUZZ_SANITY_LEVEL);

		ret = tftf_psci_stat_count(inp.x1, inp.x2);

		if (ret < 0) {
			return TEST_RESULT_FAIL;
		}
	}
	if (funcid == psci_node_hw_state_funcid) {
		long long ret;
		int ntest = rand() % 2;

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			int cpu_node;
			u_register_t target_mpid;
			uint64_t affval;
			struct vec_container vcaff2;
			struct vec_container vcaff1;
			struct vec_container vcaff0;

			vec_containerinit(&vcaff2, sizeof(uint64_t), mmod);
			vec_containerinit(&vcaff1, sizeof(uint64_t), mmod);
			vec_containerinit(&vcaff0, sizeof(uint64_t), mmod);

			for_each_cpu(cpu_node) {
				target_mpid = tftf_get_mpidr_from_node(cpu_node);
				affval = ((target_mpid >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff2, mmod);
				affval = ((target_mpid >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff1, mmod);
				affval = ((target_mpid >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK);
				pushvec((uint64_t *)&affval, &vcaff0, mmod);
			}

			struct vec_container vcpl;

			vec_containerinit(&vcpl, sizeof(uint64_t), mmod);

			uint64_t val = 0;

			pushvec(&val, &vcpl, mmod);
			val = 1;
			pushvec(&val, &vcpl, mmod);

			int nvar = 0;
			if (ntest == 1) {
				struct vec_container vcaff2neg;
				struct vec_container vcaff1neg;
				struct vec_container vcaff0neg;
				struct vec_container vcplneg;

				vec_containerinit(&vcaff2neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcaff1neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcaff0neg, sizeof(uint64_t), mmod);
				vec_containerinit(&vcplneg, sizeof(uint64_t), mmod);

				pushnegval(&vcaff2, &vcaff2neg, 8, 5, mmod);
				pushnegval(&vcaff1, &vcaff1neg, 8, 5, mmod);
				pushnegval(&vcaff0, &vcaff0neg, 8, 5, mmod);
				pushnegval(&vcpl, &vcplneg, 31, 5, mmod);

				nvar = rand() % ((1 << 4) - 1) + 1;
				if (((nvar >> 3) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2neg.elements, vcaff2neg.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if (((nvar >> 2) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1neg.elements, vcaff1neg.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if (((nvar >> 1) & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0neg.elements, vcaff0neg.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0.elements, vcaff0.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				if ((nvar & 1) == 1) {
					setconstraint(FUZZER_CONSTRAINT_SVALUE, vcplneg.elements, vcplneg.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG2_POWER_LEVEL, mmod, FUZZER_CONSTRAINT_EXCMODE);
				} else {
					setconstraint(FUZZER_CONSTRAINT_SVALUE, vcpl.elements, vcpl.nele,
					PSCI_NODE_HW_STATE_AARCH64_CALL_ARG2_POWER_LEVEL, mmod, FUZZER_CONSTRAINT_EXCMODE);
				}
				vec_containerfree(&vcplneg, mmod);
				vec_containerfree(&vcaff2neg, mmod);
				vec_containerfree(&vcaff1neg, mmod);
				vec_containerfree(&vcaff0neg, mmod);
			} else {

				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff2.elements, vcaff2.nele,
				PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF2, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff1.elements, vcaff1.nele,
				PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF1, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcaff0.elements, vcaff0.nele,
				PSCI_NODE_HW_STATE_AARCH64_CALL_ARG1_AFF0, mmod, FUZZER_CONSTRAINT_EXCMODE);
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcpl.elements, vcpl.nele,
				PSCI_NODE_HW_STATE_AARCH64_CALL_ARG2_POWER_LEVEL, mmod, FUZZER_CONSTRAINT_EXCMODE);
			}
			vec_containerfree(&vcpl, mmod);
			vec_containerfree(&vcaff2, mmod);
			vec_containerfree(&vcaff1, mmod);
			vec_containerfree(&vcaff0, mmod);
		}

		struct inputparameters inp = generate_args(PSCI_NODE_HW_STATE_AARCH64_CALL, SMC_FUZZ_SANITY_LEVEL);

		ret = tftf_psci_node_hw_state(inp.x1, inp.x2);

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			if (ntest == 0) {
				if (ret < 0) {
					return TEST_RESULT_FAIL;
				}
			} else {
				if (ret >= 0) {
					return TEST_RESULT_FAIL;
				}
			}
		} else {
			if (ret < -10) {
				return TEST_RESULT_FAIL;
			}
		}
	}
	if (funcid == psci_set_suspend_mode_funcid) {
		long long ret;

		int ntest = rand() % 2;

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			struct vec_container vcmode;

			vec_containerinit(&vcmode, sizeof(uint64_t), mmod);

			uint64_t val = 0;
			pushvec(&val, &vcmode, mmod);
			val = 1;
			pushvec(&val, &vcmode, mmod);

			if (ntest == 1) {
				struct vec_container vcmodeneg;

				vec_containerinit(&vcmodeneg, sizeof(uint64_t), mmod);

				pushnegval(&vcmode, &vcmodeneg, 31, 5, mmod);

				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcmodeneg.elements, vcmodeneg.nele,
					PSCI_SET_SUSPEND_MODE_CALL_ARG1_MODE, mmod, FUZZER_CONSTRAINT_EXCMODE);
				vec_containerfree(&vcmodeneg, mmod);
			} else {
				setconstraint(FUZZER_CONSTRAINT_VECTOR, vcmode.elements, vcmode.nele,
					PSCI_SET_SUSPEND_MODE_CALL_ARG1_MODE, mmod, FUZZER_CONSTRAINT_EXCMODE);
			}

			vec_containerfree(&vcmode, mmod);
		}

		struct inputparameters inp = generate_args(PSCI_SET_SUSPEND_MODE_CALL, SMC_FUZZ_SANITY_LEVEL);

		ret = tftf_psci_set_suspend_mode(inp.x1);

		if (SMC_FUZZ_SANITY_LEVEL  == 3) {
			if (ntest == 0) {
				if (ret < 0) {
					return TEST_RESULT_FAIL;
				}
			} else {
				if (ret >= 0) {
					return TEST_RESULT_FAIL;
				}
			}
		} else {
			if (ret < -10) {
				return TEST_RESULT_FAIL;
			}
		}
	}
	return TEST_RESULT_SUCCESS;
}

#endif
