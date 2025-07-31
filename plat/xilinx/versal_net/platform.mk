#
# Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

XLNX_COMMON_PATH :=	plat/xilinx/common
VERSAL_NET_PATH :=	plat/xilinx/versal_net

PLAT_INCLUDES	:=	-I${VERSAL_NET_PATH}/include/

PLAT_SOURCES	:=	drivers/arm/pl011/${ARCH}/pl011_console.S       \
			drivers/arm/timer/private_timer.c		\
			drivers/console/console.c                       \
			${VERSAL_NET_PATH}/versal_net_setup.c		\
			${VERSAL_NET_PATH}/versal_net_pwr_state.c	\
			${VERSAL_NET_PATH}/aarch64/plat_helpers.S	\
			${XLNX_COMMON_PATH}/timer/timers.c

PLAT_TESTS_SKIP_LIST    := ${VERSAL_NET_PATH}/tests_to_skip.txt

ifeq ($(USE_NVM),1)
$(error "Versal NET port of TFTF doesn't currently support USE_NVM=1")
endif
