/* random.h
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.  All rights reserved.
 *
 * This file is part of wolfSSL.
 *
 * Contact licensing@wolfssl.com with any questions or comments.
 *
 * https://www.wolfssl.com
 */


/*!
    \file wolfssl/wolfcrypt/random.h
*/



#ifndef WOLF_CRYPT_RANDOM_H
#define WOLF_CRYPT_RANDOM_H

#include <wolfssl/wolfcrypt/types.h>

#if defined(HAVE_FIPS) && \
    defined(HAVE_FIPS_VERSION) && (HAVE_FIPS_VERSION >= 2)
    #include <wolfssl/wolfcrypt/fips.h>
#endif /* HAVE_FIPS_VERSION >= 2 */

/* included for fips @wc_fips */
#if defined(HAVE_FIPS) && \
        (!defined(HAVE_FIPS_VERSION) || (HAVE_FIPS_VERSION < 2))
#include <cyassl/ctaocrypt/random.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

 /* Maximum generate block length */
#ifndef RNG_MAX_BLOCK_LEN
    #ifdef HAVE_INTEL_QA
        #define RNG_MAX_BLOCK_LEN (0xFFFF)
    #else
        #define RNG_MAX_BLOCK_LEN (0x10000)
    #endif
#endif

/* Size of the BRBG seed */
#ifndef DRBG_SEED_LEN
    #define DRBG_SEED_LEN (440/8)
#endif


#if !defined(CUSTOM_RAND_TYPE)
    /* To maintain compatibility the default is byte */
    #define CUSTOM_RAND_TYPE    byte
#endif

/* make sure Hash DRBG is enabled, unless WC_NO_HASHDRBG is defined
    or CUSTOM_RAND_GENERATE_BLOCK is defined */
#if !defined(WC_NO_HASHDRBG) && !defined(CUSTOM_RAND_GENERATE_BLOCK)
    #undef  HAVE_HASHDRBG
    #define HAVE_HASHDRBG
    #ifndef WC_RESEED_INTERVAL
        #define WC_RESEED_INTERVAL (1000000)
    #endif
#endif


/* avoid redefinition of structs */
#if !defined(HAVE_FIPS) || \
    (defined(HAVE_FIPS_VERSION) && (HAVE_FIPS_VERSION >= 2))

/* RNG supports the following sources (in order):
 * 1. CUSTOM_RAND_GENERATE_BLOCK: Defines name of function as RNG source and
 *     bypasses the options below.
 * 2. HAVE_INTEL_RDRAND: Uses the Intel RDRAND if supported by CPU.
 * 3. HAVE_HASHDRBG (requires SHA256 enabled): Uses SHA256 based P-RNG
 *     seeded via wc_GenerateSeed. This is the default source.
 */

 /* Seed source can be overriden by defining one of these:
      CUSTOM_RAND_GENERATE_SEED
      CUSTOM_RAND_GENERATE_SEED_OS
      CUSTOM_RAND_GENERATE */


#if defined(CUSTOM_RAND_GENERATE_BLOCK)
    /* To use define the following:
     * #define CUSTOM_RAND_GENERATE_BLOCK myRngFunc
     * extern int myRngFunc(byte* output, word32 sz);
     */
#elif defined(HAVE_HASHDRBG)
    #ifdef NO_SHA256
        #error "Hash DRBG requires SHA-256."
    #endif /* NO_SHA256 */
    #include <wolfssl/wolfcrypt/sha256.h>
#elif defined(HAVE_WNR)
     /* allow whitewood as direct RNG source using wc_GenerateSeed directly */
#elif defined(HAVE_INTEL_RDRAND)
    /* Intel RDRAND or RDSEED */
#elif !defined(WC_NO_RNG)
    #error No RNG source defined!
#endif

#ifdef HAVE_WNR
    #include <wnr.h>
#endif

#ifdef WOLFSSL_ASYNC_CRYPT
    #include <wolfssl/wolfcrypt/async.h>
#endif


