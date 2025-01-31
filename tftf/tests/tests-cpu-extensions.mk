#
# Copyright (c) 2018-2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/,			\
	extensions/afp/test_afp.c					\
	extensions/amu/test_amu.c					\
	extensions/brbe/test_brbe.c					\
	extensions/d128/test_d128.c					\
	extensions/debugv8p9/test_debugv8p9.c				\
	extensions/ecv/test_ecv.c					\
	extensions/fgt/test_fgt.c					\
	extensions/fpmr/test_fpmr.c					\
	extensions/ls64/test_ls64.c 					\
	extensions/ls64/ls64_operations.S				\
	extensions/mpam/test_mpam.c					\
	extensions/mte/test_mte.c					\
	extensions/pauth/test_pauth.c					\
	extensions/pmuv3/test_pmuv3.c					\
	extensions/sctlr2/test_sctlr2.c					\
	extensions/sme/test_sme.c					\
	extensions/sme/test_sme2.c					\
	extensions/spe/test_spe.c					\
	extensions/sve/test_sve.c					\
	extensions/sys_reg_trace/test_sys_reg_trace.c			\
	extensions/the/test_the.c					\
	extensions/trbe/test_trbe.c					\
	extensions/trf/test_trf.c					\
	extensions/wfxt/test_wfxt.c					\
	runtime_services/arm_arch_svc/smccc_arch_soc_id.c		\
	runtime_services/arm_arch_svc/smccc_arch_workaround_1.c		\
	runtime_services/arm_arch_svc/smccc_arch_workaround_2.c		\
	runtime_services/arm_arch_svc/smccc_arch_workaround_3.c		\
	runtime_services/arm_arch_svc/smccc_feature_availability.c	\
	runtime_services/arm_arch_svc/smccc_arch_workaround_4.c		\
)
