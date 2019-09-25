COMPONENT_ADD_INCLUDEDIRS := include include/wps port/include
COMPONENT_SRCDIRS := src/crypto src/wps src/fast_crypto src port

CFLAGS += -DEMBEDDED_SUPP -D__ets__ -DESPRESSIF_USE -DCONFIG_WPS2 -DCONFIG_WPS_PIN -DUSE_WPS_TASK -DESP8266_WORKAROUND -Wno-strict-aliasing