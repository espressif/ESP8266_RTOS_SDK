set(COMPONENT_SRCS "esp_http_client_example.c")

# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
set(COMPONENT_EMBED_TXTFILES howsmyssl_com_root_cert.pem)

register_component()
