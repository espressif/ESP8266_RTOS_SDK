/* crypto.h
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


/* crypto.h for openSSL */

#ifndef WOLFSSL_CRYPTO_H_
#define WOLFSSL_CRYPTO_H_

#include <wolfssl/openssl/opensslv.h>

#include <wolfssl/wolfcrypt/settings.h>

#ifdef WOLFSSL_PREFIX
#include "prefix_crypto.h"
#endif


WOLFSSL_API const char*   wolfSSLeay_version(int type);
WOLFSSL_API unsigned long wolfSSLeay(void);

#ifdef OPENSSL_EXTRA
WOLFSSL_API void wolfSSL_OPENSSL_free(void*);
WOLFSSL_API void *wolfSSL_OPENSSL_malloc(size_t a);
#endif

#define CRYPTO_THREADID void

#define SSLeay_version wolfSSLeay_version
#define SSLeay wolfSSLeay


#define SSLEAY_VERSION 0x0090600fL
#define SSLEAY_VERSION_NUMBER SSLEAY_VERSION
#define CRYPTO_lock wc_LockMutex_ex

/* this function was used to set the default malloc, free, and realloc */
#define CRYPTO_malloc_init() /* CRYPTO_malloc_init is not needed */

#define OPENSSL_free wolfSSL_OPENSSL_free
#define OPENSSL_malloc wolfSSL_OPENSSL_malloc

#if defined(OPENSSL_ALL) || defined(HAVE_STUNNEL) || defined(WOLFSSL_NGINX) || \
    defined(WOLFSSL_HAPROXY) || defined(OPENSSL_EXTRA)
#define CRYPTO_set_mem_ex_functions      wolfSSL_CRYPTO_set_mem_ex_functions
#define FIPS_mode                        wolfSSL_FIPS_mode
#define FIPS_mode_set                    wolfSSL_FIPS_mode_set
typedef struct CRYPTO_EX_DATA            CRYPTO_EX_DATA;
typedef void (CRYPTO_free_func)(void*parent, void*ptr, CRYPTO_EX_DATA *ad, int idx,
        long argl, void* argp);
#define CRYPTO_THREADID_set_callback wolfSSL_THREADID_set_callback
#define CRYPTO_THREADID_set_numeric wolfSSL_THREADID_set_numeric

#define CRYPTO_r_lock wc_LockMutex_ex
#define CRYPTO_unlock wc_LockMutex_ex

#define CRYPTO_THREAD_lock wc_LockMutex
#define CRYPTO_THREAD_r_lock wc_LockMutex
#define CRYPTO_THREAD_unlock wc_UnLockMutex

#endif /* OPENSSL_ALL || HAVE_STUNNEL || WOLFSSL_NGINX || WOLFSSL_HAPROXY */

#endif /* header */
