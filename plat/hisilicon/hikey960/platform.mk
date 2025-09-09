#
# Copyright (c) 2018-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

HIKEY960_PATH	:=	plat/hisilicon/hikey960

PLAT_INCLUDES	:=	-I${HIKEY960_PATH}/include/

PLAT_SOURCES	:=	${HIKEY960_PATH}/hikey960_setup.c		\
			${HIKEY960_PATH}/hikey960_pwr_state.c		\
			${HIKEY960_PATH}/aarch64/plat_helpers.S		\
			drivers/arm/pl011/${ARCH}/pl011_console.S	\
			drivers/arm/timer/system_timer.c		\
			drivers/arm/timer/private_timer.c		\
			drivers/console/console.c			\
			plat/arm/common/arm_timers.c

TFTF_CFLAGS		+= -Wno-maybe-uninitialized

ifeq ($(USE_NVM),1)
$(error "Hikey960 port of TFTF doesn't currently support USE_NVM=1")
endif
ifneq ($(TESTS),tftf-validation)
$(error "Hikey960 port currently only supports tftf-validation")
endif
