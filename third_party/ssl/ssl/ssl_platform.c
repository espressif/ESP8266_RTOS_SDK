/*
 * Copyright (c) 2007, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Enable a subset of espressif platom ssl compatible functions. We don't aim to be 100%
 * compatible - just to be able to do basic ports etc.
 *
 * Only really tested on mini_httpd, so I'm not too sure how extensive this
 * port is.
 */
#include "ssl/ssl_platform.h"
#include "lwip/err.h"

#ifdef MEMLEAK_DEBUG
static const char mem_debug_file[] ICACHE_RODATA_ATTR STORE_ATTR = __FILE__;
#endif

/******************************************************************************
 * FunctionName : esp_EVP_DigestInit
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_EVP_DigestInit(MD5_CTX *ctx, uint8 *out)
{
	return ;
}

/******************************************************************************
 * FunctionName : esp_EVP_DigestUpdate
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_EVP_DigestUpdate(MD5_CTX *ctx, const uint8_t *input, int ilen)
{
	return;
}

/******************************************************************************
 * FunctionName : esp_EVP_DigestFinal
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_EVP_DigestFinal(MD5_CTX *ctx, uint8_t *output, uint16* olen)
{
	return ;
}

/******************************************************************************
 * FunctionName : esp_EVP_sha1
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
char *ICACHE_FLASH_ATTR esp_EVP_sha1(void)
{
	return NULL;
}

/******************************************************************************
 * FunctionName : esp_EVP_cleanup
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
char *ICACHE_FLASH_ATTR esp_EVP_cleanup(void)
{
	return NULL;
}

static const unsigned char base64_enc_map[64] ICACHE_RODATA_ATTR STORE_ATTR =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h',	'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',	'w', 'x',
	'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '+', '/'
};

/******************************************************************************
 * FunctionName : esp_base64_encode
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
EXP_FUNC int STDCALL ICACHE_FLASH_ATTR esp_base64_encode(uint8 *dst, size_t dlen, size_t *olen,
        const uint8_t *src, size_t slen)
{
	size_t i, n;
	int C1, C2, C3;
	unsigned char *p = NULL;
	if (slen == 0){
		*olen = 0;
		return 0;
	}

	n = (slen<<3)/6;
	switch ((slen<<3) - (n * 6)){
		case 2:
			n += 3;
			break;
		case 4:
			n += 2;
			break;
		default:
			break;
	}

	if (dlen < (n + 1)){
		*olen = n + 1;
		return -42;
	}

	n = (slen/3) * 3;
	for (i = 0, p = dst; i< n; i += 3){
		C1 = *src++;
		C2 = *src++;
		C3 = *src++;

		*p ++ = system_get_data_of_array_8(base64_enc_map, (C1 >> 2) & 0x3F);
		*p ++ = system_get_data_of_array_8(base64_enc_map, (((C1 & 3) << 4) + (C2 >> 4)) & 0x3F);
		*p ++ = system_get_data_of_array_8(base64_enc_map, (((C2 & 15) << 2) + (C3 >> 6)) & 0x3F);
		*p ++ = system_get_data_of_array_8(base64_enc_map, C3 & 0x3F);
	}

	if ( i < slen){
		C1 = *src++;
		C2 = ((i + 1)< slen) ? *src++ : 0;

		*p ++ = system_get_data_of_array_8(base64_enc_map, (C1 >> 2) & 0x3F);
		*p ++ = system_get_data_of_array_8(base64_enc_map, (((C1 & 3) << 4) + (C2 >> 4)) & 0x3F);

		if ((i + 1)< slen)
			*p ++ = system_get_data_of_array_8(base64_enc_map, ((C2 & 15) << 2) & 0x3F);
		else
			*p ++ = '=';

		*p ++ = '=';

		*olen = p - dst;
		*p  = 0;

		return 0;
	}
}

static char *key_password = NULL;

/******************************************************************************
 * FunctionName : esp_SSLv23_server_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_SSLv23_server_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_SSLv3_server_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_SSLv3_server_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_TLSv1_server_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_TLSv1_server_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_TLSv1_1_server_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_TLSv1_1_server_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_TLSv1_1_client_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_TLSv1_1_client_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_SSLv23_client_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_SSLv23_client_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_SSLv3_client_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_SSLv3_client_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_TLSv1_client_method
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_TLSv1_client_method(void) { return NULL; }

/******************************************************************************
 * FunctionName : esp_ssl_CTX_new
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
SSL_CTX *ICACHE_FLASH_ATTR esp_ssl_CTX_new(ssl_func_type_t meth)
{
	uint32_t options;
	options = SSL_SERVER_VERIFY_LATER | SSL_DISPLAY_CERTS | SSL_NO_DEFAULT_KEY;
    SSL_CTX *ssl_ctx = ssl_ctx_new(options, SSL_DEFAULT_CLNT_SESS);
    ssl_ctx->bonus_attr = os_malloc(sizeof(PLATOM_CTX));
    PLATOM_CTX_ATTR->ssl_func_type = meth;
    return ssl_ctx;
}

/******************************************************************************
 * FunctionName : esp_ssl_CTX_set_option
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_set_option(SSL_CTX *ssl_ctx, uint32_t options)
{
	ssl_ctx->options = options;
    return 1;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_ssl_CTX_free(SSL_CTX *ssl_ctx)
{
    os_free(ssl_ctx->bonus_attr);
    ssl_ctx_free(ssl_ctx);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
SSL *ICACHE_FLASH_ATTR esp_ssl_new(SSL_CTX *ssl_ctx)
{
    SSL *ssl;
    ssl_func_type_t ssl_func_type;

    ssl = ssl_new(ssl_ctx, -1);        /* fd is set later */
    ssl_func_type = PLATOM_CTX_ATTR->ssl_func_type;

