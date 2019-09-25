
set(COMPONENT_SRCDIRS "src/crypto" "src/fast_crypto" "src/wps" "port" "src")
set(COMPONENT_ADD_INCLUDEDIRS "include" "port/include")
set(COMPONENT_PRIV_REQUIRES "ssl" "freertos" "heap" "newlib" "util")


register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-strict-aliasing)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    __ets__
    EMBEDDED_SUPP
    ESPRESSIF_USE
    CONFIG_WPS2
    CONFIG_WPS_PIN
    USE_WPS_TASK
    ESP8266_WORKAROUND
    )
