#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Generate random fuzzing seeds
# If no instance count is provided, default to 1 instance
# If no seeds are provided, generate them randomly
# The number of seeds provided must match the instance count
SMC_FUZZ_INSTANCE_COUNT ?= 1
SMC_FUZZ_SEEDS ?= $(shell python -c "from random import randint; seeds = [randint(0, 4294967295) for i in range($(SMC_FUZZ_INSTANCE_COUNT))];print(\",\".join(str(x) for x in seeds));")
SMC_FUZZ_CALLS_PER_INSTANCE ?= 100
# ADDED: which fuzz call to start with per instance
SMC_FUZZ_CALL_START ?= 0
SMC_FUZZ_CALL_END ?= $(SMC_FUZZ_CALLS_PER_INSTANCE)
# ADDED: define whether events should specifically be constrained
EXCLUDE_FUNCID ?= 0
CONSTRAIN_EVENTS ?= 0
INTR_ASSERT ?= 0

# Validate SMC fuzzer parameters

# Instance count must not be zero
ifeq ($(SMC_FUZZ_INSTANCE_COUNT),0)
$(error SMC_FUZZ_INSTANCE_COUNT must not be zero!)
endif

# Calls per instance must not be zero
ifeq ($(SMC_FUZZ_CALLS_PER_INSTANCE),0)
$(error SMC_FUZZ_CALLS_PER_INSTANCE must not be zero!)
endif

# Make sure seed count and instance count match
TEST_SEED_COUNT = $(shell python -c "print(len(\"$(SMC_FUZZ_SEEDS)\".split(\",\")))")
ifneq ($(TEST_SEED_COUNT), $(SMC_FUZZ_INSTANCE_COUNT))
$(error Number of seeds does not match SMC_FUZZ_INSTANCE_COUNT!)
endif

# Start must be nonnegative and less than calls per instance
ifeq ($(shell test $(SMC_FUZZ_CALL_START) -lt 0; echo $$?),0)
$(error SMC_FUZZ_CALL_START must be nonnegative!)
endif

ifeq ($(shell test $(SMC_FUZZ_CALL_START) -gt $(shell expr $(SMC_FUZZ_CALLS_PER_INSTANCE) - 1); echo $$?),0)
$(error SMC_FUZZ_CALL_START must be less than SMC_FUZZ_CALLS_PER_INSTANCE!)
endif

# End must be greater than start and less than or equal to calls per instance
ifneq ($(shell test $(SMC_FUZZ_CALL_START) -lt $(SMC_FUZZ_CALL_END); echo $$?),0)
$(error SMC_FUZZ_CALL_END must be greater than SMC_FUZZ_CALL_START!)
endif

ifeq ($(shell test $(SMC_FUZZ_CALL_END) -gt $(SMC_FUZZ_CALLS_PER_INSTANCE); echo $$?),0)
$(error SMC_FUZZ_CALL_END must not be greater than SMC_FUZZ_CALLS_PER_INSTANCE!)
endif


# Add definitions to TFTF_DEFINES so they can be used in the code
$(eval $(call add_define,TFTF_DEFINES,SMC_FUZZ_SEEDS))
$(eval $(call add_define,TFTF_DEFINES,SMC_FUZZ_INSTANCE_COUNT))
$(eval $(call add_define,TFTF_DEFINES,SMC_FUZZ_CALLS_PER_INSTANCE))
ifeq ($(SMC_FUZZER_DEBUG),1)
$(eval $(call add_define,TFTF_DEFINES,SMC_FUZZER_DEBUG))
endif
ifeq ($(MULTI_CPU_SMC_FUZZER),1)
$(eval $(call add_define,TFTF_DEFINES,MULTI_CPU_SMC_FUZZER))
endif
$(eval $(call add_define,TFTF_DEFINES,SMC_FUZZ_SANITY_LEVEL))
$(eval $(call add_define,TFTF_DEFINES,SMC_FUZZ_CALL_START))
$(eval $(call add_define,TFTF_DEFINES,SMC_FUZZ_CALL_END))
$(eval $(call add_define,TFTF_DEFINES,CONSTRAIN_EVENTS))
$(eval $(call add_define,TFTF_DEFINES,EXCLUDE_FUNCID))
$(eval $(call add_define,TFTF_DEFINES,INTR_ASSERT))

TESTS_SOURCES	+=								\
	$(addprefix tftf/tests/runtime_services/standard_service/sdei/system_tests/, \
		sdei_entrypoint.S \
		test_sdei.c \
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		${ARCH}/ffa_arch_helpers.S				\
		ffa_helpers.c			\
		spm_common.c			\
	)
TESTS_SOURCES	+=							\
	$(addprefix smc_fuzz/src/,					\
		randsmcmod.c						\
		smcmalloc.c						\
		fifo3d.c						\
		runtestfunction_helpers.c				\
		sdei_fuzz_helper.c					\
		ffa_fuzz_helper.c					\
		tsp_fuzz_helper.c					\
		nfifo.c							\
		constraint.c						\
		vendor_fuzz_helper.c \
	)
