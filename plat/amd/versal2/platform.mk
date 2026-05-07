#
# Copyright (c) 2024-2026, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

XLNX_COMMON_PATH :=	plat/xilinx/common
VERSAL2_PATH :=	plat/amd/versal2

# VERSAL2_VARIANT encodes (cluster count * 10 + cores per cluster).
# Default 42: 4 clusters, 2 cores per cluster. Use 14 for 1 cluster, 4 cores.
VERSAL2_VARIANT		?= 42

# Console selection: pl011 or pl011_0 for UART0, pl011_1 for UART1 (default)
VERSAL2_CONSOLE		?= pl011_1

# Map console selection to numeric ID for header-based base address resolution
ifeq ($(VERSAL2_CONSOLE),pl011)
VERSAL2_CONSOLE_ID := 0
else ifeq ($(VERSAL2_CONSOLE),pl011_0)
VERSAL2_CONSOLE_ID := 0
else ifeq ($(VERSAL2_CONSOLE),pl011_1)
VERSAL2_CONSOLE_ID := 1
else
$(error "Invalid VERSAL2_CONSOLE value: $(VERSAL2_CONSOLE). Use pl011, pl011_0, or pl011_1")
endif

PLAT_INCLUDES	:=	-I${XLNX_COMMON_PATH}/include/			\
			-I${VERSAL2_PATH}/include/

PLAT_SOURCES	:=	drivers/arm/pl011/${ARCH}/pl011_console.S       \
			drivers/arm/timer/private_timer.c		\
			drivers/console/console.c                       \
			${VERSAL2_PATH}/versal2_setup.c		\
			${VERSAL2_PATH}/versal2_topology.c	\
			${VERSAL2_PATH}/versal2_pwr_state.c	\
			${VERSAL2_PATH}/aarch64/plat_helpers.S	\
			${XLNX_COMMON_PATH}/timer/timers.c

PLAT_TESTS_SKIP_LIST    := ${VERSAL2_PATH}/tests_to_skip.txt

# Add VERSAL2_VARIANT and VERSAL2_CONSOLE_ID as compile-time definitions
$(eval $(call add_define,TFTF_DEFINES,VERSAL2_VARIANT))
$(eval $(call add_define,TFTF_DEFINES,VERSAL2_CONSOLE_ID))

ifeq ($(USE_NVM),1)
$(error "Versal2 port of TFTF doesn't currently support USE_NVM=1")
endif
