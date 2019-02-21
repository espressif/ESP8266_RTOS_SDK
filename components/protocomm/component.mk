COMPONENT_ADD_INCLUDEDIRS :=
COMPONENT_SRCDIRS :=

ifdef CONFIG_SSL_USING_MBEDTLS
ifdef CONFIG_MBEDTLS_CIPHER_MODE_CTR
COMPONENT_ADD_INCLUDEDIRS := include/common include/security include/transports
COMPONENT_PRIV_INCLUDEDIRS := proto-c src/common
COMPONENT_SRCDIRS := src/common src/security proto-c src/transports
endif
endif
