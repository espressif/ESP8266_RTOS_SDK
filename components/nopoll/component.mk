#
# Component Makefile
#

ifdef CONFIG_SSL_USING_MBEDTLS
COMPONENT_ADD_INCLUDEDIRS += nopoll/src port/include

COMPONENT_SRCDIRS := nopoll/src
endif