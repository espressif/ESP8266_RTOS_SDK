#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include/lwip/apps \
                             include/lwip \
                             lwip/src/include \
                             lwip/src/include/posix \
                             port/esp8266/include \
                             port/esp8266/include/port

COMPONENT_SRCDIRS += apps/dhcpserver \
                     apps/multi-threads \
                     apps/ping \
                     lwip/src/api \
                     lwip/src/apps/sntp \
                     lwip/src/core \
                     lwip/src/core/ipv4 \
                     lwip/src/core/ipv6 \
                     lwip/src/netif \
                     port/esp8266/freertos \
                     port/esp8266/netif

ifdef CONFIG_USING_ESP_VFS
COMPONENT_SRCDIRS += port
endif

COMPONENT_ADD_LDFRAGMENTS += linker.lf

CFLAGS += -Wno-address #lots of LWIP source files evaluate macros that check address of stack variables

lwip/src/apps/sntp/sntp.o: CFLAGS += -Wno-implicit-function-declaration
lwip/src/core/ipv4/ip4.o: CFLAGS += -Wno-implicit-function-declaration

ifdef CONFIG_LWIP_SOCKET_MULTITHREAD
COMPONENT_OBJEXCLUDE := lwip/src/api/sockets.o
endif
