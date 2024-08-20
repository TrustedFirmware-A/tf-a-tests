#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		${ARCH}/ffa_arch_helpers.S				\
		ffa_helpers.c						\
		spm_common.c						\
		spm_test_helpers.c					\
	)

ifeq (${ARCH},aarch64)
TESTS_SOURCES   +=                                                      \
        $(addprefix tftf/tests/runtime_services/secure_service/,        \
	  test_spm_simd.c					\
	 )

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		test_ffa_smccc.c					\
		test_ffa_smccc_asm.S					\
	)

TESTS_SOURCES   += lib/extensions/fpu/fpu.c
endif
