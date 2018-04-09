#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS := openssl/include
COMPONENT_PRIV_INCLUDEDIRS := openssl/include/internal openssl/include/openssl openssl/include/platform

COMPONENT_SRCDIRS := openssl/source/library openssl/source/platform

ifdef CONFIG_SSL_USING_MBEDTLS
COMPONENT_ADD_INCLUDEDIRS += mbedtls/include
COMPONENT_SRCDIRS += mbedtls/source/library mbedtls/source/platform
else
COMPONENT_ADD_INCLUDEDIRS += axtls/include
COMPONENT_SRCDIRS += axtls/source/ssl axtls/source/crypto
endif