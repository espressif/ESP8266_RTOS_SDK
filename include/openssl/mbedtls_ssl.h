#ifndef _MBEDTLS_SSL_H_
#define _MBEDTLS_SSL_H_

#include "ssl_ctx.h"

/*
{
*/
#define SSL_MAX_FRAG_LEN_NONE 0
#define SSL_MAX_FRAG_LEN_512  512
#define SSL_MAX_FRAG_LEN_1024 1024
#define SSL_MAX_FRAG_LEN_2048 2048
#define SSL_MAX_FRAG_LEN_4096 4096
#define SSL_MAX_FRAG_LEN_8192 8192

#define SSL_DISPLAY_CERTS     (1 << 0)
#define SSL_NO_DEFAULT_KEY    (1 << 1)

#define SSL_SERVER_VERIFY_LATER (1 << 2)
#define SSL_CLIENT_AUTHENTICATION (1 << 3)

typedef void SSL;

enum {
	SSL_OK = 0
};

/*
}
*/

void ssl_set_frag(SSL *ssl, unsigned int frag);

SSL* ssl_new(struct ssl_ctx *ssl_ctx);

int ssl_connect(SSL *ssl);

int ssl_get_verify_result(SSL *ssl);

int ssl_set_fd(SSL *ssl, int);

int ssl_free(SSL *ssl);

int ssl_accept(SSL *ssl);

int ssl_read(SSL *ssl, void *buffer, int max_len);

int ssl_write(SSL *ssl, const void *buffer, int max_len);

#endif
