#
# Component Makefile
#

ifdef CONFIG_ESP_WOLFSSL_INTERNAL
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
endif