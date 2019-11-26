COMPONENT_ADD_INCLUDEDIRS := include include/wps include/wpa2 port/include
COMPONENT_SRCDIRS := src/crypto src/wps src/fast_crypto src/wpa2/eap_peer src/wpa2/tls src/wpa2/utils src port

CFLAGS += -DEMBEDDED_SUPP -D__ets__ -DESPRESSIF_USE -DIEEE8021X_EAPOL -DEAP_PEER_METHOD -DEAP_TLS -DEAP_TTLS -DEAP_PEAP -DEAP_MSCHAPv2 -DUSE_WPA2_TASK -DCONFIG_WPS2 -DCONFIG_WPS_PIN -DUSE_WPS_TASK -DESP8266_WORKAROUND -Wno-unused-but-set-variable -Wno-empty-body