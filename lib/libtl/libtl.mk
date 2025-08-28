#
# Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

LIBTL_PATH	?= contrib/libtl

INCLUDES	+= -I$(LIBTL_PATH)/include/ \
		 -I$(LIBTL_PATH)/include/private/

LIBTL_SOURCES	+= $(addprefix ${LIBTL_PATH}/src/generic/, \
			transfer_list.c	\
			tpm_event_log.c \
			logging.c	\
			)
