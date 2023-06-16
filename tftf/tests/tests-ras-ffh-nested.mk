#
# Copyright (c) 2023, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/misc_tests/,		\
	inject_ras_error.S 						\
	test_ras_ffh_nested.c						\
)
