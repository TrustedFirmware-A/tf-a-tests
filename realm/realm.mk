#
# Copyright (c) 2022-2023, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include branch_protection.mk

REALM_INCLUDES :=							\
	-Itftf/framework/include					\
	-Iinclude							\
	-Iinclude/common						\
	-Iinclude/common/${ARCH}					\
	-Iinclude/lib							\
	-Iinclude/lib/extensions					\
	-Iinclude/lib/${ARCH}						\
	-Iinclude/lib/utils						\
	-Iinclude/lib/xlat_tables					\
	-Iinclude/runtime_services					\
	-Iinclude/runtime_services/host_realm_managment			\
	-Irealm								\
	-Irealm/aarch64							\
	-Irealm/include

REALM_SOURCES:=								\
	$(addprefix realm/,						\
	aarch64/realm_entrypoint.S					\
	aarch64/realm_exceptions.S					\
	realm_attestation.c						\
	realm_exception_report.c					\
	realm_debug.c							\
	realm_helpers.c							\
	realm_interrupt.c						\
	realm_multiple_rec.c						\
	realm_pauth.c							\
	realm_payload_main.c						\
	realm_plane.c							\
	realm_pmuv3.c							\
	realm_psci.c							\
	realm_psi.c							\
	realm_rsi.c							\
	realm_shared_data.c						\
	realm_simd.c							\
	)

REALM_SOURCES += lib/${ARCH}/cache_helpers.S				\
	lib/${ARCH}/misc_helpers.S					\
	lib/smc/${ARCH}/asm_smc.S					\
	lib/smc/${ARCH}/hvc.c						\
	lib/smc/${ARCH}/smc.c						\
	lib/exceptions/${ARCH}/serror.c					\
	lib/exceptions/${ARCH}/sync.c					\
	lib/locks/${ARCH}/spinlock.S					\
	lib/delay/delay.c						\
	lib/extensions/fpu/fpu.c					\
	lib/extensions/sve/aarch64/sve.c				\
	lib/extensions/sve/aarch64/sve_helpers.S			\
	lib/extensions/sme/aarch64/sme.c				\
	lib/extensions/sme/aarch64/sme_helpers.S

REALM_LINKERFILE:=	realm/realm.ld.S

# ARMv8.3 Pointer Authentication support files
REALM_SOURCES +=	lib/extensions/pauth/aarch64/pauth.c            \
			lib/extensions/pauth/aarch64/pauth_helpers.S

REALM_INCLUDES +=	-Iinclude/lib/extensions

REALM_DEFINES:=
$(eval $(call add_define,REALM_DEFINES,ARM_ARCH_MAJOR))
$(eval $(call add_define,REALM_DEFINES,ARM_ARCH_MINOR))
$(eval $(call add_define,REALM_DEFINES,ENABLE_BTI))
$(eval $(call add_define,REALM_DEFINES,ENABLE_PAUTH))
$(eval $(call add_define,REALM_DEFINES,LOG_LEVEL))
$(eval $(call add_define,REALM_DEFINES,IMAGE_REALM))
