
if(CONFIG_SSL_USING_WOLFSSL)
set(COMPONENT_ADD_INCLUDEDIRS include wolfssl/include)
else()
if(CONFIG_SSL_USING_MBEDTLS)
set(COMPONENT_ADD_INCLUDEDIRS
    openssl/include
    mbedtls/mbedtls/include
    mbedtls/port/include)

set(COMPONENT_PRIV_INCLUDEDIRS
    openssl/include/internal
    openssl/include/openssl
    openssl/include/platform)

set(COMPONENT_SRCDIRS
    openssl/source/library
    openssl/source/platform
    mbedtls/mbedtls/library
    mbedtls/port)
else()
set(COMPONENT_ADD_INCLUDEDIRS include axtls/include)

set(COMPONENT_SRCDIRS axtls/source/ssl axtls/source/crypto)
endif()
endif()

set(COMPONENT_REQUIRES lwip esp8266)

register_component()

if(CONFIG_SSL_USING_WOLFSSL)
target_compile_options(${COMPONENT_NAME} PUBLIC -DWOLFSSL_USER_SETTINGS)

target_link_libraries(ssl "-L ${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/lib")
target_link_libraries(ssl wolfssl)
else()
if(CONFIG_SSL_USING_MBEDTLS)
target_compile_options(${COMPONENT_NAME} PUBLIC -DMBEDTLS_CONFIG_FILE="esp_config.h")
endif()
endif()
