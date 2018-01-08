#!/bin/bash

# set ca crt for use in the client
xxd -i ca.crt | sed -e "s/ca_crt/ca_crt/" > ssl_client_crt.h

# set client crt for use in the client
xxd -i client.crt | sed -e "s/client_crt/client_crt/" >> ssl_client_crt.h

# set private key for use in the client
xxd -i client.key | sed -e "s/client_key/client_key/" >> ssl_client_crt.h

mv ssl_client_crt.h ./include
