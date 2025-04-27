#
# Component Makefile
#
ifdef CONFIG_MBEDTLS_V2
include $(IDF_PATH)/components/mbedtls/mbedtls_v2/component.mk
endif

ifdef CONFIG_MBEDTLS_V3
include $(IDF_PATH)/components/mbedtls/mbedtls_v3/component.mk
endif