#ifdef CONFIG_SSL_ENABLE_CLIENT
    if (ssl_func_type == esp_SSLv23_client_method ||
        ssl_func_type == esp_SSLv3_client_method ||
        ssl_func_type == esp_TLSv1_client_method)
    {
        SET_SSL_FLAG(SSL_IS_CLIENT);
    }
    else
#endif
    {
        ssl->next_state = HS_CLIENT_HELLO;
    }

    return ssl;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_set_fd(SSL *s, int fd)
{
    s->client_fd = fd;
    return 1;   /* always succeeds */
}

/*
 * Do the handshaking from the beginning.
 */
int ICACHE_FLASH_ATTR do_server_accept(SSL *ssl)
{
    int ret = SSL_OK;
	
    ssl->bm_read_index = 0;
    ssl->next_state = HS_CLIENT_HELLO;
    ssl->hs_status = SSL_NOT_OK;            /* not connected */

    /* sit in a loop until it all looks good */
    if (!IS_SET_SSL_FLAG(SSL_CONNECT_IN_PARTS))
    {
        while (ssl->hs_status != SSL_OK)
        {
            ret = ssl_read(ssl, NULL);
 
            if (ret < SSL_OK)
                break;
        }

        ssl->hs_status = ret;            /* connected? */    
    }

    return ret;
}


/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_accept(SSL *ssl)
{
    return do_server_accept(ssl) == SSL_OK ? 1 : -1;
}

