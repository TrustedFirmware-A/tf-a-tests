#
# Copyright (c) 2025-2026, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=						\
	$(addprefix tftf/tests/runtime_services/firme_service/,	\
		test_firme_attestation.c			\
		test_firme_service.c				\
		test_firme_ide_km.c				\
	)

TESTS_SOURCES	+=						\
	$(addprefix lib/pcie/,					\
		pcie.c						\
		pcie_dvsec_rmeda.c				\
	)

ifeq (${ARCH},aarch64)

TFTF_INCLUDES +=							\
	-Iinclude/runtime_services/host_realm_managment

TESTS_SOURCES	+=							\
	tftf/tests/runtime_services/firme_service/test_firme_attestation_gpf.c

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_realm_rmi.c					\
		host_realm_helper.c					\
	)

TESTS_SOURCES	+=							\
	lib/heap/page_alloc.c

endif
