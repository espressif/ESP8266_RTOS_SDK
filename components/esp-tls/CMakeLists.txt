set(srcs esp_tls.c)
if(CONFIG_ESP_TLS_USING_MBEDTLS)
    list(APPEND srcs
        "esp_tls_mbedtls.c")
endif()

if(CONFIG_ESP_TLS_USING_WOLFSSL)
    list(APPEND srcs
        "esp_tls_wolfssl.c")
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "private_include"
                    REQUIRES mbedtls wolfssl
                    PRIV_REQUIRES lwip http_parser)

if(CONFIG_ESP_TLS_USING_WOLFSSL)
    if(CONFIG_ESP_WOLFSSL_INTERNAL)
        idf_component_get_property(wolfssl esp-wolfssl COMPONENT_LIB)
        target_link_libraries(${COMPONENT_LIB} PUBLIC ${wolfssl})
    elseif(CONFIG_TLS_STACK_WOLFSSL)
        component_compile_options("-DWOLFSSL_USER_SETTINGS")
    endif()
endif()
