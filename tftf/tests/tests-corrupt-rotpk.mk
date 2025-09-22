#
# Copyright (c) 2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/neg_scenario_tests/,	\
					test_invalid_rotpk.c \
					neg_scenario_test_infra.c \
)

include lib/ext_mbedtls/mbedtls.mk


