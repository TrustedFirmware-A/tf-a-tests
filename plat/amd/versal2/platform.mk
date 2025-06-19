#
# Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

XLNX_COMMON_PATH :=	plat/xilinx/common
VERSAL2_PATH :=	plat/amd/versal2

PLAT_INCLUDES	:=	-I${XLNX_COMMON_PATH}/include/			\
			-I${VERSAL2_PATH}/include/

PLAT_SOURCES	:=	drivers/arm/gic/arm_gic.c			\
			drivers/arm/gic/gic_v2v3_common.c               \
			drivers/arm/gic/gic_v2.c                        \
			drivers/arm/gic/gic_v3.c                        \
			drivers/arm/pl011/${ARCH}/pl011_console.S       \
			drivers/arm/timer/private_timer.c		\
			drivers/console/console.c                       \
			${VERSAL2_PATH}/versal2_setup.c		\
			${VERSAL2_PATH}/versal2_pwr_state.c	\
			${VERSAL2_PATH}/aarch64/plat_helpers.S	\
			${XLNX_COMMON_PATH}/timer/timers.c

PLAT_TESTS_SKIP_LIST    := ${VERSAL2_PATH}/tests_to_skip.txt

ifeq ($(USE_NVM),1)
$(error "Versal2 port of TFTF doesn't currently support USE_NVM=1")
endif
