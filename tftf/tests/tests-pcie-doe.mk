#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=			\
	$(addprefix plat/arm/fvp/,	\
		fvp_pcie.c		\
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