#if defined(USE_WINDOWS_API)
    #if defined(_WIN64)
        typedef unsigned __int64 ProviderHandle;
        /* type HCRYPTPROV, avoid #include <windows.h> */
    #else
        typedef unsigned long ProviderHandle;
    #endif
#endif


/* OS specific seeder */
typedef struct OS_Seed {
    #if defined(USE_WINDOWS_API)
        ProviderHandle handle;
    #else
        int fd;
    #endif
} OS_Seed;


#ifndef WC_RNG_TYPE_DEFINED /* guard on redeclaration */
    typedef struct WC_RNG WC_RNG;
    #define WC_RNG_TYPE_DEFINED
#endif

/* RNG context */
struct WC_RNG {
    OS_Seed seed;
    void* heap;
#ifdef HAVE_HASHDRBG
    /* Hash-based Deterministic Random Bit Generator */
    struct DRBG* drbg;
    byte status;
#endif
#ifdef WOLFSSL_ASYNC_CRYPT
    WC_ASYNC_DEV asyncDev;
    int devId;
#endif
};

#endif /* NO FIPS or have FIPS v2*/

/* NO_OLD_RNGNAME removes RNG struct name to prevent possible type conflicts,
 * can't be used with CTaoCrypt FIPS */
#if !defined(NO_OLD_RNGNAME) && !defined(HAVE_FIPS)
    #define RNG WC_RNG
#endif


WOLFSSL_LOCAL
int wc_GenerateSeed(OS_Seed* os, byte* seed, word32 sz);


#ifdef HAVE_WNR
    /* Whitewood netRandom client library */
    WOLFSSL_API int  wc_InitNetRandom(const char*, wnr_hmac_key, int);
    WOLFSSL_API int  wc_FreeNetRandom(void);
#endif /* HAVE_WNR */

#ifndef WC_NO_RNG
WOLFSSL_API int  wc_InitRng(WC_RNG*);
WOLFSSL_API int  wc_InitRng_ex(WC_RNG* rng, void* heap, int devId);
WOLFSSL_API int  wc_InitRngNonce(WC_RNG* rng, byte* nonce, word32 nonceSz);
WOLFSSL_API int  wc_InitRngNonce_ex(WC_RNG* rng, byte* nonce, word32 nonceSz,
                                    void* heap, int devId);
WOLFSSL_API int  wc_RNG_GenerateBlock(WC_RNG*, byte*, word32 sz);
WOLFSSL_API int  wc_RNG_GenerateByte(WC_RNG*, byte*);
WOLFSSL_API int  wc_FreeRng(WC_RNG*);
#else
#include <wolfssl/wolfcrypt/error-crypt.h>
#define wc_InitRng(rng) NOT_COMPILED_IN
#define wc_InitRng_ex(rng, h, d) NOT_COMPILED_IN
#define wc_InitRngNonce(rng, n, s) NOT_COMPILED_IN
#define wc_InitRngNonce_ex(rng, n, s, h, d) NOT_COMPILED_IN
#define wc_RNG_GenerateBlock(rng, b, s) NOT_COMPILED_IN
#define wc_RNG_GenerateByte(rng, b) NOT_COMPILED_IN
#define wc_FreeRng(rng) (void)NOT_COMPILED_IN
#endif



#ifdef HAVE_HASHDRBG
    WOLFSSL_LOCAL int wc_RNG_DRBG_Reseed(WC_RNG* rng, const byte* entropy,
                                        word32 entropySz);
    WOLFSSL_API int wc_RNG_TestSeed(const byte* seed, word32 seedSz);
    WOLFSSL_API int wc_RNG_HealthTest(int reseed,
                                        const byte* entropyA, word32 entropyASz,
                                        const byte* entropyB, word32 entropyBSz,
                                        byte* output, word32 outputSz);
    WOLFSSL_API int wc_RNG_HealthTest_ex(int reseed,
                                        const byte* nonce, word32 nonceSz,
                                        const byte* entropyA, word32 entropyASz,
                                        const byte* entropyB, word32 entropyBSz,
                                        byte* output, word32 outputSz,
                                        void* heap, int devId);
#endif /* HAVE_HASHDRBG */

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* WOLF_CRYPT_RANDOM_H */

