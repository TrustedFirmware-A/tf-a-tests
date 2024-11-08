#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include tftf/tests/tests-spm.mk

TESTS_SOURCES	+=	$(addprefix tftf/tests/,				\
		context_mgmt_tests/el1/test_tsp_el1_context_mgmt.c		\
		context_mgmt_tests/el2/test_spm_el2_context_mgmt.c		\
)
