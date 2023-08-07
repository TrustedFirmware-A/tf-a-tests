#
# Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
TESTS_SOURCES   +=      $(addprefix tftf/tests/plat/xilinx/common/,            \
        plat_pm.c                                                              \
)


include tftf/tests/tests-standard.mk
TESTS_SOURCES += $(sort ${TESTS_SOURCES})
