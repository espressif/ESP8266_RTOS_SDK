
if(CONFIG_ESP_WOLFSSL_INTERNAL)
set(COMPONENT_ADD_INCLUDEDIRS wolfssl/include wolfssl/wolfssl wolfssl/wolfssl/wolfssl)
set(COMPONENT_SRCDIRS "wolfssl/source")

set(COMPONENT_REQUIRES "lwip" "esp8266" "util")
endif()

register_component()

if(CONFIG_ESP_WOLFSSL_INTERNAL)
target_compile_options(${COMPONENT_LIB} PUBLIC -DWOLFSSL_USER_SETTINGS)

target_link_libraries(${COMPONENT_LIB} PUBLIC "-L ${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/lib")

if(CONFIG_WOLFSSL_DEBUG)
target_link_libraries(${COMPONENT_LIB} PUBLIC "wolfssl_debug")
else()
target_link_libraries(${COMPONENT_LIB} PUBLIC "wolfssl")
endif()
endif()
