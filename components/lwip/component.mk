#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include/ipv4 include/ipv6 include/posix

COMPONENT_SRCDIRS += source/api source/api/multi-threads source/apps source/arch \
                     source/netif source/core source/core/ipv4 source/core/ipv6

CFLAGS += -D_POSIX_SOURCE -DLWIP_OPEN_SRC -DPBUF_RSV_FOR_WLAN -DEBUF_LWIP
