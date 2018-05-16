
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
#include "ssl_pm.h"
#include "lwip/err.h"
#include "openssl/ssl.h"

typedef int MD5_CTX;
typedef int X509_CTX;

/*
Sets up digest context ctx to use a digest type from ENGINE impl.
Type will typically be supplied by a function such as EVP_sha1().
If impl is NULL then the default implementation of digest type is used.
*/
void EVP_DigestInit(MD5_CTX* ctx, uint8* out)
{
    return;
}

/*
Hashes ilen bytes of data at input into the digest context ctx.
This function can be called several times on the same ctx to hash additional data.
*/
void EVP_DigestUpdate(MD5_CTX* ctx, const uint8_t* input, int ilen)
{
    return;
}

/*
Retrieves the digest value from ctx and places it in output.
If the olen parameter is not NULL then the number of bytes of data written (i.e. the length of the digest)
will be written to the integer at s, at most EVP_MAX_MD_SIZE bytes will be written.
After calling EVP_DigestFinal() no additional calls to EVP_DigestUpdate() can be made,
but EVP_DigestInit() can be called to initialize a new digest operation.
*/
void EVP_DigestFinal(MD5_CTX* ctx, uint8_t* output, uint16* olen)
{
    return;
}

/*
Return EVP_MD structures for the SHA1 digest algorithms respectively.
The associated signature algorithm is RSA in each case.
*/
char* EVP_sha1(void)
{
    return NULL;
}

/*
cleans up EVP.
*/
char* EVP_cleanup(void)
{
    return NULL;
}

static const unsigned char base64_enc_map[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
};

/******************************************************************************
 * FunctionName : base64_encode
 * Description  : Encode Base64 data
 * Parameters	 : dst -- destination buffer
 * 			   dlen -- destination buffer len
 * 			   olen -- output buffer len
 * 			   src -- source buffer
 * 			   slen -- source buffer len
 * Returns 	 : none
*******************************************************************************/
int base64_encode(uint8* dst, size_t dlen, size_t* olen,
                  const uint8_t* src, size_t slen)
{
    size_t i, n;
    int C1, C2, C3;
    unsigned char* p = NULL;

    if (slen == 0) {
        *olen = 0;
        return 0;
    }

    n = (slen << 3) / 6;

    switch ((slen << 3) - (n * 6)) {
        case 2:
            n += 3;
            break;

        case 4:
            n += 2;
            break;

        default:
            break;
    }

    if (dlen < (n + 1)) {
        *olen = n + 1;
        return -42;
    }

    n = (slen / 3) * 3;

    for (i = 0, p = dst; i < n; i += 3) {
        C1 = *src++;
        C2 = *src++;
        C3 = *src++;

        *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map[(((C1 &  3) << 4) + (C2 >> 4)) & 0x3F];
        *p++ = base64_enc_map[(((C2 & 15) << 2) + (C3 >> 6)) & 0x3F];
        *p++ = base64_enc_map[C3 & 0x3F];
    }

    if (i < slen) {
        C1 = *src++;
        C2 = ((i + 1) < slen) ? *src++ : 0;

        *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];

        if ((i + 1) < slen) {
            *p++ = base64_enc_map[((C2 & 15) << 2) & 0x3F];
        } else {
            *p++ = '=';
        }

        *p++ = '=';

        *olen = p - dst;
        *p  = 0;

        return 0;
    }
}

/*
Return server SSLv23 method.
*/
const SSL_METHOD* SSLv23_server_method(void)
{
    return NULL;
}

/*
Return client SSLv23 method.
*/
const SSL_METHOD* SSLv23_client_method(void)
{
    return NULL;
}

/*
Add crt file for ssl_ctx.
*/
int SSL_CTX_use_certificate_chain_file(SSL_CTX* ssl_ctx, const char* file)
{
    return 1;
}

/******************************************************************************
 * FunctionName : SSL_CTX_load_verify_locations
 * Description  : load verify locations
 * Parameters	 : ctx -- espconn to set for client or server
 * 			   cafile -- ca file
 * 			   CApath -- no use
 * Returns 	 : 1
*******************************************************************************/
int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* CAfile,
                                  const char* CApath)
{
    X509* cacrt = NULL;
    cacrt = d2i_X509(NULL, CAfile, strlen(CAfile));

    if (cacrt) {
        SSL_CTX_add_client_CA(ctx, cacrt);
    }

    return 1;
}

/*
Return SSLv23 method.
*/
void SSLv23_method(void)
{
    return;
}

/*
Check private key in ctx.
*/
int SSL_CTX_check_private_key(const SSL_CTX* ctx)
{
    return 1;
}

/*
Init SSL library.
*/
void SSL_library_init(void)
{
    return;
}

/*
Set SSL_CTX verify paths.
*/
int SSL_CTX_set_default_verify_paths(SSL_CTX* ssl_ctx)
{
    return 1;
}

/*
Get current cert in x509 store ctx.
*/
X509_CTX* X509_STORE_CTX_get_current_cert(X509_CTX* store)
{
    return NULL;
}

/*
Prints an ASCII version of x509 ctx.
*/
void X509_NAME_oneline(X509_CTX* x509_CTX)
{
    return;
}

/*
Get issuer name.
*/
char* X509_get_issuer_name(X509_CTX* x509_CTX)
{
    return NULL;
}

/*
Get subject name.
*/
char* X509_get_subject_name(X509_CTX* x509_CTX)
{
    return NULL;
}

/*
Returns the depth of the error.
*/
void X509_STORE_CTX_get_error_depth(X509_CTX* x509_CTX)
{
    return;
}
/*
Returns the error code of ctx.
*/
char* X509_STORE_CTX_get_error(X509_CTX* x509_CTX)
{
    return NULL;
}

/*
Returns a human readable error string for verification error n.
*/
char* X509_verify_cert_error_string(X509_CTX* x509_CTX)
{
    return NULL;
}

/*
Cleanup extra crypto data.
*/
void CRYPTO_cleanup_all_ex_data(void)
{
    return;
}

/*
Get error number.
*/
int ERR_get_error(void)
{
    return 0;
}

/*
Generates a human-readable string representing the error code e,
and places it at buf. buf must be at least 120 bytes long.
Buf may not be NULL.
*/
void ERR_error_string_n(uint32 error, char* out, uint32 olen)
{
    return;
}

/*
Generates a human-readable string representing the error code e,
and places it at buf. buf must be at least 120 bytes long.
If buf is NULL , the error string is placed in a static buffer.
*/
char* ERR_error_string(unsigned long e, char* ret)
{
    return;
}

/*
Frees all previously loaded error strings.
*/
void ERR_free_strings(void)
{
    return;
}

/*
Convert an internal error to a string representation.
*/
const char* ERR_strerror(uint32 error)
{
    return lwip_strerr(error);
}
