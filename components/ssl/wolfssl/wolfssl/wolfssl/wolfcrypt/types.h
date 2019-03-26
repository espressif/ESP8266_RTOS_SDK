/* types.h
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
    \file wolfssl/wolfcrypt/types.h
*/

#ifndef WOLF_CRYPT_TYPES_H
#define WOLF_CRYPT_TYPES_H

    #include <wolfssl/wolfcrypt/settings.h>
    #include <wolfssl/wolfcrypt/wc_port.h>

    #ifdef __cplusplus
        extern "C" {
    #endif


    #if defined(WORDS_BIGENDIAN)
        #define BIG_ENDIAN_ORDER
    #endif

    #ifndef BIG_ENDIAN_ORDER
        #define LITTLE_ENDIAN_ORDER
    #endif

    #ifndef WOLFSSL_TYPES
        #ifndef byte
            typedef unsigned char  byte;
        #endif
        typedef unsigned short word16;
        typedef unsigned int   word32;
        typedef byte           word24[3];
    #endif


    /* try to set SIZEOF_LONG or LONG_LONG if user didn't */
    #if !defined(_MSC_VER) && !defined(__BCPLUSPLUS__) && !defined(__EMSCRIPTEN__)
        #if !defined(SIZEOF_LONG_LONG) && !defined(SIZEOF_LONG)
            #if (defined(__alpha__) || defined(__ia64__) || \
                defined(_ARCH_PPC64) || defined(__mips64) || \
                defined(__x86_64__) || \
                ((defined(sun) || defined(__sun)) && \
                 (defined(LP64) || defined(_LP64))))
                /* long should be 64bit */
                #define SIZEOF_LONG 8
            #elif defined(__i386__) || defined(__CORTEX_M3__)
                /* long long should be 64bit */
                #define SIZEOF_LONG_LONG 8
            #endif
         #endif
    #endif

    #if defined(_MSC_VER) || defined(__BCPLUSPLUS__)
        #define WORD64_AVAILABLE
        #define W64LIT(x) x##ui64
        typedef unsigned __int64 word64;
    #elif defined(__EMSCRIPTEN__)
        #define WORD64_AVAILABLE
        #define W64LIT(x) x##ull
        typedef unsigned long long word64;
    #elif defined(SIZEOF_LONG) && SIZEOF_LONG == 8
        #define WORD64_AVAILABLE
        #define W64LIT(x) x##LL
        typedef unsigned long word64;
    #elif defined(SIZEOF_LONG_LONG) && SIZEOF_LONG_LONG == 8
        #define WORD64_AVAILABLE
        #define W64LIT(x) x##LL
        typedef unsigned long long word64;
    #elif defined(__SIZEOF_LONG_LONG__) && __SIZEOF_LONG_LONG__ == 8
        #define WORD64_AVAILABLE
        #define W64LIT(x) x##LL
        typedef unsigned long long word64;
    #endif

#if !defined(NO_64BIT) && defined(WORD64_AVAILABLE)
    /* These platforms have 64-bit CPU registers.  */
    #if (defined(__alpha__) || defined(__ia64__) || defined(_ARCH_PPC64) || \
         defined(__mips64)  || defined(__x86_64__) || defined(_M_X64)) || \
         defined(__aarch64__) || defined(__sparc64__)
        typedef word64 wolfssl_word;
        #define WC_64BIT_CPU
    #elif (defined(sun) || defined(__sun)) && \
          (defined(LP64) || defined(_LP64))
        /* LP64 with GNU GCC compiler is reserved for when long int is 64 bits
         * and int uses 32 bits. When using Solaris Studio sparc and __sparc are
         * available for 32 bit detection but __sparc64__ could be missed. This
         * uses LP64 for checking 64 bit CPU arch. */
        typedef word64 wolfssl_word;
        #define WC_64BIT_CPU
    #else
        typedef word32 wolfssl_word;
        #ifdef WORD64_AVAILABLE
            #define WOLFCRYPT_SLOW_WORD64
        #endif
    #endif
#else
        #undef WORD64_AVAILABLE
        typedef word32 wolfssl_word;
        #define MP_16BIT  /* for mp_int, mp_word needs to be twice as big as
                             mp_digit, no 64 bit type so make mp_digit 16 bit */
