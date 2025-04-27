if(CONFIG_MBEDTLS_V3)
    return()
endif()

idf_build_get_property(target IDF_TARGET)

set(current_dir ${COMPONENT_DIR}/mbedtls_v2)

idf_component_register(INCLUDE_DIRS "mbedtls_v2/port/include" "mbedtls_v2/port/include/${target}" "mbedtls_v2/mbedtls/include"
                    REQUIRES lwip)

# Only build mbedtls libraries
set(ENABLE_TESTING CACHE BOOL OFF)
set(ENABLE_PROGRAMS CACHE BOOL OFF)

# Needed to for include_next includes to work from within mbedtls
include_directories("${current_dir}/port/include" "${current_dir}/port/include/${target}")

# Import mbedtls library targets
add_subdirectory(mbedtls_v2/mbedtls)

# Use port specific implementation of net_socket.c instead of one from mbedtls
get_target_property(src_tls mbedtls SOURCES)
list(REMOVE_ITEM src_tls net_sockets.c)
set_property(TARGET mbedtls PROPERTY SOURCES ${src_tls})

set(mbedtls_targets mbedtls mbedcrypto mbedx509)

# Add port files to mbedtls targets
set(mbedtls_target_sources "${current_dir}/port/mbedtls_debug.c"
                           "${current_dir}/port/net_sockets.c")

if(CONFIG_MBEDTLS_DYNAMIC_BUFFER)
set(mbedtls_target_sources ${mbedtls_target_sources}
                           "${current_dir}/port/dynamic/esp_mbedtls_dynamic_impl.c"
                           "${current_dir}/port/dynamic/esp_ssl_cli.c"
                           "${current_dir}/port/dynamic/esp_ssl_srv.c"
                           "${current_dir}/port/dynamic/esp_ssl_tls.c")
endif()

target_sources(mbedtls PRIVATE ${mbedtls_target_sources})

target_sources(mbedcrypto PRIVATE "${current_dir}/port/esp_hardware.c"
                                  "${current_dir}/port/esp_mem.c"
                                  "${current_dir}/port/esp_timing.c"
                                  "${current_dir}/port/esp_aes.c"
                                  "${current_dir}/port/esp_sha1.c"
                                  "${current_dir}/port/esp_sha256.c"
                                  "${current_dir}/port/esp_sha512.c"
                                  "${current_dir}/port/${target}/aes.c"
                                  "${current_dir}/port/${target}/arc4.c"
                                  "${current_dir}/port/${target}/base64.c"
                                  "${current_dir}/port/${target}/md5.c"
                                  "${current_dir}/port/${target}/sha1.c"
                                  "${current_dir}/port/${target}/sha256.c"
                                  "${current_dir}/port/${target}/sha512.c")


foreach(target ${mbedtls_targets})
    target_compile_definitions(${target} PUBLIC -DMBEDTLS_CONFIG_FILE="mbedtls/esp_config.h" -DCONFIG_SSL_USING_MBEDTLS)
endforeach()

if(CONFIG_MBEDTLS_DYNAMIC_BUFFER)
    set(WRAP_FUNCTIONS
        mbedtls_ssl_handshake_client_step
        mbedtls_ssl_handshake_server_step
        mbedtls_ssl_read
        mbedtls_ssl_write
        mbedtls_ssl_session_reset
        mbedtls_ssl_free
        mbedtls_ssl_setup
        mbedtls_ssl_send_alert_message
        mbedtls_ssl_close_notify)

    foreach(wrap ${WRAP_FUNCTIONS})
        target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=${wrap}")
    endforeach()
endif()

# Link mbedtls libraries to component library
target_link_libraries(${COMPONENT_LIB} INTERFACE ${mbedtls_targets})
