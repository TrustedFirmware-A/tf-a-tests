#
# Copyright (c) 2023, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq (${TRANSFER_LIST}, 1)

TESTS_SOURCES	+=	$(addprefix tftf/tests/misc_tests/,		\
	test_firmware_handoff.c						\
)

endif
