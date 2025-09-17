#
# Copyright (c) 2020-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

LIBEVLOG_PATH	?= contrib/libeventlog

EVENT_LOG_INCLUDES	+=	-I$(LIBEVLOG_PATH)/include \
				-I$(LIBEVLOG_PATH)/include/private


EVENT_LOG_SOURCES	:=	$(LIBEVLOG_PATH)/src/event_print.c \
				$(LIBEVLOG_PATH)/src/digest.c
