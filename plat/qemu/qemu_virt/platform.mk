#
# Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
# Copyright (c) 2024, 2026, Linaro Limited.
#
# SPDX-License-Identifier: BSD-3-Clause
#

QEMU_VIRT_PATH	:= plat/qemu/qemu_virt
PLAT_INCLUDES	:= -I$(QEMU_VIRT_PATH)/include

PLAT_SOURCES	:=	drivers/arm/pl011/${ARCH}/pl011_console.S       \
			drivers/arm/timer/private_timer.c		\
			drivers/console/console.c                       \
			${QEMU_VIRT_PATH}/qemu_setup.c			\
			${QEMU_VIRT_PATH}/qemu_pwr_state.c		\
			${QEMU_VIRT_PATH}/aarch64/plat_helpers.S	\
			${QEMU_VIRT_PATH}/qemu_timer.c			\

PLAT_TESTS_SKIP_LIST    := ${QEMU_VIRT_PATH}/tests_to_skip.txt

ifeq ($(USE_NVM),1)
$(error "QEMU port of TFTF doesn't currently support USE_NVM=1")
endif
