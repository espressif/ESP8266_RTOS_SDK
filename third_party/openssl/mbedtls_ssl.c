/******************************************************************************
 * Copyright 2016-2017 Espressif Systems (Wuxi)
 *
 * FileName: ssc_mbedtls.c
 *
 * Description: SSC mbedtls command
 *
 * Modification history:
 *     2016/6/27,    create.    dongheng
 *******************************************************************************/
#include "mbedtls_ssl.h"
#include "ssl_debug.h"
#include "ssl_ctx.h"


/*******************************************************************************/
struct ssl_verify {

	mbedtls_x509_crt own_crt;

	mbedtls_x509_crt ca_crt;

	mbedtls_pk_context pk;
};


struct ssl_fd
{
	/* local socket file description */
	mbedtls_net_context fd;
	/* remote client socket file description */
	mbedtls_net_context cl_fd;

	mbedtls_ssl_config conf;

	mbedtls_ctr_drbg_context ctr_drbg;

	mbedtls_ssl_context ssl;

	mbedtls_entropy_context entropy;
};

struct mbed_ssl
{
	struct ssl_fd   *ssl_fd;

	struct ssl_ctx  *ssl_ctx;

	struct ssl_verify *ssl_verify;

	SSL_MUTEX_DEF(mutex);
};


#define SSL_CHECK_ENDIAN_MODE(ssl_fd, mode) (ssl_fd->conf.endpoint & mode)

#define SSL_SEND_DATA_MAX_LENGTH 1460
/*******************************************************************************/
/*******************************************************************************/

unsigned int max_content_len;

/*******************************************************************************/
/*******************************************************************************/

LOCAL struct mbed_ssl* mbed_ssl_alloc(void)
{
	struct mbed_ssl *mbed_ssl;

	mbed_ssl = (struct mbed_ssl *)ssl_mem_zalloc(sizeof(struct mbed_ssl));
	if (!mbed_ssl) HANDLE_RET(go_failed1, "ssl_self_alloc:[1]\n");

	mbed_ssl->ssl_fd = (struct ssl_fd *)ssl_mem_zalloc(sizeof(struct ssl_fd));
	if (!mbed_ssl->ssl_fd) HANDLE_RET(go_failed2, "ssl_self_alloc:[2]\n");

	mbed_ssl->ssl_verify = (struct ssl_verify *)ssl_mem_zalloc(sizeof(struct ssl_verify));
	if (!mbed_ssl->ssl_verify) HANDLE_RET(go_failed3, "ssl_self_alloc:[3]\n");

	return mbed_ssl;

go_failed3:
	ssl_mem_free(mbed_ssl->ssl_fd);
go_failed2:
	ssl_mem_free(mbed_ssl);
go_failed1:
	return NULL;
}

LOCAL void mbed_ssl_free(struct mbed_ssl *mbed_ssl)
{
	ssl_mem_free(mbed_ssl->ssl_verify);
	ssl_mem_free(mbed_ssl->ssl_fd);
	ssl_mem_free(mbed_ssl);
}

