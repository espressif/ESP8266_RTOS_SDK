#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := port/include mbedtls/include port/esp8266/include

COMPONENT_SRCDIRS := mbedtls/library port port/esp8266

COMPONENT_OBJEXCLUDE := mbedtls/library/net_sockets.o

COMPONENT_SUBMODULES += mbedtls

