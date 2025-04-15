#
# Component Makefile
#

CURRENT_DIR := mbedtls_v2

COMPONENT_ADD_INCLUDEDIRS := $(CURRENT_DIR)/port/include $(CURRENT_DIR)/mbedtls/include $(CURRENT_DIR)/port/include/$(IDF_TARGET)

COMPONENT_SRCDIRS := $(CURRENT_DIR)/mbedtls/library $(CURRENT_DIR)/port $(CURRENT_DIR)/port/$(IDF_TARGET)

COMPONENT_OBJEXCLUDE := $(CURRENT_DIR)/mbedtls/library/net_sockets.o

COMPONENT_SUBMODULES += $(CURRENT_DIR)/mbedtls

ifdef CONFIG_MBEDTLS_DYNAMIC_BUFFER

WRAP_FUNCTIONS = mbedtls_ssl_handshake_client_step \
                 mbedtls_ssl_handshake_server_step \
                 mbedtls_ssl_read \
                 mbedtls_ssl_write \
                 mbedtls_ssl_session_reset \
                 mbedtls_ssl_free \
                 mbedtls_ssl_setup \
                 mbedtls_ssl_send_alert_message \
                 mbedtls_ssl_close_notify

WRAP_ARGUMENT := -Wl,--wrap=

COMPONENT_ADD_LDFLAGS = -l$(COMPONENT_NAME) $(addprefix $(WRAP_ARGUMENT),$(WRAP_FUNCTIONS))

COMPONENT_SRCDIRS += $(CURRENT_DIR)/port/dynamic

endif
