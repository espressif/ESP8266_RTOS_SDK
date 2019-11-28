/* md5.h
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


/* md5.h for openssl */


#ifndef WOLFSSL_MD5_H_
#define WOLFSSL_MD5_H_

#include <wolfssl/wolfcrypt/settings.h>

#ifndef NO_MD5

#include <wolfssl/wolfcrypt/hash.h>

#ifdef WOLFSSL_PREFIX
#include "prefix_md5.h"
#endif

#ifdef __cplusplus
    extern "C" {
#endif


typedef struct WOLFSSL_MD5_CTX {
    void* holder[(112 + WC_ASYNC_DEV_SIZE) / sizeof(void*)];   /* big enough to hold wolfcrypt md5, but check on init */
} WOLFSSL_MD5_CTX;

WOLFSSL_API int wolfSSL_MD5_Init(WOLFSSL_MD5_CTX*);
WOLFSSL_API int wolfSSL_MD5_Update(WOLFSSL_MD5_CTX*, const void*, unsigned long);
WOLFSSL_API int wolfSSL_MD5_Final(unsigned char*, WOLFSSL_MD5_CTX*);


typedef WOLFSSL_MD5_CTX MD5_CTX;

#define MD5_Init wolfSSL_MD5_Init
#define MD5_Update wolfSSL_MD5_Update
#define MD5_Final wolfSSL_MD5_Final

#ifdef OPENSSL_EXTRA_BSD
    #define MD5Init wolfSSL_MD5_Init
    #define MD5Update wolfSSL_MD5_Update
    #define MD5Final wolfSSL_MD5_Final
#endif

#ifndef MD5
#define MD5(d, n, md) wc_Md5Hash((d), (n), (md))
#endif

#define MD5_DIGEST_LENGTH MD5_DIGEST_SIZE

#ifdef __cplusplus
    }  /* extern "C" */
#endif

#endif /* NO_MD5 */

#endif /* WOLFSSL_MD5_H_ */
