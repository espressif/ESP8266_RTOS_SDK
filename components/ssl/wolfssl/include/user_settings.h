/* user_settings.h
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.  All rights reserved.
 * Additions Copyright 2018 Espressif Systems (Shanghai) PTE LTD.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is distributed in binary form as licensed by Espressif Systems.
 * See README file or contact licensing@wolfssl.com with any questions or comments.
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
#define USE_WOLFSSL_IO
#define WOLFSSL_STATIC_RSA
#define NO_DH
#define NO_MD4
#define NO_DES3
#define NO_DSA
#define NO_RC4
#define NO_RABBIT
#define HAVE_ECC
#define HAVE_AES_ECB
#define WOLFSSL_AES_DIRECT
#define WC_NO_HARDEN
#define FREERTOS
#define WOLFSSL_TYPES
#define NO_FILESYSTEM
#define WOLFSSL_ALT_CERT_CHAINS
#define WOLFSSL_ALLOW_TLSV10
#define WOLFSSL_SMALL_STACK
#define SMALL_SESSION_CACHE
#define OPENSSL_EXTRA

#define SSL_CTX_use_certificate_ASN1(ctx,len,buf) wolfSSL_CTX_use_certificate_buffer(ctx,buf,len,WOLFSSL_FILETYPE_PEM)
#define SSL_CTX_use_PrivateKey_ASN1(type,ctx,buf,len) wolfSSL_CTX_use_PrivateKey_buffer(ctx,buf,len, WOLFSSL_FILETYPE_PEM)
#define SSL_CTX_load_verify_buffer(ctx,buf,len) wolfSSL_CTX_load_verify_buffer(ctx,buf,len, WOLFSSL_FILETYPE_PEM)

#ifdef WOLFSSL_TYPES
    #ifndef byte
        typedef unsigned char  byte;
    #endif
    typedef unsigned short word16;
    typedef unsigned int   word32;
    typedef byte           word24[3];
#endif

#ifndef CUSTOM_RAND_GENERATE_BLOCK
#include "esp_libc.h"
    /* To use define the following:*/
    #define CUSTOM_RAND_GENERATE_BLOCK os_get_random
#endif

#endif