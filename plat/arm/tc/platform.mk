#
# Copyright (c) 2020-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Default number of threads per CPU on FVP
TC_MAX_PE_PER_CPU	:= 1

# Check the PE per core count
ifneq ($(TC_MAX_PE_PER_CPU),$(filter $(TC_MAX_PE_PER_CPU),1 2))
$(error "Incorrect TC_MAX_PE_PER_CPU specified for TC port")
endif

# Pass FVP_MAX_PE_PER_CPU to the build system
$(eval $(call add_define,TFTF_DEFINES,TC_MAX_PE_PER_CPU))
$(eval $(call add_define,NS_BL1U_DEFINES,TC_MAX_PE_PER_CPU))
$(eval $(call add_define,NS_BL2U_DEFINES,TC_MAX_PE_PER_CPU))

$(eval $(call add_define,TFTF_DEFINES,TARGET_PLATFORM))

PLAT_INCLUDES	+=	-Iplat/arm/tc/include/

PLAT_SOURCES	:=	drivers/arm/gic/arm_gic.c		\
			drivers/arm/gic/gic_v2.c		\
			drivers/arm/gic/gic_v3.c		\
			drivers/arm/sp805/sp805.c		\
			drivers/arm/timer/private_timer.c	\
			drivers/arm/timer/system_timer.c	\
			plat/arm/tc/${ARCH}/plat_helpers.S	\
			plat/arm/tc/plat_setup.c		\
			plat/arm/tc/tc_mem_prot.c		\
			plat/arm/tc/tc_pwr_state.c		\
			plat/arm/tc/tc_topology.c

CACTUS_SOURCES	+=	plat/arm/tc/${ARCH}/plat_helpers.S
IVY_SOURCES	+=	plat/arm/tc/${ARCH}/plat_helpers.S

PLAT_TESTS_SKIP_LIST	:=	plat/arm/tc/tests_to_skip.txt

ifeq (${USE_NVM},1)
$(error "USE_NVM is not supported on TC platforms")
endif

include plat/arm/common/arm_common.mk
