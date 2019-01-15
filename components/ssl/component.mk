#
# Component Makefile
#

ifdef CONFIG_SSL_USING_WOLFSSL
COMPONENT_ADD_INCLUDEDIRS := wolfssl/include wolfssl/wolfssl wolfssl/wolfssl/wolfssl

ifdef CONFIG_WOLFSSL_DEBUG
WOLFSSLLIB = wolfssl_debug
COMPONENT_ADD_LDFLAGS     := -L $(COMPONENT_PATH)/wolfssl/lib -lwolfssl_debug
else
WOLFSSLLIB = wolfssl
COMPONENT_ADD_LDFLAGS     := -L $(COMPONENT_PATH)/wolfssl/lib -lwolfssl
endif

# re-link program if wolfssl library change
COMPONENT_ADD_LINKER_DEPS := $(patsubst %,$(COMPONENT_PATH)/wolfssl/lib/lib%.a,$(WOLFSSLLIB))
else
ifdef CONFIG_SSL_USING_MBEDTLS
COMPONENT_PRIV_INCLUDEDIRS := mbedtls/port/openssl/include/internal mbedtls/port/openssl/include/openssl mbedtls/port/openssl/include/platform
COMPONENT_ADD_INCLUDEDIRS += mbedtls/mbedtls/include mbedtls/port/esp8266/include mbedtls/port/openssl/include
COMPONENT_SRCDIRS += mbedtls/mbedtls/library mbedtls/port/esp8266 mbedtls/port/openssl/source/library mbedtls/port/openssl/source/platform
else
COMPONENT_ADD_INCLUDEDIRS := axtls/include
COMPONENT_SRCDIRS := axtls/source/ssl axtls/source/crypto
endif
endif