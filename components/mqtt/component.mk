#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include/mqtt

COMPONENT_SRCDIRS += source/library source/platform

CFLAGS += -DMQTT_TASK
