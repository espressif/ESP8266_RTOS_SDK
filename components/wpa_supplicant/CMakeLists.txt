
set(COMPONENT_SRCDIRS "port" "port/esp_supplicant" "src/ap" "src/common" "src/crypto" "src/eap_peer" "src/fast_crypto" "src/rsn_supp" "src/tls" "src/utils" "src/wps")
set(COMPONENT_ADD_INCLUDEDIRS "port" "port/include" "src")
set(COMPONENT_PRIV_REQUIRES "mbedtls" "freertos" "heap" "newlib")


register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-strict-aliasing  -Wno-unused-value  -Wno-empty-body)
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
    CONFIG_ECC
    CONFIG_IEEE80211W
    CONFIG_WPA3_SAE
    )
