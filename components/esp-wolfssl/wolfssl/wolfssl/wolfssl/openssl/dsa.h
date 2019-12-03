/* dsa.h
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


/* dsa.h for openSSL */


#ifndef WOLFSSL_DSA_H_
#define WOLFSSL_DSA_H_

#include <wolfssl/openssl/bn.h>

#ifdef __cplusplus
    extern "C" {
#endif

#ifndef WOLFSSL_DSA_TYPE_DEFINED /* guard on redeclaration */
typedef struct WOLFSSL_DSA            WOLFSSL_DSA;
#define WOLFSSL_DSA_TYPE_DEFINED
#endif

typedef WOLFSSL_DSA                   DSA;

struct WOLFSSL_DSA {
    WOLFSSL_BIGNUM* p;
    WOLFSSL_BIGNUM* q;
    WOLFSSL_BIGNUM* g;
    WOLFSSL_BIGNUM* pub_key;      /* our y */
    WOLFSSL_BIGNUM* priv_key;     /* our x */
    void*          internal;     /* our Dsa Key */
    char           inSet;        /* internal set from external ? */
    char           exSet;        /* external set from internal ? */
};


WOLFSSL_API WOLFSSL_DSA* wolfSSL_DSA_new(void);
WOLFSSL_API void wolfSSL_DSA_free(WOLFSSL_DSA*);

WOLFSSL_API int wolfSSL_DSA_generate_key(WOLFSSL_DSA*);

typedef void (*WOLFSSL_BN_CB)(int i, int j, void* exArg);
WOLFSSL_API WOLFSSL_DSA* wolfSSL_DSA_generate_parameters(int bits,
                   unsigned char* seed, int seedLen, int* counterRet,
                   unsigned long* hRet, WOLFSSL_BN_CB cb, void* CBArg);
WOLFSSL_API int wolfSSL_DSA_generate_parameters_ex(WOLFSSL_DSA*, int bits,
                   unsigned char* seed, int seedLen, int* counterRet,
                   unsigned long* hRet, void* cb);

WOLFSSL_API int wolfSSL_DSA_LoadDer(WOLFSSL_DSA*, const unsigned char*, int sz);

WOLFSSL_API int wolfSSL_DSA_do_sign(const unsigned char* d,
                                    unsigned char* sigRet, WOLFSSL_DSA* dsa);

WOLFSSL_API int wolfSSL_DSA_do_verify(const unsigned char* d,
                                      unsigned char* sig,
                                      WOLFSSL_DSA* dsa, int *dsacheck);

#define DSA_new wolfSSL_DSA_new
#define DSA_free wolfSSL_DSA_free

#define DSA_generate_key           wolfSSL_DSA_generate_key
#define DSA_generate_parameters    wolfSSL_DSA_generate_parameters
#define DSA_generate_parameters_ex wolfSSL_DSA_generate_parameters_ex


#ifdef __cplusplus
    }  /* extern "C" */ 
#endif

#endif /* header */
