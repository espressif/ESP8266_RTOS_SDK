
set(COMPONENT_SRCDIRS "src/crypto" "src/fast_crypto" "src/wps" "src/wpa2/eap_peer" "src/wpa2/tls" "src/wpa2/utils" "port" "src")
set(COMPONENT_ADD_INCLUDEDIRS "include" "port/include")
set(COMPONENT_PRIV_REQUIRES "mbedtls" "freertos" "heap" "newlib" "util")


register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-strict-aliasing)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    __ets__
    EMBEDDED_SUPP
    ESPRESSIF_USE
    IEEE8021X_EAPOL
    EAP_PEER_METHOD
    EAP_MSCHAPv2
    EAP_TTLS
    EAP_TLS
    EAP_PEAP
    USE_WPA2_TASK
    CONFIG_WPS2
    CONFIG_WPS_PIN
    USE_WPS_TASK
    ESP8266_WORKAROUND
    )
