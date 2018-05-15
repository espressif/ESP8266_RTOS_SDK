/* arc4.h
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.  All rights reserved.
 *
 * This file is part of wolfSSL.
 *
 * Contact licensing@wolfssl.com with any questions or comments.
 *
 * http://www.wolfssl.com
 */




#ifndef WOLF_CRYPT_ARC4_H
#define WOLF_CRYPT_ARC4_H

#include <wolfssl/wolfcrypt/types.h>

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef WOLFSSL_ASYNC_CRYPT
    #include <wolfssl/wolfcrypt/async.h>
#endif

enum {
	ARC4_ENC_TYPE   = 4,    /* cipher unique type */
    ARC4_STATE_SIZE = 256
};

/* ARC4 encryption and decryption */
typedef struct Arc4 {
    byte x;
    byte y;
    byte state[ARC4_STATE_SIZE];
#ifdef WOLFSSL_ASYNC_CRYPT
    WC_ASYNC_DEV asyncDev;
#endif
    void* heap;
} Arc4;

WOLFSSL_API int wc_Arc4Process(Arc4*, byte*, const byte*, word32);
WOLFSSL_API int wc_Arc4SetKey(Arc4*, const byte*, word32);

WOLFSSL_API int  wc_Arc4Init(Arc4*, void*, int);
WOLFSSL_API void wc_Arc4Free(Arc4*);

#ifdef __cplusplus
    } /* extern "C" */
#endif


#endif /* WOLF_CRYPT_ARC4_H */

