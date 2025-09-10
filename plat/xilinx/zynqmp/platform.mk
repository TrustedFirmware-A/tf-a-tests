#
# Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

XLNX_COMMON_PATH :=     plat/xilinx/common
ZYNQMP_PATH     :=      plat/xilinx/zynqmp

PLAT_INCLUDES	+=	-Iplat/xilinx/zynqmp/include/

PLAT_SOURCES	:=	drivers/arm/timer/private_timer.c               \
			drivers/cadence/uart/aarch64/cdns_console.S	\
			plat/xilinx/zynqmp/aarch64/plat_helpers.S	\
			plat/xilinx/zynqmp/zynqmp_pwr_state.c		\
			plat/xilinx/zynqmp/zynqmp_topology.c		\
			plat/xilinx/zynqmp/zynqmp_setup.c		\
			${XLNX_COMMON_PATH}/timer/timers.c

PLAT_TESTS_SKIP_LIST	:=	plat/xilinx/zynqmp/tests_to_skip.txt

TFTF_CFLAGS             +=      -Wno-maybe-uninitialized -Wno-unused-variable

ENABLE_ASSERTIONS       :=      1

PLAT_SUPPORTS_NS_RESET  :=      1

# Process PLAT_SUPPORTS_NS_RESET flag
$(eval $(call assert_boolean,PLAT_SUPPORTS_NS_RESET))
$(eval $(call add_define,TFTF_DEFINES,PLAT_SUPPORTS_NS_RESET))

ifeq ($(USE_NVM),1)
$(error "zynqmp port of TFTF doesn't currently support USE_NVM=1")
endif
