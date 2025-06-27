#
# Copyright (c) 2018-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# TFTF Version
VERSION_MAJOR		:= 2
VERSION_MINOR		:= 13

MAKE_HELPERS_DIRECTORY := make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk

################################################################################
# Default values for build configurations, and their dependencies
################################################################################

include ${MAKE_HELPERS_DIRECTORY}defaults.mk

PLAT			:= ${DEFAULT_PLAT}

# Assertions enabled for DEBUG builds by default
ENABLE_ASSERTIONS	:= ${DEBUG}

################################################################################
# Checkpatch script options
################################################################################

CHECKCODE_ARGS		:=	--no-patch
# Do not check the coding style on imported library files or documentation files
INC_LIB_DIRS_TO_CHECK	:=	$(sort $(filter-out			\
					include/lib/libfdt		\
					include/lib/libc,		\
					$(wildcard include/lib/*)))
LIB_DIRS_TO_CHECK	:=	$(sort $(filter-out			\
					lib/compiler-rt			\
					lib/libfdt%			\
					lib/libc,			\
					$(wildcard lib/*)))
ROOT_DIRS_TO_CHECK	:=	$(sort $(filter-out			\
					lib				\
					include				\
					docs				\
					%.md				\
					%.rst,				\
					$(wildcard *)))
CHECK_PATHS		:=	${ROOT_DIRS_TO_CHECK}			\
				${INC_LIB_DIRS_TO_CHECK}		\
				${LIB_DIRS_TO_CHECK}

ifeq (${V},0)
	Q=@
else
	Q=
endif
export Q

################################################################################
# Toolchain configs
################################################################################
CC			:=	${CROSS_COMPILE}gcc
CPP			:=	${CROSS_COMPILE}cpp
AS			:=	${CROSS_COMPILE}gcc
AR			:=	${CROSS_COMPILE}ar
LD			:=	${CROSS_COMPILE}ld
OC			:=	${CROSS_COMPILE}objcopy
OD			:=	${CROSS_COMPILE}objdump
NM			:=	${CROSS_COMPILE}nm
PP			:=	${CROSS_COMPILE}gcc

ifneq (${DEBUG}, 0)
	BUILD_TYPE	:=	debug
	# Use LOG_LEVEL_INFO by default for debug builds
	LOG_LEVEL       :=      40
else
	BUILD_TYPE	:=	release
	# Use LOG_LEVEL_ERROR by default for release builds
	LOG_LEVEL       :=      20
endif

# Default build string (git branch and commit)
ifeq (${BUILD_STRING},)
        BUILD_STRING  :=  $(shell git describe --always --dirty --tags 2> /dev/null)
endif

VERSION_STRING		:= 	v${VERSION_MAJOR}.${VERSION_MINOR}(${PLAT},${BUILD_TYPE}):${BUILD_STRING}

BUILD_BASE		:=	./build
BUILD_PLAT		:=	${BUILD_BASE}/${PLAT}/${BUILD_TYPE}

PLAT_MAKEFILE		:=	platform.mk
# Generate the platforms list by recursively searching for all directories
# under /plat containing a PLAT_MAKEFILE. Append each platform with a `|`
# char and strip out the final '|'.
PLATFORMS		:=	$(shell find plat/ -name '${PLAT_MAKEFILE}' -print0 |			\
					sed -r 's%[^\x00]*\/([^/]*)\/${PLAT_MAKEFILE}\x00%\1|%g' |	\
					sed -r 's/\|$$//')

DOCS_PATH		:=	docs

# Path to Transfer List Library (LibTL)
LIBTL_PATH		:=	contrib/libtl

ifeq (${PLAT},)
  $(error "Error: Unknown platform. Please use PLAT=<platform name> to specify the platform")
endif
PLAT_PATH		:=	$(shell find plat/ -wholename '*/${PLAT}')
PLAT_MAKEFILE_FULL	:=	${PLAT_PATH}/${PLAT_MAKEFILE}
ifeq ($(wildcard ${PLAT_MAKEFILE_FULL}),)
  $(error "Error: Invalid platform. The following platforms are available: ${PLATFORMS}")
endif


EL3_PAYLOAD_PLAT_PATH		:=	$(shell find el3_payload/plat/ -wholename '*/${PLAT}')
EL3_PAYLOAD_PLAT_MAKEFILE_FULL	:=	${EL3_PAYLOAD_PLAT_PATH}/${PLAT_MAKEFILE}

.PHONY: all
all: msg_start

.PHONY: msg_start
msg_start:
	@echo "Building ${PLAT}"
	@echo "Selected set of tests: ${TESTS}"

# Set flags for Realm Payload Tests
ifeq (${ENABLE_REALM_PAYLOAD_TESTS},1)
ARM_ARCH_MINOR := 5
BRANCH_PROTECTION := 2
endif

# Include test images makefiles.
include tftf/framework/framework.mk
include tftf/tests/tests.mk
include fwu/ns_bl1u/ns_bl1u.mk
include fwu/ns_bl2u/ns_bl2u.mk

# List of secure partitions present.
SECURE_PARTITIONS	:=

# Only platform fvp and rdv3 supports cactus_mm
ifeq (${ARCH}-${PLAT},$(filter ${ARCH}-${PLAT},aarch64-fvp aarch64-rdv3))
include spm/cactus_mm/cactus_mm.mk
include realm/realm.mk
endif

# cactus and ivy are supported on platforms: fvp, tc
ifeq (${ARCH}-${PLAT},$(filter ${ARCH}-${PLAT},aarch64-fvp aarch64-tc))
include spm/cactus/cactus.mk
include spm/ivy/ivy.mk
endif

################################################################################
# Include libc
################################################################################
include lib/libc/libc.mk

# Include platform specific makefile last because:
# - the platform makefile may use all previous definitions in this file.
# - the platform makefile may wish overwriting some of them.
include ${PLAT_MAKEFILE_FULL}

.SUFFIXES:

################################################################################
# Build options checks
################################################################################
$(eval $(call assert_boolean,DEBUG))
$(eval $(call assert_boolean,ENABLE_ASSERTIONS))
$(eval $(call assert_boolean,FIRMWARE_UPDATE))
$(eval $(call assert_boolean,FWU_BL_TEST))
$(eval $(call assert_boolean,NEW_TEST_SESSION))
$(eval $(call assert_boolean,USE_NVM))
$(eval $(call assert_numeric,BRANCH_PROTECTION))
$(eval $(call assert_boolean,ENABLE_REALM_PAYLOAD_TESTS))
$(eval $(call assert_boolean,TRANSFER_LIST))
$(eval $(call assert_boolean,SPMC_AT_EL3))
$(eval $(call assert_boolean,CACTUS_PWR_MGMT_SUPPORT))

################################################################################
# Process build options
################################################################################

# Process BRANCH_PROTECTION value and set
# Pointer Authentication and Branch Target Identification flags
include branch_protection.mk

################################################################################
# Add definitions to the cpp preprocessor based on the current build options.
# This is done after including the platform specific makefile to allow the
# platform to overwrite the default options
################################################################################
$(eval $(call add_define,TFTF_DEFINES,ARM_ARCH_MAJOR))
$(eval $(call add_define,TFTF_DEFINES,ARM_ARCH_MINOR))
$(eval $(call add_define,TFTF_DEFINES,DEBUG))
$(eval $(call add_define,TFTF_DEFINES,ENABLE_ASSERTIONS))
$(eval $(call add_define,TFTF_DEFINES,ENABLE_BTI))
$(eval $(call add_define,TFTF_DEFINES,ENABLE_PAUTH))
$(eval $(call add_define,TFTF_DEFINES,LOG_LEVEL))
$(eval $(call add_define,TFTF_DEFINES,NEW_TEST_SESSION))
$(eval $(call add_define,TFTF_DEFINES,PLAT_${PLAT}))
$(eval $(call add_define,TFTF_DEFINES,USE_NVM))
$(eval $(call add_define,TFTF_DEFINES,ENABLE_REALM_PAYLOAD_TESTS))
$(eval $(call add_define,TFTF_DEFINES,TRANSFER_LIST))
$(eval $(call add_define,TFTF_DEFINES,PLAT_AMU_GROUP1_COUNTERS_MASK))
$(eval $(call add_define,TFTF_DEFINES,SPMC_AT_EL3))
$(eval $(call add_define,TFTF_DEFINES,CACTUS_PWR_MGMT_SUPPORT))

################################################################################

################################################################################
# Assembler, compiler and linker flags shared across all test images.
################################################################################
COMMON_ASFLAGS		:=
COMMON_CFLAGS		:=
COMMON_LDFLAGS		:=

ifeq (${DEBUG},1)
COMMON_CFLAGS		+= 	-g -gdwarf-4
COMMON_ASFLAGS		+= 	-g -Wa,--gdwarf-4
endif

# Set the compiler's target architecture profile based on ARM_ARCH_MINOR option
ifeq (${ARM_ARCH_MINOR},0)
march32-directive	= 	-march=armv${ARM_ARCH_MAJOR}-a
march64-directive	= 	-march=armv${ARM_ARCH_MAJOR}-a
else
march32-directive	= 	-march=armv${ARM_ARCH_MAJOR}.${ARM_ARCH_MINOR}-a
march64-directive	= 	-march=armv${ARM_ARCH_MAJOR}.${ARM_ARCH_MINOR}-a
endif

# Get architecture feature modifiers
arch-features		=	${ARM_ARCH_FEATURE}

# Set the compiler's architecture feature modifiers
ifneq ($(arch-features), none)
ifeq ($(ARCH), aarch32)
march32-directive	:=	$(march32-directive)+$(arch-features)
else
march64-directive	:=	$(march64-directive)+$(arch-features)
endif
# Print features
$(info Arm Architecture Features specified: $(subst +, ,$(arch-features)))
endif	# arch-features

################################################################################
# Compiler settings
################################################################################
ifneq ($(findstring clang,$(notdir $(CC))),)
CLANG_CFLAGS_aarch64	:=	-target aarch64-elf

CPP			:=	$(CC) -E $(COMMON_CFLAGS_$(ARCH))
PP			:=	$(CC) -E $(COMMON_CFLAGS_$(ARCH))

CLANG_WARNINGS		+=	-nostdinc -ffreestanding -Wall	\
				-Wmissing-include-dirs $(CLANG_CFLAGS_$(ARCH))	\
				-Wlogical-op-parentheses \
				-Wno-initializer-overrides \
				-Wno-sometimes-uninitialized \
				-Wno-unused-function \
				-Wno-unused-variable \
				-Wno-unused-parameter \
				-Wno-tautological-compare \
				-Wno-memset-transposed-args \
				-Wno-parentheses

CLANG_CFLAGS		+= 	-Wno-error=deprecated-declarations \
				-Wno-error=cpp \
				$(CLANG_WARNINGS)
endif #(clang)

ifneq ($(findstring gcc,$(notdir $(CC))),)
GCC_CFLAGS_aarch32	:=	${march32-directive} -mno-unaligned-access
GCC_CFLAGS_aarch64	:=	-mgeneral-regs-only

GCC_ASFLAGS_aarch32	:=	${march32-directive}
GCC_ASFLAGS_aarch64	:=	-mgeneral-regs-only ${march64-directive}

GCC_WARNINGS		+=	-nostdinc -ffreestanding -Wall -Werror 	\
				-Wmissing-include-dirs  $(GCC_CFLAGS_$(ARCH)) \
				-std=gnu99 -Os

# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105523
GCC_CFLAGS		+=	$(call cc_option, --param=min-pagesize=0)
GCC_CFLAGS		+= 	$(GCC_WARNINGS)
endif #(gcc)

COMMON_CFLAGS_aarch64	+=	${march64-directive} -mstrict-align \
				$(CLANG_CFLAGS_$(ARCH)) $(GCC_CFLAGS_$(ARCH))

COMMON_CFLAGS		+=	$(COMMON_CFLAGS_$(ARCH))
COMMON_CFLAGS		+=	-ffunction-sections -fdata-sections

# Get the content of CFLAGS user defined value last so they are appended after
# the options defined in the Makefile
COMMON_CFLAGS 		+=	${CLANG_CFLAGS} ${GCC_CFLAGS} ${INCLUDES}

COMMON_ASFLAGS		+=	-nostdinc -ffreestanding -Wa,--fatal-warnings	\
				-Werror -Wmissing-include-dirs			\
				-D__ASSEMBLY__ $(GCC_ASFLAGS_$(ARCH))	\
				${INCLUDES}

COMMON_LDFLAGS		+=	${LDFLAGS} --fatal-warnings -O1 --gc-sections --build-id=none

# With ld.bfd version 2.39 and newer new warnings are added. Skip those since we
# are not loaded by a elf loader.
COMMON_LDFLAGS		+=	$(call ld_option, --no-warn-rwx-segments)

################################################################################

TFTF_SOURCES		:= ${FRAMEWORK_SOURCES}	${TESTS_SOURCES} ${PLAT_SOURCES} ${LIBC_SRCS} ${LIBFDT_SRCS}
TFTF_INCLUDES		+= ${PLAT_INCLUDES}
TFTF_CFLAGS		+= ${COMMON_CFLAGS}
TFTF_ASFLAGS		+= ${COMMON_ASFLAGS}
TFTF_LDFLAGS		+= ${COMMON_LDFLAGS}
TFTF_EXTRA_OBJS 	:=

ifneq (${BP_OPTION},none)
TFTF_CFLAGS		+= -mbranch-protection=${BP_OPTION}
NS_BL1U_CFLAGS		+= -mbranch-protection=${BP_OPTION}
NS_BL2U_CFLAGS		+= -mbranch-protection=${BP_OPTION}
CACTUS_MM_CFLAGS	+= -mbranch-protection=${BP_OPTION}
CACTUS_CFLAGS		+= -mbranch-protection=${BP_OPTION}
IVY_CFLAGS		+= -mbranch-protection=${BP_OPTION}
REALM_CFLAGS		+= -mbranch-protection=${BP_OPTION}
endif

ifeq ($(SMC_FUZZING), 1)
TFTF_EXTRA_OBJS += ${BUILD_PLAT}/smcf/dtb.o
endif

#####################################################################################
ifneq ($(findstring gcc,$(notdir $(LD))),)
	PIE_LDFLAGS	+=	-Wl,-pie -Wl,--no-dynamic-linker
else
	PIE_LDFLAGS	+=	-pie --no-dynamic-linker
endif

#####################################################################################
NS_BL1U_SOURCES		+= ${PLAT_SOURCES} ${LIBC_SRCS}
NS_BL1U_INCLUDES	+= ${PLAT_INCLUDES}
NS_BL1U_CFLAGS		+= ${COMMON_CFLAGS}
NS_BL1U_ASFLAGS		+= ${COMMON_ASFLAGS}
NS_BL1U_LDFLAGS		+= ${COMMON_LDFLAGS}

NS_BL2U_SOURCES		+= ${PLAT_SOURCES} ${LIBC_SRCS}
NS_BL2U_INCLUDES	+= ${PLAT_INCLUDES}
NS_BL2U_CFLAGS		+= ${COMMON_CFLAGS}
NS_BL2U_ASFLAGS		+= ${COMMON_ASFLAGS}
NS_BL2U_LDFLAGS		+= ${COMMON_LDFLAGS}

CACTUS_MM_SOURCES	+= ${LIBC_SRCS}
CACTUS_MM_INCLUDES	+= ${PLAT_INCLUDES}
CACTUS_MM_CFLAGS	+= ${COMMON_CFLAGS}
CACTUS_MM_ASFLAGS	+= ${COMMON_ASFLAGS}
CACTUS_MM_LDFLAGS	+= ${COMMON_LDFLAGS}

CACTUS_SOURCES		+= ${LIBC_SRCS}
CACTUS_INCLUDES		+= ${PLAT_INCLUDES}
CACTUS_CFLAGS		+= ${COMMON_CFLAGS} -fpie
CACTUS_ASFLAGS		+= ${COMMON_ASFLAGS}
CACTUS_LDFLAGS		+= ${COMMON_LDFLAGS} $(PIE_LDFLAGS)

IVY_SOURCES		+= ${LIBC_SRCS}
IVY_INCLUDES		+= ${PLAT_INCLUDES}
IVY_CFLAGS		+= ${COMMON_CFLAGS} -fpie
IVY_ASFLAGS		+= ${COMMON_ASFLAGS}
IVY_LDFLAGS		+= ${COMMON_LDFLAGS} $(PIE_LDFLAGS)

REALM_SOURCES		+= ${LIBC_SRCS}
REALM_CFLAGS		+= ${COMMON_CFLAGS} -fpie
REALM_ASFLAGS		+= ${COMMON_ASFLAGS}
REALM_LDFLAGS		+= ${COMMON_LDFLAGS} $(PIE_LDFLAGS)

.PHONY: locate-checkpatch
locate-checkpatch:
ifndef CHECKPATCH
	$(error "Please set CHECKPATCH to point to the Linux checkpatch.pl file, eg: CHECKPATCH=../linux/script/checkpatch.pl")
else
ifeq (,$(wildcard ${CHECKPATCH}))
	$(error "The file CHECKPATCH points to cannot be found, use eg: CHECKPATCH=../linux/script/checkpatch.pl")
endif
endif

.PHONY: clean
clean:
			@echo "  CLEAN"
			${Q}rm -rf ${BUILD_PLAT}
ifneq ($(wildcard ${EL3_PAYLOAD_PLAT_MAKEFILE_FULL}),)
			${MAKE} -C el3_payload clean
endif

.PHONY: realclean distclean
realclean distclean:
			@echo "  REALCLEAN"
			${Q}rm -rf ${BUILD_BASE}
			${Q}rm -f ${CURDIR}/cscope.*
ifneq ($(wildcard ${EL3_PAYLOAD_PLAT_MAKEFILE_FULL}),)
			${MAKE} -C el3_payload distclean
endif

.PHONY: checkcodebase
checkcodebase:		locate-checkpatch
	@echo "  CHECKING STYLE"
	@if test -d .git ; then						\
		git ls-files | grep -E -v 'libfdt|libc|docs|\.md|\.rst' |	\
		while read GIT_FILE ;					\
		do ${CHECKPATCH} ${CHECKCODE_ARGS} -f $$GIT_FILE ;	\
		done ;							\
	else								\
		 find . -type f -not -iwholename "*.git*"		\
		 -not -iwholename "*build*"				\
		 -not -iwholename "*libfdt*"				\
		 -not -iwholename "*libc*"				\
		 -not -iwholename "*docs*"				\
		 -not -iwholename "*.md"				\
		 -not -iwholename "*.rst"				\
		 -exec ${CHECKPATCH} ${CHECKCODE_ARGS} -f {} \; ;	\
	fi

.PHONY: checkpatch
checkpatch:		locate-checkpatch
			@echo "  CHECKING STYLE"
			${Q}COMMON_COMMIT=$$(git merge-base HEAD ${BASE_COMMIT});	\
			for commit in `git rev-list $$COMMON_COMMIT..HEAD`; do		\
				printf "\n[*] Checking style of '$$commit'\n\n";	\
				git log --format=email "$$commit~..$$commit"		\
					-- ${CHECK_PATHS} | ${CHECKPATCH} - || true;	\
				git diff --format=email "$$commit~..$$commit"		\
					-- ${CHECK_PATHS} | ${CHECKPATCH} - || true;	\
			done

ifneq (${FIRMWARE_UPDATE},1)
.PHONY: ns_bl1u ns_bl2u
ns_bl1u ns_bl2u:
	@echo "ERROR: Can't build $@ because Firmware Update is not supported \
	on this platform."
	@exit 1
endif

ifneq (${ARCH}-${PLAT},$(filter ${ARCH}-${PLAT},aarch64-fvp aarch64-rdv3))
.PHONY: cactus_mm
cactus_mm:
	@echo "ERROR: $@ is supported only on AArch64 FVP."
	@exit 1

.PHONY: realm
realm:
	@echo "ERROR: $@ is supported only on AArch64 FVP and RD-V3."
	@exit 1

endif

ifneq (${ARCH}-${PLAT},$(filter ${ARCH}-${PLAT},aarch64-fvp aarch64-tc))
.PHONY: cactus
cactus:
	@echo "ERROR: $@ is supported only on AArch64 FVP or TC."
	@exit 1

.PHONY: ivy
ivy:
	@echo "ERROR: $@ is supported only on AArch64 FVP or TC."
	@exit 1
endif

MAKE_DEP = -Wp,-MD,$(DEP) -MT $$@

define MAKE_C

$(eval OBJ := $(1)/$(patsubst %.c,%.o,$(notdir $(2))))
$(eval DEP := $(patsubst %.o,%.d,$(OBJ)))

$(OBJ) : $(2) | $(AUTOGEN_DIR)/tests_list.h
	@echo "  CC      $$<"
	$$(Q)$$(CC) $$($(3)_CFLAGS) ${$(3)_INCLUDES} ${$(3)_DEFINES} -DIMAGE_$(3) $(MAKE_DEP) -c $$< -o $$@

-include $(DEP)
endef


define MAKE_S

$(eval OBJ := $(1)/$(patsubst %.S,%.o,$(notdir $(2))))
$(eval DEP := $(patsubst %.o,%.d,$(OBJ)))

$(OBJ) : $(2) | $(AUTOGEN_DIR)/tests_list.h
	@echo "  AS      $$<"
	$$(Q)$$(AS) $$($(3)_ASFLAGS) ${$(3)_INCLUDES} ${$(3)_DEFINES} -DIMAGE_$(3) $(MAKE_DEP) -c $$< -o $$@

-include $(DEP)
endef


define MAKE_LD

$(eval DEP := $(1).d)

$(1) : $(2) | $(AUTOGEN_DIR)/tests_list.h
	@echo "  PP      $$<"
	$$(Q)$$(AS) $$($(3)_ASFLAGS) ${$(3)_INCLUDES} ${$(3)_DEFINES} -P -E $(MAKE_DEP) -o $$@ $$<

-include $(DEP)
endef


define MAKE_OBJS
	$(eval C_OBJS := $(filter %.c,$(2)))
	$(eval REMAIN := $(filter-out %.c,$(2)))
	$(eval $(foreach obj,$(C_OBJS),$(call MAKE_C,$(1),$(obj),$(3))))

	$(eval S_OBJS := $(filter %.S,$(REMAIN)))
	$(eval REMAIN := $(filter-out %.S,$(REMAIN)))
	$(eval $(foreach obj,$(S_OBJS),$(call MAKE_S,$(1),$(obj),$(3))))

	$(and $(REMAIN),$(error Unexpected source files present: $(REMAIN)))
endef

${BUILD_PLAT}:
	$(Q)mkdir -p "$@"

# NOTE: The line continuation '\' is required in the next define otherwise we
# end up with a line-feed characer at the end of the last c filename.
# Also bare this issue in mind if extending the list of supported filetypes.
define SOURCES_TO_OBJS
	$(notdir $(patsubst %.c,%.o,$(filter %.c,$(1)))) \
	$(notdir $(patsubst %.S,%.o,$(filter %.S,$(1))))
endef

define uppercase
$(shell echo $(1) | tr '[:lower:]' '[:upper:]')
endef

define MAKE_IMG
	$(eval IMG_PREFIX := $(call uppercase, $(1)))
	$(eval BUILD_DIR  := ${BUILD_PLAT}/$(1))
	$(eval SOURCES    := $(${IMG_PREFIX}_SOURCES))
	$(eval OBJS       := $(addprefix $(BUILD_DIR)/,$(call SOURCES_TO_OBJS,$(SOURCES))))
	$(eval OBJS       += $(${IMG_PREFIX}_EXTRA_OBJS))
	$(eval LINKERFILE := $(BUILD_DIR)/$(1).ld)
	$(eval MAPFILE    := $(BUILD_DIR)/$(1).map)
	$(eval ELF        := $(BUILD_DIR)/$(1).elf)
	$(eval DUMP       := $(BUILD_DIR)/$(1).dump)
	$(eval BIN        := $(BUILD_PLAT)/$(1).bin)

	$(eval $(call MAKE_OBJS,$(BUILD_DIR),$(SOURCES),${IMG_PREFIX}))
	$(eval $(call MAKE_LD,$(LINKERFILE),$(${IMG_PREFIX}_LINKERFILE),${IMG_PREFIX}))

$(BUILD_DIR) : ${BUILD_PLAT}
	$$(Q)mkdir -p "$$@"

$(ELF) : $(OBJS) $(LINKERFILE)
	@echo "  LD      $$@"
	@echo 'const char build_message[] = "Built : "__TIME__", "__DATE__; \
               const char version_string[] = "${VERSION_STRING}";' | \
		$$(CC) $$(${IMG_PREFIX}_CFLAGS) ${${IMG_PREFIX}_INCLUDES} ${${IMG_PREFIX}_DEFINES} -c -xc - -o $(BUILD_DIR)/build_message.o
	$$(Q)$$(LD) -o $$@ $$(${IMG_PREFIX}_LDFLAGS) -Map=$(MAPFILE) \
		-T $(LINKERFILE) $(BUILD_DIR)/build_message.o $(OBJS)

$(DUMP) : $(ELF)
	@echo "  OD      $$@"
	$${Q}$${OD} -dx $$< > $$@

$(BIN) : $(ELF)
	@echo "  BIN     $$@"
	$$(Q)$$(OC) -O binary $$< $$@
	@echo
	@echo "Built $$@ successfully"
	@echo

.PHONY : $(1)
$(1) : $(BUILD_DIR) $(BIN) $(DUMP)

all : $(1)

endef

ifeq (${ARCH},aarch32)
        ARCH_TESTS_SKIP_LIST    := tftf/tests/aarch32_tests_to_skip.txt
endif

$(AUTOGEN_DIR):
	$(Q)mkdir -p "$@"

$(AUTOGEN_DIR)/tests_list.c $(AUTOGEN_DIR)/tests_list.h &: $(AUTOGEN_DIR) ${TESTS_FILE} ${PLAT_TESTS_SKIP_LIST} $(ARCH_TESTS_SKIP_LIST)
	@echo "  AUTOGEN $(AUTOGEN_DIR)/tests_list.c $(AUTOGEN_DIR)/tests_list.h"
	tools/generate_test_list/generate_test_list.py $(AUTOGEN_DIR)/tests_list.c \
		$(AUTOGEN_DIR)/tests_list.h  ${TESTS_FILE} \
		--plat-skip-file=$(PLAT_TESTS_SKIP_LIST) \
		--arch-skip-file=$(ARCH_TESTS_SKIP_LIST)
ifeq ($(SMC_FUZZING), 1)
	$(Q)mkdir -p  ${BUILD_PLAT}/smcf
	dtc ${SMC_FUZZ_DTS} >> ${BUILD_PLAT}/smcf/dtb
	$(OC) -I binary -O elf64-littleaarch64 -B aarch64 ${BUILD_PLAT}/smcf/dtb ${BUILD_PLAT}/smcf/dtb.o \
	--redefine-sym _binary___build_$(PLAT)_$(BUILD_TYPE)_smcf_dtb_start=_binary___dtb_start \
	--redefine-sym _binary___build_$(PLAT)_$(BUILD_TYPE)_smcf_dtb_end=_binary___dtb_end
endif

ifeq ($(FIRMWARE_UPDATE), 1)
  $(eval $(call MAKE_IMG,ns_bl1u))
  $(eval $(call MAKE_IMG,ns_bl2u))
endif

ifeq (${ARCH}-${PLAT},aarch64-fvp)
  $(eval $(call MAKE_IMG,cactus_mm))
  $(eval $(call MAKE_IMG,cactus))
  $(eval $(call MAKE_IMG,ivy))
endif

.PHONY : tftf
  $(eval $(call MAKE_IMG,tftf))

# Build flag 'ENABLE_REALM_PAYLOAD_TESTS=1' builds and pack Realm Payload Tests
ifeq (${ENABLE_REALM_PAYLOAD_TESTS},1)
  $(eval $(call MAKE_IMG,realm))

# This forces to rebuild tftf.bin. For incremental build this re-creates tftf.bin
# and removes the old realm payload packed by the last build.
.PHONY : $(BUILD_PLAT)/tftf.bin

tftf: realm
	@echo "  PACK REALM PAYLOAD"
	$(shell dd if=$(BUILD_PLAT)/realm.bin of=$(BUILD_PLAT)/tftf.bin obs=1 \
	oflag=append conv=notrunc)
endif

ifeq (${ARCH}-${PLAT},aarch64-tc)
  $(eval $(call MAKE_IMG,cactus))
  $(eval $(call MAKE_IMG,ivy))
endif

SP_LAYOUT: ${BUILD_PLAT}
	${Q}tools/generate_json/generate_json.sh \
		$(BUILD_PLAT) $(SECURE_PARTITIONS)

# The EL3 test payload is only supported in AArch64. It has an independent build
# system.
.PHONY: el3_payload
# TODO: EL3 test payload currently is supported for GCC only. It has an independent
# build system and support for Clang to be added.
ifneq ($(findstring gcc,$(notdir $(CC))),)
ifneq (${ARCH},aarch32)
ifneq ($(wildcard ${EL3_PAYLOAD_PLAT_MAKEFILE_FULL}),)
el3_payload: $(BUILD_DIR)
	${Q}${MAKE} -C el3_payload PLAT=${PLAT}
	${Q}find "el3_payload/build/${PLAT}" -name '*.bin' -exec cp {} "${BUILD_PLAT}" \;

all: el3_payload
endif
endif
endif

doc:
	@echo "  BUILD DOCUMENTATION"
	${Q}${MAKE} --no-print-directory -C ${DOCS_PATH} html

.PHONY: cscope
cscope:
	@echo "  CSCOPE"
	${Q}find ${CURDIR} -name "*.[chsS]" > cscope.files
	${Q}cscope -b -q -k

.PHONY: help
.SILENT: help
help:
	echo "usage: ${MAKE} PLAT=<${PLATFORMS}> \
<all|tftf|ns_bl1u|ns_bl2u|cactus|ivy|el3_payload|distclean|clean|checkcodebase|checkpatch|help_tests>"
	echo ""
	echo "PLAT is used to specify which platform you wish to build."
	echo "If no platform is specified, PLAT defaults to: ${DEFAULT_PLAT}"
	echo ""
	echo "Supported Targets:"
	echo "  all            Build all supported binaries for this platform"
	echo "                 (i.e. TFTF and FWU images)"
	echo "  tftf           Build the TFTF image"
	echo "  ns_bl1u        Build the NS_BL1U image"
	echo "  ns_bl2u        Build the NS_BL2U image"
	echo "  cactus         Build the Cactus image (FF-A S-EL1 test payload)."
	echo "  cactus_mm      Build the Cactus-MM image (SPM-MM S-EL0 test payload)."
	echo "  ivy            Build the Ivy image (FF-A S-EL0 test payload)."
	echo "  el3_payload    Build the EL3 test payload"
	echo "  checkcodebase  Check the coding style of the entire source tree"
	echo "  checkpatch     Check the coding style on changes in the current"
	echo "                 branch against BASE_COMMIT (default origin/master)"
	echo "  doc            Build html based documentation using Sphinx tool"
	echo "  clean          Clean the build for the selected platform"
	echo "  cscope         Generate cscope index"
	echo "  distclean      Remove all build artifacts for all platforms"
	echo "  help_tests     List all possible sets of tests"
	echo ""
	echo "note: most build targets require PLAT to be set to a specific platform."
	echo ""
	echo "example: build all targets for the FVP platform:"
	echo "  CROSS_COMPILE=aarch64-none-elf- make PLAT=fvp all"
