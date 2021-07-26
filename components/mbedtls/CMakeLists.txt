idf_build_get_property(target IDF_TARGET)
idf_component_register(INCLUDE_DIRS "port/include" "port/include/${target}" "mbedtls/include"
                    REQUIRES lwip)

# Only build mbedtls libraries
set(ENABLE_TESTING CACHE BOOL OFF)
set(ENABLE_PROGRAMS CACHE BOOL OFF)

# Needed to for include_next includes to work from within mbedtls
include_directories("${COMPONENT_DIR}/port/include" "${COMPONENT_DIR}/port/include/${target}")

# Import mbedtls library targets
add_subdirectory(mbedtls)

# Use port specific implementation of net_socket.c instead of one from mbedtls
get_target_property(src_tls mbedtls SOURCES)
list(REMOVE_ITEM src_tls net_sockets.c)
set_property(TARGET mbedtls PROPERTY SOURCES ${src_tls})

if(CONFIG_MBEDTLS_SSL_PROTO_GMTSSL1_1)
get_target_property(src_tls mbedtls SOURCES)
list(REMOVE_ITEM src_tls ssl_ciphersuites.c ssl_cli.c ssl_tls.c)
set_property(TARGET mbedtls PROPERTY SOURCES ${src_tls})

get_target_property(src_crypto mbedcrypto SOURCES)
list(REMOVE_ITEM src_crypto cipher_wrap.c ecdsa.c ecp.c ecp_curves.c oid.c  pk_wrap.c)
set_property(TARGET mbedcrypto PROPERTY SOURCES ${src_crypto})

get_target_property(src_x509 mbedx509 SOURCES)
list(REMOVE_ITEM src_x509 x509_crt.c)
set_property(TARGET mbedx509 PROPERTY SOURCES ${src_x509})
endif()

set(mbedtls_targets mbedtls mbedcrypto mbedx509)

# Add port files to mbedtls targets
set(mbedtls_target_sources "${COMPONENT_DIR}/port/mbedtls_debug.c"
                           "${COMPONENT_DIR}/port/net_sockets.c")

if(CONFIG_MBEDTLS_DYNAMIC_BUFFER)
set(mbedtls_target_sources ${mbedtls_target_sources}
                           "${COMPONENT_DIR}/port/dynamic/esp_mbedtls_dynamic_impl.c"
                           "${COMPONENT_DIR}/port/dynamic/esp_ssl_cli.c"
                           "${COMPONENT_DIR}/port/dynamic/esp_ssl_srv.c"
                           "${COMPONENT_DIR}/port/dynamic/esp_ssl_tls.c")
endif()

target_sources(mbedtls PRIVATE ${mbedtls_target_sources})

target_sources(mbedcrypto PRIVATE "${COMPONENT_DIR}/port/esp_hardware.c"
                                  "${COMPONENT_DIR}/port/esp_mem.c"
                                  "${COMPONENT_DIR}/port/esp_timing.c"
                                  "${COMPONENT_DIR}/port/esp_aes.c"
                                  "${COMPONENT_DIR}/port/esp_sha1.c"
                                  "${COMPONENT_DIR}/port/esp_sha256.c"
                                  "${COMPONENT_DIR}/port/esp_sha512.c"
                                  "${COMPONENT_DIR}/port/${target}/aes.c"
                                  "${COMPONENT_DIR}/port/${target}/arc4.c"
                                  "${COMPONENT_DIR}/port/${target}/base64.c"
                                  "${COMPONENT_DIR}/port/${target}/md5.c"
                                  "${COMPONENT_DIR}/port/${target}/sha1.c"
                                  "${COMPONENT_DIR}/port/${target}/sha256.c"
                                  "${COMPONENT_DIR}/port/${target}/sha512.c")


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
