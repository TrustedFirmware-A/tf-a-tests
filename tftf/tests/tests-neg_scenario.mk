#
# Copyright (c) 2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/neg_scenario_tests/,	\
				test_fwu_image_size.c \
)

TESTS_SOURCES	+=	plat/common/fwu_nvm_accessors.c \
				plat/common/image_loader.c \
				plat/arm/common/arm_fwu_io_storage.c
