#
# Copyright (c) 2018-2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

AUTOGEN_DIR		:=	$(BUILD_PLAT)/autogen

include lib/xlat_tables_v2/xlat_tables.mk
include lib/compiler-rt/compiler-rt.mk
include lib/libfdt/libfdt.mk

TFTF_INCLUDES	:= 					\
	-I${AUTOGEN_DIR} 				\
	-Itftf/framework/include			\
	-Iinclude					\
	-Iinclude/common				\
	-Iinclude/common/${ARCH}			\
	-Iinclude/lib					\
	-Iinclude/lib/${ARCH}				\
	-Iinclude/lib/extensions			\
	-Iinclude/lib/pcie				\
	-Iinclude/lib/spdm				\
	-Iinclude/lib/utils				\
	-Iinclude/lib/xlat_tables			\
	-Iinclude/plat/common				\
	-Iinclude/runtime_services			\
	-Iinclude/runtime_services/secure_el0_payloads	\
	-Iinclude/runtime_services/secure_el1_payloads	\
	-Iinclude/runtime_services/host_realm_managment	\
	-Ispm/cactus					\
	-Ispm/ivy					\
	-Irealm						\
	-Ismc_fuzz/include

FRAMEWORK_SOURCES	:=	${AUTOGEN_DIR}/tests_list.c

FRAMEWORK_SOURCES	+=	$(addprefix tftf/,			\
	framework/${ARCH}/arch.c					\
	framework/${ARCH}/asm_debug.S					\
	framework/${ARCH}/entrypoint.S					\
	framework/${ARCH}/exceptions.S					\
	framework/${ARCH}/exception_report.c				\
	framework/debug.c						\
	framework/main.c						\
	framework/nvm_results_helpers.c					\
	framework/report.c						\
	framework/timer/timer_framework.c				\
	tests/common/test_helpers.c					\
)

FRAMEWORK_SOURCES	+=						\
	lib/${ARCH}/cache_helpers.S					\
	lib/${ARCH}/misc_helpers.S					\
	lib/delay/delay.c						\
	lib/events/events.c						\
	lib/extensions/amu/${ARCH}/amu.c				\
	lib/extensions/amu/${ARCH}/amu_helpers.S			\
	lib/exceptions/irq.c						\
	lib/locks/${ARCH}/spinlock.S					\
	lib/power_management/hotplug/hotplug.c				\
	lib/power_management/suspend/${ARCH}/asm_tftf_suspend.S		\
	lib/power_management/suspend/tftf_suspend.c			\
	lib/psci/psci.c							\
	lib/sdei/sdei.c							\
	lib/smc/${ARCH}/asm_smc.S					\
	lib/smc/${ARCH}/smc.c						\
	lib/trng/trng.c							\
        lib/errata_abi/errata_abi.c                                     \
	lib/transfer_list/transfer_list.c				\
	lib/trusted_os/trusted_os.c					\
	lib/utils/mp_printf.c						\
	lib/utils/uuid.c						\
	${XLAT_TABLES_LIB_SRCS}						\
	plat/common/${ARCH}/platform_mp_stack.S 			\
	plat/common/plat_common.c					\
	plat/common/plat_state_id.c					\
	plat/common/plat_topology.c					\
	plat/common/tftf_nvm_accessors.c


FRAMEWORK_SOURCES	+=	${COMPILER_RT_SRCS}

ifeq (${ARCH},aarch64)
# ARMv8.3 Pointer Authentication support files
FRAMEWORK_SOURCES	+=						\
	lib/exceptions/aarch64/sync.c					\
	lib/exceptions/aarch64/serror.c					\
	lib/extensions/pauth/aarch64/pauth.c				\
	lib/extensions/pauth/aarch64/pauth_helpers.S			\
	lib/extensions/sme/aarch64/sme.c				\
	lib/extensions/sme/aarch64/sme_helpers.S			\
	lib/extensions/sme/aarch64/sme2_helpers.S			\
	lib/extensions/sve/aarch64/sve.c				\
	lib/extensions/sve/aarch64/sve_helpers.S
endif

ifeq (${ARCH},aarch64)
# Context Management Library support files
FRAMEWORK_SOURCES	+=						\
	lib/context_mgmt/aarch64/context_el1.c
endif

TFTF_LINKERFILE		:=	tftf/framework/tftf.ld.S


TFTF_DEFINES :=

# Enable dynamic translation tables
PLAT_XLAT_TABLES_DYNAMIC := 1
$(eval $(call add_define,TFTF_DEFINES,PLAT_XLAT_TABLES_DYNAMIC))
