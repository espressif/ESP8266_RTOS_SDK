/* asn1.h
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


/* asn1.h for openssl */

#ifndef WOLFSSL_ASN1_H_
#define WOLFSSL_ASN1_H_

#include <wolfssl/openssl/ssl.h>

#define ASN1_STRING_new      wolfSSL_ASN1_STRING_type_new
#define ASN1_STRING_type_new wolfSSL_ASN1_STRING_type_new
#define ASN1_STRING_set      wolfSSL_ASN1_STRING_set
#define ASN1_STRING_free     wolfSSL_ASN1_STRING_free

#define V_ASN1_OCTET_STRING              0x04 /* tag for ASN1_OCTET_STRING */
#define V_ASN1_NEG                       0x100
#define V_ASN1_NEG_INTEGER               (2 | V_ASN1_NEG)
#define V_ASN1_NEG_ENUMERATED            (10 | V_ASN1_NEG)

/* Type for ASN1_print_ex */
# define ASN1_STRFLGS_ESC_2253           1
# define ASN1_STRFLGS_ESC_CTRL           2
# define ASN1_STRFLGS_ESC_MSB            4
# define ASN1_STRFLGS_ESC_QUOTE          8
# define ASN1_STRFLGS_UTF8_CONVERT       0x10
# define ASN1_STRFLGS_IGNORE_TYPE        0x20
# define ASN1_STRFLGS_SHOW_TYPE          0x40
# define ASN1_STRFLGS_DUMP_ALL           0x80
# define ASN1_STRFLGS_DUMP_UNKNOWN       0x100
# define ASN1_STRFLGS_DUMP_DER           0x200
# define ASN1_STRFLGS_RFC2253            (ASN1_STRFLGS_ESC_2253 | \
                                          ASN1_STRFLGS_ESC_CTRL | \
                                          ASN1_STRFLGS_ESC_MSB | \
                                          ASN1_STRFLGS_UTF8_CONVERT | \
                                          ASN1_STRFLGS_DUMP_UNKNOWN | \
                                          ASN1_STRFLGS_DUMP_DER)

#define MBSTRING_UTF8                    0x1000

#endif /* WOLFSSL_ASN1_H_ */
