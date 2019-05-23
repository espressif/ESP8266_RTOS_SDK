
if(CONFIG_SSL_USING_WOLFSSL)
set(COMPONENT_ADD_INCLUDEDIRS wolfssl/include wolfssl/wolfssl wolfssl/wolfssl/wolfssl)
set(COMPONENT_SRCDIRS "wolfssl/source")
else()
if(CONFIG_SSL_USING_MBEDTLS)
set(COMPONENT_ADD_INCLUDEDIRS
    mbedtls/port/openssl/include
    mbedtls/mbedtls/include
    mbedtls/port/esp8266/include)

set(COMPONENT_PRIV_INCLUDEDIRS
    mbedtls/port/openssl/include/internal
    mbedtls/port/openssl/include/openssl
    mbedtls/port/openssl/include/platform)

set(COMPONENT_SRCDIRS
    mbedtls/port/openssl/source/library
    mbedtls/port/openssl/source/platform
    mbedtls/mbedtls/library
    mbedtls/port/esp8266)
endif()
endif()

set(COMPONENT_REQUIRES "lwip" "esp8266" "util")

register_component()

if(CONFIG_SSL_USING_WOLFSSL)
target_compile_options(${COMPONENT_NAME} PUBLIC -DWOLFSSL_USER_SETTINGS)

target_link_libraries(${COMPONENT_NAME} "-L ${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/lib")
target_link_libraries(${COMPONENT_NAME} wolfssl)
else()
if(CONFIG_SSL_USING_MBEDTLS)
target_compile_options(${COMPONENT_NAME} PUBLIC -DMBEDTLS_CONFIG_FILE="mbedtls/esp_config.h")
endif()
endif()
