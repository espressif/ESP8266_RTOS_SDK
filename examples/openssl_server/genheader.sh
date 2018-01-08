#!/bin/bash

# set ca crt for use in the server
xxd -i ca.crt | sed -e "s/ca_crt/ca_crt/" > ssl_server_crt.h

# set server crt for use in the server
xxd -i server.crt | sed -e "s/server_crt/server_crt/" >> ssl_server_crt.h

# set private key for use in the server
xxd -i server.key | sed -e "s/server_key/server_key/" >> ssl_server_crt.h

mv ssl_server_crt.h ./include
