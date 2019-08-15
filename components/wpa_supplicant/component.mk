COMPONENT_ADD_INCLUDEDIRS := include port/include
COMPONENT_SRCDIRS := src/crypto src/wps port

CFLAGS += -DEMBEDDED_SUPP -D__ets__ -DESPRESSIF_USE -DCONFIG_WPS2 -DCONFIG_WPS_PIN -DUSE_WPS_TASK -Wno-unused-but-set-variable -Wno-empty-body