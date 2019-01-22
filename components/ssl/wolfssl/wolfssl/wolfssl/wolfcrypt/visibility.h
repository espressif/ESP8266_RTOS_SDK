/* visibility.h
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



/* Visibility control macros */

#ifndef WOLF_CRYPT_VISIBILITY_H
#define WOLF_CRYPT_VISIBILITY_H


/* for compatibility and so that fips is using same name of macro @wc_fips */
/* The following visibility wrappers are for old FIPS. New FIPS should use
 * the same as a non-FIPS build. */
#if defined(HAVE_FIPS) && \
    (!defined(HAVE_FIPS_VERSION) || (HAVE_FIPS_VERSION < 2))
    #include <cyassl/ctaocrypt/visibility.h>
    #define WOLFSSL_API   CYASSL_API
	#define WOLFSSL_LOCAL CYASSL_LOCAL
#else

/* WOLFSSL_API is used for the public API symbols.
        It either imports or exports (or does nothing for static builds)

   WOLFSSL_LOCAL is used for non-API symbols (private).
*/

#if defined(BUILDING_WOLFSSL)
    #if defined(_MSC_VER) || defined(__MINGW32__)
        #if defined(WOLFSSL_DLL)
            #define WOLFSSL_API __declspec(dllexport)
        #else
            #define WOLFSSL_API
        #endif
        #define WOLFSSL_LOCAL
    #elif defined(HAVE_VISIBILITY) && HAVE_VISIBILITY
        #define WOLFSSL_API   __attribute__ ((visibility("default")))
        #define WOLFSSL_LOCAL __attribute__ ((visibility("hidden")))
    #elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
        #define WOLFSSL_API   __global
        #define WOLFSSL_LOCAL __hidden
    #else
        #define WOLFSSL_API
        #define WOLFSSL_LOCAL
    #endif /* HAVE_VISIBILITY */
#else /* BUILDING_WOLFSSL */
    #if defined(_MSC_VER) || defined(__MINGW32__)
        #if defined(WOLFSSL_DLL)
            #define WOLFSSL_API __declspec(dllimport)
        #else
            #define WOLFSSL_API
        #endif
        #define WOLFSSL_LOCAL
    #else
        #define WOLFSSL_API
        #define WOLFSSL_LOCAL
    #endif
#endif /* BUILDING_WOLFSSL */

#endif /* HAVE_FIPS */
#endif /* WOLF_CRYPT_VISIBILITY_H */