#ifdef CONFIG_SSL_ENABLE_CLIENT
/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_connect(SSL *ssl)
{
	ssl->version = SSL_PROTOCOL_VERSION_MAX;
	SET_SSL_FLAG(SSL_IS_CLIENT);
    return do_client_connect(ssl) == SSL_OK ? 1 : -1;
}
#endif

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_ssl_free(SSL *ssl)
{
    ssl_free(ssl);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_read(SSL *ssl, void *buf, int num)
{
    uint8_t *read_buf = NULL;
    int ret;
    static uint8_t *in_offt = NULL;
    static int in_msg_len = 0;

    if (in_offt == NULL){
    	while ((ret = ssl_read(ssl, &read_buf)) == SSL_OK);

		if (ret > SSL_OK){
			in_offt = read_buf;
			in_msg_len = ret;
		}
    }
    ret = (num < in_msg_len) ? num : in_msg_len;
    memcpy(buf, in_offt, ret);
    in_msg_len -= ret;
    if (in_msg_len == 0)
    	in_offt = NULL;
    else
    	in_offt += ret;
    return ret;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_write(SSL *ssl, const void *buf, int num)
{
    return ssl_write(ssl, buf, num);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_use_certificate_file(SSL_CTX *ssl_ctx, const char *file, int type)
{
    return (ssl_obj_load(ssl_ctx, SSL_OBJ_X509_CERT, file, NULL) == SSL_OK);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_use_certificate_auth_file(SSL_CTX *ssl_ctx, const char *file, int type)
{
    return (ssl_obj_load(ssl_ctx, SSL_OBJ_X509_CACERT, file, NULL) == SSL_OK);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_use_PrivateKey_file(SSL_CTX *ssl_ctx, const char *file, int type)
{
    return (ssl_obj_load(ssl_ctx, SSL_OBJ_RSA_KEY, file, key_password) == SSL_OK);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_CTX_use_certificate_ASN1(SSL_CTX *ssl_ctx, int len, const uint8_t *d)
{
    return (ssl_obj_memory_load(ssl_ctx,
                        SSL_OBJ_X509_CERT, d, len, NULL) == SSL_OK);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_CTX_set_session_id_context(SSL_CTX *ctx, const unsigned char *sid_ctx,
        unsigned int sid_ctx_len)
{
    return 1;
}

//int ICACHE_FLASH_ATTR esp_ssl_CTX_set_verify_depth(SSL_CTX *ctx)
//{
//    return 1;
//}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_use_certificate_chain_file(SSL_CTX *ssl_ctx, const char *file)
{
    return (ssl_obj_load(ssl_ctx,SSL_OBJ_X509_CERT, file, NULL) == SSL_OK);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_shutdown(SSL *ssl)
{
    return 1;
}

/*** get/set session ***/
/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
SSL_SESSION *ICACHE_FLASH_ATTR SSL_get1_session(SSL *ssl)
{
    return (SSL_SESSION *)ssl_get_session_id(ssl); /* note: wrong cast */
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_set_session(SSL *ssl, SSL_SESSION *session)
{
    memcpy(ssl->session_id, (uint8_t *)session, SSL_SESSION_ID_SIZE);
    return 1;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSL_SESSION_free(SSL_SESSION *session) { }
/*** end get/set session ***/

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
long ICACHE_FLASH_ATTR SSL_CTX_ctrl(SSL_CTX *ctx, int cmd, long larg, void *parg)
{
    return 0;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_ssl_CTX_set_verify(SSL_CTX *ctx, int mode,
        int (*verify_callback)(int, void *)) { }

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_ssl_CTX_set_verify_depth(SSL_CTX *ctx, int depth) { }

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
        const char *CApath)
{
    return 1;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR SSL_load_client_CA_file(const char *file)
{
    return (void *)file;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSL_CTX_set_client_CA_list(SSL_CTX *ssl_ctx, void *file)
{
    ssl_obj_load(ssl_ctx, SSL_OBJ_X509_CERT, (const char *)file, NULL);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSLv23_method(void) { }

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSL_CTX_set_default_passwd_cb(SSL_CTX *ctx, void *cb) { }

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSL_CTX_set_default_passwd_cb_userdata(SSL_CTX *ctx, void *u)
{
    key_password = (char *)u;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_peek(SSL *ssl, void *buf, int num)
{
    memcpy(buf, ssl->bm_data, num);
    return num;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSL_set_bio(SSL *ssl, void *rbio, void *wbio) { }

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
long ICACHE_FLASH_ATTR esp_ssl_get_verify_result(const SSL *ssl)
{
    return ssl_handshake_status(ssl);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_state(SSL *ssl)
{
    return 0x03; // ok state
}

/** end of could do better list */
/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void *ICACHE_FLASH_ATTR esp_ssl_get_peer_certificate(const SSL *ssl)
{
    return &ssl->ssl_ctx->certs[0];
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_clear(SSL *ssl)
{
    return 1;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_check_private_key(const SSL_CTX *ctx)
{
    return 1;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR SSL_CTX_set_cipher_list(SSL *s, const char *str)
{
    return 1;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_get_error(const SSL *ssl, int ret)
{
//    ssl_display_error(ret);
    return 0;   /* TODO: return proper return code */
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSL_CTX_set_options(SSL_CTX *ssl_ctx, int option) {}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_library_init(void ) { return 1; }

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR SSL_load_error_strings(void ) {}
//void ICACHE_FLASH_ATTR ERR_print_errors_fp(FILE *fp) {}
//
//#ifndef CONFIG_SSL_SKELETON_MODE
//long ICACHE_FLASH_ATTR SSL_CTX_get_timeout(const SSL_CTX *ssl_ctx) {
//                            return CONFIG_SSL_EXPIRY_TIME*3600; }
//long ICACHE_FLASH_ATTR SSL_CTX_set_timeout(SSL_CTX *ssl_ctx, long t) {
//                            return SSL_CTX_get_timeout(ssl_ctx); }
//#endif
//void ICACHE_FLASH_ATTR BIO_printf(FILE *f, const char *format, ...)
//{
//    va_list(ap);
//    va_start(ap, format);
//    vfprintf(f, format, ap);
//    va_end(ap);
//}
//
//void* ICACHE_FLASH_ATTR BIO_s_null(void) { return NULL; }
//FILE *ICACHE_FLASH_ATTR BIO_new(bio_func_type_t func)
//{
//    if (func == BIO_s_null)
//        return fopen("/dev/null", "r");
//    else
//        return NULL;
//}
//
//FILE *ICACHE_FLASH_ATTR BIO_new_fp(FILE *stream, int close_flag) { return stream; }
//int ICACHE_FLASH_ATTR BIO_free(FILE *a) { if (a != stdout && a != stderr) fclose(a); return 1; }
//

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ssl_CTX_set_default_verify_paths(SSL_CTX *ssl_ctx)
{
	return 1;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
X509_CTX * ICACHE_FLASH_ATTR esp_X509_store_ctx_get_current_cert(X509_CTX * store)
{
	return NULL;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_X509_free(X509_CTX * x509_CTX)
{
//	x509_free(x509_CTX);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_X509_NAME_oneline(X509_CTX * x509_CTX)
{
	x509_free(x509_CTX);
}


/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
char* ICACHE_FLASH_ATTR esp_X509_get_issuer_name(X509_CTX * x509_CTX)
{
	return NULL;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
char* ICACHE_FLASH_ATTR esp_X509_get_subject_name(X509_CTX * x509_CTX)
{
	return NULL;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_X509_STORE_CTX_get_error_depth(X509_CTX * x509_CTX)
{
//	x509_free(x509_CTX);
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
char* ICACHE_FLASH_ATTR esp_X509_STORE_CTX_get_error(X509_CTX * x509_CTX)
{
	return NULL;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
char* ICACHE_FLASH_ATTR esp_X509_verify_cert_error_string(X509_CTX * x509_CTX)
{
	return NULL;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR esp_ERR_get_error(void)
{
	return 0;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_ERR_error_string_n(uint32 error, char* out, uint32 olen)
{
	return;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_ERR_error_string(uint32 error, char* out)
{
	return;
}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_ERR_free_strings(void)
{

}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR esp_CRYPTO_cleanup_all_ex_data(void)
{

}

/******************************************************************************
 * FunctionName : esp_ssl_set_fd
 * Description  : sent data for client or server
 * Parameters   : s -- espconn to set for client or server
 *                fd -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/
const char* ICACHE_FLASH_ATTR esp_ERR_strerror(uint32 error)
{
	return lwip_strerr(error);
}

void ICACHE_FLASH_ATTR esp_ssl_sleep(uint16 mseconds)
{
	vTaskDelay(mseconds / portTICK_RATE_MS);
}

esp_ssl_CTX_set_client_cert_cb(){}

esp_ssl_CTX_set_mode(){}

esp_ssl_pending()
{

}

bool ICACHE_FLASH_ATTR esp_ssl_fragment_length_negotiation(SSL* ssl, int fragmet_level)
{
	return ssl_fragment_length_negotiation(ssl, fragmet_level);
}
