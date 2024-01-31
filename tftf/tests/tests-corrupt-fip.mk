#
# Copyright (c) 2023, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/tbb-tests/,	\
	test_tbb_corrupt_fip.c						\
	tbb_test_infra.c						\
)

TESTS_SOURCES	+=	plat/common/fwu_nvm_accessors.c \
	plat/arm/common/arm_fwu_io_storage.c \
	drivers/io/io_fip.c \
	drivers/io/io_memmap.c