LOCAL int mbed_ssl_init(struct mbed_ssl *mbed_ssl, struct ssl_ctx *ssl_ctx)
{
	int ret;
	int endpoint;
	unsigned char *pers;

	struct ssl_fd *ssl_fd = mbed_ssl->ssl_fd;
	struct ssl_verify *ssl_verify = mbed_ssl->ssl_verify;

	mbed_ssl->ssl_ctx = ssl_ctx;
	SSL_MUTEX_INIT(&ssl_self->mutex);
	max_content_len = 2048;

	mbedtls_x509_crt_init(&ssl_verify->ca_crt);
	mbedtls_x509_crt_init(&ssl_verify->own_crt);
	mbedtls_pk_init(&ssl_verify->pk);

	mbedtls_net_init(&ssl_fd->fd);
	mbedtls_net_init(&ssl_fd->cl_fd);
	mbedtls_ssl_config_init(&ssl_fd->conf);
	mbedtls_ctr_drbg_init(&ssl_fd->ctr_drbg);
	mbedtls_entropy_init(&ssl_fd->entropy);
	mbedtls_ssl_init(&ssl_fd->ssl);

	if (ssl_ctx->option & SSL_SERVER_VERIFY_LATER) {
		pers = "server"; endpoint = MBEDTLS_SSL_IS_SERVER;
	} else {
		pers = "client"; endpoint = MBEDTLS_SSL_IS_CLIENT;
	}

	ret = mbedtls_ctr_drbg_seed(&ssl_fd->ctr_drbg, mbedtls_entropy_func, &ssl_fd->entropy, pers, strlen(pers));
	if (ret) HANDLE_ERR(-4, failed4, "mbedtls_ctr_drbg_seed:[%d]\n", ret);

	ret = mbedtls_ssl_config_defaults(&ssl_fd->conf, endpoint, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret) HANDLE_ERR(-5, failed5, "mbedtls_ssl_config_defaults:[%d]\n", ret);

	mbedtls_ssl_conf_rng(&ssl_fd->conf, mbedtls_ctr_drbg_random, &ssl_fd->ctr_drbg);
	mbedtls_ssl_conf_dbg(&ssl_fd->conf, NULL, NULL);
	mbedtls_ssl_set_bio(&ssl_fd->ssl, &ssl_fd->fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	if (SSL_CTX_VERIFY(ssl_ctx)) {
		struct ssl_ctx_verify *ctx_verify = mbed_ssl->ssl_ctx->verify;

		if (ctx_verify->ca_crt) {
			ret = mbedtls_x509_crt_parse(&ssl_verify->ca_crt, ctx_verify->ca_crt, ctx_verify->ca_crt_len);
			if (ret) HANDLE_ERR(-6, failed6, "mbedtls_x509_crt_parse:[CA]\n", ret);

			mbedtls_ssl_conf_ca_chain(&ssl_fd->conf, &ssl_verify->ca_crt, NULL);
			mbedtls_ssl_conf_authmode(&ssl_fd->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
		}

		if (ctx_verify->own_crt) {
			ret = mbedtls_x509_crt_parse(&ssl_verify->own_crt, ctx_verify->own_crt, ctx_verify->own_crt_len);
			if (ret) HANDLE_ERR(-7, failed7, "mbedtls_x509_crt_parse:[OWN]\n", ret);
		}

		if (ctx_verify->pk) {
			ret = mbedtls_pk_parse_key(&ssl_verify->pk, ctx_verify->pk, ctx_verify->pk_len, NULL, 0);
			if (ret) HANDLE_ERR(-8, failed8, "mbedtls_x509_crt_parse:[PK]\n", ret);

			ret = mbedtls_ssl_conf_own_cert(&ssl_fd->conf, &ssl_verify->own_crt, &ssl_verify->pk);
			if (ret) HANDLE_ERR(-9, failed9, "mbedtls_ssl_conf_own_cert:[%d]\n", ret);
		}
	} else {
		mbedtls_ssl_conf_authmode(&ssl_fd->conf, MBEDTLS_SSL_VERIFY_NONE);
	}

	return 0;

failed9:
	mbedtls_pk_free(&ssl_verify->pk);
failed8:
	mbedtls_x509_crt_free(&ssl_verify->own_crt);
failed7:
	mbedtls_x509_crt_free(&ssl_verify->ca_crt);
failed6:
	mbedtls_ssl_config_free(&ssl_fd->conf);
failed5:
	mbedtls_ctr_drbg_free(&ssl_fd->ctr_drbg);
failed4:
failed3:
failed2:
	mbedtls_entropy_free(&ssl_fd->entropy);
failed1:
	return ret;
}

LOCAL void mbed_ssl_deinit(struct mbed_ssl *mbed_ssl)
{
	struct ssl_fd *ssl_fd = mbed_ssl->ssl_fd;
	struct ssl_verify *ssl_verify = mbed_ssl->ssl_verify;

	mbedtls_x509_crt_free(&ssl_verify->ca_crt);
	mbedtls_x509_crt_free(&ssl_verify->own_crt);
	mbedtls_pk_free(&ssl_verify->pk);

	mbedtls_entropy_free(&ssl_fd->entropy);
	mbedtls_ctr_drbg_free(&ssl_fd->ctr_drbg);
	mbedtls_ssl_config_free(&ssl_fd->conf);
	mbedtls_ssl_free(&ssl_fd->ssl);
}

LOCAL void mbed_ssl_disconnect(struct mbed_ssl *mbed_ssl)
{
	struct ssl_fd *ssl_fd = mbed_ssl->ssl_fd;

	mbedtls_ssl_close_notify(&ssl_fd->ssl);
}

LOCAL int mbed_ssl_setup(struct mbed_ssl *mbed_ssl)
{
	int ret;
	struct ssl_fd *ssl_fd = mbed_ssl->ssl_fd;

	ret = mbedtls_ssl_setup(&ssl_fd->ssl, &ssl_fd->conf);
	if (ret) HANDLE_ERR(-1, failed1, "mbedtls_ssl_setup:[%d]\n", ret);

	return 0;

failed1:
	return ret;
}

/*******************************************************************************/
/*******************************************************************************/

int ssl_libary_init(void)
{
	return 0;
}

/*******************************************************************************/
/*******************************************************************************/

SSL* ssl_new(struct ssl_ctx *ssl_ctx)
{
	int ret;
	struct mbed_ssl *mbed_ssl;

	if (!ssl_ctx) HANDLE_ERR(-1, go_failed1, "ssl_new[1]\n");

	mbed_ssl = mbed_ssl_alloc();
	if (!mbed_ssl) HANDLE_ERR(-2, go_failed1, "ssc_soc_ssl_create\n");

	ret = mbed_ssl_init(mbed_ssl, ssl_ctx);
	if (ret) HANDLE_ERR(-3, go_failed2, "ssc_soc_ssl_init\n");

	ret = mbed_ssl_setup(mbed_ssl);
	if (ret) HANDLE_ERR(-4, go_failed3, "ssc_soc_ssl_setup\n");

	return (SSL *)mbed_ssl;

go_failed3:
	mbed_ssl_deinit(mbed_ssl);
go_failed2:
	mbed_ssl_free(mbed_ssl);
go_failed1:
	return NULL;
}

int ssl_set_fd(SSL *ssl, int fd)
{
	int ret;
	struct mbed_ssl *mbed_ssl = (struct mbed_ssl *)ssl;

	if (!mbed_ssl) HANDLE_ERR(-1, go_failed1, "ssl_set_fd[1]\n");
	if (fd < 0) HANDLE_ERR(-1, go_failed2, "ssl_set_fd[2]\n");

	mbed_ssl->ssl_fd->fd.fd = fd;

	return 0;

go_failed2:
go_failed1:
	return ret;
}

int ssl_set_rfd(SSL *ssl, int fd)
{
	return ssl_set_fd(ssl, fd);
}

int ssl_set_wfd(SSL *ssl, int fd)
{
	return ssl_set_fd(ssl, fd);
}

int ssl_shutdown(SSL *ssl)
{
	int ret;
	struct ssl_fd *ssl_fd;
	struct mbed_ssl *mbed_ssl = (struct mbed_ssl *)ssl;

	if (!mbed_ssl || !mbed_ssl->ssl_fd) HANDLE_ERR(-1, go_failed1, "ssl_shutdown\n");

	ssl_fd = mbed_ssl->ssl_fd;
	if (ssl_fd->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER) return 0;

	mbed_ssl_disconnect(mbed_ssl);

	return 0;

go_failed1:
	return ret;
}

int ICACHE_FLASH_ATTR
ssl_free(SSL *ssl)
{
	int ret;
	struct mbed_ssl *mbed_ssl = (struct mbed_ssl *)ssl;

	if (!mbed_ssl) HANDLE_ERR(-1, go_failed1, "ssl_free\n");

	mbed_ssl_deinit(mbed_ssl);
	mbed_ssl_free(mbed_ssl);

	return 0;

go_failed1:
	return ret;
}

int ssl_connect(SSL *ssl)
{
	int ret;
	struct ssl_fd *ssl_fd;
	struct mbed_ssl *mbed_ssl = (struct mbed_ssl *)ssl;

	if (!mbed_ssl) HANDLE_ERR(-1, go_failed1, "ssl_connect\n");

	ssl_fd = mbed_ssl->ssl_fd;
	while((ret = mbedtls_ssl_handshake(&ssl_fd->ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			HANDLE_ERR(-2, go_failed2, "mbedtls_ssl_handshake:[-0x%x]\n", -ret);
		}
	}

	if (SSL_CHECK_ENDIAN_MODE(ssl_fd, MBEDTLS_SSL_IS_SERVER)) {
		MBED_SSL_DEBUG(1, "server");
	} else {
		MBED_SSL_DEBUG(1, "client");
	}

	if (ssl_fd->ssl.state == MBEDTLS_SSL_HANDSHAKE_OVER) {
		MBED_SSL_DEBUG(1, " handshake OK\n");
	} else {
		MBED_SSL_DEBUG(1, " handshake failed\n");
	}

	return 0;

go_failed2:
go_failed1:
	return ret;
}

int ssl_accept(SSL *ssl)
{
	int ret;
	struct ssl_fd *ssl_fd;
	struct mbed_ssl *mbed_ssl = (struct mbed_ssl *)ssl;

	if (!mbed_ssl) HANDLE_ERR(-1, go_failed1, "ssl_connect\n");

	while((ret = mbedtls_ssl_handshake(&ssl_fd->ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			HANDLE_ERR(-2, go_failed2, "mbedtls_ssl_handshake\n");
		}
	}

	if (SSL_CHECK_ENDIAN_MODE(ssl_fd, MBEDTLS_SSL_IS_SERVER)) {
		MBED_SSL_DEBUG(1, "server");
	} else {
		MBED_SSL_DEBUG(1, "client");
	}

	if (ssl_fd->ssl.state == MBEDTLS_SSL_HANDSHAKE_OVER) {
		MBED_SSL_DEBUG(1, " handshake OK\n");
	} else {
		MBED_SSL_DEBUG(1, " handshake failed\n");
	}

go_failed2:
go_failed1:
	return ret;
}

int ssl_read(SSL *ssl, void *buffer, int max_len)
{
	int ret;
	struct ssl_fd *ssl_fd;
	struct mbed_ssl *mbed_ssl = (struct mbed_ssl *)ssl;

	if (!mbed_ssl || !mbed_ssl->ssl_fd) HANDLE_ERR(-1, go_failed1, "ssl_read\n");

	ssl_fd = mbed_ssl->ssl_fd;
	while((ret = mbedtls_ssl_read(&ssl_fd->ssl, buffer, max_len)) < 0) { //return = 0 >> EOF
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			HANDLE_ERR(-2, go_failed2, "ssl_read\n");
		}
	}

	return ret;

go_failed2:
go_failed1:
	return ret;
}

static int ssl_write_data(struct mbed_ssl *mbed_ssl, const char *buffer, int max_len)
{
	int ret;
	struct ssl_fd *ssl_fd = mbed_ssl->ssl_fd;

	if (!ssl_fd) HANDLE_ERR(-1, go_failed1, "ssl_write_data\n");

	while((ret = mbedtls_ssl_write(&ssl_fd->ssl, buffer, max_len)) < 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			HANDLE_ERR(-2, go_failed2, "ssl_write\n");
		}
	}

	return ret;

go_failed2:
go_failed1:
	return ret;
}

int ssl_write(SSL *ssl, const void *buffer, int max_len)
{
	int ret;
	int send_len = max_len;
	const char *pbuf = (const char *)buffer;
	struct mbed_ssl *mbed_ssl = (struct mbed_ssl *)ssl;

	if (!mbed_ssl) return -1;

	do {
		int send_bytes;

		if (send_len > SSL_SEND_DATA_MAX_LENGTH)
			send_bytes = SSL_SEND_DATA_MAX_LENGTH;
		else
			send_bytes = send_len;

		ret = ssl_write_data(mbed_ssl, pbuf, send_bytes);
		if (ret > 0) {
			pbuf += send_bytes;
			send_len -= send_bytes;
		}
	} while (ret > 0 && send_len);

	return ret;
}

int ssl_fragment_length_negotiation(SSL *ssl, unsigned int frag)
{
	if (frag < 2048 || frag > 8192) return -1;

	max_content_len = frag;

	return 0;

go_failed1:
	return -1;
}

int ssl_get_verify_result(SSL *ssl)
{
	return 0;
}


