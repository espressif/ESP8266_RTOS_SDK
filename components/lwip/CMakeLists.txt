set(COMPONENT_ADD_INCLUDEDIRS
    include
    lwip/src/include
    lwip/src/include/lwip
    lwip/src/include/posix
    lwip/src/include/lwip
    include/lwip/apps
    include/lwip/port
    apps)

set(COMPONENT_SRCDIRS
    lwip/src/api
    lwip/src/apps/sntp
    lwip/src/netif
    lwip/src/core
    lwip/src/core/ipv4
    lwip/src/core/ipv6
    port/freertos
    port/netif
    apps/dhcpserver
    apps/multi-threads)

set(COMPONENT_REQUIRES tcpip_adapter esp8266 freertos)

register_component()

component_compile_options(-Wno-address)
