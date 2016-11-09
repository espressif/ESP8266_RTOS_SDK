#ifndef __SSL_CTX_H__
#define __SSL_CTX_H__

#include "ssl_opt.h"

#define SSL_METHOD_CLIENT 0
#define SSL_METHOD_SERVER 1

#define SSL_METHOD_SSL_V3 0
#define SSL_METHOD_TLS_V1 1
#define SSL_METHOD_TLS_V1_1 2
#define SSL_METHOD_SSL_V2_3 3

#define SSL_CTX_VERIFY(ctx) (ctx->verify->ca_crt || ctx->verify->own_crt || ctx->verify->pk)

enum {
	SSL_OBJ_X509_CACERT = 1,
	SSL_OBJ_X509_CERT,
	SSL_OBJ_RSA_KEY,
};

struct x590_store_ctx {
	int x;
};

struct ssl_method {
	int x;
};

struct ssl_ctx_verify
{
	int verify_mode;

	int (*verify_cb)(int, struct x590_store_ctx *);

	char *ca_crt;
	unsigned int ca_crt_len;

	char *own_crt;
	unsigned int own_crt_len;

	char *pk;
	unsigned int pk_len;
};

struct ssl_ctx
{
	unsigned char type;
	unsigned char version;
	unsigned char option;

	/************************************/
	unsigned int ca_sect;
	unsigned int crt_sect;
	unsigned int pk_sect;

	/************************************/

	struct ssl_ctx_verify *verify;
};


struct ssl_ctx* ssl_ctx_new(struct ssl_method *method);

struct ssl_method* ssl_method_create(int mode, int ver);

int ssl_obj_memory_load(struct ssl_ctx *ctx, int type, char *buf, unsigned int len , void *attr);

int ssl_ctx_set_option(struct ssl_ctx *ctx, int opt);

#endif
