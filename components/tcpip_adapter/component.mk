#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include

COMPONENT_SRCDIRS := ./

CFLAGS += -DLWIP_OPEN_SRC -DMEMLEAK_DEBUG -U__FILE__ -D__FILE__='"$(subst $(dir $<),,$<)"'
