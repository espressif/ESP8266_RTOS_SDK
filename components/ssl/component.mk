#
# Component Makefile
#

ifdef CONFIG_SSL_USING_WOLFSSL
COMPONENT_ADD_INCLUDEDIRS := wolfssl/include wolfssl/wolfssl wolfssl/wolfssl/wolfssl

WOLFSSLLIB = wolfssl
COMPONENT_ADD_LDFLAGS     := -L $(COMPONENT_PATH)/wolfssl/lib -lwolfssl

# re-link program if wolfssl library change
COMPONENT_ADD_LINKER_DEPS := $(patsubst %,$(COMPONENT_PATH)/wolfssl/lib/lib%.a,$(WOLFSSLLIB))
else
ifdef CONFIG_SSL_USING_MBEDTLS
COMPONENT_ADD_INCLUDEDIRS := openssl/include
COMPONENT_PRIV_INCLUDEDIRS := openssl/include/internal openssl/include/openssl openssl/include/platform

COMPONENT_SRCDIRS := openssl/source/library openssl/source/platform

COMPONENT_ADD_INCLUDEDIRS += mbedtls/mbedtls/include mbedtls/port/esp8266/include
COMPONENT_SRCDIRS += mbedtls/mbedtls/library mbedtls/port/esp8266
else
COMPONENT_ADD_INCLUDEDIRS := axtls/include
COMPONENT_SRCDIRS := axtls/source/ssl axtls/source/crypto
endif
endif