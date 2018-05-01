/* user_settings.h
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.  All rights reserved.
 * Additions Copyright 2018 Espressif Systems (Shanghai) PTE LTD.
 *
 * This file is part of wolfSSL.
 *
 * Contact licensing@wolfssl.com with any questions or comments.
 *
 * http://www.wolfssl.com
 */


#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__

#define WOLFSSL_LWIP
#define NO_WRITEV
#define NO_WOLFSSL_DIR
#define NO_INLINE
#define NO_WOLFSSL_MEMORY
#define HAVE_PK_CALLBACKS
#define WOLFSSL_KEY_GEN
#define WOLFSSL_RIPEMD
#define ESP_PLATFORM
#define DEBUG_ESP_PLATFORM
#define USE_WOLFSSL_IO
#define WOLFSSL_STATIC_RSA
#define NO_DH
#define NO_MD4
#define NO_MD5
#define NO_DES3
#define NO_DSA
#define NO_RC4
#define NO_RABBIT
#define NO_OLD_TLS
#define HAVE_ECC
#define WC_NO_HARDEN
#define FREERTOS
#define WOLFSSL_TYPES
#define NO_FILESYSTEM
#define WOLFSSL_ALT_CERT_CHAINS

#ifdef WOLFSSL_TYPES
    #ifndef byte
        typedef unsigned char  byte;
    #endif
    typedef unsigned short word16;
    typedef unsigned int   word32;
    typedef byte           word24[3];
#endif

#ifndef CUSTOM_RAND_GENERATE_BLOCK

    /* To use define the following:*/
    #define CUSTOM_RAND_GENERATE_BLOCK myRngFunc
    extern int myRngFunc(byte* output, word32 sz);

#endif

#endif

