#
# Copyright (c) 2018-2019, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include ../make_helpers/build_macros.mk

# Default number of threads per CPU on FVP
FVP_MAX_PE_PER_CPU	:= 1

# Check the PE per core count
ifneq ($(FVP_MAX_PE_PER_CPU),$(filter $(FVP_MAX_PE_PER_CPU),1 2))
$(error "Incorrect FVP_MAX_PE_PER_CPU specified for FVP port")
endif

# Convenience function for adding build definitions
# $(eval $(call add_define,BAR_DEFINES,FOO)) will have:
# -DFOO if $(FOO) is empty; -DFOO=$(FOO) otherwise
# inside the BAR_DEFINES variable.
define add_define
$(1) += -D$(2)$(if $(value $(2)),=$(value $(2)),)
endef

CC		:=	${CROSS_COMPILE}gcc
AS		:=	${CROSS_COMPILE}as
LD		:=	${CROSS_COMPILE}ld
OC		:=	${CROSS_COMPILE}objcopy
OD		:=	${CROSS_COMPILE}objdump

PLAT		?=	fvp

ASFLAGS		:=	-nostdinc -ffreestanding -Wa,--fatal-warnings -Werror
ASFLAGS		+=	-Iplat/${PLAT}/ -I.

LDFLAGS		+=	$(call ld_option,--no-warn-rwx-segments)

PLAT_BUILD_DIR	:=	build/${PLAT}
SOURCES		:=	entrypoint.S spin.S uart.S plat/${PLAT}/platform.S
OBJS		:=	$(patsubst %,$(PLAT_BUILD_DIR)/%,$(notdir $(SOURCES:.S=.o)))

PROGRAM		:=	el3_payload
LINKER_SCRIPT	:=	${PLAT_BUILD_DIR}/${PROGRAM}.ld
ELF		:=	${PLAT_BUILD_DIR}/${PROGRAM}.elf
BIN		:=	${PLAT_BUILD_DIR}/${PROGRAM}.bin
DUMP		:=	${PLAT_BUILD_DIR}/${PROGRAM}.dump

include plat/${PLAT}/platform.mk

# Pass FVP_MAX_PE_PER_CPU to the build system
$(eval $(call add_define,ASFLAGS,FVP_MAX_PE_PER_CPU))

all: ${BIN}

${PLAT_BUILD_DIR}:
	mkdir -p $@

${PLAT_BUILD_DIR}/%.o: %.S ${PLAT_BUILD_DIR}
	@echo "  CC      $<"
	${CC} ${ASFLAGS} -c $< -o $@

# Specific rule for this '.o' file to avoid worrying about
# plat/${PLAT}/platform.S being in a sub-directory...
# TODO: Fix this workaround.
${PLAT_BUILD_DIR}/platform.o: plat/${PLAT}/platform.S ${PLAT_BUILD_DIR}
	@echo "  CC      $<"
	${CC} ${ASFLAGS} -c $< -o $@

${PLAT_BUILD_DIR}/%.ld: %.ld.S ${PLAT_BUILD_DIR}
	@echo "  PP      $<"
	${CC} -DDRAM_BASE=${DRAM_BASE} -DDRAM_SIZE=${DRAM_SIZE} -E -P -o $@ $<

${ELF}: ${LINKER_SCRIPT} ${OBJS}
	@echo "  LD      $<"
	${LD} ${LDFLAGS} ${OBJS} --script ${LINKER_SCRIPT} -o $@

${BIN}: ${ELF}
	@echo "  BIN     $@"
	${OC} -O binary $< $@

${DUMP}: $(ELF)
	@echo "  OD      $$@"
	${OD} -dx $< > $@

dump: ${DUMP}

clean:
	rm -rf ${PLAT_BUILD_DIR}

distclean: clean
	rm -rf build/

run: run_${PLAT}

run_juno: ${ELF} scripts/juno/run_armds_script.sh scripts/juno/load_el3_payload.ds
	scripts/juno/run_armds_script.sh scripts/juno/load_el3_payload.ds

run_fvp: scripts/fvp/run_fvp.sh
	scripts/fvp/run_fvp.sh

help:
	@echo "EL3 test payload"
	@echo
	@echo "To build:"
	@echo "make [PLAT=fvp|juno] [TARGETS]"
	@echo ""
	@echo "The default platform is fvp."
	@echo
	@echo "TARGETS:"
	@echo "  all		Build the payload  [default target]"
	@echo "  dump		Generate a dump file of the program"
	@echo "  run		Run the payload on the given platform."
	@echo "         	/!\ For Juno, requires a connection to a Juno board via DSTREAM"
