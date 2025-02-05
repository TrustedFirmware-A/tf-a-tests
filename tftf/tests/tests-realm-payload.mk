#
# Copyright (c) 2021-2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq (${ARCH},aarch64)
TFTF_INCLUDES +=							\
	-Iinclude/runtime_services/host_realm_managment

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/realm_payload/,		\
		host_realm_payload_multiple_rec_tests.c			\
		host_realm_payload_tests.c				\
		host_realm_simd_common.c				\
		host_realm_spm.c					\
		host_realm_payload_simd_tests.c				\
		host_realm_lpa2_tests.c					\
		host_realm_mpam_tests.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_pmuv3.c						\
		host_realm_rmi.c					\
		host_realm_helper.c					\
		host_shared_data.c					\
		rmi_delegate_tests.c					\
		host_rmi_pdev.c						\
		host_crypto_utils.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		${ARCH}/ffa_arch_helpers.S				\
		ffa_helpers.c						\
		spm_common.c						\
		spm_test_helpers.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix lib/heap/,						\
		page_alloc.c						\
	)

TESTS_SOURCES	+=							\
	$(addprefix lib/extensions/fpu/,				\
		fpu.c							\
	)

TESTS_SOURCES	+=				\
	$(addprefix tftf/tests/doe_tests/,	\
		doe_helpers.c			\
		test_doe.c			\
	)

TESTS_SOURCES	+=		\
	$(addprefix lib/pcie/,	\
		pcie.c		\
		pcie_doe.c	\
	)

include lib/ext_mbedtls/mbedtls.mk

endif