#endif

    enum {
        WOLFSSL_WORD_SIZE  = sizeof(wolfssl_word),
        WOLFSSL_BIT_SIZE   = 8,
        WOLFSSL_WORD_BITS  = WOLFSSL_WORD_SIZE * WOLFSSL_BIT_SIZE
    };

    #define WOLFSSL_MAX_16BIT 0xffffU

    /* use inlining if compiler allows */
    #ifndef WC_INLINE
    #ifndef NO_INLINE
        #ifdef _MSC_VER
            #define WC_INLINE __inline
        #elif defined(__GNUC__)
               #ifdef WOLFSSL_VXWORKS
                   #define WC_INLINE __inline__
               #else
                   #define WC_INLINE inline
               #endif
        #elif defined(__IAR_SYSTEMS_ICC__)
            #define WC_INLINE inline
        #elif defined(THREADX)
            #define WC_INLINE _Inline
        #else
            #define WC_INLINE
        #endif
    #else
        #define WC_INLINE
    #endif
    #endif

    #if defined(HAVE_FIPS) || defined(HAVE_SELFTEST)
        #define INLINE WC_INLINE
    #endif


    /* set up rotate style */
    #if (defined(_MSC_VER) || defined(__BCPLUSPLUS__)) && \
        !defined(WOLFSSL_SGX) && !defined(INTIME_RTOS)
        #define INTEL_INTRINSICS
        #define FAST_ROTATE
    #elif defined(__MWERKS__) && TARGET_CPU_PPC
        #define PPC_INTRINSICS
        #define FAST_ROTATE
    #elif defined(__GNUC__)  && (defined(__i386__) || defined(__x86_64__))
        /* GCC does peephole optimizations which should result in using rotate
           instructions  */
        #define FAST_ROTATE
    #endif


    /* set up thread local storage if available */
    #ifdef HAVE_THREAD_LS
        #if defined(_MSC_VER)
            #define THREAD_LS_T __declspec(thread)
        /* Thread local storage only in FreeRTOS v8.2.1 and higher */
        #elif defined(FREERTOS) || defined(FREERTOS_TCP)
            #define THREAD_LS_T
        #else
            #define THREAD_LS_T __thread
        #endif
    #else
        #define THREAD_LS_T
    #endif

    /* GCC 7 has new switch() fall-through detection */
    #if defined(__GNUC__)
        #if ((__GNUC__ > 7) || ((__GNUC__ == 7) && (__GNUC_MINOR__ >= 1)))
            #define FALL_THROUGH __attribute__ ((fallthrough))
        #endif
    #endif
    #ifndef FALL_THROUGH
        #define FALL_THROUGH
    #endif

    /* Micrium will use Visual Studio for compilation but not the Win32 API */
    #if defined(_WIN32) && !defined(MICRIUM) && !defined(FREERTOS) && \
        !defined(FREERTOS_TCP) && !defined(EBSNET) && \
        !defined(WOLFSSL_UTASKER) && !defined(INTIME_RTOS)
        #define USE_WINDOWS_API
    #endif


    /* idea to add global alloc override by Moises Guimaraes  */
    /* default to libc stuff */
    /* XREALLOC is used once in normal math lib, not in fast math lib */
    /* XFREE on some embedded systems doesn't like free(0) so test  */
    #if defined(HAVE_IO_POOL)
        WOLFSSL_API void* XMALLOC(size_t n, void* heap, int type);
        WOLFSSL_API void* XREALLOC(void *p, size_t n, void* heap, int type);
        WOLFSSL_API void XFREE(void *p, void* heap, int type);
    #elif defined(WOLFSSL_ASYNC_CRYPT) && defined(HAVE_INTEL_QA)
        #include <wolfssl/wolfcrypt/port/intel/quickassist_mem.h>
        #undef USE_WOLFSSL_MEMORY
        #ifdef WOLFSSL_DEBUG_MEMORY
            #define XMALLOC(s, h, t)     IntelQaMalloc((s), (h), (t), __func__, __LINE__)
            #define XFREE(p, h, t)       IntelQaFree((p), (h), (t), __func__, __LINE__)
            #define XREALLOC(p, n, h, t) IntelQaRealloc((p), (n), (h), (t), __func__, __LINE__)
        #else
            #define XMALLOC(s, h, t)     IntelQaMalloc((s), (h), (t))
            #define XFREE(p, h, t)       IntelQaFree((p), (h), (t))
            #define XREALLOC(p, n, h, t) IntelQaRealloc((p), (n), (h), (t))
        #endif /* WOLFSSL_DEBUG_MEMORY */
    #elif defined(XMALLOC_USER)
        /* prototypes for user heap override functions */
        #include <stddef.h>  /* for size_t */
        extern void *XMALLOC(size_t n, void* heap, int type);
        extern void *XREALLOC(void *p, size_t n, void* heap, int type);
        extern void XFREE(void *p, void* heap, int type);
    #elif defined(WOLFSSL_MEMORY_LOG)
        #define XMALLOC(n, h, t)        xmalloc(n, h, t, __func__, __FILE__, __LINE__)
        #define XREALLOC(p, n, h, t)    xrealloc(p, n, h, t, __func__,  __FILE__, __LINE__)
        #define XFREE(p, h, t)          xfree(p, h, t, __func__, __FILE__, __LINE__)

        /* prototypes for user heap override functions */
        #include <stddef.h>  /* for size_t */
        #include <stdlib.h>
        extern void *xmalloc(size_t n, void* heap, int type, const char* func,
              const char* file, unsigned int line);
        extern void *xrealloc(void *p, size_t n, void* heap, int type,
              const char* func, const char* file, unsigned int line);
        extern void xfree(void *p, void* heap, int type, const char* func,
              const char* file, unsigned int line);
    #elif defined(XMALLOC_OVERRIDE)
        /* override the XMALLOC, XFREE and XREALLOC macros */
    #elif defined(NO_WOLFSSL_MEMORY)
        /* just use plain C stdlib stuff if desired */
        #include <stdlib.h>
        #define XMALLOC(s, h, t)     ((void)h, (void)t, malloc((s)))
        #define XFREE(p, h, t)       {void* xp = (p); if((xp)) free((xp));}
        #define XREALLOC(p, n, h, t) realloc((p), (n))
    #elif !defined(MICRIUM_MALLOC) && !defined(EBSNET) \
            && !defined(WOLFSSL_SAFERTOS) && !defined(FREESCALE_MQX) \
            && !defined(FREESCALE_KSDK_MQX) && !defined(FREESCALE_FREE_RTOS) \
            && !defined(WOLFSSL_LEANPSK) && !defined(WOLFSSL_uITRON4)
        /* default C runtime, can install different routines at runtime via cbs */
        #include <wolfssl/wolfcrypt/memory.h>
        #ifdef WOLFSSL_STATIC_MEMORY
            #ifdef WOLFSSL_DEBUG_MEMORY
                #define XMALLOC(s, h, t)     wolfSSL_Malloc((s), (h), (t), __func__, __LINE__)
                #define XFREE(p, h, t)       {void* xp = (p); if((xp)) wolfSSL_Free((xp), (h), (t), __func__, __LINE__);}
                #define XREALLOC(p, n, h, t) wolfSSL_Realloc((p), (n), (h), (t), __func__, __LINE__)
            #else
                #define XMALLOC(s, h, t)     wolfSSL_Malloc((s), (h), (t))
                #define XFREE(p, h, t)       {void* xp = (p); if((xp)) wolfSSL_Free((xp), (h), (t));}
                #define XREALLOC(p, n, h, t) wolfSSL_Realloc((p), (n), (h), (t))
            #endif /* WOLFSSL_DEBUG_MEMORY */
        #elif !defined(FREERTOS) && !defined(FREERTOS_TCP)
            #ifdef WOLFSSL_DEBUG_MEMORY
                #define XMALLOC(s, h, t)     ((void)h, (void)t, wolfSSL_Malloc((s), __func__, __LINE__))
                #define XFREE(p, h, t)       {void* xp = (p); if((xp)) wolfSSL_Free((xp), __func__, __LINE__);}
                #define XREALLOC(p, n, h, t) wolfSSL_Realloc((p), (n), __func__, __LINE__)
            #else
                #define XMALLOC(s, h, t)     ((void)h, (void)t, wolfSSL_Malloc((s)))
                #define XFREE(p, h, t)       {void* xp = (p); if((xp)) wolfSSL_Free((xp));}
                #define XREALLOC(p, n, h, t) wolfSSL_Realloc((p), (n))
            #endif /* WOLFSSL_DEBUG_MEMORY */
        #endif /* WOLFSSL_STATIC_MEMORY */
    #endif

    /* declare/free variable handling for async */
    #ifdef WOLFSSL_ASYNC_CRYPT
        #define DECLARE_VAR(VAR_NAME, VAR_TYPE, VAR_SIZE, HEAP) \
            VAR_TYPE* VAR_NAME = (VAR_TYPE*)XMALLOC(sizeof(VAR_TYPE) * VAR_SIZE, (HEAP), DYNAMIC_TYPE_WOLF_BIGINT);
        #define DECLARE_VAR_INIT(VAR_NAME, VAR_TYPE, VAR_SIZE, INIT_VALUE, HEAP) \
            VAR_TYPE* VAR_NAME = ({ \
                VAR_TYPE* ptr = (VAR_TYPE*)XMALLOC(sizeof(VAR_TYPE) * VAR_SIZE, (HEAP), DYNAMIC_TYPE_WOLF_BIGINT); \
                if (ptr && INIT_VALUE) { \
                    XMEMCPY(ptr, INIT_VALUE, sizeof(VAR_TYPE) * VAR_SIZE); \
                } \
                ptr; \
            })
        #define DECLARE_ARRAY(VAR_NAME, VAR_TYPE, VAR_ITEMS, VAR_SIZE, HEAP) \
            VAR_TYPE* VAR_NAME[VAR_ITEMS]; \
            int idx##VAR_NAME; \
            for (idx##VAR_NAME=0; idx##VAR_NAME<VAR_ITEMS; idx##VAR_NAME++) { \
                VAR_NAME[idx##VAR_NAME] = (VAR_TYPE*)XMALLOC(VAR_SIZE, (HEAP), DYNAMIC_TYPE_WOLF_BIGINT); \
            }
        #define FREE_VAR(VAR_NAME, HEAP) \
            XFREE(VAR_NAME, (HEAP), DYNAMIC_TYPE_WOLF_BIGINT);
        #define FREE_ARRAY(VAR_NAME, VAR_ITEMS, HEAP) \
            for (idx##VAR_NAME=0; idx##VAR_NAME<VAR_ITEMS; idx##VAR_NAME++) { \
                XFREE(VAR_NAME[idx##VAR_NAME], (HEAP), DYNAMIC_TYPE_WOLF_BIGINT); \
            }
    #else
        #define DECLARE_VAR(VAR_NAME, VAR_TYPE, VAR_SIZE, HEAP) \
            VAR_TYPE VAR_NAME[VAR_SIZE]
        #define DECLARE_VAR_INIT(VAR_NAME, VAR_TYPE, VAR_SIZE, INIT_VALUE, HEAP) \
            VAR_TYPE* VAR_NAME = (VAR_TYPE*)INIT_VALUE
        #define DECLARE_ARRAY(VAR_NAME, VAR_TYPE, VAR_ITEMS, VAR_SIZE, HEAP) \
            VAR_TYPE VAR_NAME[VAR_ITEMS][VAR_SIZE]
        #define FREE_VAR(VAR_NAME, HEAP) /* nothing to free, its stack */
        #define FREE_ARRAY(VAR_NAME, VAR_ITEMS, HEAP)  /* nothing to free, its stack */
    #endif

    #if !defined(USE_WOLF_STRTOK) && \
            ((defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)) || \
             defined(WOLFSSL_TIRTOS) || defined(WOLF_C99))
        #define USE_WOLF_STRTOK
    #endif
    #if !defined(USE_WOLF_STRSEP) && (defined(WOLF_C99))
        #define USE_WOLF_STRSEP
    #endif

    #ifndef STRING_USER
        #include <string.h>
        #define XMEMCPY(d,s,l)    memcpy((d),(s),(l))
        #define XMEMSET(b,c,l)    memset((b),(c),(l))
        #define XMEMCMP(s1,s2,n)  memcmp((s1),(s2),(n))
        #define XMEMMOVE(d,s,l)   memmove((d),(s),(l))

        #define XSTRLEN(s1)       strlen((s1))
        #define XSTRNCPY(s1,s2,n) strncpy((s1),(s2),(n))
        /* strstr, strncmp, and strncat only used by wolfSSL proper,
         * not required for wolfCrypt only */
        #define XSTRSTR(s1,s2)    strstr((s1),(s2))
        #define XSTRNSTR(s1,s2,n) mystrnstr((s1),(s2),(n))
        #define XSTRNCMP(s1,s2,n) strncmp((s1),(s2),(n))
        #define XSTRNCAT(s1,s2,n) strncat((s1),(s2),(n))

        #ifdef USE_WOLF_STRSEP
            #define XSTRSEP(s1,d) wc_strsep((s1),(d))
        #else
            #define XSTRSEP(s1,d) strsep((s1),(d))
        #endif

        #if defined(MICROCHIP_PIC32) || defined(WOLFSSL_TIRTOS)
            /* XC32 does not support strncasecmp, so use case sensitive one */
            #define XSTRNCASECMP(s1,s2,n) strncmp((s1),(s2),(n))
        #elif defined(USE_WINDOWS_API) || defined(FREERTOS_TCP_WINSIM)
            #define XSTRNCASECMP(s1,s2,n) _strnicmp((s1),(s2),(n))
        #else
            #if defined(HAVE_STRINGS_H) && defined(WOLF_C99) && \
                !defined(WOLFSSL_SGX)
                #include <strings.h>
            #endif
            #define XSTRNCASECMP(s1,s2,n) strncasecmp((s1),(s2),(n))
        #endif

        /* snprintf is used in asn.c for GetTimeString, PKCS7 test, and when
           debugging is turned on */
        #ifndef USE_WINDOWS_API
            #if defined(NO_FILESYSTEM) && (defined(OPENSSL_EXTRA) || \
                   defined(HAVE_PKCS7)) && !defined(NO_STDIO_FILESYSTEM)
                /* case where stdio is not included else where but is needed for
                 * snprintf */
                #include <stdio.h>
            #endif
            #define XSNPRINTF snprintf
        #else
            #define XSNPRINTF _snprintf
        #endif

        #if defined(WOLFSSL_CERT_EXT) || defined(HAVE_ALPN)
            /* use only Thread Safe version of strtok */
            #if defined(USE_WOLF_STRTOK)
                #define XSTRTOK(s1,d,ptr) wc_strtok((s1),(d),(ptr))
            #elif defined(USE_WINDOWS_API) || defined(INTIME_RTOS)
                #define XSTRTOK(s1,d,ptr) strtok_s((s1),(d),(ptr))
            #else
                #define XSTRTOK(s1,d,ptr) strtok_r((s1),(d),(ptr))
            #endif
        #endif
    #endif

    #ifdef USE_WOLF_STRTOK
        WOLFSSL_API char* wc_strtok(char *str, const char *delim, char **nextp);
    #endif
    #ifdef USE_WOLF_STRSEP
        WOLFSSL_API char* wc_strsep(char **stringp, const char *delim);
    #endif

    #if !defined(NO_FILESYSTEM) && defined(OPENSSL_EXTRA) && \
        !defined(NO_STDIO_FILESYSTEM)
        #ifndef XGETENV
            #include <stdlib.h>
            #define XGETENV getenv
        #endif
    #endif /* OPENSSL_EXTRA */

    #ifndef CTYPE_USER
        #include <ctype.h>
        #if defined(HAVE_ECC) || defined(HAVE_OCSP) || \
            defined(WOLFSSL_KEY_GEN) || !defined(NO_DSA)
            #define XTOUPPER(c)     toupper((c))
            #define XISALPHA(c)     isalpha((c))
        #endif
        /* needed by wolfSSL_check_domain_name() */
        #define XTOLOWER(c)      tolower((c))
    #endif


    /* memory allocation types for user hints */
    enum {
        DYNAMIC_TYPE_CA           = 1,
        DYNAMIC_TYPE_CERT         = 2,
        DYNAMIC_TYPE_KEY          = 3,
        DYNAMIC_TYPE_FILE         = 4,
        DYNAMIC_TYPE_SUBJECT_CN   = 5,
        DYNAMIC_TYPE_PUBLIC_KEY   = 6,
        DYNAMIC_TYPE_SIGNER       = 7,
        DYNAMIC_TYPE_NONE         = 8,
        DYNAMIC_TYPE_BIGINT       = 9,
        DYNAMIC_TYPE_RSA          = 10,
        DYNAMIC_TYPE_METHOD       = 11,
        DYNAMIC_TYPE_OUT_BUFFER   = 12,
        DYNAMIC_TYPE_IN_BUFFER    = 13,
        DYNAMIC_TYPE_INFO         = 14,
        DYNAMIC_TYPE_DH           = 15,
        DYNAMIC_TYPE_DOMAIN       = 16,
        DYNAMIC_TYPE_SSL          = 17,
        DYNAMIC_TYPE_CTX          = 18,
        DYNAMIC_TYPE_WRITEV       = 19,
        DYNAMIC_TYPE_OPENSSL      = 20,
        DYNAMIC_TYPE_DSA          = 21,
        DYNAMIC_TYPE_CRL          = 22,
        DYNAMIC_TYPE_REVOKED      = 23,
        DYNAMIC_TYPE_CRL_ENTRY    = 24,
        DYNAMIC_TYPE_CERT_MANAGER = 25,
        DYNAMIC_TYPE_CRL_MONITOR  = 26,
        DYNAMIC_TYPE_OCSP_STATUS  = 27,
        DYNAMIC_TYPE_OCSP_ENTRY   = 28,
        DYNAMIC_TYPE_ALTNAME      = 29,
        DYNAMIC_TYPE_SUITES       = 30,
        DYNAMIC_TYPE_CIPHER       = 31,
        DYNAMIC_TYPE_RNG          = 32,
        DYNAMIC_TYPE_ARRAYS       = 33,
        DYNAMIC_TYPE_DTLS_POOL    = 34,
        DYNAMIC_TYPE_SOCKADDR     = 35,
        DYNAMIC_TYPE_LIBZ         = 36,
        DYNAMIC_TYPE_ECC          = 37,
        DYNAMIC_TYPE_TMP_BUFFER   = 38,
        DYNAMIC_TYPE_DTLS_MSG     = 39,
        DYNAMIC_TYPE_X509         = 40,
        DYNAMIC_TYPE_TLSX         = 41,
        DYNAMIC_TYPE_OCSP         = 42,
        DYNAMIC_TYPE_SIGNATURE    = 43,
        DYNAMIC_TYPE_HASHES       = 44,
        DYNAMIC_TYPE_SRP          = 45,
        DYNAMIC_TYPE_COOKIE_PWD   = 46,
        DYNAMIC_TYPE_USER_CRYPTO  = 47,
        DYNAMIC_TYPE_OCSP_REQUEST = 48,
        DYNAMIC_TYPE_X509_EXT     = 49,
        DYNAMIC_TYPE_X509_STORE   = 50,
        DYNAMIC_TYPE_X509_CTX     = 51,
        DYNAMIC_TYPE_URL          = 52,
        DYNAMIC_TYPE_DTLS_FRAG    = 53,
        DYNAMIC_TYPE_DTLS_BUFFER  = 54,
        DYNAMIC_TYPE_SESSION_TICK = 55,
        DYNAMIC_TYPE_PKCS         = 56,
        DYNAMIC_TYPE_MUTEX        = 57,
        DYNAMIC_TYPE_PKCS7        = 58,
        DYNAMIC_TYPE_AES_BUFFER   = 59,
        DYNAMIC_TYPE_WOLF_BIGINT  = 60,
        DYNAMIC_TYPE_ASN1         = 61,
        DYNAMIC_TYPE_LOG          = 62,
        DYNAMIC_TYPE_WRITEDUP     = 63,
        DYNAMIC_TYPE_PRIVATE_KEY  = 64,
        DYNAMIC_TYPE_HMAC         = 65,
        DYNAMIC_TYPE_ASYNC        = 66,
        DYNAMIC_TYPE_ASYNC_NUMA   = 67,
        DYNAMIC_TYPE_ASYNC_NUMA64 = 68,
        DYNAMIC_TYPE_CURVE25519   = 69,
        DYNAMIC_TYPE_ED25519      = 70,
        DYNAMIC_TYPE_SECRET       = 71,
        DYNAMIC_TYPE_DIGEST       = 72,
        DYNAMIC_TYPE_RSA_BUFFER   = 73,
        DYNAMIC_TYPE_DCERT        = 74,
        DYNAMIC_TYPE_STRING       = 75,
        DYNAMIC_TYPE_PEM          = 76,
        DYNAMIC_TYPE_DER          = 77,
        DYNAMIC_TYPE_CERT_EXT     = 78,
        DYNAMIC_TYPE_ALPN         = 79,
        DYNAMIC_TYPE_ENCRYPTEDINFO= 80,
        DYNAMIC_TYPE_DIRCTX       = 81,
        DYNAMIC_TYPE_HASHCTX      = 82,
        DYNAMIC_TYPE_SEED         = 83,
        DYNAMIC_TYPE_SYMMETRIC_KEY= 84,
        DYNAMIC_TYPE_ECC_BUFFER   = 85,
        DYNAMIC_TYPE_QSH          = 86,
        DYNAMIC_TYPE_SALT         = 87,
        DYNAMIC_TYPE_HASH_TMP     = 88,
        DYNAMIC_TYPE_BLOB         = 89,
        DYNAMIC_TYPE_NAME_ENTRY   = 90,
    };

    /* max error buffer string size */
    #ifndef WOLFSSL_MAX_ERROR_SZ
        #define WOLFSSL_MAX_ERROR_SZ 80
    #endif

    /* stack protection */
    enum {
        MIN_STACK_BUFFER = 8
    };


    /* Algorithm Types */
    enum wc_AlgoType {
        WC_ALGO_TYPE_NONE = 0,
        WC_ALGO_TYPE_HASH = 1,
        WC_ALGO_TYPE_CIPHER = 2,
        WC_ALGO_TYPE_PK = 3,

        WC_ALGO_TYPE_MAX = WC_ALGO_TYPE_PK
    };

    /* hash types */
    enum wc_HashType {
    #if defined(HAVE_SELFTEST) || defined(HAVE_FIPS)
        /* In selftest build, WC_* types are not mapped to WC_HASH_TYPE types.
         * Values here are based on old selftest hmac.h enum, with additions */
        WC_HASH_TYPE_NONE = 15,
        WC_HASH_TYPE_MD2 = 16,
        WC_HASH_TYPE_MD4 = 17,
        WC_HASH_TYPE_MD5 = 0,
        WC_HASH_TYPE_SHA = 1, /* SHA-1 (not old SHA-0) */
        WC_HASH_TYPE_SHA224 = 8,
        WC_HASH_TYPE_SHA256 = 2,
        WC_HASH_TYPE_SHA384 = 5,
        WC_HASH_TYPE_SHA512 = 4,
        WC_HASH_TYPE_MD5_SHA = 18,
        WC_HASH_TYPE_SHA3_224 = 10,
        WC_HASH_TYPE_SHA3_256 = 11,
        WC_HASH_TYPE_SHA3_384 = 12,
        WC_HASH_TYPE_SHA3_512 = 13,
        WC_HASH_TYPE_BLAKE2B = 14,

        WC_HASH_TYPE_MAX = WC_HASH_TYPE_MD5_SHA
    #else
        WC_HASH_TYPE_NONE = 0,
        WC_HASH_TYPE_MD2 = 1,
        WC_HASH_TYPE_MD4 = 2,
        WC_HASH_TYPE_MD5 = 3,
        WC_HASH_TYPE_SHA = 4, /* SHA-1 (not old SHA-0) */
        WC_HASH_TYPE_SHA224 = 5,
        WC_HASH_TYPE_SHA256 = 6,
        WC_HASH_TYPE_SHA384 = 7,
        WC_HASH_TYPE_SHA512 = 8,
        WC_HASH_TYPE_MD5_SHA = 9,
        WC_HASH_TYPE_SHA3_224 = 10,
        WC_HASH_TYPE_SHA3_256 = 11,
        WC_HASH_TYPE_SHA3_384 = 12,
        WC_HASH_TYPE_SHA3_512 = 13,
        WC_HASH_TYPE_BLAKE2B = 14,

        WC_HASH_TYPE_MAX = WC_HASH_TYPE_BLAKE2B
    #endif /* HAVE_SELFTEST */
    };

    /* cipher types */
    enum wc_CipherType {
        WC_CIPHER_NONE = 0,
        WC_CIPHER_AES = 1,
        WC_CIPHER_AES_CBC = 2,
        WC_CIPHER_AES_GCM = 3,
        WC_CIPHER_AES_CTR = 4,
        WC_CIPHER_AES_XTS = 5,
        WC_CIPHER_AES_CFB = 6,
        WC_CIPHER_DES3 = 7,
        WC_CIPHER_DES = 8,
        WC_CIPHER_CHACHA = 9,
        WC_CIPHER_HC128 = 10,
        WC_CIPHER_IDEA = 11,

        WC_CIPHER_MAX = WC_CIPHER_HC128
    };

    /* PK=public key (asymmetric) based algorithms */
    enum wc_PkType {
        WC_PK_TYPE_NONE = 0,
        WC_PK_TYPE_RSA = 1,
        WC_PK_TYPE_DH = 2,
        WC_PK_TYPE_ECDH = 3,
        WC_PK_TYPE_ECDSA_SIGN = 4,
        WC_PK_TYPE_ECDSA_VERIFY = 5,
        WC_PK_TYPE_ED25519 = 6,
        WC_PK_TYPE_CURVE25519 = 7,
        WC_PK_TYPE_RSA_KEYGEN = 8,
        WC_PK_TYPE_EC_KEYGEN = 9,

        WC_PK_TYPE_MAX = WC_PK_TYPE_EC_KEYGEN
    };


    /* settings detection for compile vs runtime math incompatibilities */
    enum {
    #if !defined(USE_FAST_MATH) && !defined(SIZEOF_LONG) && !defined(SIZEOF_LONG_LONG)
        CTC_SETTINGS = 0x0
    #elif !defined(USE_FAST_MATH) && defined(SIZEOF_LONG) && (SIZEOF_LONG == 8)
        CTC_SETTINGS = 0x1
    #elif !defined(USE_FAST_MATH) && defined(SIZEOF_LONG_LONG) && (SIZEOF_LONG_LONG == 8)
        CTC_SETTINGS = 0x2
    #elif !defined(USE_FAST_MATH) && defined(SIZEOF_LONG_LONG) && (SIZEOF_LONG_LONG == 4)
        CTC_SETTINGS = 0x4
    #elif defined(USE_FAST_MATH) && !defined(SIZEOF_LONG) && !defined(SIZEOF_LONG_LONG)
        CTC_SETTINGS = 0x8
    #elif defined(USE_FAST_MATH) && defined(SIZEOF_LONG) && (SIZEOF_LONG == 8)
        CTC_SETTINGS = 0x10
    #elif defined(USE_FAST_MATH) && defined(SIZEOF_LONG_LONG) && (SIZEOF_LONG_LONG == 8)
        CTC_SETTINGS = 0x20
    #elif defined(USE_FAST_MATH) && defined(SIZEOF_LONG_LONG) && (SIZEOF_LONG_LONG == 4)
        CTC_SETTINGS = 0x40
    #else
        #error "bad math long / long long settings"
    #endif
    };


    WOLFSSL_API word32 CheckRunTimeSettings(void);

    /* If user uses RSA, DH, DSA, or ECC math lib directly then fast math and long
       types need to match at compile time and run time, CheckCtcSettings will
       return 1 if a match otherwise 0 */
    #define CheckCtcSettings() (CTC_SETTINGS == CheckRunTimeSettings())

    /* invalid device id */
    #define INVALID_DEVID    -2


    /* AESNI requires alignment and ARMASM gains some performance from it */
    #if defined(WOLFSSL_AESNI) || defined(WOLFSSL_ARMASM) || defined(USE_INTEL_SPEEDUP)
        #if !defined(ALIGN16)
            #if defined(__GNUC__)
                #define ALIGN16 __attribute__ ( (aligned (16)))
            #elif defined(_MSC_VER)
                /* disable align warning, we want alignment ! */
                #pragma warning(disable: 4324)
                #define ALIGN16 __declspec (align (16))
            #else
                #define ALIGN16
            #endif
        #endif /* !ALIGN16 */

        #if !defined (ALIGN32)
            #if defined (__GNUC__)
                #define ALIGN32 __attribute__ ( (aligned (32)))
            #elif defined(_MSC_VER)
                /* disable align warning, we want alignment ! */
                #pragma warning(disable: 4324)
                #define ALIGN32 __declspec (align (32))
            #else
                #define ALIGN32
            #endif
        #endif

        #if !defined(ALIGN32)
            #if defined(__GNUC__)
                #define ALIGN32 __attribute__ ( (aligned (32)))
            #elif defined(_MSC_VER)
                /* disable align warning, we want alignment ! */
                #pragma warning(disable: 4324)
                #define ALIGN32 __declspec (align (32))
            #else
                #define ALIGN32
            #endif
        #endif /* !ALIGN32 */

        #if defined(__GNUC__)
            #define ALIGN128 __attribute__ ( (aligned (128)))
        #elif defined(_MSC_VER)
            /* disable align warning, we want alignment ! */
            #pragma warning(disable: 4324)
            #define ALIGN128 __declspec (align (128))
        #else
            #define ALIGN128
        #endif

        #if defined(__GNUC__)
            #define ALIGN256 __attribute__ ( (aligned (256)))
        #elif defined(_MSC_VER)
            /* disable align warning, we want alignment ! */
            #pragma warning(disable: 4324)
            #define ALIGN256 __declspec (align (256))
        #else
            #define ALIGN256
        #endif

    #else
        #ifndef ALIGN16
            #define ALIGN16
        #endif
        #ifndef ALIGN32
            #define ALIGN32
        #endif
        #ifndef ALIGN128
            #define ALIGN128
        #endif
        #ifndef ALIGN256
            #define ALIGN256
        #endif
    #endif /* WOLFSSL_AESNI || WOLFSSL_ARMASM */


    #ifndef TRUE
        #define TRUE  1
    #endif
    #ifndef FALSE
        #define FALSE 0
    #endif


    #ifdef WOLFSSL_RIOT_OS
        #define EXIT_TEST(ret) exit(ret)
    #elif defined(HAVE_STACK_SIZE)
        #define EXIT_TEST(ret) return (void*)((size_t)(ret))
    #else
        #define EXIT_TEST(ret) return ret
    #endif


    #if defined(__GNUC__)
        #define WOLFSSL_PACK __attribute__ ((packed))
    #else
        #define WOLFSSL_PACK
    #endif

    #ifndef __GNUC_PREREQ
        #if defined(__GNUC__) && defined(__GNUC_MINOR__)
            #define __GNUC_PREREQ(maj, min) \
                ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
        #else
            #define __GNUC_PREREQ(maj, min) (0) /* not GNUC */
        #endif
    #endif

    #if defined(__GNUC__)
        #define WC_NORETURN __attribute__((noreturn))
    #else
        #define WC_NORETURN
    #endif

    #if defined(WOLFSSL_KEY_GEN) || defined(HAVE_COMP_KEY) || \
        defined(WOLFSSL_DEBUG_MATH) || defined(DEBUG_WOLFSSL) || \
        defined(WOLFSSL_PUBLIC_MP) || defined(OPENSSL_EXTRA) || \
            (defined(HAVE_ECC) && defined(HAVE_ECC_KEY_EXPORT))
        #undef  WC_MP_TO_RADIX
        #define WC_MP_TO_RADIX
    #endif

    #ifdef __cplusplus
        }   /* extern "C" */
    #endif

#endif /* WOLF_CRYPT_TYPES_H */
