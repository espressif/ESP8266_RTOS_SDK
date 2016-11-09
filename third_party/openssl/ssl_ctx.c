#include "ssl_ctx.h"
#include "ssl_debug.h"


#ifndef MBED_SSL_CRT_MAX_LENGTH
	#define MBED_SSL_CRT_MAX_LENGTH (8 * 1024)
#endif

struct ssl_ctx* ssl_ctx_new(struct ssl_method *method)
{
	int ret;
	struct ssl_ctx *ctx;
	struct ssl_ctx_verify *verify;

	//if (!method) HANDLE_ERR(-1, go_failed1, "ssl_ctx_new:method\n");

	ctx = (struct ssl_ctx *)ssl_mem_zalloc(sizeof(struct ssl_ctx));
	if (!ctx) HANDLE_ERR(-2, go_failed2, "ssl_ctx_new:ctx\n");

	verify = (struct ssl_ctx_verify *)ssl_mem_zalloc(sizeof(struct ssl_ctx_verify));
	if (!verify) HANDLE_ERR(-3, go_failed3, "ssl_ctx_new:verify\n");

	ctx->verify = verify;

	return ctx;

go_failed3:
	ssl_mem_free(ctx);
go_failed2:
go_failed1:
	return SSL_NULL;
}

int ssl_ctx_set_verify(struct ssl_ctx *ctx, int mode,
                       int (*cb)(int, struct x590_store_ctx *))
{
	struct ssl_ctx_verify *verify = ctx->verify;

	verify->verify_mode = mode;
	verify->verify_cb = cb;

	return 0;
}

int ssl_ctx_load_verify(struct ssl_ctx *ctx, const char *CAfile, const char *CApath)
{
	return 0;
}

int ssl_ctx_use_certificate(struct ssl_ctx *ctx, const char *file, unsigned int size)
{
	return 0;
}

int ssl_ctx_use_privatekey(struct ssl_ctx *ctx, const char *file, unsigned int size)
{
	return 0;
}

int ssl_ctx_check_private_key(struct ssl_ctx *ctx)
{
	return 0;
}

LOCAL void ssl_ctx_verify_free(struct ssl_ctx_verify *ctx_verify)
{
	if (ctx_verify->ca_crt) ssl_mem_free(ctx_verify->ca_crt);
	if (ctx_verify->own_crt) ssl_mem_free(ctx_verify->own_crt);
	if (ctx_verify->pk) ssl_mem_free(ctx_verify->pk);

	ssl_mem_free(ctx_verify);
}

void ssl_ctx_free(struct ssl_ctx *ctx)
{
	if (!ctx) return ;

	ssl_ctx_verify_free(ctx->verify);
	ssl_mem_free(ctx);
}

struct ssl_method* ssl_method_create(int mode, int ver)
{
	return SSL_NULL;
}

LOCAL char* ssl_ctx_malloc_crt(const char *buf, unsigned int len, unsigned int *act_len)
{
	int ret;
	char *crt;

	crt = (char *)ssl_mem_zalloc(len + 1);
	if (!crt) HANDLE_ERR(-1, go_failed1, "ssl_ctx_malloc_crt\n");

	ssl_memcpy(crt, buf, len);
	crt[len] = '\0';
	*act_len = len + 1;

	return crt;

go_failed1:
	return NULL;
}

int ssl_obj_memory_load(struct ssl_ctx *ctx, int type, char *buf, unsigned int len , void *attr)
{
	int ret;
	struct ssl_ctx_verify *verify;

	if (!ctx || !ctx->verify || MBED_SSL_CRT_MAX_LENGTH < len) return -1;

	verify = ctx->verify;
	switch (type) {
		case SSL_OBJ_X509_CACERT :
		{
			char *crt;
			unsigned int crt_len;

			crt = ssl_ctx_malloc_crt(buf, len, &crt_len);
			if (!crt) HANDLE_ERR(-2, go_failed1, "ssl_obj_memory_load:CA\n");

			verify->ca_crt = crt;
			verify->ca_crt_len = crt_len;

			ret = 0;

			break;
		}
		case SSL_OBJ_X509_CERT :
		{
			char *crt;
			unsigned int crt_len;

			crt = ssl_ctx_malloc_crt(buf, len, &crt_len);
			if (!crt) HANDLE_ERR(-3, go_failed1, "ssl_obj_memory_load:CA\n");

			verify->own_crt = crt;
			verify->own_crt_len = crt_len;

			ret = 0;

			break;
		}

		case SSL_OBJ_RSA_KEY :
		{
			char *crt;
			unsigned int crt_len;

			crt = ssl_ctx_malloc_crt(buf, len, &crt_len);
			if (!crt) HANDLE_ERR(-4, go_failed1, "ssl_obj_memory_load:CA\n");

			verify->pk = crt;
			verify->pk_len = crt_len;

			ret = 0;

			break;
		}

		default :
		{
			ret = -2;

			break;
		}
	}

	return 0;

go_failed1:
	return ret;
}

int ssl_ctx_set_option(struct ssl_ctx *ctx, int opt)
{
	ctx->option = opt;

	return 0;
}
