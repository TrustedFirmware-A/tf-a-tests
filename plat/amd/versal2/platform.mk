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

# Add VERSAL2_VARIANT as a compile-time definition
$(eval $(call add_define,TFTF_DEFINES,VERSAL2_VARIANT))

ifeq ($(USE_NVM),1)
$(error "Versal2 port of TFTF doesn't currently support USE_NVM=1")
endif
