#ifndef __MBEDTLS_SSL_CPT_H__
#define __MBEDTLS_SSL_CPT_H__

#include "mbedtls_ssl.h"
#include "ssl_ctx.h"

/*
{
*/
typedef struct ssl_ctx SSL_CTX;
/*
}
*/

/*
{
*/
#define SSL_CTX_new ssl_ctx_new
#define SSL_CTX_free ssl_ctx_free

#define SSL_new ssl_new
#define SSL_free ssl_free

#define SSL_connect ssl_connect
#define SSL_accept ssl_accept
#define SSL_shutdown ssl_shutdown

#define SSL_read ssl_read
#define SSL_write ssl_write

#define SSL_get_verify_result ssl_get_verify_result
#define SSL_set_fd ssl_set_fd

#define SSL_CTX_set_option ssl_ctx_set_option

#define SSLv23_client_method() ssl_method_create(SSL_METHOD_CLIENT, SSL_METHOD_SSL_V2_3)
#define TLSv1_1_client_method() ssl_method_create(SSL_METHOD_CLIENT, SSL_METHOD_TLS_V1_1)
#define TLSv1_client_method() ssl_method_create(SSL_METHOD_CLIENT, SSL_METHOD_TLS_V1)
#define SSLv3_client_method() ssl_method_create(SSL_METHOD_CLIENT, SSL_METHOD_SSL_V3)

#define SSLv23_server_method() ssl_method_create(SSL_METHOD_SERVER, SSL_METHOD_SSL_V2_3)
#define TLSv1_1_server_method() ssl_method_create(SSL_METHOD_SERVER, SSL_METHOD_TLS_V1_1)
#define TLSv1_server_method() ssl_method_create(SSL_METHOD_SERVER, SSL_METHOD_TLS_V1)
#define SSLv3_server_method() ssl_method_create(SSL_METHOD_SERVER, SSL_METHOD_SSL_V3)
/*
}
*/


#endif
