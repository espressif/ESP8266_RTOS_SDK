/* sha.h
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


/*!
    \file wolfssl/wolfcrypt/sha.h
*/


#ifndef WOLF_CRYPT_SHA_H
#define WOLF_CRYPT_SHA_H

#include <wolfssl/wolfcrypt/types.h>

#ifndef NO_SHA

#if defined(HAVE_FIPS) && \
    defined(HAVE_FIPS_VERSION) && (HAVE_FIPS_VERSION >= 2)
    #include <wolfssl/wolfcrypt/fips.h>
#endif /* HAVE_FIPS_VERSION >= 2 */

#if defined(HAVE_FIPS) && \
	(!defined(HAVE_FIPS_VERSION) || (HAVE_FIPS_VERSION < 2))
#define wc_Sha             Sha
#define WC_SHA             SHA
#define WC_SHA_BLOCK_SIZE  SHA_BLOCK_SIZE
#define WC_SHA_DIGEST_SIZE SHA_DIGEST_SIZE
#define WC_SHA_PAD_SIZE    SHA_PAD_SIZE

/* for fips @wc_fips */
#include <cyassl/ctaocrypt/sha.h>
#endif

#ifdef FREESCALE_LTC_SHA
    #include "fsl_ltc.h"
#endif

#ifdef __cplusplus
    extern "C" {
#endif

/* avoid redefinition of structs */
#if !defined(HAVE_FIPS) || \
    (defined(HAVE_FIPS_VERSION) && (HAVE_FIPS_VERSION >= 2))

#ifdef WOLFSSL_MICROCHIP_PIC32MZ
    #include <wolfssl/wolfcrypt/port/pic32/pic32mz-crypt.h>
#endif
#ifdef STM32_HASH
    #include <wolfssl/wolfcrypt/port/st/stm32.h>
#endif
#ifdef WOLFSSL_ASYNC_CRYPT
    #include <wolfssl/wolfcrypt/async.h>
#endif

#if !defined(NO_OLD_SHA_NAMES)
    #define SHA             WC_SHA
#endif

#ifndef NO_OLD_WC_NAMES
    #define Sha             wc_Sha
    #define SHA_BLOCK_SIZE  WC_SHA_BLOCK_SIZE
    #define SHA_DIGEST_SIZE WC_SHA_DIGEST_SIZE
    #define SHA_PAD_SIZE    WC_SHA_PAD_SIZE
#endif

/* in bytes */
enum {
    WC_SHA              =  WC_HASH_TYPE_SHA,
    WC_SHA_BLOCK_SIZE   = 64,
    WC_SHA_DIGEST_SIZE  = 20,
    WC_SHA_PAD_SIZE     = 56
};


#if defined(WOLFSSL_TI_HASH)
    #include "wolfssl/wolfcrypt/port/ti/ti-hash.h"

#elif defined(WOLFSSL_IMX6_CAAM)
    #include "wolfssl/wolfcrypt/port/caam/wolfcaam_sha.h"

#else
/* Sha digest */
typedef struct wc_Sha {
#ifdef FREESCALE_LTC_SHA
        ltc_hash_ctx_t ctx;
#elif defined(STM32_HASH)
        STM32_HASH_Context stmCtx;
#else
        word32  buffLen;   /* in bytes          */
        word32  loLen;     /* length in bytes   */
        word32  hiLen;     /* length in bytes   */
        word32  buffer[WC_SHA_BLOCK_SIZE  / sizeof(word32)];
    #ifdef WOLFSSL_PIC32MZ_HASH
        word32  digest[PIC32_DIGEST_SIZE / sizeof(word32)];
    #else
        word32  digest[WC_SHA_DIGEST_SIZE / sizeof(word32)];
    #endif
        void*   heap;
    #ifdef WOLFSSL_PIC32MZ_HASH
        hashUpdCache cache; /* cache for updates */
    #endif
    #ifdef WOLFSSL_ASYNC_CRYPT
        WC_ASYNC_DEV asyncDev;
    #endif /* WOLFSSL_ASYNC_CRYPT */
#endif
} wc_Sha;

#endif /* WOLFSSL_TI_HASH */


#endif /* HAVE_FIPS */

WOLFSSL_API int wc_InitSha(wc_Sha*);
WOLFSSL_API int wc_InitSha_ex(wc_Sha* sha, void* heap, int devId);
WOLFSSL_API int wc_ShaUpdate(wc_Sha*, const byte*, word32);
WOLFSSL_API int wc_ShaFinalRaw(wc_Sha*, byte*);
WOLFSSL_API int wc_ShaFinal(wc_Sha*, byte*);
WOLFSSL_API void wc_ShaFree(wc_Sha*);

WOLFSSL_API int wc_ShaGetHash(wc_Sha*, byte*);
WOLFSSL_API int wc_ShaCopy(wc_Sha*, wc_Sha*);

#ifdef WOLFSSL_PIC32MZ_HASH
WOLFSSL_API void wc_ShaSizeSet(wc_Sha* sha, word32 len);
#endif

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* NO_SHA */
#endif /* WOLF_CRYPT_SHA_H */

