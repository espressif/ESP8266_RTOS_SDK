/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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
#define WC_NO_HARDEN
#define FREERTOS
#define WOLFSSL_TYPES
#define NO_FILESYSTEM
#define WOLFSSL_ALT_CERT_CHAINS
#define WOLFSSL_ALLOW_TLSV10
#define WOLFSSL_SMALL_STACK
#define SMALL_SESSION_CACHE

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
    /* To use define the following:*/
    #define CUSTOM_RAND_GENERATE_BLOCK os_get_random
#endif

#endif

