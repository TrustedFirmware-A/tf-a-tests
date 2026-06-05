#
# Copyright (c) 2025-2026, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=						\
	$(addprefix tftf/tests/runtime_services/firme_service/,	\
		test_firme_service.c				\
		test_firme_ide_km.c				\
	)

TESTS_SOURCES	+=						\
	$(addprefix lib/pcie/,					\
		pcie.c						\
		pcie_dvsec_rmeda.c				\
	)
