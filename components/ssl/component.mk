#
# Component Makefile
#

ifdef CONFIG_SSL_USING_MBEDTLS
COMPONENT_ADD_INCLUDEDIRS := openssl/include
COMPONENT_PRIV_INCLUDEDIRS := openssl/include/internal openssl/include/openssl openssl/include/platform

COMPONENT_SRCDIRS := openssl/source/library openssl/source/platform

COMPONENT_ADD_INCLUDEDIRS += mbedtls/mbedtls/include mbedtls/port/include
COMPONENT_SRCDIRS += mbedtls/mbedtls/library mbedtls/port
else
COMPONENT_ADD_INCLUDEDIRS := axtls/include
COMPONENT_SRCDIRS := axtls/source/ssl axtls/source/crypto
endif
