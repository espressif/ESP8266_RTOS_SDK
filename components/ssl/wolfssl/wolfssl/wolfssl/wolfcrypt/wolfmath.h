/* wolfmath.h
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


#if defined(HAVE_WOLF_BIGINT) && !defined(WOLF_BIGINT_DEFINED)
    /* raw big integer */
    typedef struct WC_BIGINT {
        byte*   buf;
        word32  len;
        void*   heap;
    } WC_BIGINT;

    #define WOLF_BIGINT_DEFINED
#endif


/* only define functions if mp_int has been declared */
#ifdef MP_INT_DEFINED

#ifndef __WOLFMATH_H__
#define __WOLFMATH_H__

    /* timing resistance array */
    #if !defined(WC_NO_CACHE_RESISTANT) && \
        ((defined(HAVE_ECC) && defined(ECC_TIMING_RESISTANT)) || \
         (defined(USE_FAST_MATH) && defined(TFM_TIMING_RESISTANT)))

        extern const wolfssl_word wc_off_on_addr[2];
    #endif

    /* common math functions */
    int get_digit_count(mp_int* a);
    mp_digit get_digit(mp_int* a, int n);
    int get_rand_digit(WC_RNG* rng, mp_digit* d);
    int mp_rand(mp_int* a, int digits, WC_RNG* rng);

    enum {
        /* format type */
        WC_TYPE_HEX_STR = 1,
        WC_TYPE_UNSIGNED_BIN = 2,
    };

    WOLFSSL_API int wc_export_int(mp_int* mp, byte* buf, word32* len, 
        word32 keySz, int encType);

    #ifdef HAVE_WOLF_BIGINT
        void wc_bigint_init(WC_BIGINT* a);
        int wc_bigint_alloc(WC_BIGINT* a, word32 sz);
        int wc_bigint_from_unsigned_bin(WC_BIGINT* a, const byte* in, word32 inlen);
        int wc_bigint_to_unsigned_bin(WC_BIGINT* a, byte* out, word32* outlen);
        void wc_bigint_zero(WC_BIGINT* a);
        void wc_bigint_free(WC_BIGINT* a);

        int wc_mp_to_bigint(mp_int* src, WC_BIGINT* dst);
        int wc_mp_to_bigint_sz(mp_int* src, WC_BIGINT* dst, word32 sz);
        int wc_bigint_to_mp(WC_BIGINT* src, mp_int* dst);
    #endif /* HAVE_WOLF_BIGINT */

#endif /* __WOLFMATH_H__ */

#endif /* MP_INT_DEFINED */
