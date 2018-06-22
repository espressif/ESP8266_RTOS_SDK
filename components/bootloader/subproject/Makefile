#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
ifeq ("$(MAKELEVEL)","0")
$(error Bootloader makefile expects to be run as part of 'make bootloader' from a top-level project.)
endif

PROJECT_NAME := bootloader

COMPONENTS := esptool_py main bootloader_support spi_flash log esp8266 util

# Clear C and CXX from top level project
CFLAGS =
CXXFLAGS =

#We cannot include the esp8266 component directly but we need its includes.
CFLAGS += -I $(IDF_PATH)/components/esp8266/include -I $(IDF_PATH)/components/util/include

# The bootloader pseudo-component is also included in this build, for its Kconfig.projbuild to be included.
#
# IS_BOOTLOADER_BUILD tells the component Makefile.projbuild to be a no-op
IS_BOOTLOADER_BUILD := 1
export IS_BOOTLOADER_BUILD

# BOOTLOADER_BUILD macro is the same, for source file changes
CFLAGS += -D BOOTLOADER_BUILD=1

# include the top-level "project" include directory, for sdkconfig.h
CFLAGS += -I$(BUILD_DIR_BASE)/../include

include $(IDF_PATH)/make/project.mk
