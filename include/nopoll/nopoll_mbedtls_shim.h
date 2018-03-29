#ifndef __NOPOLL_MBEDTLS_SHIM_H__
#define __NOPOLL_MBEDTLS_SHIM_H__

#include "mbedtls/ssl.h"
#include "mbedtls/net.h"

int mbedtls_library_init(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf, mbedtls_net_context *server_fd, const char *host, const char *port);
void mbedtls_library_free(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf, mbedtls_net_context *server_fd);

#endif // __NOPOLL_MBEDTLS_SHIM_H__

