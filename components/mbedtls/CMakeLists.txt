idf_component_register(INCLUDE_DIRS "port/include" "port/esp8266/include" "mbedtls/include"
                    REQUIRES lwip util)

# Only build mbedtls libraries
set(ENABLE_TESTING CACHE BOOL OFF)
set(ENABLE_PROGRAMS CACHE BOOL OFF)

# Needed to for include_next includes to work from within mbedtls
include_directories("${COMPONENT_DIR}/port/include" "${COMPONENT_DIR}/port/esp8266/include")

# Import mbedtls library targets
add_subdirectory(mbedtls)

# Use port specific implementation of net_socket.c instead of one from mbedtls
get_target_property(src_tls mbedtls SOURCES)
list(REMOVE_ITEM src_tls net_sockets.c)
set_property(TARGET mbedtls PROPERTY SOURCES ${src_tls})

set(mbedtls_targets mbedtls mbedcrypto mbedx509)

# Add port files to mbedtls targets
target_sources(mbedtls PRIVATE  "${COMPONENT_DIR}/port/mbedtls_debug.c"
                                "${COMPONENT_DIR}/port/net_sockets.c")

target_sources(mbedcrypto PRIVATE "${COMPONENT_DIR}/port/esp_hardware.c"
                                  "${COMPONENT_DIR}/port/esp_mem.c"
                                  "${COMPONENT_DIR}/port/esp_timing.c"
                                  "${COMPONENT_DIR}/port/esp8266/aes.c"
                                  "${COMPONENT_DIR}/port/esp8266/sha1.c"
                                  "${COMPONENT_DIR}/port/esp8266/sha256.c"
                                  "${COMPONENT_DIR}/port/esp8266/sha512.c")

foreach(target ${mbedtls_targets})
    target_compile_definitions(${target} PUBLIC -DMBEDTLS_CONFIG_FILE="mbedtls/esp_config.h" -DCONFIG_SSL_USING_MBEDTLS)
endforeach()

# Link mbedtls libraries to component library
target_link_libraries(${COMPONENT_LIB} INTERFACE ${mbedtls_targets})
