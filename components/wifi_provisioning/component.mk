COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

ifdef CONFIG_ENABLE_UNIFIED_PROVISIONING
COMPONENT_SRCDIRS := src proto-c
COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_PRIV_INCLUDEDIRS := src proto-c ../protocomm/proto-c/

# To avoid warning for strncpy in "handlers.c" and "scheme_softap.c"
CPPFLAGS += -Wno-stringop-truncation
endif
