#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += lwip/src/include lwip/src/include/lwip lwip/src/include/posix lwip/include/lwip \
								include/lwip/apps include/lwip/port apps

COMPONENT_SRCDIRS += lwip/src/api lwip/src/apps/sntp lwip/src/netif lwip/src/core lwip/src/core/ipv4 lwip/src/core/ipv6 \
								port/freertos port/netif apps/dhcpserver apps/multi-threads

CFLAGS += -Wno-address #lots of LWIP source files evaluate macros that check address of stack variables