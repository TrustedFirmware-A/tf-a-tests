#
# Copyright (c) 2025-2026, Arm Limited, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=						\
	$(addprefix tftf/tests/plat/arm/automotive_rd/rdaspen/,	\
		test_rdaspen_cpu_uc_ras.c			\
	)

include tftf/tests/tests-standard.mk
TESTS_SOURCES += $(sort ${TESTS_SOURCES})

