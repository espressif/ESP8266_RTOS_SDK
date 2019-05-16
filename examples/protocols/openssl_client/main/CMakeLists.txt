set(COMPONENT_SRCS "openssl_client_example_main.c")

# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
set(COMPONENT_EMBED_TXTFILES "client.pem" "client.key" "ca.pem")

register_component()
