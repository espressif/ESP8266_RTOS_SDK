/*
 * Copyright (c) 2007, Cameron Rich
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file os_port.h
 *
 * Some stuff to minimise the differences between windows and linux/unix
 */

#ifndef HEADER_OS_PORT_H
#define HEADER_OS_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/unistd.h>

#include "rom/ets_sys.h"

#if 0
#define ssl_printf(fmt, args...) printf(fmt,## args)
#else
#define ssl_printf(fmt, args...)
#endif

void *zalloc(size_t n);
uint32_t system_get_data_of_array_8(const uint8_t*, uint8_t);
void system_get_string_from_flash(const char *, char *, size_t n);

#define STDCALL
#define EXP_FUNC

//struct timeval {
//  long    tv_sec;         /* seconds */
//  long    tv_usec;        /* and microseconds */
//};

#define tls_htons(x) ((uint16_t)((((x) & 0xff) << 8) | (((x) >> 8) & 0xff)))
#define tls_ntohs(x) tls_htons(x)
#define tls_htonl(_n) ((uint32_t)( (((_n) & 0xff) << 24) | (((_n) & 0xff00) << 8) | (((_n) >> 8)  & 0xff00) | (((_n) >> 24) & 0xff) ))
#define tls_ntohl(x) tls_htonl(x)

#ifndef be16toh
#define be16toh(x) ((uint16_t)tls_ntohs((uint16_t)(x)))
#endif

#ifndef htobe16
#define htobe16(x) ((uint16_t)tls_htons((uint16_t)(x)))
#endif

#ifndef be32toh
#define be32toh(x) ((uint32_t)tls_ntohl((uint32_t)(x)))
#endif

#ifndef htobe32
#define htobe32(x) ((uint32_t)tls_htonl((uint32_t)(x)))
#endif

#ifndef be64toh
static __inline__ uint64_t be64toh(uint64_t __x);
static __inline__ uint64_t be64toh(uint64_t __x) {return (((uint64_t)be32toh(__x & (uint64_t)0xFFFFFFFFULL)) << 32) | ((uint64_t)be32toh((__x & (uint64_t)0xFFFFFFFF00000000ULL) >> 32));}
#define be64toh(x) be64toh(x)
#endif

#ifndef htobe64
#define htobe64(x) be64toh(x)
#endif

#define SSL_MALLOC(size)          os_malloc(size)
#define SSL_REALLOC(mem_ref,size) os_realloc(mem_ref, size)
#define SSL_CALLOC(element, size) os_calloc(element, size)
#define SSL_ZALLOC(size)          os_zalloc(size)
#define SSL_FREE(mem_ref)         os_free(mem_ref)

#if 0
#define  FILE_NAME_LENGTH 		   25
//#define	OUTPUT_FILE 				"leak_info.txt" 	 //¡ä?¡¤??¨²¡ä?D1??¦Ì?D??¡é
//#define	SSL_MALLOC(size)				xmalloc (size, __FILE__, __LINE__)	 //??D?¨º¦Ì??malloc?¡écalloco¨ªfree
//#define	CALLOC(elements, size)		xcalloc (elements, size, __FILE__, __LINE__)
//#define	FREE(mem_ref)				xfree(mem_ref)
 
struct _MEM_INFO  
{
	void 		   *address;			
	unsigned int    size;						   	
	char 		   file_name[FILE_NAME_LENGTH];   	
	unsigned int    line;							
};

typedef struct _MEM_INFO MEM_INFO;

struct _MEM_LEAK {					
	MEM_INFO mem_info;
	struct _MEM_LEAK * next;
};

typedef struct _MEM_LEAK MEM_LEAK;

void add(MEM_INFO alloc_info);
void erase(unsigned pos);
void clear(void);

void * xmalloc(unsigned int size, const char * file, unsigned int line);
void * xcalloc(unsigned int elements, unsigned int size, const char * file, unsigned int line);
void xfree(void * mem_ref);

void add_mem_info (void * mem_ref, unsigned int size,  const char * file, unsigned int line);
void remove_mem_info (void * mem_ref);
void report_mem_leak(void);       
#endif
#ifdef __cplusplus
}
#endif

#endif 
