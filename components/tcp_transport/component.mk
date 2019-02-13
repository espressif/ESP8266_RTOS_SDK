#
# Component Makefile
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

ifdef CONFIG_SSL_USING_MBEDTLS 
COMPONENT_SRCDIRS := .
COMPONENT_ADD_INCLUDEDIRS := include
endif

ifdef CONFIG_SSL_USING_WOLFSSL 
COMPONENT_SRCDIRS := .
COMPONENT_ADD_INCLUDEDIRS := include
endif