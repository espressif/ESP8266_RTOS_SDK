/* rsa.h
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.  All rights reserved.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is distributed in binary form as licensed by Espressif Systems.
 * See README file or contact licensing@wolfssl.com with any questions or comments.
 *
 * https://www.wolfssl.com
 */


/* rsa.h for openSSL */


#ifndef WOLFSSL_RSA_H_
#define WOLFSSL_RSA_H_

#include <wolfssl/openssl/bn.h>


#ifdef __cplusplus
    extern "C" {
#endif

/* Padding types */
#define RSA_PKCS1_PADDING      0
#define RSA_PKCS1_OAEP_PADDING 1

#ifndef WOLFSSL_RSA_TYPE_DEFINED /* guard on redeclaration */
typedef struct WOLFSSL_RSA            WOLFSSL_RSA;
#define WOLFSSL_RSA_TYPE_DEFINED
#endif

typedef WOLFSSL_RSA                   RSA;

struct WOLFSSL_RSA {
#ifdef WC_RSA_BLINDING
    WC_RNG* rng;              /* for PrivateDecrypt blinding */
#endif
	WOLFSSL_BIGNUM* n;
	WOLFSSL_BIGNUM* e;
	WOLFSSL_BIGNUM* d;
	WOLFSSL_BIGNUM* p;
	WOLFSSL_BIGNUM* q;
	WOLFSSL_BIGNUM* dmp1;      /* dP */
	WOLFSSL_BIGNUM* dmq1;      /* dQ */
	WOLFSSL_BIGNUM* iqmp;      /* u */
    void*          heap;
    void*          internal;  /* our RSA */
    char           inSet;     /* internal set from external ? */
    char           exSet;     /* external set from internal ? */
    char           ownRng;    /* flag for if the rng should be free'd */
};


WOLFSSL_API WOLFSSL_RSA* wolfSSL_RSA_new(void);
WOLFSSL_API void        wolfSSL_RSA_free(WOLFSSL_RSA*);

WOLFSSL_API int wolfSSL_RSA_generate_key_ex(WOLFSSL_RSA*, int bits, WOLFSSL_BIGNUM*,
                                          void* cb);

WOLFSSL_API int wolfSSL_RSA_blinding_on(WOLFSSL_RSA*, WOLFSSL_BN_CTX*);
WOLFSSL_API int wolfSSL_RSA_public_encrypt(int len, const unsigned char* fr,
	                               unsigned char* to, WOLFSSL_RSA*, int padding);
WOLFSSL_API int wolfSSL_RSA_private_decrypt(int len, const unsigned char* fr,
	                               unsigned char* to, WOLFSSL_RSA*, int padding);
WOLFSSL_API int wolfSSL_RSA_private_encrypt(int len, unsigned char* in,
                            unsigned char* out, WOLFSSL_RSA* rsa, int padding);

WOLFSSL_API int wolfSSL_RSA_size(const WOLFSSL_RSA*);
WOLFSSL_API int wolfSSL_RSA_sign(int type, const unsigned char* m,
                               unsigned int mLen, unsigned char* sigRet,
                               unsigned int* sigLen, WOLFSSL_RSA*);
WOLFSSL_API int wolfSSL_RSA_sign_ex(int type, const unsigned char* m,
                               unsigned int mLen, unsigned char* sigRet,
                               unsigned int* sigLen, WOLFSSL_RSA*, int);
WOLFSSL_API int wolfSSL_RSA_verify(int type, const unsigned char* m,
                               unsigned int mLen, const unsigned char* sig,
                               unsigned int sigLen, WOLFSSL_RSA*);
WOLFSSL_API int wolfSSL_RSA_public_decrypt(int flen, const unsigned char* from,
                                  unsigned char* to, WOLFSSL_RSA*, int padding);
WOLFSSL_API int wolfSSL_RSA_GenAdd(WOLFSSL_RSA*);
WOLFSSL_API int wolfSSL_RSA_LoadDer(WOLFSSL_RSA*, const unsigned char*, int sz);
WOLFSSL_API int wolfSSL_RSA_LoadDer_ex(WOLFSSL_RSA*, const unsigned char*, int sz, int opt);

#define WOLFSSL_RSA_LOAD_PRIVATE 1
#define WOLFSSL_RSA_LOAD_PUBLIC  2
#define WOLFSSL_RSA_F4           0x10001L

#define RSA_new  wolfSSL_RSA_new
#define RSA_free wolfSSL_RSA_free

#define RSA_generate_key_ex wolfSSL_RSA_generate_key_ex

#define RSA_blinding_on     wolfSSL_RSA_blinding_on
#define RSA_public_encrypt  wolfSSL_RSA_public_encrypt
#define RSA_private_decrypt wolfSSL_RSA_private_decrypt
#define RSA_private_encrypt wolfSSL_RSA_private_encrypt

#define RSA_size           wolfSSL_RSA_size
#define RSA_sign           wolfSSL_RSA_sign
#define RSA_verify          wolfSSL_RSA_verify
#define RSA_public_decrypt wolfSSL_RSA_public_decrypt

#define RSA_F4             WOLFSSL_RSA_F4

#ifdef __cplusplus
    }  /* extern "C" */
#endif

#endif /* header */
