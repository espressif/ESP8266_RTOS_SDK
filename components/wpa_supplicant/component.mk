COMPONENT_ADD_INCLUDEDIRS := port/include port
COMPONENT_PRIV_INCLUDEDIRS := src
COMPONENT_SRCDIRS := port port/esp_supplicant src/ap src/common src/crypto src/eap_peer src/fast_crypto src/rsn_supp \
                     src/tls src/utils src/wps

CFLAGS += -DEMBEDDED_SUPP -D__ets__ -DESPRESSIF_USE -DIEEE8021X_EAPOL -DEAP_PEER_METHOD -DEAP_TLS -DEAP_TTLS -DEAP_PEAP -DEAP_MSCHAPv2 \
    -DUSE_WPA2_TASK -DCONFIG_WPS2 -DCONFIG_WPS_PIN -DUSE_WPS_TASK -DESP8266_WORKAROUND -Wno-unused-but-set-variable -Wno-empty-body -Wno-unused-value \
    -DCONFIG_ECC -DCONFIG_WPA3_SAE -DCONFIG_IEEE80211W
