#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include/mbedtls

COMPONENT_SRCDIRS += source/library source/platform

CFLAGS += -DMBEDTLS_CONFIG_FILE='"config_esp.h"'
