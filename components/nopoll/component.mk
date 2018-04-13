#
# Component Makefile
#

ifdef CONFIG_SSL_USING_MBEDTLS
COMPONENT_ADD_INCLUDEDIRS += include/nopoll

COMPONENT_SRCDIRS := library
endif