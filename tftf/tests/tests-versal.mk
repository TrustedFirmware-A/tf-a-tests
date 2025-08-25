#
# Copyright (c) 2022-2025, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TFTF_INCLUDES   +=	-Itftf/tests/plat/amd/common/common_files/

TESTS_SOURCES	+=	$(wildcard tftf/tests/plat/amd/common/*/*.c)

include tftf/tests/tests-standard.mk
TESTS_SOURCES += $(sort ${TESTS_SOURCES})
