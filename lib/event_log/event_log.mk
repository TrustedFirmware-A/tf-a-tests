#
# Copyright (c) 2020-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Path to Event Log Library (LibEventLog)
LIBEVLOG_PATH		?=	contrib/libeventlog

INCLUDES		+=	-I$(LIBEVLOG_PATH)/include

LIBEVLOG_SOURCES	+= ${LIBEVLOG_PATH}/src/event_print.c
