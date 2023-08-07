#
# Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

VERSAL_PATH	:=	plat/xilinx/versal

PLAT_INCLUDES	:=	-I${VERSAL_PATH}/include/

PLAT_SOURCES	:=	drivers/arm/gic/arm_gic_v2v3.c                  \
			drivers/arm/gic/gic_common.c                    \
			drivers/arm/gic/gic_v2.c                        \
			drivers/arm/gic/gic_v3.c                        \
			drivers/arm/pl011/${ARCH}/pl011_console.S       \
			drivers/arm/timer/private_timer.c		\
			drivers/console/console.c                       \
			${VERSAL_PATH}/versal_setup.c			\
			${VERSAL_PATH}/versal_pwr_state.c		\
			${VERSAL_PATH}/aarch64/plat_helpers.S		\
			${VERSAL_PATH}/timers.c

PLAT_TESTS_SKIP_LIST    := ${VERSAL_PATH}/tests_to_skip.txt

ifeq ($(USE_NVM),1)
$(error "Versal port of TFTF doesn't currently support USE_NVM=1")
endif
