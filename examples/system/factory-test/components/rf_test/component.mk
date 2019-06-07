#
# Component Makefile
#

LIBS := rftest

COMPONENT_ADD_LDFLAGS += -L$(COMPONENT_PATH)/lib $(addprefix -l,$(LIBS))
COMPONENT_ADD_LINKER_DEPS += $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))
