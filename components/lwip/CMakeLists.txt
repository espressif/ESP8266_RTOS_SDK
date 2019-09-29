set(COMPONENT_ADD_INCLUDEDIRS
    "include/lwip"
    "include/lwip/apps"
    "lwip/src/include"
    "lwip/src/include/posix"
    "port/esp8266/include"
    "port/esp8266/include/port"
    )

set(COMPONENT_SRCDIRS
    "apps/dhcpserver"
    "apps/multi-threads"
    "apps/ping"
    "lwip/src/api"
    "lwip/src/apps/sntp"
    "lwip/src/core"
    "lwip/src/core/ipv4"
    "lwip/src/core/ipv6"
    "lwip/src/netif"
    "port/esp8266/freertos"
    "port/esp8266/netif"
    )

if(CONFIG_LWIP_SOCKET_MULTITHREAD)
set(COMPONENT_OBJEXCLUDE lwip/src/api/sockets.c)
endif()

if(CONFIG_USING_ESP_VFS)
set(COMPONENT_SRCDIRS ${COMPONENT_SRCDIRS} "port")
endif()

set(COMPONENT_REQUIRES vfs)
set(COMPONENT_PRIV_REQUIRES tcpip_adapter)

set(COMPONENT_ADD_LDFRAGMENTS "linker.lf")

register_component()

component_compile_options(-Wno-address)

# patch around warnings in third-party files
set_source_files_properties(lwip/src/apps/sntp/sntp.c lwip/src/core/ipv4/ip4.c
    PROPERTIES COMPILE_FLAGS
    -Wno-implicit-function-declaration
    )
