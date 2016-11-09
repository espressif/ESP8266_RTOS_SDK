#ifndef _SSL_OPT_H_
#define _SSL_OPT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* mbedtls include */
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/platform.h"

#include "c_types.h"

#define ssl_print printf
#define ssl_mem_zalloc zalloc
#define ssl_mem_free free

#define ssl_memcpy memcpy

#define SSL_MUTEX_DEF(x) int x
#define SSL_MUTEX_INIT(x)

#define SSL_NULL NULL

#endif
