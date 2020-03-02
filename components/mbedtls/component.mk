#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := port/include mbedtls/include port/include/$(IDF_TARGET)

COMPONENT_SRCDIRS := mbedtls/library port port/$(IDF_TARGET)

COMPONENT_OBJEXCLUDE := mbedtls/library/net_sockets.o

COMPONENT_SUBMODULES += mbedtls

