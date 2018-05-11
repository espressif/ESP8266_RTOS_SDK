#
# Component Makefile
#
COMPONENT_SRCDIRS := .

ifdef CONFIG_NEWLIB_ENABLE

ADD_NEW_NEWLIB := 0

ifdef CONFIG_NEWLIB_LIBRARY_LEVEL_NORMAL
LIBC_PATH := $(COMPONENT_PATH)/newlib/lib/libc.a
LIBM_PATH := $(COMPONENT_PATH)/newlib/lib/libm.a
ADD_NEW_NEWLIB := 1
else
ifdef CONFIG_NEWLIB_LIBRARY_LEVEL_NANO
LIBC_PATH := $(COMPONENT_PATH)/newlib/lib/libc_nano.a
LIBM_PATH := $(COMPONENT_PATH)/newlib/lib/libm.a
ADD_NEW_NEWLIB := 1
endif
endif

ifeq ($(ADD_NEW_NEWLIB),1)
COMPONENT_ADD_INCLUDEDIRS += newlib/include newlib/port/include
COMPONENT_SRCDIRS += newlib/port
COMPONENT_ADD_LDFLAGS := $(LIBC_PATH) $(LIBM_PATH) -lnewlib
COMPONENT_ADD_LINKER_DEPS := $(LIBC_PATH) $(LIBM_PATH)
endif

endif