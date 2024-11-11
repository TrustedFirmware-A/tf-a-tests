#
# Copyright (c) 2018-2023, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TFTF_INCLUDES +=							\
	-Iinclude/runtime_services/host_realm_managment

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		${ARCH}/ffa_arch_helpers.S				\
		ffa_helpers.c						\
		spm_common.c						\
		spm_test_helpers.c					\
		test_ffa_direct_messaging.c				\
		test_ffa_indirect_messaging.c				\
		test_ffa_interrupts.c					\
		test_ffa_secure_interrupts.c				\
		test_ffa_memory_sharing.c				\
		test_ffa_setup_and_discovery.c				\
		test_ffa_notifications.c				\
		test_spm_smmu.c						\
		test_ffa_exceptions.c					\
		test_ffa_group0_interrupts.c				\
		test_ffa_arch_timer.c					\
	)

ifeq (${ARCH},aarch64)
TESTS_SOURCES   +=                                                      \
        $(addprefix tftf/tests/runtime_services/secure_service/,        \
	  test_spm_simd.c					\
	 )

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_realm_rmi.c					\
		host_realm_helper.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		test_ffa_smccc.c					\
		test_ffa_smccc_asm.S					\
		test_spm_context_mgmt.c					\
	)

TESTS_SOURCES   += lib/extensions/fpu/fpu.c
endif
