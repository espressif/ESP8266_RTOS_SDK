/* hash.h
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
    \file wolfssl/wolfcrypt/hash.h
*/

#ifndef WOLF_CRYPT_HASH_H
#define WOLF_CRYPT_HASH_H

#include <wolfssl/wolfcrypt/types.h>

#ifndef NO_MD5
    #include <wolfssl/wolfcrypt/md5.h>
#endif
#ifndef NO_SHA
    #include <wolfssl/wolfcrypt/sha.h>
#endif
#if defined(WOLFSSL_SHA224) || !defined(NO_SHA256)
    #include <wolfssl/wolfcrypt/sha256.h>
#endif
#if defined(WOLFSSL_SHA384) || defined(WOLFSSL_SHA512)
    #include <wolfssl/wolfcrypt/sha512.h>
#endif
#ifdef HAVE_BLAKE2
    #include <wolfssl/wolfcrypt/blake2.h>
#endif
#ifdef WOLFSSL_SHA3
    #include <wolfssl/wolfcrypt/sha3.h>
#endif
#ifndef NO_MD4
    #include <wolfssl/wolfcrypt/md4.h>
#endif
#ifdef WOLFSSL_MD2
    #include <wolfssl/wolfcrypt/md2.h>
#endif


#ifdef __cplusplus
    extern "C" {
#endif

#if !defined(HAVE_FIPS) && !defined(NO_OLD_WC_NAMES)
    #define MAX_DIGEST_SIZE WC_MAX_DIGEST_SIZE
#endif


typedef union {
    #ifndef NO_MD5
        wc_Md5 md5;
    #endif
    #ifndef NO_SHA
        wc_Sha sha;
    #endif
    #ifdef WOLFSSL_SHA224
        wc_Sha224 sha224;
    #endif
    #ifndef NO_SHA256
        wc_Sha256 sha256;
    #endif
    #ifdef WOLFSSL_SHA384
        wc_Sha384 sha384;
    #endif
    #ifdef WOLFSSL_SHA512
        wc_Sha512 sha512;
    #endif
} wc_HashAlg;

/* Find largest possible digest size
   Note if this gets up to the size of 80 or over check smallstack build */
#if defined(WOLFSSL_SHA3)
    #define WC_MAX_DIGEST_SIZE WC_SHA3_512_DIGEST_SIZE
    #define WC_MAX_BLOCK_SIZE  WC_SHA3_224_BLOCK_SIZE /* 224 is the largest block size */
#elif defined(WOLFSSL_SHA512)
    #define WC_MAX_DIGEST_SIZE WC_SHA512_DIGEST_SIZE
    #define WC_MAX_BLOCK_SIZE  WC_SHA512_BLOCK_SIZE
#elif defined(HAVE_BLAKE2)
    #define WC_MAX_DIGEST_SIZE BLAKE2B_OUTBYTES
    #define WC_MAX_BLOCK_SIZE  BLAKE2B_BLOCKBYTES
#elif defined(WOLFSSL_SHA384)
    #define WC_MAX_DIGEST_SIZE WC_SHA384_DIGEST_SIZE
    #define WC_MAX_BLOCK_SIZE  WC_SHA384_BLOCK_SIZE
#elif !defined(NO_SHA256)
    #define WC_MAX_DIGEST_SIZE WC_SHA256_DIGEST_SIZE
    #define WC_MAX_BLOCK_SIZE  WC_SHA256_BLOCK_SIZE
#elif defined(WOLFSSL_SHA224)
    #define WC_MAX_DIGEST_SIZE WC_SHA224_DIGEST_SIZE
    #define WC_MAX_BLOCK_SIZE  WC_SHA224_BLOCK_SIZE
#elif !defined(NO_SHA)
    #define WC_MAX_DIGEST_SIZE WC_SHA_DIGEST_SIZE
    #define WC_MAX_BLOCK_SIZE  WC_SHA_BLOCK_SIZE
#elif !defined(NO_MD5)
    #define WC_MAX_DIGEST_SIZE WC_MD5_DIGEST_SIZE
    #define WC_MAX_BLOCK_SIZE  WC_MD5_BLOCK_SIZE
#else
    #define WC_MAX_DIGEST_SIZE 64 /* default to max size of 64 */
    #define WC_MAX_BLOCK_SIZE  128
#endif

#if !defined(NO_ASN) || !defined(NO_DH) || defined(HAVE_ECC)
WOLFSSL_API int wc_HashGetOID(enum wc_HashType hash_type);
WOLFSSL_API enum wc_HashType wc_OidGetHash(int oid);
#endif

WOLFSSL_API enum wc_HashType wc_HashTypeConvert(int hashType);

WOLFSSL_API int wc_HashGetDigestSize(enum wc_HashType hash_type);
WOLFSSL_API int wc_HashGetBlockSize(enum wc_HashType hash_type);
WOLFSSL_API int wc_Hash(enum wc_HashType hash_type,
    const byte* data, word32 data_len,
    byte* hash, word32 hash_len);

/* generic hash operation wrappers */
WOLFSSL_API int wc_HashInit(wc_HashAlg* hash, enum wc_HashType type);
WOLFSSL_API int wc_HashUpdate(wc_HashAlg* hash, enum wc_HashType type,
    const byte* data, word32 dataSz);
WOLFSSL_API int wc_HashFinal(wc_HashAlg* hash, enum wc_HashType type,
    byte* out);
WOLFSSL_API int wc_HashFree(wc_HashAlg* hash, enum wc_HashType type);

#ifndef NO_MD5
#include <wolfssl/wolfcrypt/md5.h>
WOLFSSL_API int wc_Md5Hash(const byte* data, word32 len, byte* hash);
#endif

#ifndef NO_SHA
#include <wolfssl/wolfcrypt/sha.h>
WOLFSSL_API int wc_ShaHash(const byte*, word32, byte*);
#endif

#ifdef WOLFSSL_SHA224
#include <wolfssl/wolfcrypt/sha256.h>
WOLFSSL_API int wc_Sha224Hash(const byte*, word32, byte*);
#endif /* defined(WOLFSSL_SHA224) */

#ifndef NO_SHA256
#include <wolfssl/wolfcrypt/sha256.h>
WOLFSSL_API int wc_Sha256Hash(const byte*, word32, byte*);
#endif

#ifdef WOLFSSL_SHA384
#include <wolfssl/wolfcrypt/sha512.h>
WOLFSSL_API int wc_Sha384Hash(const byte*, word32, byte*);
#endif /* defined(WOLFSSL_SHA384) */

#ifdef WOLFSSL_SHA512
#include <wolfssl/wolfcrypt/sha512.h>
WOLFSSL_API int wc_Sha512Hash(const byte*, word32, byte*);
#endif /* WOLFSSL_SHA512 */

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* WOLF_CRYPT_HASH_H */
