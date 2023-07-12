#
# Copyright (c) 2015-2023, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Convenience function for adding build definitions
# $(eval $(call add_define,BAR_DEFINES,FOO)) will have:
# -DFOO if $(FOO) is empty; -DFOO=$(FOO) otherwise
# inside the BAR_DEFINES variable.
define add_define
$(1) += -D$(2)$(if $(value $(2)),=$(value $(2)),)
endef

# Convenience function for verifying option has a boolean value
# $(eval $(call assert_boolean,FOO)) will assert FOO is 0 or 1
define assert_boolean
$(and $(patsubst 0,,$(value $(1))),$(patsubst 1,,$(value $(1))),$(error $(1) must be boolean))
endef

0-9 := 0 1 2 3 4 5 6 7 8 9

# Function to verify that a given option $(1) contains a numeric value
define assert_numeric
$(if $($(1)),,$(error $(1) must not be empty))
$(eval __numeric := $($(1)))
$(foreach d,$(0-9),$(eval __numeric := $(subst $(d),,$(__numeric))))
$(if $(__numeric),$(error $(1) must be numeric))
endef

# Convenience function for verifying options have numeric values
# $(eval $(call assert_numerics,FOO BOO)) will assert FOO and BOO contain numeric values
define assert_numerics
    $(foreach num,$1,$(eval $(call assert_numeric,$(num))))
endef

# CREATE_SEQ is a recursive function to create sequence of numbers from 1 to
# $(2) and assign the sequence to $(1)
define CREATE_SEQ
$(if $(word $(2), $($(1))),\
  $(eval $(1) += $(words $($(1))))\
  $(eval $(1) := $(filter-out 0,$($(1)))),\
  $(eval $(1) += $(words $($(1))))\
  $(call CREATE_SEQ,$(1),$(2))\
)
endef

# Convenience function to check for a given linker option. An call to
# $(call ld_option, --no-XYZ) will return --no-XYZ if supported by the linker
define ld_option
	$(shell if $(LD) $(1) -v >/dev/null 2>&1; then echo $(1); fi )
endef

# Convenience function to check for a given compiler option. An call to
# $(call cc_option, --no-XYZ) will return --no-XYZ if supported by the compiler
define cc_option
	$(shell if $(CC) $(1) -c -x c /dev/null -o /dev/null >/dev/null 2>&1; then echo $(1); fi )
endef

