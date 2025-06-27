#
# Copyright (c) 2023, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq (${TRANSFER_LIST}, 1)

include lib/event_log/event_log.mk
include lib/libtl/libtl.mk

TESTS_SOURCES	+=	$(addprefix tftf/tests/misc_tests/,		\
	test_firmware_handoff.c						\
)

TESTS_SOURCES	+=	${EVENT_LOG_SOURCES} ${LIBTL_SOURCES}

endif
