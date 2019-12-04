#
# Component Makefile
#
ifndef CONFIG_NEWLIB_LIBRARY_CUSTOMER

ifdef CONFIG_NEWLIB_LIBRARY_LEVEL_NORMAL
LIB_PATH := $(COMPONENT_PATH)/newlib/lib/libc.a $(COMPONENT_PATH)/newlib/lib/libm.a
else ifdef CONFIG_NEWLIB_LIBRARY_LEVEL_NANO
LIB_PATH := $(COMPONENT_PATH)/newlib/lib/libc_nano.a $(COMPONENT_PATH)/newlib/lib/libm.a
else ifdef CONFIG_NEWLIB_LIBRARY_LEVEL_FLOAT_NANO
LIB_PATH := $(COMPONENT_PATH)/newlib/lib/libc_fnano.a $(COMPONENT_PATH)/newlib/lib/libm.a
endif

COMPONENT_ADD_INCLUDEDIRS += newlib/port/include newlib/include
COMPONENT_SRCDIRS += newlib/port
COMPONENT_ADD_LDFLAGS := $(LIB_PATH) -lnewlib
COMPONENT_ADD_LINKER_DEPS := $(LIB_PATH)

ifdef CONFIG_NEWLIB_LIBRARY_LEVEL_FLOAT_NANO
COMPONENT_ADD_LDFLAGS := $(COMPONENT_ADD_LDFLAGS) -u _printf_float -u _scanf_float
endif

endif # CONFIG_NEWLIB_LIBRARY_CUSTOMER