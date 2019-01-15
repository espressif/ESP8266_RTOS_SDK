set(COMPONENT_ADD_INCLUDEDIRS
    include/lwip/apps
    lwip/src/include
    lwip/src/include/posix
    port/esp8266/include)

set(COMPONENT_SRCDIRS
    apps/dhcpserver
    apps/multi-threads
    lwip/src/api
    lwip/src/apps/sntp
    lwip/src/core
    lwip/src/core/ipv4
    lwip/src/core/ipv6
    lwip/src/netif
    port/esp8266/freertos
    port/esp8266/netif)

set(COMPONENT_REQUIRES tcpip_adapter esp8266 freertos)

register_component()

component_compile_options(-Wno-address)
