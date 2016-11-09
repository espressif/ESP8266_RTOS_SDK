#ifndef _SSL_DEBUG_H_
#define _SSL_DEBUG_H_

#include "ssl_opt.h"

#define MBED_SSL_DEBUG_ENBALE 1
#define MBED_SSL_DEBUG_LEVEL 0

#define HANDLE_ERR(err, go, ...) { if (MBED_SSL_DEBUG_ENBALE) ssl_print(__VA_ARGS__); ret = err; goto go; }
#define HANDLE_RET(go, ...) { if (MBED_SSL_DEBUG_ENBALE) ssl_print(__VA_ARGS__); goto go; }

#define MBED_SSL_DEBUG(level, ...) { if (level > MBED_SSL_DEBUG_LEVEL && MBED_SSL_DEBUG_ENBALE) ssl_print(__VA_ARGS__); }

#endif
